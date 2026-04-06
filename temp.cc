
#include <cstddef>
#include <cstdio>
#include <cstdlib>
template<size_t T,size_t... Rest>
struct getMax {
    static constexpr size_t value = T > getMax<Rest...>::value 
    ? T : getMax<Rest...>::value;
};
template<size_t T>
struct getMax<T> { static constexpr size_t value = T; };

template <typename T, typename... Types>
struct match;

// Match found!
template <typename T, typename... Rest>
struct match<T, T, Rest...> {
    static constexpr int value = 0;
};

// Not a match, keep looking...
template <typename T, typename U, typename... Rest>
struct match<T, U, Rest...> {
    static constexpr int value = 1 + match<T, Rest...>::value;
};

template <typename T> struct removeArray      { using type = T;};
template <typename T> struct removeArray<T[]> { using type = T;};

template<typename... Types>
struct variant{
    static constexpr size_t data_size = getMax<sizeof(Types)...>::value;
    static constexpr size_t data_align = getMax<alignof(Types)...>::value;
    alignas(data_align) char buffer[data_size];
    int type_index = -1;

    template <size_t Index, typename T, typename... Rest>
    void destroy_at_index(int target) {
        if (Index == target) {
            static_cast<T*>(static_cast<void*>(buffer))->~T();
        } else if constexpr (sizeof...(Rest) > 0) {
            destroy_at_index<Index + 1, Rest...>(target);
        }
    }
    void destroy_current() {
        if (type_index == -1) return;
        
        // Start the recursive search for the correct type to destroy
        destroy_at_index<0, Types...>(type_index);
        
        type_index = -1; 
    }
    template <typename T>
    void set(T&& value) {
        // 1. Destroy whatever was there before
        if (type_index != -1)
        {
            destroy_current();
        }
        // T* ptr = static_cast<T*>(static_cast<void*>(buffer));
        // *ptr = static_cast<T&&>(value);
        // 2. "Placement New" - Construct T exactly at the address of our buffer
        new (buffer) T(static_cast<T&&>(value));
        
        // 3. Update the index so we know how to delete it later
        type_index = match<T,Types...>::value; 
    }
    template <typename T>
    inline T& get(int e = __LINE__) {
        // 1. Get the compile-time index of type T
        constexpr int target_index = match<T, Types...>::value;
        
        // 2. Runtime Check: Does it match what's actually in the buffer?
        if (type_index != target_index) {
            // In a no-stdlib environment, you might log an error or assert
            // instead of throwing an exception.
            printf( "%d  Invalid variant access! \n", e);
            std::exit(1);
        }
        // 3. The Magic Trick: Reinterpret Cast
        // We treat the address of 'buffer' as the address of an object of type T.
        return *static_cast<T*>(static_cast<void*>(buffer));
    }
    template <typename T>
    const T& get() const {
        constexpr int target_index = match<T, Types...>::value;
        if (type_index != target_index) printf("Invalid access");

        return *static_cast<T*>(static_cast<void*>(buffer));
    }
    ~variant() { destroy_current();}
};