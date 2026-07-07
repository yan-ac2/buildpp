#include <cstdio>

using size_t = decltype(sizeof(0));
using nullptr_t = decltype(nullptr);
using f32 = decltype(0.0f);
using f64 = decltype(0.0);

template <class Ty, Ty Val>
struct integral_constant {
    enum : Ty {value = Val};
    using value_type = Ty;
    using type       = integral_constant;
    
    consteval operator value_type() const noexcept   {return static_cast<value_type>(value);}
    consteval value_type operator()() const noexcept {return static_cast<value_type>(value);}
};
template <bool Val> struct bool_constant : integral_constant<bool,Val> {};
using false_t = bool_constant<false>;
using true_t  = bool_constant<true>;

template<class T,class U> struct is_same : false_t{};
template<class T> struct is_same<T,T>    : true_t {};

template<typename  T,typename U> struct is_enum : false_t {};


template<typename T> struct is_ptr     :false_t {};
template<typename T> struct is_ptr<T*> :true_t  {};

template <typename T, typename... Args>
constexpr size_t count_occurrences() {
    return (size_t(is_same<T, Args>::value) + ... + 0);
}

// Helper trait to check if any type in the pack has a duplicate
template <typename... Args>
constexpr bool has_duplicates() {
    // For every type in Args, check if its occurrence count is greater than 1
    return ( (count_occurrences<Args, Args...>() > 1) || ... || false );
}

template <size_t N>
struct staticString {
    char string[N]{}; // Initialize to clear garbage

    // Constructor copies the string literal (excluding the null terminator)
    constexpr staticString(const char (&str)[N + 1]) {
        for (size_t i = 0; i < N; ++i) {
            string[i] = str[i];
        }
    }

    // Helper to expose the size if needed externally
    constexpr size_t size() const { return N; }
};
template <size_t N>
staticString(const char (&)[N]) -> staticString<N - 1>;

template <typename Type,size_t ID, staticString T = "">
struct varUtl {
    using type = Type;
    static constexpr auto id = ID;
    static constexpr auto name = T.string;
    // Fixed the constructor and matched the string literal size to T's size + 1
    // constexpr varUtl(size_t, const char (&)[T.size() + 1]) : value(ID) {}
    constexpr bool operator == (auto other) {
        return is_same<type, typename decltype(other)::type>::value ? true : false;
    }
};

template<varUtl... V>
struct StaticMap {
    static_assert(!has_duplicates<typename decltype(V)::type...>(), 
                  "StaticMap Error: Duplicate types are not allowed!");
    template<typename T> requires ((is_same<T, typename decltype(V)::type>()) || ...)
    static constexpr const char* getname(T) {
        const char* result = nullptr;
        
        // C++17 Fold Expression to search through the template pack at compile time
        ((is_same<T, typename decltype(V)::type>::value ? (result = decltype(V)::name) : nullptr), ...);
        
        return result;
    } 
};

using type_t = StaticMap<
    varUtl<void,0,"void">{},
    varUtl<nullptr_t,1,"nullptr">{},
    varUtl<int,2,"int">{},
    varUtl<f32,3,"float">{},
    varUtl<double,4,"double">{}
>;
auto main() -> int {
    double var = 12.123;
    printf("%s",type_t::getname(1.689f + 2));
    return 0;
}