// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_EVENT
#define RPT_EVENT

#include<atomic>
#include<memory>
#include<algorithm>
#include<condition_variable>

#include <cassert>

namespace rpt::event_detail {

#if __cpp_lib_atomic_shared_ptr
    struct has_std_atomic_shared_ptr : std::true_type {};
#else // !__cpp_lib_atomic_shared_ptr
    struct has_std_atomic_shared_ptr : std::false_type {};
#endif //__cpp_lib_atomic_shared_ptr

    /*
        atomic_shared_ptr
    */
    template<typename T>
    struct atomic_shared_ptr_helper {
    public:
        std::shared_ptr<T> load();

        bool compare_exchange(std::shared_ptr<T> a, std::shared_ptr<T>b);

    private:
        std::shared_ptr<T> value;
    };

    //template<typename T>
    //using atomic_shared_ptr = enable_if<has_shared_ptr<T>, std::atomic<std::shared_ptr<T>>;

   // template<typename T>
    //using atomic_shared_ptr = atomic_shared_ptr_helper<T>;

    /*
    *   base class for listener
    */
    template<typename... Params>
    struct listener_base {
        using callback_ref_type = void(*)(listener_base&, Params...);
        using detach_type = bool(*)(listener_base&);

        callback_ref_type _callback;

        detach_type _detatch;

        template<typename... Args>
        void operator()(Args...);

        bool detatch();
    };

    template<typename... Params>
    template<typename... Args>
    void listener_base<Params...>::operator()(Args... args) {
        _callback(*this, Params(std::move(args))...);
    }

    template<typename... Params>
    bool listener_base<Params...>::detatch() {
        return _detatch(*this);
    }

    /*
    *   listener_shared_array
    */
    template<typename T, typename U = void>
    struct atomic_shared_array {
        class detail {
        public:
            detail() noexcept = default;
            detail(std::shared_ptr<T[]> desired) noexcept
                : data{std::move(desired)} {}

            std::shared_ptr<T[]> load(std::memory_order order = std::memory_order_seq_cst) const noexcept { return std::atomic_load_explicit(&data, order); }

            void store(std::shared_ptr<T[]> desired, std::memory_order order = std::memory_order_seq_cst) noexcept
            { return std::atomic_store_explicit(&data, desired, order); }

            bool compare_exchange_strong(std::shared_ptr<T[]>& expected, std::shared_ptr<T[]> desired,
                std::memory_order order = std::memory_order_seq_cst) noexcept
            { return atomic_compare_exchange_strong_explicit (&data, &expected, desired, order, order); }

        private:
            std::shared_ptr<T[]> data;
        };
        using type = detail;
    };

    //template<typename T>
    //struct atomic_shared_array<T> :
    //    std::enable_if<has_std_atomic_shared_ptr::value,
    //        std::atomic<
    //            std::shared_ptr<
    //                T[]
    //    >>>{};

    template<typename T>
    using atomic_shared_array_t = typename atomic_shared_array<T>::type;

    template<typename U>
    using shared_array = std::shared_ptr<U[]>;

    /*
    * only for use with listener array types. 
    */
    template<typename... Params>
    using array_entry_type = listener_base<Params...>*;

    template<typename T>
    class waitable_atomic {
    public:
        constexpr waitable_atomic(T desired) noexcept;
        void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept;
        T load(std::memory_order order = std::memory_order_seq_cst) const noexcept;
        T operator++(int) noexcept;
        void notify_all();// noexcept;
        void wait(T old, std::memory_order order = std::memory_order_seq_cst) const;// noexcept;

    private:
        std::atomic<T> atom;
        mutable std::condition_variable cv;
        mutable std::mutex mtx;
    };

    template<typename T>
    constexpr waitable_atomic<T>::waitable_atomic(T desired) noexcept
        : atom{ desired }
    {}

    template<typename T>
    void waitable_atomic<T>::store(T desired, std::memory_order order) noexcept {
        atom.store(std:move(desired), order);
    }

    template<typename T>
    T waitable_atomic<T>::load(std::memory_order order) const noexcept {
        return atom.load(order);
    }

    template<typename T>
    T waitable_atomic<T>::operator++(int) noexcept {
        static_assert(std::is_integral_v<T>);
        return atom++;
    }

    template<typename T>
    void waitable_atomic<T>::notify_all() {
        std::unique_lock{ mtx }; //dont hold lock;
        cv.notify_all();
    }

    template<typename T>
    void waitable_atomic<T>::wait(T old, std::memory_order order) const {
        if (atom.load(order) != old) {
            auto lock = std::unique_lock{ mtx };
            cv.wait(lock, [&] { return atom.load(order) != old; });
        }
    }

    //using atomic_uintptr_t = waitable_atomic<std::uintptr_t>;
    /*
        The counter type is tricky since it may be valid for the bits of a 
        pointer to not correspondt to nice integer values.
    */
    using counter_t = std::uint64_t;

    template<typename T>
    struct array_generator
    {
        static_assert(std::is_pointer_v<T>);
        using allocator_type = std::pmr::polymorphic_allocator<T>;
        using array_type = std::shared_ptr<T[]>;
        using array_iterator = T*;

        array_generator() = default;

        explicit array_generator(allocator_type alloc);

        allocator_type get_allocator() const;

        array_type make_array(std::remove_pointer_t<T>&) const;
        array_type make_array() const;

        array_type copy_push_back(const array_type&, std::remove_pointer_t<T>&) const;

        array_type copy_remove(const array_type&, std::remove_pointer_t<T>&) const;

        static std::size_t get_size(const array_type&);
        static std::uintptr_t get_generation(const array_type&);
        static array_iterator get_data(const array_type&);
    private:
        array_type make_array_for_overwrite(std::size_t, std::uintptr_t) const;

        mutable allocator_type alloc{};
    };

    template<typename T>
    array_generator<T>::array_generator(allocator_type alloc)
        : alloc{ alloc } {
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::make_array(std::remove_pointer_t<T>& r) const {
        auto output = make_array_for_overwrite(1, 1);
        output[2] = &r;
        return output;
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::make_array() const {
        auto output = make_array_for_overwrite(0, 0);
        return output;
    }

    template<typename T>
    typename array_generator<T>::allocator_type array_generator<T>::get_allocator() const {
        return alloc;
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::copy_push_back(const typename array_generator<T>::array_type& arr, std::remove_pointer_t<T>& t) const {
        auto size = get_size(arr) + 1;
        auto output = make_array_for_overwrite(size, get_generation(arr) + 1);
        *std::copy_n(get_data(arr), size - 1, get_data(output)) = &t;
        return output;
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::copy_remove(const typename array_generator<T>::array_type& arr, std::remove_pointer_t<T>& t) const {
        auto size = get_size(arr) - 1;
        //if (size != 0)
        //{
            auto output = make_array_for_overwrite(size, get_generation(arr) + 1);
            std::remove_copy(get_data(arr), std::next(get_data(arr), size + 1), get_data(output), &t);
            return output;
       // }
       // return nullptr;
    }

    template<typename T>
    std::size_t array_generator<T>::get_size(const array_type& arr) {
        assert(arr != nullptr);
        //if (arr == nullptr)
        //    return 0;

        return reinterpret_cast<std::size_t>(arr[0]);
    }

    template<typename T>
    std::uintptr_t array_generator<T>::get_generation(const array_type& arr) {
        assert(arr != nullptr);
        //if (arr == nullptr)
        //    return 0;

        return reinterpret_cast<std::uintptr_t>(arr[1]);
    }

    template<typename T>
    T* array_generator<T>::get_data(const array_type& arr) {
        assert(arr != nullptr);
        //if (arr == nullptr)
        //    return 0;

        return std::next(arr.get(), 2);
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::make_array_for_overwrite(std::size_t size, std::uintptr_t generation) const {
        //if constexpr (std::is_invocable_v<allocate_shared_for_overwrite<T, allocator_type>, allocator_type, std::size_t>) {
            //auto output = std::allocate_shared_for_overwrite<T, allocator_type>(alloc, size + 2);
            //reinterpret_cast<std::size_t>(output[0]) = size;
            //reinterpret_cast<std::uintptr_t>(output[1]) = generation;
            //return output;
        //    return {};
        //}
        //else {
            //struct deleter {
            //    using allocator_type = std::pmr::polymorphic_allocator<T>;

            //    void operator()(T* p) {
            //        auto size = reinterpret_cast<std::size_t>(p[0]);
            //        std::allocator_traits<allocator_type>::deallocate(alloc, p, size + 2);
            //    }
            //    allocator_type alloc{};
            //};

            //auto ptr = std::allocator_traits<allocator_type>::template rebind_alloc<T>::allocate(alloc, size + 2);
            auto ptr = alloc.allocate(size + 2);
            std::uninitialized_default_construct_n(ptr, size + 2);
            reinterpret_cast<std::size_t&>(ptr[0]) = size;
            reinterpret_cast<std::uintptr_t&>(ptr[1]) = generation;
            return std::shared_ptr<T[]>{ ptr,
                                         [alloc = alloc](auto* p) mutable {
                                             auto size = reinterpret_cast<std::size_t>(p[0]);
                                             alloc.deallocate(p, size + 2);
                                         },
                                         alloc };
        //}
    } 

    template<typename T>
    class array_viewer {
    public:
        using entry_type = std::remove_reference_t<T>*;

        struct iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = T;
            using pointer = T*;
            using reference = T&;

            iterator(pointer* ptr) : ptr(ptr) {}

            //iterator(iterator it) : ptr(it.ptr) {}

            reference operator*() const { return **ptr; }
            pointer operator->() { return (*ptr); }
            iterator& operator++() { ptr++; return *this; }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator== (const iterator& a, const iterator& b) { return a.ptr == b.ptr; };
            friend bool operator!= (const iterator& a, const iterator& b) { return a.ptr != b.ptr; };

        private:
            pointer* ptr;
        };

        array_viewer(std::shared_ptr<entry_type[]>&&, std::size_t);
        array_viewer(const std::shared_ptr<entry_type[]>&, std::size_t);

        array_viewer(array_viewer&&) = default;
        array_viewer(const array_viewer&) = default;

        array_viewer& operator=(array_viewer&&) = default;
        array_viewer& operator=(const array_viewer&) = default;

        iterator begin();

        iterator end();

        std::size_t size();


    private:
        std::shared_ptr<entry_type[]> arr;
        std::size_t sz;
    };

    template<typename T>
    array_viewer<T>::array_viewer(std::shared_ptr<entry_type[]>&& arr, std::size_t size)
        : arr{std::move(arr)}
        , sz{size}
    {}

    template<typename T>
    array_viewer<T>::array_viewer(const std::shared_ptr<entry_type[]>& arr, std::size_t size)
        : arr{ arr }
        , sz{ size }
    {}

    template<typename T>
    typename array_viewer<T>::iterator array_viewer<T>::begin() {
        return iterator{ arr.get() };
    }

    template<typename T>
    typename array_viewer<T>::iterator array_viewer<T>::end() {
        return iterator{ std::next(arr.get(), sz) };
    }

    template<typename T>
    std::size_t array_viewer<T>::size() {
        return sz;
    }
    
    using atomic_counter_t = waitable_atomic<uint64_t>;
    using atomic_uintptr_t = waitable_atomic<uint64_t>;

    /*
        Technically needed because the lifetime of the event class ends
        when the destructor is called and would lead to UB if an attempt
        to unsubscribe was made mid destruction.

        The solution is to handle all the un-subscribing in event and then 
        call the implicit destructor of event_base after there are no more 
        references being held by listeners
    */
    template<typename... Params>
    struct event_base {
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

namespace rpt {

    template<typename... Params>
    class event;

    template<typename Callback, typename... Params>
    class listener : event_detail::listener_base<Params...> {
    public:
        template<typename CB>
        listener(event<Params...>&, CB&&);

        listener() = delete;
        listener(const listener&) = delete;
        listener(listener&&) = delete;
        listener& operator= (const listener&) = delete;
        listener& operator= (listener&&) = delete;

        ~listener();
    private:
        using base_type = typename event_detail::listener_base<Params...>;
        using event_base_type = typename event_detail::event_base<Params...>;
        using repudiate_fn_type = void(*)(event_base_type&, base_type&);

        event_base_type const & event_ref; 
        std::atomic<repudiate_fn_type> repudiate_fn;
        Callback cb;
    };

    template<typename Callback, typename... Params>
    template<typename CB>
    listener<Callback, Params...>::listener(event<Params...>&, CB&&)
        : base_type{
            [](base_type& l, Params... params) {
                static_cast<listener<Callback, Params...>&>(l).cb(params...);
            },
            [](base_type& l)
                return static_cast<listener<Callback, Params...>&>(l).repudiate_fn.exchange(nullptr) != nullptr;
            }
    {
    }

    template<typename Callback, typename... Params>
    listener<Callback, Params...>::~listener() {
        if (auto stash = repudiate_fn.exchange(nullptr)) {

        }
        else {
            stash(event_ref, *this);
        }
    }

    template<typename... Params>
    class event : event_detail::event_base<Params...>{
    public:
        event() = default;
        event(const event&) = delete;
        event(event&&) = delete;
        event& operator=(const event&) = delete;
        event& operator=(event&&) = delete;

        ~event();

        template<typename... Args>
        void operator()(Args&&...);

        template<typename Callback>
        listener<Callback> subscribe(Callback&&);
    private:
        using base_type = event_detail::event_base<Params...>;

        std::atomic_uintptr_t invokers{ 0 };
    };

    template<>
    class event<void> : event_detail::event_base<> {
    public:
        event() = default;
        event(const event&) = delete;
        event(event&&) = delete;
        event& operator=(const event&) = delete;
        event& operator=(event&&) = delete;

        ~event();

        void operator()();

        template<typename Callback>
        listener<Callback> subscribe(Callback&&);
    private:
        using base_type = event_detail::event_base<>;

        std::atomic_uintptr_t invokers{ 0 };
    };

    template<typename... Params>
    event<Params...>::~event() {
        base_type::clear();
    }

    event<void>::~event() {
        base_type::clear();
    }

    template<typename... Params>
    template<typename... Args>
    void event<Params...>::operator()(Args&&... args) {
        base_type::operator()(std::move(args)...);
    }

    void event<void>::operator()() {
        base_type::operator()();
    }

    template<typename... Params>
    template<typename Callback>
    listener<Callback> event<Params...>::subscribe(Callback&& cb)
    {
        return listener<Callback>(*this, std::move(cb));
    }
}

#endif //RPT_EVENT