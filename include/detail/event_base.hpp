// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_EVENT_BASE
#define RPT_DETAIL_EVENT_BASE

#include "array_generator.hpp"
#include "array_viewer.hpp"
#include "listener_base.hpp"
#include "waitable_atomic.hpp"
#include "atomic_shared_array.hpp"


namespace rpt::event_detail {

    using atomic_uintptr_t = waitable_atomic<uint64_t>;

    template<typename... Params>
    struct event_base {

        using allocator_type = std::pmr::polymorphic_allocator<listener_base<Params...>*>;

        event_base() = default;
        explicit event_base(allocator_type alloc)
            : generator(alloc)
        {}

        using array_type = std::shared_ptr<listener_base<Params...>* []>;

        /*
            Method to call from listener base in order to remove the litener from the array.
        */
        void repudiate(listener_base<Params...>&);

        template<typename... Args>
        void operator()(Args&&...);

        /*
            Add a listener to the list of subscribers
            from the start of this function call the lisener may be invoked.
        */
        void subscribe(listener_base<Params...>&);

        /*
            Clear the list and wait for it to be safe to destruct event_base
        */
        void clear();

        /*
            returns tru if there are currently no listeners subscribed.
        */
        bool empty() const;

        array_viewer<listener_base<Params...>> view_lock();

    private:
        using generator_type = array_generator<listener_base<Params...>*>;
        using listener_type = listener_base<Params...>;
        
        /*
            for 20 ArrModFn should be a concept
        */
        template<typename ArrModFn>
        void replace_data_using(ArrModFn&& fn);

        /*
            get a copy of hte list that cannot be detructed while the shared pointer (array_type) is held.
        */
        array_type lock();

        /*
            block untill the generation atomic changes value. Then return the new value.
        */
        void wait_on_generatrion_equal(uint64_t value);

        /*
            block until the number of viewers is equal to the requested value

            only call when viewers can only decreas
        */
        void wait_on_viewer_count(const array_type& arr, long count);

        using atomic_array_type = atomic_shared_array_t<listener_base<Params...>*>;

        generator_type generator;
        
        atomic_uintptr_t call_complete{ 0 };
        atomic_uintptr_t free_generation{ 0 };
        atomic_array_type data{ generator.make_array() };
    };
}

//TODO remove this
namespace rpt {
    template<typename... Params>
    class event;
}

namespace rpt::event_detail{

    //TODO move this into listener.hpp
    template<typename... P>
    struct get_event_base {
        static constexpr event_base<P...>& get(rpt::event<P...>&);
    };

    //TODO move this into listener.hpp
    template<>
    struct get_event_base<void> {
        static constexpr event_base<>& get(rpt::event<void>&);
    };

    template<typename... Params>
    array_viewer<listener_base<Params...>> event_base<Params...>::view_lock()
    {
        auto holder = lock();
        auto size = generator.get_size(holder);
        auto ptr = generator.get_data(holder);
        return array_viewer<listener_base<Params...>> {
            std::shared_ptr<listener_base<Params...>*[]>{std::move(holder), ptr},
            size
        };
    }

    template<typename... Params>
    template<typename... Args>
    void event_base<Params...>::operator()(Args&&... args) {
        auto view = event_detail::array_viewer{ view_lock() };
        std::for_each(view.begin(), view.end(), [args...](auto& l) {
            l(args...);
       });
       call_complete++;
       call_complete.notify_all();
    }

    template<typename... Params>
    bool event_base<Params...>::empty() const {
        return (generator.get_size(data.load()) == 0);
        //return data.load() == nullptr;
    }

    template<typename... Params>
    typename event_base<Params...>::array_type event_base<Params...>::lock() {
        return data.load();
    }

    template<typename... Params>
    void event_base<Params...>::wait_on_generatrion_equal(uint64_t value) {
        auto current_generation = free_generation.load();
        while (current_generation != value) {
            free_generation.wait(current_generation);
            current_generation = free_generation.load();
        }
    }

    template<typename... Params>
    void event_base<Params...>::wait_on_viewer_count(const array_type& arr, long target_count) {
        auto current_complete = call_complete.load();
        auto current_count = arr.use_count();
        while (current_count > target_count) {
            call_complete.wait(current_complete);
            current_complete = call_complete.load();
            current_count = arr.use_count();
        }
    }

    template<typename... Params>
    template<typename ArrModFn>
    void event_base<Params...>::replace_data_using(ArrModFn&& fn) {
        auto holder = lock();
        auto next = fn(holder);
        while (!data.compare_exchange_strong(holder, next))
        {
            next = fn(holder);
        }

        wait_on_viewer_count(holder, 1);

        auto generatrion = generator.get_generation(holder);
        holder = nullptr;

        wait_on_generatrion_equal(generatrion);

        free_generation++;
    }

    template<typename... Params>
    void event_base<Params...>::subscribe(listener_base<Params...>& l) {
        replace_data_using([&](const auto& arr) {
            return generator.copy_push_back(arr, l);
        });
    }

    template<typename... Params>
    void event_base<Params...>::repudiate(listener_base<Params...>& l) {
        replace_data_using([&](const auto& arr) {
            return generator.copy_remove(arr, l);
        });
    }

    template<typename... Params>
    void event_base<Params...>::clear() {
        auto holder = lock();
        auto size = generator.get_size(holder);

        auto removed = 0;
        std::for_each_n(generator.get_data(holder), generator.get_size(holder),[&removed](auto l){
            if (l->detatch())
                removed++;
        });

        auto generatrion = generator.get_generation(holder);
        holder = nullptr;

        wait_on_generatrion_equal(generatrion);

        //data.store(nullptr);
    }
}

#endif // RPT_DETAIL_EVENT_BASE