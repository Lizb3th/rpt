// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_EVENT
#define RPT_EVENT

#include "detail/event_base.hpp"
#include "listener.hpp"

#include <atomic>

namespace rpt {

    template<typename... Params>
    class event : event_detail::event_base<Params...>{
    public:
        //static_assert(sizeof...(Params) != 0 || !std::is_void_v<Params...>);

        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

        event() = default;
        event(const event&) = delete;
        event(event&&) = delete;

        explicit event(allocator_type alloc)
            : event_detail::event_base<Params...>{alloc}
        {}

        event& operator=(const event&) = delete;
        event& operator=(event&&) = delete;

        /*
        * needs to be explicitely defined because the base destructor cannot be
        * entered until the listeners are all un-subscribed.
        */
        ~event();

        template<typename... Args>
        void operator()(Args&&... args) {
            base_type::operator()(std::move(args)...);
        }

        template<typename Callback>
        listener<Callback, Params...> subscribe(Callback&&);
    private:
        friend event_detail::get_event_base<Params...>;

        using base_type = event_detail::event_base<Params...>;

        std::atomic_uintptr_t invokers{ 0 };
    };

    template<typename... Params>
    template<typename Callback>
    listener<Callback, Params...> event<Params...>::subscribe(Callback&& cb) {
        return listener<Callback, Params...>(*this, std::move(cb));
    }


    template<typename... Params>
    event<Params...>::~event() {
        event_base::clear();
    }
}

namespace rpt::event_detail {

    template<typename... Params>
    constexpr event_base<Params...>& get_event_base<Params...>::get(event<Params...>& ref) {
        return ref;
    }
}

#endif //RPT_EVENT