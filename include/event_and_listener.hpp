#ifndef RPT_EVENT_AND_LISTENER
#define RPT_EVENT_AND_LISTENER

/*
*	Embedded system event handler.
*
*	Specificaly in order to not block event invocation.
*
*   REQUIRES C++17
*/

//#include "lockable_dynamic_array_head.hpp"

namespace rpt::_detail {

	/*
	*/
	template<typename... Args>
	struct _listener_base_interface {

		virtual void invoke(Args... ) = 0;

		virtual ~_listener_base_interface() = default;
	};

	struct _event_base_interface
	{
		virtual void repudiate(_listener_base_interface*) = 0;

		virtual ~_event_base_interface() = default;
	}

	template<typename... Args>
	class _listener_base: public _listener_base_interface<Args...>  {

		_listener_base(event<Args...>& e);

		_listener_base(event<void>& e);

		_listener_base(std::weak_ptr<event_base<Args...>> event)
			: eventptr( std::move(event) )
		{
		}

		_listener_base(const _listener_base&) = delete;
		_listener_base(_listener_base&&) = delete;

		_listener_base& operator=(const _listener_base&) = delete;
		_listener_base& operator=(_listener_base&&) = delete;

		~_listener_base() {
			if(auto event = eventptr.lock())
				event->repudiate(this);
		}

		std::atomic_bool inuse{false};

		const std::weak_ptr<event_base<Args...>> eventptr;
	};

	template<typename... Args>
	class _event_base : private _event_base_interface<Args...> {
		void repudiate(_listener_base_interface*) override final {
			
		}
	}

} // namespace rpt::_detail

namespace rpt {

	/*

	*/
	template<typename Callback, typename... Args>
	class listener : private detail::_listener_base<Args...> {

	};

	template<typename... Args>
	class event : private detail::_event_base{

	};
} // namespece rpt
#endif // RPT_EVENT_AND_LISTENER