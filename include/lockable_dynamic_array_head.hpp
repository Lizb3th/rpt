#ifndef RPT_LOCKABLE_DYNAMIC_ARRAY
#define RPT_LOCKABLE_DYNAMIC_ARRAY

#include <atomic>
#include <cstddef>
#include <memory_resource>
#include <memory>

/// TODO: document the purpose of this
template<typename head_type, typename entry_type>
struct headded_array {

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
        : headded_array(0, alloc)
    {
    }

    explicit headded_array(const headded_array& other, entry_type&& type, allocator_type alloc = {})
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

    ~headded_array() {
        auto alloc = get_allocator();
        std::destroy_n(get_entries(), size_v);
        alloc.destroy(&get_bumper());
    }

    entry_type* get_entries() const {
        return reinterpret_cast<entry_type*>(data + bumper_size);
    }

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

#endif // RPT_LOCKABLE_DYNAMIC_ARRAY