// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_DETAIL_LISTENER_BASE
#define RPT_DETAIL_LISTENER_BASE

#include <memory>

namespace rpt::event_detail {
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
}

#endif // RPT_DETAIL_LISTENER_BASE