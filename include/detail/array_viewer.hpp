// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_ARRAY_VIEWER
#define RPT_DETAIL_ARRAY_VIEWER

#include <cassert>
#include <memory>

namespace rpt::event_detail {

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
}

#endif // RPT_DETAIL_ARRAY_VIEWER