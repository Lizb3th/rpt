// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_WAITABLE_ATOMIC
#define RPT_DETAIL_WAITABLE_ATOMIC

#include <atomic>
#include <condition_variable>

namespace rpt::event_detail {

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
        void lock_unlock(); //force single point synchronisation 

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
        lock_unlock();
        cv.notify_all();
    }

    template<typename T>
    void waitable_atomic<T>::lock_unlock()
    {
        auto lock = std::lock_guard{ mtx };
    }

    template<typename T>
    void waitable_atomic<T>::wait(T old, std::memory_order order) const {
        if (atom.load(order) != old) {
            auto lock = std::unique_lock{ mtx };
            cv.wait(lock, [&] { return atom.load(order) != old; });
        }
    }

}

#endif // RPT_DETAIL_WAITABLE_ATOMIC