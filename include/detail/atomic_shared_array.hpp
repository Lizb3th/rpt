// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_ATOMIC_SHARED_ARRAY
#define RPT_DETAIL_ATOMIC_SHARED_ARRAY

#include <memory>
#include <atomic>

namespace rpt::event_detail {

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

    template<typename T>
    using atomic_shared_array_t = typename atomic_shared_array<T>::type;

}

#endif // RPT_DETAIL_ATOMIC_SHARED_ARRAY