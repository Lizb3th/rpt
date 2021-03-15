# RPT ***Fast Event-Listener***

## 

### **Motivation**

To creat a way of safely listening and un-listening to a event that may occure from a diferent thread.

for example, a high frequency osscilation from an input device that should be acted apon imediatly. 

the intent is to allow efficient, multi cross thread communication when an event is triggered.

### sample

```C++
template<typename... Args>
class event;

template<typename Callback, typename Params>
class listener;

 ⋮

int get_1() {

    auto my_event = event<int, int>{};

    auto i = 0;
    auto my_listener = my_event.subscribe([&i](auto, auto){
        i++;
    });

    my_event(1,2);

    return i;
}

int get_6() {

    auto my_event = event<int, int>{};

    auto i = 0;
    auto my_listener_dif = my_event.subscribe([&i](auto a, auto b) {
        i += a - b;
    });

    auto my_listener_sum = my_event.subscribe([&i](auto a, auto b) {
        i += a + b;
    });

    my_event(3,1);

    return i;
}
```

### Detail
If an event is triggered of a diferent thread to the listener constructing thread, there should be no diference as if it was called from that constructing thread.



#### PROBLEM
how to handle the lifetime of a stack based function?

* option 1 no heap allocation
  *


#### Test Code

``` C++
template<typename T>
struct atomic_shared_ptr {

    using value_type = std::shared_ptr<T>;

    value_type load(std::memory_order order = std::memory_order_seq_cst) {
        return std::atomic_load_explicit(&p, order);
    }

    bool compare_exchange_strong(
        value_type& expected,
        value_type desired,
        std::memory_order order = std::memory_order_seq_cst 
        ) noexcept {
        return atomic_compare_exchange_strong_explicit(p, &expected, desired, order);
    }

    value_type p;
};

/*
    this is going to be less efficient than std::allocate_shared
    also, not safe to use anywhere else
*/

template< class T, class Alloc >
shared_ptr<T> allocate_shared( const Alloc& alloc, std::size_t N ) {

    struct deleter {
        void operator()(T* a) {
            alloc.destroy_n(a, size);
        }
        const std::size_t size;
        Alloc& alloc;
    }

    auto output = std::shared_ptr{nullptr, deleter{N, alloc}, alloc};

    output.reset(std::allocator_traits<Alloc>::allocate(alloc, n));

    return output;
}
```

``` C++
template<typename... Params>
struct listener_base {
    void(*_callback)(listener_base&, Params...);

    template<typename Args...>
    void operator()(Args... args) {
        _callback(*this, std::move(args)...);
    }
};

template<typename Callback, typename... Params>
class listener : listener_base<Params...>{

    static_assert(std::is_invocable_r_v<void, Callback, Params...>, "Callback type must be callable with Param... pack");

    using base_type = listener_base<Params...>;
    using event_type = class event<Params...>;

    struct event_repudeator{
        void operator()(event_type* e){
            e->repudiate(l);
        }

        listener& l;
    };

public:
    listener()
        : base_type {
            [](base_type& _this, Params... params) {
                static_cast<listener&>(_this)._callback(std::move(params)...);
            }
        }
    {
    }

    template<typename C>
    listener(event<Params...>& e, C&& cb)
        : base_type { [](base_type& _this, Params... params) {
                static_cast<listener&>(_this)._callback(std::move(params)...);
        }}
        , _callback{std::forward<C>(cb)}
        , _eventptr{&e, *this}
    {
    }

    using callback_type = Callback;

private:

    callback_type _callback;
    std::unique_ptr<event_type, event_repudeator> _eventptr;
};
```

```C++
namespace {
    template<typename T>
    class const_pointer_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T*;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        const_iterator(entry_type<T>* ptr) : ptr(ptr) {}

        //const_iterator(iterator<T> it) : ptr(it.ptr) {}

        reference operator*() const { return *ptr; }
        pointer operator->() { return *ptr; }
        const_iterator& operator++() { ptr++; return *this; }  
        //const_iterator operator++(T) { const_iterator tmp = *this; ++(*this); return tmp; }
        friend bool operator== (const const_iterator& a, const const_iterator& b) { return a.ptr == b.ptr; };
        friend bool operator!= (const const_iterator& a, const const_iterator& b) { return a.ptr != b.ptr; };  

    private:
        pointer ptr;

    };
}
```

``` C++
template<typename... Param>
class event {
    using listener_type = listener_base<Param...>;
    using listener_entry = listener_type*
    using clistener_iterator = const_pointer_iterator<listener_type>;
    using array_type = std::shared_ptr<listener_entry[]>;
    using atomic_array_type = atomic_shared_ptr<listener_entry[]>;
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

    event() : event(allocator_type{}) {}
    explicit event(allocator_type alloc) : alloc{alloc} {}

    template<typename Callback>
    listener<Callback, Param...> subscribe(Callback);

    template<typename Args...>
    void operator()(Args... args) {
        static_assert(std::is_copyable_v<Args> && ... );

    }

    allocator_type get_allocator(){
        return alloc;
    }

private:

    void repudiate(listener_entry);

    array_type lock();

    /*
    *   going to convert the reference into a pointer
    */
    array_type push_copy(const array_type&, listener_type&);

    array_type erase_copy(const array_type&, listener_entry*);

    array_type make_array(listener_type&);

    atomic_array_type ptr;

    allocator_type alloc;
};
```

#### event constructor
```C++
template<>
event::event() 
    : event{}
{
}

template<>
event::event(allocator_type alloc) 
    : event{}
{
}
```

#### Subscribe
```C++
template<typename... Args>
template<typename Callback>
listener<Callback, Args...> event<Args>::subscribe<Callback>(Callback callback) {
    static_assert(sizeof...(Args...) != 0);
    return listener<Callback, Args...>{};
}
```

# Code

## Detail


## Listener Base object

Simple struct to hold callback for calling function reference to invoke listener class callback.

```C++
template<typename... Params>
struct listener_base {
    void(&_callback)(listener_base&, Params...);

    template<typename Args...>
    void operator()(Args... args) const {
        _callback(*this, std::move(args)...);
    }
};
```

publicaly accessable member function to call stored function reference which will 
call stored function in listener.

Params must be constructable from args.
```C++
template<typename... Params>
template<typename... Args>
void listener_base<Params...>operator<Args...>()(Args... args) {
    _callback(*this, std::move(args)...);
}
```


## Listener object

Container for callable callback that can be subscibed to an event

```C++
template<typename Callback, typename... Param>
class listener : listener_base<Param...> {
public:
    template<typename C>
    listener(event<Params...>& e, C&& cb);

    ~listener();

private:
    using base = listener_base<Param...>;
    using repudiate_fn_type = void(*base);

    std::atomic<repudiate_fn_type> 

};
```

Callback must be constructable from C.                          \
Callback must be callable with Params... and return void.

```C++
template<typename Callback, typename... Param>
template<typename C>
listener<Callback, Param...>::listener<C>(event<Params...>& e, C&& cb)
    : base_type { [](base_type& _this, Params... params) {
            static_cast<listener&>(_this)._callback(std::move(params)...);
    }}
    , _callback{std::forward<C>(cb)}
    , _eventptr{&e, *this}
{
}
```

```C++
template<typename Callback, typename... Param>
listener<Callback, Param...>::~listener()
{
}
```
## Listener array 

```C++
template<typename... Params>
using listener_array_entry = const std::reference_wrapper<listener_base<Params...>>;

template<typename... Params>
using listener_array = std::shared_ptr<listener_array_entry<Params...>>;
```



## Atomic listener array

```C++
struct atomic_listener_array{
    
    template<typename... Params>
    static size(const listener_array<Params...>&);

    template<typename... Params>
    static generatrion(const listener_array<Params...>&);

    template<typename... Params>
    static copy_push(const listener_array<Params...>&);

    template<typename... Params>
    static copy_erase(const listener_array<Params...>&);
};
```

```C++
// template<typename T>
// class atomic_listener array {
//     using listner_p = T*;

//     std::shared_ptr<listner_p> load();

//     std::size_t size();

//     std::size_t 

//     atomic_shared_ptr<listner_p*> array;
// };
```


## Listener array viewer

simple object to formalize the lifetime extension of the array of listeners.

A viewer on it's own must not be the last object holding the array.

```C++
template<typename... Params>
class listeners_view {
public:
    using iterator = *listener_array_entry;

    [[nodiscard]] iterator begin();

    [[nodiscard]] iterator end();

    [[nodiscard]] bool empty();

    [[nodiscard]] std::size_t size();

private:
    std::size_t length;
    listener_array<Params...> lock;
}
```

```C++
template<typename... Params>
listeners_view<Params...>::iterator listeners_view<Params...>::begin() {
    return lock.get();
}
```

```C++
template<typename... Params>
listeners_view<Params...>::iterator listeners_view<Params...>::end() {
    return std::next(lock.get(), size);
}
```

```C++
template<typename... Params>
std::size_t listeners_view<Params...>::size() {
    return length;
}
```

```C++
template<typename... Params>
bool listeners_view<Params...>::empty() {
    return length == 0;
}
```

## Event object
```C++
template<typename... Params>
class event {
    using listener_type = listener_base<Params...>;
    using listener_entry = listener_type*
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

    event() : event(allocator_type{}) {}
    explicit event(allocator_type alloc) : alloc{alloc} {}

    template<typename Callback>
    listener<Callback, Params...> subscribe(Callback);

    template<typename... Args>
    void operator()(Args... args);

    allocator_type get_allocator();
private:

    void _repudiate(listener_entry);

    array_type lock();

    /*
    *   going to convert the reference into a pointer
    */
    array_type copy_push(const array_type&, listener_type&);

    array_type copy_erase(const array_type&, listener_entry*);

    array_type make_array(listener_type&);

    atomic_array_type ptr;

    /*
    *   Indecate what generation of array can be safely deleted if it has no current listners.
    */
    atomic_size_t gen{0};

    allocator_type alloc;
};
```

```C++
template<typename... Params>
template<typename Callback>
listener<Callback, Params...> event<Params...>::subscribe(Callback callback) {
    auto output = listener<Callback, Params...>{&this, std::move(callback)};
    
    for(auto current = ptr.load(); !ptr.compare_exchange(current,copy_push(current, output)););

    return output;
}
```

```C++
template<typename... Params>
template<typename... Args>
void event<Params...>::operator<Args...>()(Args... args) {
    auto view = lock();

    if(!view.empty()) {
        std::for_each(view.begin(), std::prev(view.end()), [args...](auto l) {
            std::invoke(l, args);
        });
        std::invoke(view.back(), std::move(args));
    }
}
```

```C++
template<typename... Params>
event<Params...>::allocator_type event<Params...>::get_allocator() {
    return alloc;
}
```

```C++
template<typename... Params>
void event<Params...>::_repudiate(listener_entry listener) {

    for(auto current = ptr.load(); 
        !ptr.compare_exchange(
            current, 
            copy_erase(current, std::find(view.begin(), view.end(), listener)));
    );

    return output;
}
```

```C++
template<typename... Params>
event<Params...>::array_type event<Params...>::copy_push(const array_type&, listener_type&) {
    
}
```

```C++
template<typename... Params>
event<Params...>::array_type event<Params...>::copy_erase(const array_type& copy, listener_entry*){
    if(auto new_size = copy.size(); size > 1) {
        auto output = allocate_shared<listener_type[]>(array_type.size() + 2)
    }
    else 
        return nullptr;
}
```