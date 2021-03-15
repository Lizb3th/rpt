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

	template<typename... Args>
	struct _listener_base
	{
		/*
			invoke the listener callback
		*/
		virtual void operator(Args&&... args);
	};

	template<typename... Args>
	struct _event_base
	{
		/*
			Un-subscrube from the event
		*/
		virtual void repudiate(const _listener_base<Args>*); //maybe should be ref
	};

	template<typename entry_type>
	class _lock_array_view {

		using const_iterator = class _lock_array_const_iterator;

		const_iterator begin();

		const_iterator end();

		const entry_type& back() const;

		std::size_t size();
	};

	template<typename entry_type>
	struct _lock_array {
		_lock_array_view lock();

		template<typename T>
		_lock_array_view.remove(const T& value);
	};

} // namespace rpt::_detail

namespace rpt {

	template<typename... Args>
	class event;

	/*

	*/
	template<typename Callback, typename... Params>
	class listener : _listener_base{
	public:
		listener() = default;
		listener(listener& const) = delete;
		listener(listener&&) = delete;
		listener& operator=(listener& const) = delete;
		listener& operator=(listener&&) = delete;

		/*
		*
		*/
		template<typename Forward>
		listener(event<Params>&, Forward&&);

	private:
		friend class event<Params...>;

		void operator()() override;

		template<typename... Args>
		void operator()(Args...&& args) override;

		Callback callback;
		std::weak_ptr<event<Params...>> event_ptr;
	};

	template<>
	template<typename Forward>
	listener<>::listener(event<Params>& e, Forward&& forward) 
		: callback{ std::forward<callback>(forward) }
		, event_ptr{ e.get_weak() }
	{
	}

	template<typename... Param>
	class event : _event_base<Params...>{
	public:
		event() = default;
		event(event& const) = delete;
		event(event&&) = delete;
		event& operator=(event& const) = delete;
		event& operator=(event&&) = delete;

		template<typename Forward, typename Callback>
		listener<Callback, Args...> subscribe(Forward&& forword);

		void operator()();

		template<typename... Args>
		void operator()(Args...&&);

	private:
		void repudiate(const _listener_base<Params...>&);

		std::weak_ptr<event<Params...>> get_weak();

		_lock_array<const _listener_base<Params...>&> array;
	};

	template<>
	event<void>::operator()(){
		view = array.lock();
		for_each(view.begin(), view.end(), [](auto& l){
			l();
		});
	}

	template<>
	event<>::operator()() = delete;

	template<typename... Args>
	template<typename... Params>
	event<Args...>::operator()(Params...&& params){
		view = array.lock();
		if(view.size() >= 1) {
			for_each(view.begin(), std::prev(view.end()), [&params...](auto& l){
				l(std::forward<Args>(params)...);
			});
			view.back()(std::forward<Args>(std::move(params)));
		}
	}

	template<typename... Args>
	void event<Args...>::repudiate(const _listener_base<Args...>&) {
		array.remove()
	}

} // namespece rpt
#endif // RPT_EVENT_AND_LISTENER