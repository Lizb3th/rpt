# RPT ***Fast Event-Listener***

## 

### **Motivation**

To creat a way of safely listening and un-listening to an event that may occure from a diferent thread.

for example, a high frequency osscilation from an input device that should be acted upon imediatly. 

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
If an event is triggered off a diferent thread to the listener constructing thread, there should be no diference as if it were called from that constructing thread.
