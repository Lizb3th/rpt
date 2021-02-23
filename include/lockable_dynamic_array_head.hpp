#ifndef RPT_HEADDED_IMMUTABLE_DYNAMIC_ARRAY
#define RPT_HEADDED_IMMUTABLE_DYNAMIC_ARRAY

#include <atomic>
#include <cstddef>
#include <memory_resource>
#include <memory>

namespace rpt {

    /*

    */
    template<typename entry_type>
    struct headded_array_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_cv<entry_type>;
        using pointer = entry_type*;
        using reference = entry_type&;

        iterator(pointer ptr) : ptr(ptr) {}

        reference operator*() const { return *ptr; }
        pointer operator->() { return ptr; }
        iterator& operator++() { ptr++; return *this; }
        iterator operator++(T) { iterator tmp = *this; ++(*this); return tmp; }
        friend bool operator== (const iterator& a, const iterator& b) { return a.ptr == b.ptr; };
        friend bool operator!= (const iterator& a, const iterator& b) { return a.ptr != b.ptr; };

    private:
        pointer ptr;
    };

    /// TODO: document the purpose of this
    template<typename head_type, typename entry_type>
    struct headded_array {

        //using iterator = headded_array_iterator<const entry_type>;

        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

        // | imagineary class data{
        // | 
        // | head_type head;
        // | allocator_type& alloc;
        // | entry_type entries[]...;
        // | };

        headded_array()
            : headded_array(allocator_type{})
        {
        }

        headded_array(allocator_type alloc)
            : headded_array(0, alloc) // invoke private constructor
        {
        }

        headded_array(const headded_array& other, entry_type&& type, allocator_type alloc = {})
            : headded_array(other.size_v + 1, alloc) //allocate all and construct the bumper
        {
            try {
                std::uninitialized_copy_n(other.get_entries(), other.size_v, get_entries());
                try {
                    alloc.construct(get_entries() + other.size_v, std::move(type));
                }
                catch (...) {
                    std::destroy_n(get_entries(), other.size_v);
                    throw;
                }
            }
            catch (...) {
                alloc.destroy(&get_bumper());
                alloc.deallocate(data, bumper_size + (entry_size * size_v));
                throw;
            }
        }

        headded_array(const headded_array& other, const_iterator remove, allocator_type alloc = {})
            : headded_array(other.size_v - 1, alloc) //allocate all and construct the bumper
        {
            try {
                auto next = std::uninitialized_copy(other.begin(), remove, get_entries());
                try {
                    std::uninitialized_copy(std::next(remove), other.end(), next);
                }
                catch (...) {
                    std::destroy(other.begin(), next);
                    throw;
                }
            }
            catch (...) {
                alloc.destroy(&get_bumper());
                alloc.deallocate(data, bumper_size + (entry_size * size_v));
                throw;
            }
        }

        ~headded_array() {
            auto alloc = get_allocator();
            std::destroy_n(get_entries(), size_v);
            alloc.destroy(&get_bumper());
        }

        entry_type* get_entries() const {
            return reinterpret_cast<entry_type*>(data + bumper_size);
        }

        // iterator begin() const {
        //     return iterator{get_entries()};
        // }

        // iterator end() const {
        //     return iterator{ get_entries() + size };
        // }

        std::size_t size() {
            return size_v;
        }

        head_type& get_head() {
            return get_bumper().head;
        }

        allocator_type& get_allocator() {
            return get_bumper().alloc;
        }

    private:

        //using mutable_iterator = headded_array_iterator<entry_type>;

        struct bumper {

            template<typename... Args>
            bumper(allocator_type alloc, Args&&... args)
                : alloc{ alloc }
                , head{ std::forward<Args>(args)... }
            {}

            allocator_type alloc;
            head_type head;
        };

        bumper& get_bumper() {
            return *reinterpret_cast<bumper*>(data);
        }

        // mutable_iterator begin() {
        //     return iterator{ get_entries() };
        // }

        // mutable_iterator end() {
        //     return iterator{ get_entries() + size };
        // }

        constexpr static std::size_t bumper_size = sizeof(head_type);
        constexpr static std::size_t entry_size = sizeof(entry_type);

        explicit headded_array(std::size_t size, allocator_type alloc)
            : data{ alloc.allocate(bumper_size + (entry_size * size)) }
            , size_v{ size }
        {
            try {
                alloc.construct(&get_bumper(), alloc);
            }
            catch (...) {
                alloc.deallocate(data, bumper_size + (entry_size * size_v));
                throw;
            }
        }

        std::byte* data;
        std::size_t size_v;
    };

    template<typename T>
    class lockable_array_view;

    // simple guard to maintain ref count while view exists
    template<typename T>
    struct count_guard {
        count_guard(T& t) noexcept(std::is_nothrow_invocable_v<T::operator++()>)
            : ptr{ &t++ }
        {}

        count_guard(const count_guard&) = delete;
        count_guard(count_guard&& other) noexcept
            : ptr{other.ptr}
        {
            other.release();
        } 

        ~count_guard() noexcept(std::is_nothrow_invocable_v<T::operator--()>) {
            if(auto target = ptr)
                target--;
        }

        void release() noexcept {
            ptr = nullptr;
        }

        T* ptr
    }

    /*
    *   iteration ref counted dynamic imutable array
    */
    template<typename T>
    class lockable_array {
    public:
        using view_type = lockable_array_view<T>;

        //wait free generation of the underlaying view (assuming data_ptr_type load is wait free)
        view_type lock() {
            locking_count++;
            auto lock = count_guard{data.load().get_head()}; //lock the array to prevent destruction
            locked_count++;

            return view_type{std::move(lock), data};
        }

        // atomicaly replace data with a complete copy plus an additional entry.
        // New entry must be copyable.
        // returns after all views of the previus data heve returned
        void push_back( T&& new_entry ) {
            auto view = lock();
            while (!compare_exchange(view.data, _array_data<T>(view, new_entry, alloc))) {
                view = lock();
            }
            wait_for_locking_out(locking_in.load());

            //view.data.wait_on_views(1);
        }

        // atomicaly replace data with a complete copy minus a specified entry.
        // returns after all views of the previus data heve returned
        void remove( T remove) {
            
        }

    private:
        using ref_count_type = std::atomic_uint64_t;
        using data_ptr_type = std::atomic<headded_array<ref_count_type, T>>;

        ref_count_type locking_count {0};
        ref_count_type locked_count {0};

        data_ptr_type data; 
    }

    template<typename T>
    class lockable_array_view {
    public:

        class iterator {};

        lockable_array_view(const lockable_array_view& other)
            : data = other.data
        {
        }

        iterator being();

        iterator end();

    private:
        friend class lockable_array<T>;
        using lockable_array::ref_count_type;

        lockable_array_view(count_guard<T>&& lock, const headded_array<ref_count_type, T>& arr)
            : data{arr.get_data()}
            , size{arr.size()}
            , lock{std::move(lock)}
        {}

        T* data;
        std::size_t size;
        count_guard<ref_count_type> lock;
    }

}
#endif // RPT_HEADDED_IMMUTABLE_DYNAMIC_ARRAY