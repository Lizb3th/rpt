// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_EVENT
#define RPT_EVENT

#include<atomic>
#include<memory>
#include<algorithm>

namespace rpt::detail {

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
        class type {
        public:
            std::shared_ptr<T[]> load();

            void store();

            bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T> desired,
                std::memory_order order = std::memory_order_seq_cst) noexcept;
        private:
            std::shared_ptr<T[]> data;
        };
    };

    template<typename T>
    struct atomic_shared_array<T> :
        std::enable_if<has_std_atomic_shared_ptr::value,
            std::atomic<
                std::shared_ptr<
                    T[]
        >>>{};

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
        auto output = make_array_for_overwrite(size, get_generation(arr) + 1);
        std::remove_copy(get_data(arr), std::next(get_data(arr),size + 1), get_data(output), &t);
        return output;
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
            using value_type = std::remove_reference_t<T>;
            using pointer = std::remove_reference_t<T>*;
            using reference = std::remove_reference_t<T>&;

            iterator(pointer ptr) : ptr(ptr) {}

            //iterator(iterator it) : ptr(it.ptr) {}

            reference operator*() const { return *ptr; }
            pointer operator->() { return ptr; }
            iterator& operator++() { ptr++; return *this; }
            //iterator operator++(T) { iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator== (const iterator& a, const iterator& b) { return a.ptr == b.ptr; };
            friend bool operator!= (const iterator& a, const iterator& b) { return a.ptr != b.ptr; };

        private:
            pointer ptr;
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
        return iterator{ std::next(arr.get(), size) };
    }

    template<typename T>
    std::size_t array_viewer<T>::size() {
        return sz;
    }

}

namespace rpt::event {

}

#endif //RPT_EVENT