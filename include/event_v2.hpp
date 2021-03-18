// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_EVENT
#define RPT_EVENT

#include<atomic>
#include<memory>
#include<algorithm>

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

        callback_ref_type _callback;

        template<typename... Args>
        void operator()(Args...);
    };

    template<typename... Params>
    template<typename... Args>
    void listener_base<Params...>::operator()(Args... args) {
        _callback(*this, Params(std::move(args))...);
    }


    /*
    *   listener_shared_array
    */
    template<typename T, typename U = void>
    struct atomic_shared_array {
        class detail {
        public:
            std::shared_ptr<T[]> load(std::memory_order order = std::memory_order_seq_cst) noexcept { return std::atomic_load_explicit(&data, order); }

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
        : alloc{ alloc } {}

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::make_array(std::remove_pointer_t<T>& r) const {
        auto output = make_array_for_overwrite(1, 1);
        output[2] = &r;
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
        if (size != 0)
        {
            auto output = make_array_for_overwrite(size, get_generation(arr) + 1);
            std::remove_copy(get_data(arr), std::next(get_data(arr), size + 1), get_data(output), &t);
            return output;
        }
        return nullptr;
    }

    template<typename T>
    std::size_t array_generator<T>::get_size(const array_type& arr) {
        return reinterpret_cast<std::size_t>(arr[0]);
    }

    template<typename T>
    std::uintptr_t array_generator<T>::get_generation(const array_type& arr) {
        return reinterpret_cast<std::uintptr_t>(arr[1]);
    }

    template<typename T>
    T* array_generator<T>::get_data(const array_type& arr) {
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

        /*
            get a copy of hte list that cannot be detructed while the shared pointer (array_type) is held.
        */
        array_type lock();

        /*
            Add a listener to the list of subscribers
            from the start of this function call the lisener may be invoked.
        */
        void subscribe(listener_base<Params...>&);

        /*
            Clear the list and wait for it to be safe to destruct event_base
        */
        void clear();

    private:
        using atomic_array_type = atomic_shared_array_t<listener_base<Params...>*>;

        array_generator<listener_base<Params...>*> generator;

        std::atomic_uintptr_t free_generation{ 0 };
        atomic_array_type data{};
    };

    template<typename... Params>
    void event_base<Params...>::repudiate(listener_base<Params...>& l) {
        auto holder = lock();
        assert(holder);
        auto next = generator.copy_remove(holder, l);
        while (!data.compare_exchange_strong(holder, next))
        {
            next = generator.copy_remove(holder, l);
        }
    }

    template<typename... Params>
    typename event_base<Params...>::array_type event_base<Params...>::lock() {
        return data.load();
    }

    template<typename... Params>
    void event_base<Params...>::subscribe(listener_base<Params...>& l) {
        auto holder = lock();
        auto next = holder ? generator.copy_push_back(holder, l) : generator.make_array(l);
        while (!data.compare_exchange_strong(holder, next))
        {
            next = holder ? generator.copy_push_back(holder, l) : generator.make_array(l);
        }
    }

    template<typename... Params>
    void event_base<Params...>::clear() {

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
    }}
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
        event();
        event(const event&) = delete;
        event(event&&) = delete;
        event& operator=(const event&) = delete;
        event& operator=(event&&) = delete;

        template<typename... Args>
        void operator()(Args&&...);

        template<typename Callback>
        listener<Callback> subscribe(Callback&&);
    };

    template<typename... Params>
    template<typename... Args>
    void event<Params...>::operator()(Args&&...)
    {

    }

    template<typename... Params>
    template<typename Callback>
    listener<Callback> event<Params...>::subscribe(Callback&& cb)
    {
        return listener<Callback>(*this, std::move(cb));
    }
}

#endif //RPT_EVENT