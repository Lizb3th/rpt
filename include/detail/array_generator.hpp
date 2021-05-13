// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_ARRAY_GENERATOR
#define RPT_DETAIL_ARRAY_GENERATOR

#include <cassert>
#include <algorithm>

namespace rpt::event_detail {

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

        auto output = make_array_for_overwrite(size, get_generation(arr) + 1);
        std::remove_copy(get_data(arr), std::next(get_data(arr), size + 1), get_data(output), &t);
        return output;
    }

    template<typename T>
    std::size_t array_generator<T>::get_size(const array_type& arr) {
        assert(arr != nullptr);

        return reinterpret_cast<std::size_t>(arr[0]);
    }

    template<typename T>
    std::uintptr_t array_generator<T>::get_generation(const array_type& arr) {
        assert(arr != nullptr);

        return reinterpret_cast<std::uintptr_t>(arr[1]);
    }

    template<typename T>
    T* array_generator<T>::get_data(const array_type& arr) {
        assert(arr != nullptr);

        return std::next(arr.get(), 2);
    }

    template<typename T>
    typename array_generator<T>::array_type array_generator<T>::make_array_for_overwrite(std::size_t size, std::uintptr_t generation) const {

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
    } 

}

#endif // RPT_DETAIL_ARRAY_GENERATOR