// This is a part of the RPT (Realy Poor Tech) Framework.
// Copyright (C) Elizabeth Williams
// All rights reserved.

#ifndef RPT_LISTENER
#define RPT_LISTENER

#include "detail/listener_base.hpp"

namespace rpt::event_detail {
    template<typename... Params>
    struct event_base;
}

namespace rpt {
    template<typename... Params>
    class event;

    template<typename Callback, typename... Params>
    class listener : event_detail::listener_base<Params...> {
    public:

        template<typename CB>
        explicit listener(event<Params...>&, CB&&);

        listener() = delete;
        listener(const listener&) = delete;
        listener(listener&&) = delete;
        listener& operator= (const listener&) = delete;
        listener& operator= (listener&&) = delete;

        ~listener();
    private:
        using base_type = typename event_detail::listener_base<Params...>;
        using event_base_type = typename event_detail::event_base<Params...>;
        //using event_base_type = typename event_detail::event_base<Params...>;
        using repudiate_fn_type = void(*)(event_base_type&, base_type&);

        event_base_type& event_base_ref;
        std::atomic<repudiate_fn_type> repudiate_fn = [](event_base_type& ref, base_type& base) {
            ref.repudiate(base);
        };
        Callback cb;
    };

    template<typename... E, typename Fn>
    listener(event<E...>&, Fn&&)->listener<Fn, E...>;

    template<typename Callback, typename... Params>
    template<typename CB>
    listener<Callback, Params...>::listener(event<Params...>& event_ref, CB&& cb)
        : base_type{
            //TODO later make these static functions
            [](base_type& l, Params... params) {
                static_cast<listener<Callback, Params...>&>(l).cb(params...);
            },
            [](base_type& l) {
                return static_cast<listener<Callback, Params...>&>(l).repudiate_fn.exchange(nullptr) != nullptr;
            } }
        , event_base_ref{ event_detail::get_event_base<Params...>::get(event_ref) }
        , cb{ cb }
    {
                event_base_ref.subscribe(*this);
    }

    template<typename Callback, typename... Params>
    listener<Callback, Params...>::~listener() {
        if (auto stash = repudiate_fn.exchange(nullptr)) {
            stash(event_base_ref, *this);
        }
        else {

        }
    }
}

#endif // RPT_LISTENER