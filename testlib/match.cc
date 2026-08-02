#include <iostream>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>


// ============================================================================
// 1. FREESTANDING LIGHTWEIGHT COMPILER & METAPROGRAMMING UTILITIES
// ============================================================================

namespace mini_std {
    // --- BASIC TYPES & CONSTANTS ---
    using size_t = decltype(sizeof(0));
    using nullptr_t = decltype(nullptr);
    using ptrdiff_t = decltype([]{ char temp[2]; return (temp + 1) - temp; }());

    template <class Ty, Ty Val>
    struct integral_constant {
        static constexpr Ty value = Val;
        using value_type = Ty;
        using type       = integral_constant;
        
        constexpr operator value_type() const noexcept   { return static_cast<value_type>(value); }
        constexpr value_type operator()() const noexcept { return static_cast<value_type>(value); }
    };

    template <bool Val> struct bool_constant : mini_std::integral_constant<bool, Val> {};
    using false_type = mini_std::bool_constant<false>;
    using true_type  = mini_std::bool_constant<true>;
    template<bool B, typename T = void>
    struct enable_if {};

    template<typename T>
    struct enable_if<true, T> {
        using type = T; 
    };
    // --- UTILITIES & TYPE TRANSFORMATIONS ---
    template <typename...> using void_t = void;

    template <typename T, typename U> struct is_same : mini_std::false_type {};
    template <typename T> struct is_same<T, T> : mini_std::true_type {};
    template <typename T, typename U> constexpr bool is_same_v = mini_std::is_same<T, U>::value;

    template <typename T> struct remove_reference      { using type = T; };
    template <typename T> struct remove_reference<T&>  { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };
    template <typename T> using remove_reference_t = typename mini_std::remove_reference<T>::type;

    template <typename T> struct remove_const          { using type = T; };
    template <typename T> struct remove_const<const T>  { using type = T; };
    template <typename T> using remove_const_t = typename mini_std::remove_const<T>::type;

    template <typename T> struct remove_volatile             { using type = T; };
    template <typename T> struct remove_volatile<volatile T> { using type = T; };
    template <typename T> using remove_volatile_t = typename mini_std::remove_volatile<T>::type;

    template <typename T>
    struct remove_cv {
        using type = mini_std::remove_volatile_t<mini_std::remove_const_t<T>>;
    };
    template <typename T> using remove_cv_t = typename mini_std::remove_cv<T>::type;

    template <typename T>
    struct remove_cvref {
        using type = mini_std::remove_cv_t<remove_reference_t<T>>;
    };
    template <typename T> using remove_cvref_t = typename mini_std::remove_cvref<T>::type;

    // --- FORWARD & MOVE ---
    template <typename T> constexpr T&& forward(mini_std::remove_reference_t<T>& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr T&& forward(mini_std::remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr mini_std::remove_reference_t<T>&& move(T&& t) noexcept { return static_cast<mini_std::remove_reference_t<T>&&>(t); }

    template <typename T> struct add_lvalue_reference { using type = T&; };
    template <> struct add_lvalue_reference<void> { using type = void; };
    template <> struct add_lvalue_reference<const void> { using type = const void; };
    template <> struct add_lvalue_reference<volatile void> { using type = volatile void; };
    template <> struct add_lvalue_reference<const volatile void> { using type = const volatile void; };
    template <typename T> using add_lvalue_reference_t = typename mini_std::add_lvalue_reference<T>::type;

    template <typename T, typename = void> struct add_rvalue_reference { using type = T; };
    template <typename T> struct add_rvalue_reference<T, mini_std::void_t<T&&>> { using type = T&&; };
    template <typename T> using add_rvalue_reference_t = typename mini_std::add_rvalue_reference<T>::type;

    template <typename T> add_rvalue_reference_t<T> declval() noexcept;

    // --- PRIMARY TYPE CATEGORIES ---
    template <typename T> struct is_void : mini_std::false_type {};
    template <> struct is_void<void> : mini_std::true_type {};
    template <> struct is_void<const void> : mini_std::true_type {};
    template <> struct is_void<volatile void> : mini_std::true_type {};
    template <> struct is_void<const volatile void> : mini_std::true_type {};

    template <typename T> struct is_null_pointer : mini_std::false_type {};
    template <> struct is_null_pointer<mini_std::nullptr_t> : mini_std::true_type {};
    template <> struct is_null_pointer<const mini_std::nullptr_t> : mini_std::true_type {};
    template <> struct is_null_pointer<volatile mini_std::nullptr_t> : mini_std::true_type {};
    template <> struct is_null_pointer<const volatile mini_std::nullptr_t> : mini_std::true_type {};

    template <typename T> struct is_integral_helper : mini_std::false_type {};
    template <> struct is_integral_helper<bool> : mini_std::true_type {};
    template <> struct is_integral_helper<char> : mini_std::true_type {};
    template <> struct is_integral_helper<signed char> : mini_std::true_type {};
    template <> struct is_integral_helper<unsigned char> : mini_std::true_type {};
    template <> struct is_integral_helper<wchar_t> : mini_std::true_type {};
    template <> struct is_integral_helper<char16_t> : mini_std::true_type {};
    template <> struct is_integral_helper<char32_t> : mini_std::true_type {};
    #if defined(__cpp_char8_t)
    template <> struct is_integral_helper<char8_t> : mini_std::true_type {};
    #endif
    template <> struct is_integral_helper<short> : mini_std::true_type {};
    template <> struct is_integral_helper<unsigned short> : mini_std::true_type {};
    template <> struct is_integral_helper<int> : mini_std::true_type {};
    template <> struct is_integral_helper<unsigned int> : mini_std::true_type {};
    template <> struct is_integral_helper<long> : mini_std::true_type {};
    template <> struct is_integral_helper<unsigned long> : mini_std::true_type {};
    template <> struct is_integral_helper<long long> : mini_std::true_type {};
    template <> struct is_integral_helper<unsigned long long> : mini_std::true_type {};

    template <typename T>
    struct is_integral : mini_std::is_integral_helper<mini_std::remove_cv_t<T>> {};

    template <typename T> struct is_floating_point_helper : mini_std::false_type {};
    template <> struct is_floating_point_helper<float> : mini_std::true_type {};
    template <> struct is_floating_point_helper<double> : mini_std::true_type {};
    template <> struct is_floating_point_helper<long double> : mini_std::true_type {};

    template <typename T>
    struct is_floating_point : mini_std::is_floating_point_helper<mini_std::remove_cv_t<T>> {};

    template <typename T> struct is_array : mini_std::false_type {};
    template <typename T> struct is_array<T[]> : mini_std::true_type {};
    template <typename T, size_t N> struct is_array<T[N]> : mini_std::true_type {};

    template <typename T> struct is_pointer_helper : mini_std::false_type {};
    template <typename T> struct is_pointer_helper<T*> : mini_std::true_type {};
    template <typename T> struct is_pointer : mini_std::is_pointer_helper<mini_std::remove_cv_t<T>> {};
    template <typename T> constexpr bool is_pointer_v = is_pointer<T>::value;

    template <typename T> struct is_enum : mini_std::bool_constant<__is_enum(T)> {};
    template <typename T> struct is_union : mini_std::bool_constant<__is_union(T)> {};
    template <typename T> struct is_class : mini_std::bool_constant<__is_class(T)> {};

    // --- COMPOSITE TYPE CATEGORIES ---
    template <typename T>
    struct is_arithmetic 
        : mini_std::bool_constant<
            is_integral<T>::value || is_floating_point<T>::value
        > {};
    template <typename T> inline constexpr bool is_arithmetic_v = mini_std::is_arithmetic<T>::value;

    template <typename T>
    struct is_fundamental 
        : mini_std::bool_constant<
            mini_std::is_arithmetic<T>::value ||
            mini_std::is_void<T>::value ||
            mini_std::is_null_pointer<T>::value
        > {};

    template <typename T>
    struct is_scalar
        : mini_std::bool_constant<
            mini_std::is_arithmetic<T>::value ||
            mini_std::is_enum<T>::value ||
            mini_std::is_pointer<T>::value ||
            mini_std::is_null_pointer<T>::value
        > {};

    template <typename T>
    struct is_object : mini_std::bool_constant<
        mini_std::is_scalar<T>::value ||
        mini_std::is_array<T>::value  ||
        mini_std::is_union<T>::value  ||
        mini_std::is_class<T>::value
    > {};

    // --- POINTER & FUNCTION DETECTION ---
    
    template <typename F> struct is_function : mini_std::false_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...)> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) volatile> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const volatile> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) &> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const &> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) volatile &> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const volatile &> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) &&> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const &&> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) volatile &&> : mini_std::true_type {};
    template <typename Ret, typename... Args> struct is_function<Ret(Args...) const volatile &&> : mini_std::true_type {};

    // --- MEMBER FUNCTION POINTER ---
    template <typename T>
    struct is_member_function_pointer : mini_std::false_type {};

    template <typename Ret, typename Class, typename... Args>
    struct is_member_function_pointer<Ret (Class::*)(Args...)> : mini_std::true_type {};

    template <typename Ret, typename Class, typename... Args>
    struct is_member_function_pointer<Ret (Class::*)(Args...) const> : mini_std::true_type {};

    template <typename T>
    inline constexpr bool is_member_function_pointer_v = 
        is_member_function_pointer<mini_std::remove_cvref_t<T>>::value;

    // --- MEMBER OBJECT POINTER ---
    template <typename T>
    struct is_member_object_pointer : mini_std::false_type {};

    // MUST explicitly match T Class::* syntax!
    template <typename T, typename Class>
    struct is_member_object_pointer<T Class::*> 
        : mini_std::bool_constant<!mini_std::is_function<T>::value> {};

    template <typename T>
    inline constexpr bool is_member_object_pointer_v = 
        is_member_object_pointer<mini_std::remove_cvref_t<T>>::value;

    template <typename T> struct is_member_pointer_helper : mini_std::false_type {};
    template <typename T, typename C> struct is_member_pointer_helper<T C::*> : mini_std::true_type {};

    template <typename T>
    struct is_member_pointer : mini_std::is_member_pointer_helper<mini_std::remove_cv_t<T>> {};

    // --- ADD POINTER METAPROGRAMMING ---
    namespace addPtrdetail {
        template <class T> struct type_identity { using type = T; };

        template <class T>
        auto try_add_pointer(int) -> type_identity<typename mini_std::remove_reference<T>::type*>;

        template <class T>
        auto try_add_pointer(...) -> type_identity<T>;  
    } 
    template <class T> struct add_pointer : decltype(mini_std::addPtrdetail::try_add_pointer<T>(0)) {};
    template <class T> using add_pointer_t = typename mini_std::add_pointer<T>::type;

    template <typename T>
    [[nodiscard]] constexpr T* addressof(T& arg) noexcept {
        return __builtin_addressof(arg);
    }

    template <typename T>
    const T* addressof(const T&&) = delete;

    // --- CONDITIONALS & EXTENTS ---
    template <bool B, typename T, typename U> struct conditional { using type = T; };
    template <typename T, typename U> struct conditional<false, T, U> { using type = U; };
    template <bool B, typename T, typename U> using conditional_t = typename mini_std::conditional<B, T, U>::type;

    template <typename T> struct remove_extent          { using type = T; };
    template <typename T> struct remove_extent<T[]>     { using type = T; };
    template <typename T, size_t N> struct remove_extent<T[N]> { using type = T; };

    template <typename T>
    struct decay {
    private:
        using U = mini_std::remove_reference_t<T>;
    public:
        using type = mini_std::conditional_t<
            mini_std::is_array<U>::value,
            typename mini_std::add_pointer<typename mini_std::remove_extent<U>::type>::type,
            mini_std::conditional_t<
                mini_std::is_function<U>::value,
                typename mini_std::add_pointer<U>::type,
                mini_std::remove_cv_t<U>
            >
        >;
    };
    template <typename T> using decay_t = typename decay<T>::type;

    // --- CONVERSIONS & RELATIONSHIPS ---
    template <typename From, typename To>
    class is_convertible {
    private:
        static void test_aux(To);
        
        template <typename F, typename = decltype(test_aux(mini_std::declval<F>()))>
        static mini_std::true_type test(int);

        template <typename>
        static mini_std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<From>(0))::value;
    };

    template <typename Base, typename Derived>
    struct is_base_of : mini_std::bool_constant<__is_base_of(Base, Derived)> {};

    
    // --- INDEX SEQUENCE ---
    template <size_t... Is> struct index_sequence {};
    template <size_t N, size_t... Is> struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};
    template <size_t... Is> struct make_index_sequence_impl<0, Is...> { using type = index_sequence<Is...>; };
    template <size_t N> using make_index_sequence = typename mini_std::make_index_sequence_impl<N>::type;

    // --- TUPLE IMPL ---
    template <typename... Args> struct tuple;
    template <> struct tuple<> {};
    
    template <typename Head, typename... Tail> 
    struct tuple<Head, Tail...> : tuple<Tail...> {
        constexpr tuple() = default;
        constexpr tuple(Head h, Tail... t) 
            : tuple<Tail...>(mini_std::forward<Tail>(t)...), value(mini_std::forward<Head>(h)) {}
        Head value;
    };

    // Deduction Guides
    tuple() -> tuple<>;
    template <typename... Args> tuple(Args...) -> tuple<Args...>;

    // --- TUPLE SIZE & ELEMENT ---
    template <typename T> struct tuple_size;
    template <typename... Types> 
    struct tuple_size<tuple<Types...>> : mini_std::integral_constant<mini_std::size_t, sizeof...(Types)> {};
    template <typename T> struct tuple_size<const T> : tuple_size<T> {};
    template <typename T> struct tuple_size<volatile T> : tuple_size<T> {};
    template <typename T> struct tuple_size<const volatile T> : tuple_size<T> {};

    template <typename T> 
    inline constexpr mini_std::size_t tuple_size_v = mini_std::tuple_size<mini_std::remove_cvref_t<T>>::value;

    template <size_t I, typename Tuple> struct tuple_element;
    template <typename Head, typename... Tail>
    struct tuple_element<0, tuple<Head, Tail...>> { using type = Head; };
    template <size_t I, typename Head, typename... Tail>
    struct tuple_element<I, tuple<Head, Tail...>> {
        using type = typename mini_std::tuple_element<I - 1, tuple<Tail...>>::type;
    };
    template <size_t I, typename Tuple>
    using tuple_element_t = typename mini_std::tuple_element<I, mini_std::remove_cvref_t<Tuple>>::type;

    // --- IS TUPLE LIKE ---
    template <typename T, typename = void>
    struct is_tuple_like : mini_std::false_type {};
    template <typename T>
    struct is_tuple_like<T, mini_std::void_t<decltype(mini_std::tuple_size<mini_std::remove_cvref_t<T>>::value)>> 
        : mini_std::true_type {};

    template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

    template <typename... Args>
    constexpr mini_std::tuple<Args&&...> forward_as_tuple(Args&&... args) noexcept {
        return mini_std::tuple<Args&&...>(mini_std::forward<Args>(args)...);
    }

    // --- TUPLE GET INTERFACE ---
    template <size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(tuple<Head, Tail...>& t) noexcept {
        if constexpr (I == 0) {
            return (t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<mini_std::tuple<Tail...>&>(t));
        }
    }

    template <size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(const mini_std::tuple<Head, Tail...>& t) noexcept {
        if constexpr (I == 0) {
            return (t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<const mini_std::tuple<Tail...>&>(t));
        }
    }

    template <mini_std::size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(mini_std::tuple<Head, Tail...>&& t) noexcept {
        if constexpr (I == 0) {
            return mini_std::move(t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<mini_std::tuple<Tail...>&&>(t));
        }
    }

    // --- INVOKE IMPLEMENTATION ---
    namespace detail {
       template <typename F, typename Obj, typename... Args>
        constexpr decltype(auto) invoke_memfn(F&& f, Obj&& obj, Args&&... args) {
            using CleanObj = mini_std::remove_reference_t<Obj>;
            if constexpr (mini_std::is_pointer_v<CleanObj>) {
                return ((*mini_std::forward<Obj>(obj)).*f)(mini_std::forward<Args>(args)...);
            } else {
                return (mini_std::forward<Obj>(obj).*f)(mini_std::forward<Args>(args)...);
            }
        }

        template <typename MemObj, typename Obj>
        constexpr decltype(auto) invoke_memobj(MemObj&& f, Obj&& obj) {
            using ClassType = typename mini_std::is_member_pointer_helper<mini_std::remove_cvref_t<MemObj>>::type;
            using RawObj = mini_std::remove_cvref_t<Obj>;

            if constexpr (mini_std::is_base_of<ClassType, RawObj>::value) {
                return (mini_std::forward<Obj>(obj).*f);
            } else if constexpr (requires { obj.get(); }) {
                return (obj.get().*f);
            } else {
                return ((*mini_std::forward<Obj>(obj)).*f);
            }
        }
    }

    template <typename F, typename... Args>
    requires requires(F&& f, Args&&... args) { mini_std::forward<F>(f)(mini_std::forward<Args>(args)...); }
    constexpr decltype(auto) invoke(F&& f, Args&&... args) {
        return mini_std::forward<F>(f)(mini_std::forward<Args>(args)...);
    }

    // Overload 2: Member functions (SFINAE guarded)
    template <typename F, typename Obj, typename... Args>
    requires mini_std::is_member_function_pointer_v<mini_std::remove_cvref_t<F>>
    constexpr decltype(auto) invoke(F&& f, Obj&& obj, Args&&... args) {
        return detail::invoke_memfn(mini_std::forward<F>(f), mini_std::forward<Obj>(obj), mini_std::forward<Args>(args)...);
    }

    namespace detail {
    template <typename AlwaysVoid, typename F, typename... Args>
        struct is_nothrow_invocable_impl : mini_std::false_type {};

        template <typename F, typename... Args>
        struct is_nothrow_invocable_impl<
            mini_std::void_t<decltype(mini_std::invoke(mini_std::declval<F>(), mini_std::declval<Args>()...))>,
            F, 
            Args...
        > : mini_std::bool_constant<
                noexcept(mini_std::invoke(mini_std::declval<F>(), mini_std::declval<Args>()...))
            > {};

        template <typename AlwaysVoid, typename F, typename... Args>
        struct invoke_result_impl {};
    
        
        template <typename F, typename... Args>
        struct invoke_result_impl<mini_std::void_t<decltype(mini_std::declval<F>()(mini_std::declval<Args>()...))>,
            F, Args... > {
            using type = decltype(mini_std::declval<F>()(mini_std::declval<Args>()...)); 
        };

        template <typename F, typename Tuple, size_t... Is>
        constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, mini_std::index_sequence<Is...>) {
            return mini_std::invoke(mini_std::forward<F>(f), mini_std::get<Is>(mini_std::forward<Tuple>(t))...);
        }
    }

    // 3. User-facing public interface
    template <typename F, typename... Args>
    struct invoke_result : detail::invoke_result_impl<void, F, Args...> {};

    // Helper alias template (C++17 style)
    template <typename F, typename... Args>
    using invoke_result_t = typename mini_std::invoke_result<F, Args...>::type;

    template <typename F, typename... Args>
    struct is_nothrow_invocable 
        : detail::is_nothrow_invocable_impl<void, F, Args...> {};

    // 3. Convenience variable template (C++17 style)
    template <typename F, typename... Args>
    inline constexpr bool is_nothrow_invocable_v = mini_std::is_nothrow_invocable<F, Args...>::value;
    
    template <class T>
    class reference_wrapper {
    public:
        using type = T;

        template <class U, class = mini_std::enable_if<!mini_std::is_same_v<reference_wrapper, mini_std::decay_t<U>>>::type>
        constexpr reference_wrapper(U&& val) noexcept(noexcept(mini_std::addressof(val))) 
            : ptr_(mini_std::addressof(mini_std::forward<U>(val))) {}

        constexpr reference_wrapper(const reference_wrapper&) noexcept = default;
        constexpr reference_wrapper& operator=(const reference_wrapper&) noexcept = default;

        constexpr operator T& () const noexcept { return *ptr_; }
        constexpr T& get() const noexcept { return *ptr_; }

        template <class... Args>
        constexpr mini_std::invoke_result_t<T&, Args...> 
        operator()(Args&&... args) const noexcept(mini_std::is_nothrow_invocable_v<T&, Args...>) {
            return mini_std::invoke(get(), mini_std::forward<Args>(args)...);
        }

    private:
        T* ptr_;
    };

     template <class T>
    struct unwrap_refwrapper {
        using type = T;
    };

    template <class T>
    struct unwrap_refwrapper<mini_std::reference_wrapper<T>> {
        using type = T&;
    };

    template <class T>
    using unwrap_decay_t = typename mini_std::unwrap_refwrapper<typename mini_std::decay<T>::type>::type;

    template <typename F, typename Tuple>
    constexpr decltype(auto) apply(F&& f, Tuple&& t) {
        return detail::apply_impl(
            mini_std::forward<F>(f), 
            mini_std::forward<Tuple>(t), 
            mini_std::make_index_sequence<mini_std::tuple_size_v<Tuple>>{}
        );
    }

    template <class... Types>
    constexpr mini_std::tuple<unwrap_decay_t<Types>...> make_tuple(Types&&... args) 
    {
        return mini_std::tuple<unwrap_decay_t<Types>...>(mini_std::forward<Types>(args)...);
    }

    // --- CONCEPTS ---
    

    // --- UNREACHABLE ---
    [[noreturn]] inline void unreachable() noexcept {
#       if defined(__GNACT__) || defined(__GNUC__) || defined(__clang__)
            __builtin_unreachable();
#       elif defined(_MSC_VER)
            __assume(0);
#       endif
    }
}


// ============================================================================
// BEGIN. USED DEFINITIONS
// ============================================================================

namespace used_std {
    using namespace std; 

    template <typename T>
    concept is_tuple = requires { mini_std::is_tuple_like_v<T>; };

    template <typename T, typename U>
    struct is_same_template : used_std::false_type {};

    template <template <typename...> class TemplateClass, typename... Args1, typename... Args2>
    struct is_same_template<TemplateClass<Args1...>, TemplateClass<Args2...>> : used_std::true_type {};
    // 1. Primary template takes EXACTLY ONE type (the function signature or pointer)
    template <typename T>
    struct function_traits;

    // 2. Specialization for free function signatures: Fn(Args...)
    template <typename Ret, typename... Args>
    struct function_traits<Ret(Args...)> {
        using return_type = Ret;
        using fn_type     = Ret(Args...);
        using args_tuple  = used_std::tuple<Args...>;
        
        static constexpr used_std::size_t args = sizeof...(Args);

        template <used_std::size_t N>
        using arg_type = used_std::tuple_element_t<N, args_tuple>;
    };
    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using return_type = Ret;
        using fn_type     = Ret(*)(Args...); 
        using args_tuple  = used_std::tuple<Args...>;
        
        static constexpr used_std::size_t args = sizeof...(Args);

        template <used_std::size_t N>
        using arg_type = used_std::tuple_element_t<N, args_tuple>;
    };
    template <typename Ret, typename... Args>
    struct function_traits<Ret(&)(Args...)> {
        using return_type = Ret;
        using fn_type     = Ret(&)(Args...);
        using args_tuple  = used_std::tuple<Args...>;
        
        static constexpr used_std::size_t args = sizeof...(Args);

        template <used_std::size_t N>
        using arg_type = used_std::tuple_element_t<N, args_tuple>;
    };

    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> {
        using return_type = Ret;
        using class_type  = Class;
        using fn_type     = Ret(Class::*)(Args...); // Keeps the member function pointer identity!
        using args_tuple  = used_std::tuple<Args...>;
        static constexpr used_std::size_t args = sizeof...(Args);
    };
    // Specialization for CONST member functions: Fn(Class::*)(Args...) const
    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> {
        using return_type = Ret;
        using class_type  = Class;
        using fn_type     = Ret(Class::*)(Args...) const; // Keeps the member function pointer identity!
        using args_tuple  = used_std::tuple<Args...>;
        static constexpr used_std::size_t args = sizeof...(Args);
    };
    template <typename T>
    struct callable_traits {
        using type = typename used_std::function_traits<T>;
    };

    // Handle Functors/Lambdas (whether they have a const or non-const operator())
    template <typename T>
    requires requires { &T::operator(); }
    struct callable_traits<T> {
        using type = typename used_std::function_traits<decltype(&T::operator())>;
    };

    template <typename T>
    using callable_traits_t = typename callable_traits<used_std::remove_cvref_t<T>>::type;

   template <typename Fn, typename... Args>
    concept invocable = requires (Fn&& a,Args&&... args) {
        used_std::invoke(used_std::forward<Fn>(a),used_std::forward<Args>(args)...);
    };


    // Overload + operator to merge std::index_sequence types
    template <used_std::size_t... Is, used_std::size_t... Js>
    constexpr auto operator+(used_std::index_sequence<Is...>, used_std::index_sequence<Js...>) {
        return used_std::index_sequence<Is..., Js...>{};
    }

    // Internal helper using fold expressions over an index pack
    template <typename TupleA, typename TupleB, used_std::size_t... Is>
    constexpr auto get_matching_indices_impl(used_std::index_sequence<Is...>) {
        return (
            used_std::conditional_t<
                used_std::is_same_v<
                    used_std::remove_cvref_t<used_std::tuple_element_t<Is, TupleA>>, 
                    used_std::remove_cvref_t<used_std::tuple_element_t<Is, TupleB>>
                >,
                used_std::index_sequence<Is>,
                used_std::index_sequence<>
            >{} + ... + used_std::index_sequence<>{}
        );
    }
    // Public alias template
    template <typename TupleA, typename TupleB>
    using get_matching_indices_t = decltype(
        get_matching_indices_impl<TupleA, TupleB>(
            used_std::make_index_sequence<used_std::tuple_size_v<TupleA>>{}
        )
    );

    template <typename TupleA, typename TupleB>
    using get_matching_indices_safe_t = decltype(
        get_matching_indices_impl<TupleA, TupleB>(
            // Only check up to the bounds of the smaller tuple
            used_std::make_index_sequence<
                (used_std::tuple_size_v<TupleA> < used_std::tuple_size_v<TupleB>) ? 
                used_std::tuple_size_v<TupleA> : used_std::tuple_size_v<TupleB>
            >{}
        )
    );

    template <typename T, typename U>
    inline constexpr bool is_same_template_v = used_std::is_same_template<T, U>::value;
    
    struct string_view {
            const char* data_ptr = nullptr;
            used_std::size_t len = 0;
            constexpr string_view() = default;
            constexpr string_view(const char* str) noexcept : data_ptr(str) { while (str[len] != '\0') { ++len; } }
            constexpr bool operator==(const string_view& other) const noexcept {
                if (len != other.len) return false;
                for (used_std::size_t i = 0; i < len; ++i) { if (data_ptr[i] != other.data_ptr[i]) return false; }
                return true;
            }
        };

    namespace strHash {
        constexpr unsigned int fnv1a_hash(const char* str, used_std::size_t length) noexcept {
            used_std::size_t hash = static_cast<used_std::size_t>(2166136261U);
            const used_std::size_t prime = static_cast<used_std::size_t>(16777619U);
            
            for (used_std::size_t i = 0; i < length; ++i) {
                hash ^= static_cast<used_std::size_t>(str[i]);
                hash *= prime;
            }
            return hash;
        }
    }

    template <typename T>
    struct UniversalView {
        const T* ptr = nullptr;
        mini_std::size_t length = 0;

        constexpr UniversalView() = default;

        template <mini_std::size_t N>
        constexpr UniversalView(const T (&arr)[N]) noexcept : ptr(arr), length(N) {}

        template <typename ContainerType>
        constexpr UniversalView(const ContainerType& container) noexcept 
            : ptr(container.data()), length(container.size()) {}

        template <typename IteratorType>
        constexpr UniversalView(IteratorType first, IteratorType last) noexcept {
            if constexpr (used_std::is_pointer_v<IteratorType>) {
                ptr = first;
                length = static_cast<used_std::size_t>(last - first);
            } else {
                ptr = &(*first);
                length = static_cast<used_std::size_t>(last - first);
            }
        }

        constexpr const T* data() const noexcept { return ptr; }
        constexpr used_std::size_t size() const noexcept { return length; }
    };

}


// Global user-defined string literal hash shortcut
constexpr unsigned long long operator""_hash(const char* str, used_std::size_t) noexcept {
    used_std::string_view tmp = str;
    return used_std::strHash::fnv1a_hash(tmp.data_ptr,tmp.len);
}

template <used_std::size_t N = 0>
struct StaticLabel {
    char data[N > 0 ? N : 1]{};
    unsigned int hash{0};

    // 1. String Literal Constructor (auto-hashes and stores string)
    constexpr StaticLabel(const char (&input)[N + 1]) {
        for (used_std::size_t i = 0; i < N; ++i) {
            data[i] = input[i];
        }
        hash = used_std::strHash::fnv1a_hash(input, N);
    }

    // 2. Integer Constructor (stores int value and hashes the int)
    constexpr StaticLabel(int val) {
        char digits[24];
        int len = 0;
        int temp = val < 0 ? -val : val;

        do {
            digits[len++] = static_cast<char>('0' + (temp % 10));
            temp /= 10;
        } while (temp > 0);

        if (val < 0) digits[len++] = '-';

        char buf[24]{};
        for (int i = 0; i < len; ++i) {
            buf[i] = digits[len - 1 - i];
        }
        hash = used_std::strHash::fnv1a_hash(buf,static_cast<used_std::size_t>(len));
    }

    constexpr StaticLabel() = default;

    template <used_std::size_t M>
    constexpr bool operator==(const StaticLabel<M>& other) const noexcept {  
        return hash == other.hash;
    }

    constexpr StaticLabel& operator=(int val) noexcept {
        *this = StaticLabel<0>(val);
        return *this;
    }
};
template <used_std::size_t N>
StaticLabel(const char(&)[N]) -> StaticLabel<N - 1>;

StaticLabel(int) -> StaticLabel<0>;

// DSL range view builder for standard continuous iterators
template <typename IteratorType>
constexpr auto from_range(IteratorType first, IteratorType last) noexcept {
    using ValueType = used_std::decay_t<decltype(*first)>;
    return used_std::UniversalView<ValueType>(first, last);
}


// Forward declarations to bridge dependency layout orders
namespace mini_pack {
    template <used_std::size_t Index, typename... Ts> struct pack_element;
}

template <typename TargetType, typename KeyType>
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept;
// ============================================================================
// 1. FUNCTION PREDICATES DEFINITIONS
// ============================================================================
// --- Free Function / Lambda Predicate Wrapper / Unbound Member Function Predicate ---
template <typename Fn> requires 
(used_std::is_function<typename used_std::callable_traits_t<Fn>::fn_type>::value || 
    used_std::is_member_function_pointer_v<typename used_std::callable_traits_t<Fn>::fn_type>)
struct FnPredicate {
    Fn fn;

    template <typename Target> requires (used_std::invocable<Fn,Target>)
    constexpr bool operator()(const Target& target) const {
        if constexpr (std::is_member_function_pointer_v<Fn>) {
            return (target.*fn)();
        } else{
            return used_std::invoke(fn, target);
        }
    }
};



// ---  (Target is the instance) ---
// template <typename MemFn> 
// requires (used_std::is_member_function_pointer_v<MemFn>)
// struct MemFnPredicate {
//     MemFn mem_fn;

//     template <typename Target>
//     constexpr bool operator()(const Target& target) const {
//         if constexpr (std::is_member_function_pointer_v<MemFn>) {
//             return (target.*mem_fn)();
//         }
//     }
// };

// template <typename MemFn>
// requires (used_std::is_member_function_pointer_v<MemFn>)
// constexpr auto MemFnMatch(MemFn mem_fn) {
//     return MemFnPredicate<MemFn>{ mem_fn };
// }

// --- Bound Member Function Predicate (Instance + Member Function) ---
template <typename Class, typename MemFn> 
requires (used_std::is_class<Class>::value && used_std::is_member_function_pointer_v<MemFn>)
struct BoundMemFnPredicate {
    Class& instance;
    MemFn mem_fn;

    template <typename Target>
    constexpr bool operator()(const Target& target) const {
        return used_std::invoke(mem_fn, instance, target);
    }
};


// --- Projection Case (Pattern + Member Function Extractor) ---
template <typename MemFn, typename ExpectedPattern> requires (used_std::is_member_function_pointer_v<MemFn>)
struct ProjectionCaseimpl {
    MemFn mem_fn;
    ExpectedPattern pattern;
    using fnTraits = used_std::callable_traits_t<MemFn>;
    template <typename Target>
    constexpr bool operator()(const Target& target) const {
        if constexpr (fnTraits::args == 0) {
            decltype(auto) extracted_val = used_std::invoke(mem_fn);
            return evaluate_match(extracted_val, target);
        } else {
            decltype(auto) extracted_val = used_std::invoke(mem_fn,target);
            return evaluate_match(extracted_val, pattern);
        }
    }
};

template <typename Fn>
struct is_afnpredicate : std::false_type {};
template <typename Fn>
struct is_afnpredicate<FnPredicate<Fn>> : std::true_type {};
template <typename Fn>
concept is_fnpredicate = is_afnpredicate<FnPredicate<Fn>>::value;


template <typename Fn>
struct is_aboundfn_predicate : std::false_type {};
template <typename Class, typename MemFn>
struct is_aboundfn_predicate<BoundMemFnPredicate<Class,MemFn>> : std::true_type {};
template <typename Class, typename MemFn>
concept is_boundfn_predicate = is_aboundfn_predicate<BoundMemFnPredicate<Class,MemFn>>::value;

template <typename Fn>
struct is_projection_caseimpl : std::false_type {};
template <typename MemFn, typename ExpectedPattern>
struct is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>> : std::true_type {};
template <typename MemFn, typename ExpectedPattern>
concept is_projection_case = is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>>::value;


template <typename Fn> requires 
(used_std::is_function<typename used_std::callable_traits_t<Fn>::fn_type>::value || 
    used_std::is_member_function_pointer_v<typename used_std::callable_traits_t<Fn>::fn_type>)
constexpr auto Predicate(Fn&& fn) {
    return FnPredicate<Fn>{ used_std::forward<Fn>(fn) };
}

template <typename Class, typename MemFn>
requires (used_std::is_member_function_pointer_v<MemFn>)
constexpr auto BoundPredicate(Class& obj, MemFn mem_fn) {
    return BoundMemFnPredicate<Class, MemFn>{ obj, mem_fn };
}

// Helper builder function for Case(Pattern, &Class::member_fn)
template <typename ExpectedPattern, typename MemFn>
requires (used_std::is_member_function_pointer_v<MemFn>)
constexpr auto ProjectionCase(ExpectedPattern&& pattern, MemFn mem_fn) {
    return ProjectionCaseimpl<typename used_std::callable_traits_t<used_std::remove_cvref_t<MemFn>>::fn_type, used_std::decay_t<ExpectedPattern>>{
        mem_fn, 
        used_std::forward<ExpectedPattern>(pattern)
    };
}


// 1. Free function / Lambda predicate matcher overload
// template <typename Target, typename Fn>
// constexpr bool evaluate_match(const Target& target, const FnPredicate<Fn>& pred) {
//     return pred(target);
// }

// 2. Unbound member function matcher (Returns bool -> Predicate)
// template <typename Target, typename MemFn>
// constexpr bool evaluate_match(const Target& target, const MemFnPredicate<MemFn>& matcher) {
//     return (target.*matcher.mem_fn)();
// }

// 3. Projection matcher (Extracts value via member function, tests against pattern)
// template <typename Target, typename MemFn, typename ExpectedPattern>
// requires (!used_std::is_same_v<used_std::invoke_result_t<MemFn, const Target&>, bool>)
// constexpr bool evaluate_match(const Target& target, const ProjectionCaseimpl<MemFn, ExpectedPattern>& proj_case) {
//     decltype(auto) extracted_val = used_std::invoke(proj_case.mem_fn, target);
//     return evaluate_match(extracted_val, proj_case.pattern);
// }

// // 4. Bound member function matcher (Instance + Member Function)
// template <typename Target, typename Class, typename MemFn>
// constexpr bool evaluate_match(const Target& target, const BoundMemFnPredicate<Class, MemFn>& pred) {
//     return pred(target);
// }
// ============================================================================
// 2. MATHEMATICAL INTERVAL DEFINITIONS
// ============================================================================
enum class IntervalType { Closed, Open, HalfOpenLeft, HalfOpenRight };

template <typename T,IntervalType iType> requires (used_std::is_arithmetic_v<T>)
struct Range {
    T min_val;
    T max_val;
    
    constexpr bool contains(const T& target) const noexcept {
        if constexpr (iType == IntervalType::Closed) {
            return (target >= min_val) && (target <= max_val);
        } 
        else if constexpr (iType == IntervalType::Open) {
            return (target > min_val) && (target < max_val);
        }
        else if constexpr (iType == IntervalType::HalfOpenLeft) {
            return (target > min_val) && (target <= max_val);
        }
        else if constexpr (iType == IntervalType::HalfOpenRight) {
            return (target >= min_val) && (target < max_val);
        } else {
            return false;
        }
    }
};

template <typename T>
struct is_range_inst : std::false_type {};

template <typename T, IntervalType iType>
struct is_range_inst<Range<T, iType>> : std::true_type {};

template <typename T>
concept is_range_instance = is_range_inst<std::remove_cvref_t<T>>::value;

template <typename T,IntervalType iType = IntervalType::Closed> constexpr auto make_range(T min, T max) noexcept { return Range<T,iType>{min, max}; }
template <typename T,IntervalType iType = IntervalType::Open> constexpr auto make_range_exclusive(T min, T max) noexcept { return Range<T,iType>{min, max}; }
template <typename T,IntervalType iType = IntervalType::HalfOpenLeft> constexpr auto make_range_left_open(T min, T max) noexcept { return Range<T,iType>{min, max}; }
template <typename T,IntervalType iType = IntervalType::HalfOpenRight> constexpr auto make_range_right_open(T min, T max) noexcept { return Range<T,iType>{min, max}; }


template <typename... Ranges> requires (is_range_instance<Ranges> && ...)
struct RangeCompound {
    used_std::tuple<Ranges...> ranges;

    constexpr explicit RangeCompound(Ranges... r) : ranges(used_std::move(r)...) {}

    template <typename T>
    constexpr bool contains(const T& target) const noexcept {
        return used_std::apply([&target](const auto&... r) {
            return (r.contains(target) || ...);
        }, ranges);
    }
};
template <typename... Ranges> 
requires (is_range_instance<Ranges> && ...)
RangeCompound(Ranges...) -> RangeCompound<Ranges...>;

template <typename... Ranges>
constexpr auto make_compound_range(Ranges&&... ranges) noexcept {
    return RangeCompound<used_std::decay_t<Ranges>...>{used_std::forward<Ranges>(ranges)...};
}
// RangeCompound test {Range<int>{0,12},Range<int>{20,30}};
// ============================================================================
// 3. COMPLEX RELATIONAL MULTI-FIELD PREDICATES
// ============================================================================
enum class Op { Eq, Neq, Gt, Gte, Lt, Lte };


template <typename ClassType, typename MemberType> requires 
(used_std::is_class<ClassType>::value && (used_std::is_member_object_pointer_v<MemberType> || used_std::is_member_pointer<class Tp>::value))
struct FieldRule {
    MemberType ClassType::*member_ptr;
    Op op_tag = Op::Eq;
    MemberType value;

    constexpr bool eval(const ClassType& obj) const noexcept {
        const auto& target_field = obj.*member_ptr;
        switch (op_tag) {
            case Op::Eq:  return evaluate_match(target_field, value);
            case Op::Neq: !evaluate_match(target_field, value);
            case Op::Gt:  return target_field > value;
            case Op::Gte: return target_field >= value;
            case Op::Lt:  return target_field < value;
            case Op::Lte: return target_field <= value;
        }
        return false;
    }
};

template <typename T>
struct is_field_rule : used_std::false_type {};

template <typename Class, typename Member>
struct is_field_rule<FieldRule<Class, Member>> : used_std::true_type {
    using class_type = Class;
};

template <typename ClassType, typename... Rules> requires ((is_field_rule<Rules>::value && ...))
struct MultiFieldPredicate {
    used_std::tuple<Rules...> rules;

    constexpr bool matches(const ClassType& obj) const noexcept {
        return evaluate_all(obj, used_std::make_index_sequence<sizeof...(Rules)>{});
    }
private:
    template <used_std::size_t... Is>
    constexpr bool evaluate_all(const ClassType& obj, used_std::index_sequence<Is...>) const noexcept {
        return (used_std::get<Is>(rules).eval(obj) && ...);
    }
};
template <typename HeadRule, typename... TailRules>
requires (is_field_rule<used_std::decay_t<HeadRule>>::value && 
         (is_field_rule<used_std::decay_t<TailRules>>::value && ...))
MultiFieldPredicate(HeadRule, TailRules...) 
    -> MultiFieldPredicate<typename is_field_rule<used_std::decay_t<HeadRule>>::class_type, HeadRule, TailRules...>;

template <typename ClassType, typename MemberType>
constexpr auto field(MemberType ClassType::*member, Op op, MemberType&& val) noexcept {
    return FieldRule<ClassType, used_std::decay_t<MemberType>>{member, op, used_std::forward<MemberType>(val)};
}

template <typename ClassType, typename MemberType>
constexpr auto field(MemberType ClassType::*member, MemberType&& val) noexcept {
    return FieldRule<ClassType, used_std::decay_t<MemberType>>{member, Op::Eq, used_std::forward<MemberType>(val)};
}

template <typename ClassType, typename... Rules>
constexpr auto fields_match(Rules&&... rules) noexcept {
    return MultiFieldPredicate<ClassType, used_std::decay_t<Rules>...>{ used_std::tuple<used_std::decay_t<Rules>...>(used_std::forward<Rules>(rules)...) };
}

// ============================================================================
// 4. FLOW CONTROL STATE TRACE SIGNALS AND ENUMS
// ============================================================================
enum class BranchHint { None, Likely, Unlikely };
struct fallthrough_t {};
constexpr fallthrough_t fallthrough_to_next() noexcept { return fallthrough_t{}; }

template <typename T> struct FallthroughValue { T value; };
template <typename T> constexpr auto pass_and_fallthrough(T&& val) noexcept { return FallthroughValue<used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

template <StaticLabel LabelID> struct goto_case_t {};
template <StaticLabel LabelID> constexpr auto goto_case() noexcept { return goto_case_t<LabelID>{}; }

template <StaticLabel LabelID, typename T> struct GotoValue { T value; };
template <StaticLabel LabelID, typename T> constexpr auto pass_and_goto(T&& val) noexcept { return GotoValue<LabelID, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

struct Wildcard {};
[[maybe_unused]] inline constexpr Wildcard __{};

// ============================================================================
// 5. UNIFIED DEDUPLICATED ITERATION SUB-PREDICATES (TUPLES, VIEWS, BITS)
// ============================================================================
enum class MatchPolicy { Any, All };
template <MatchPolicy Policy, typename CriterionType>
struct TupleIterator {
    CriterionType expected_value;
};

template <typename T> constexpr auto tuple_has_any(T&& val) noexcept { return TupleIterator<MatchPolicy::Any, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto tuple_has_all(T&& val) noexcept { return TupleIterator<MatchPolicy::All, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

enum class TypePolicy { Any, All };
template <TypePolicy Policy, template <typename> class Trait>
struct TypePredicate {};

template <template <typename> class Trait> constexpr auto tuple_types_any() noexcept { return TypePredicate<TypePolicy::Any, Trait>{}; }
template <template <typename> class Trait> constexpr auto tuple_types_all() noexcept { return TypePredicate<TypePolicy::All, Trait>{}; }

template <typename T> struct AnyElementValidator { T expected_value; };
template <typename T> struct AllElementValidator { T expected_value; };
template <used_std::size_t Offset, typename T, used_std::size_t N> struct SequenceOffsetValidator { T expected_pattern[N]; };
struct SliceSizeValidator { used_std::size_t expected_size; };

template <typename T> constexpr auto array_has_any(T&& val) noexcept { return AnyElementValidator<used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto array_has_all(T&& val) noexcept { return AllElementValidator<used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
constexpr auto array_has_size(used_std::size_t sz) noexcept { return SliceSizeValidator{ sz }; }

enum class BitPolicy { AnySet, AllSet, AllClear };
template <BitPolicy Policy, typename T> struct BitwisePredicate { T bit_mask; };

template <typename T> constexpr auto bits_any_set(T mask) noexcept { return BitwisePredicate<BitPolicy::AnySet, used_std::decay_t<T>>{ mask }; }
template <typename T> constexpr auto bits_all_set(T mask) noexcept { return BitwisePredicate<BitPolicy::AllSet, used_std::decay_t<T>>{ mask }; }
template <typename T> constexpr auto bits_all_clear(T mask) noexcept { return BitwisePredicate<BitPolicy::AllClear, used_std::decay_t<T>>{ mask }; }
template <typename T> constexpr auto match_bits(T mask) noexcept { return bits_all_set(mask); }

template <used_std::size_t Offset, typename... Args>
constexpr auto match_array_from(Args&&... args) noexcept {
    using CommonType = typename used_std::decay_t<typename mini_pack::pack_element<0, Args...>::type>;
    return SequenceOffsetValidator<Offset, CommonType, sizeof...(Args)>{ static_cast<CommonType>(args)... };
}

template <typename... Args>
constexpr auto match_array(Args&&... args) noexcept {
    return match_array_from<0>(used_std::forward<Args>(args)...);
}

// Combined Direct/Stack Array literal initializer logic shortcut wrapper
template <typename T, used_std::size_t N>
struct DirectArray {
    T data[N];
    template <typename Container>
    constexpr bool operator==(const Container& other) const noexcept {
        return evaluate_match(other, SequenceOffsetValidator<0, T, N>{data});
    }
};

template <typename... Args>
constexpr auto match_array_literal(Args&&... args) noexcept {
    using CommonType = typename used_std::decay_t<typename mini_pack::pack_element<0, Args...>::type>;
    return DirectArray<CommonType, sizeof...(Args)>{ static_cast<CommonType>(args)... };
}

// ============================================================================
// 6. C++20 CONCEPT CONSTRAINTS IDENTIFICATION MATRIX
// ============================================================================
namespace mini_concepts {
    
    template <typename T> concept IsWildcard = used_std::is_same_v<used_std::decay_t<T>, Wildcard>;
    
    template <typename KeyType, typename TargetType>
    concept ContainsRange = requires(KeyType k, TargetType t) { { k.contains(t) }; };

    template <typename KeyType, typename TargetType>
    concept MatchesPredicate = requires(KeyType k, TargetType t) { { k.matches(t) };  };

    template <class From, class To>
    concept convertible_to = used_std::is_convertible<From, To>::value && requires { static_cast<To>(used_std::declval<From>()); };
    namespace detail {
        template <typename T, typename IndexSeq>
        struct has_valid_tuple_elements : used_std::false_type {};

        template <typename T, used_std::size_t... Is>
        struct has_valid_tuple_elements<T, used_std::index_sequence<Is...>> 
            : used_std::bool_constant<(requires { typename used_std::tuple_element_t<Is, T>; } && ...)> {};
    }

    template <typename T>
    concept TupleLike = requires {
        // Strips references (& / &&) and const/volatile before querying tuple_size
        typename used_std::make_index_sequence<
            used_std::tuple_size<used_std::remove_cvref_t<T>>::value
        >;
    };
    template <typename T>
    concept Primitive = used_std::is_fundamental<used_std::decay_t<T>>::value || 
                    used_std::is_enum<used_std::decay_t<T>>::value;

    namespace detail {
        template <typename F, typename Tuple, typename Indices>
        struct is_tuple_invocable_impl;

        template <typename F, typename Tuple, std::size_t... Is>
        struct is_tuple_invocable_impl<F, Tuple, std::index_sequence<Is...>> 
            // Strip inner cv-qualifiers and references from every single element type!
            : std::is_invocable<F, std::remove_cvref_t<std::tuple_element_t<Is, Tuple>>...> {};

    }
    template <typename Action, typename Tuple>
    concept TupleInvocable = detail::is_tuple_invocable_impl<
        Action, 
        Tuple, 
        used_std::make_index_sequence<used_std::tuple_size_v<used_std::decay_t<Tuple>>>
    >::value;

    
    
    template <typename T>
    struct is_pure_fallthrough : used_std::false_type {};

    template <>
    struct is_pure_fallthrough<fallthrough_t> : used_std::true_type {};

    template <typename T>
    concept IsPureFallthrough = is_pure_fallthrough<used_std::decay_t<T>>::value;

    template <typename T>
    struct is_value_fallthrough : used_std::false_type {};
    template <typename T>
    struct is_value_fallthrough<FallthroughValue<T>> : used_std::true_type {
        using value_type = T;
    };
    template <typename T>
    concept IsValueFallthrough = is_value_fallthrough<used_std::decay_t<T>>::value;

    template <typename T>
    concept IsFallthroughSignal = IsPureFallthrough<T> || IsValueFallthrough<T>;

    template <typename T>
    struct is_goto_case {
        static constexpr bool value = false;
    };

    template <StaticLabel LabelID>
    struct is_goto_case<goto_case_t<LabelID>> {
        static constexpr bool value = true;
        static constexpr auto label = LabelID;
    };

    template <typename T>
    concept IsPureGoto = is_goto_case<used_std::decay_t<T>>::value;

    template <typename T>
    struct is_goto_value {
        static constexpr bool value = false;
    };

    template <StaticLabel LabelID, typename T>
    struct is_goto_value<GotoValue<LabelID, T>> {
        static constexpr bool value = true;
        static constexpr auto label = LabelID;
        using value_type = T;
    };

    template <typename T>
    concept IsValueGoto = is_goto_value<used_std::decay_t<T>>::value;

    template <typename T>
    concept IsGotoSignal = IsPureGoto<T> || IsValueGoto<T>;

    template <typename T> concept IsAwaitable = requires(T t) { { t.operator co_await() }; } || requires(T t) { { t.await_ready() }; };

    template <typename T> struct is_tuple_iterator { static constexpr bool value = false; };
    template <MatchPolicy P, typename C> struct is_tuple_iterator<TupleIterator<P, C>> { static constexpr bool value = true; };
    template <typename T> concept IsTupleIterator = is_tuple_iterator<used_std::decay_t<T>>::value;

    template <typename T> struct is_type_predicate { static constexpr bool value = false; };
    template <TypePolicy P, template <typename> class Trait> struct is_type_predicate<TypePredicate<P, Trait>> { static constexpr bool value = true; };
    template <typename T> concept IsTypePredicate = is_type_predicate<used_std::decay_t<T>>::value;

    template <typename T> struct is_any_element { static constexpr bool value = false; };
    template <typename T> struct is_any_element<AnyElementValidator<T>> { static constexpr bool value = true; };
    template <typename T> concept IsAnyElement = is_any_element<used_std::decay_t<T>>::value;

    template <typename T> struct is_all_element { static constexpr bool value = false; };
    template <typename T> struct is_all_element<AllElementValidator<T>> { static constexpr bool value = true; };
    template <typename T> concept IsAllElement = is_all_element<used_std::decay_t<T>>::value;

    template <typename T> struct is_seq_offset { static constexpr bool value = false; };
    template <used_std::size_t O, typename T, used_std::size_t N> struct is_seq_offset<SequenceOffsetValidator<O, T, N>> { static constexpr bool value = true; };
    template <typename T> concept IsSeqOffset = is_seq_offset<used_std::decay_t<T>>::value;

    template <typename T> struct is_slice_size { static constexpr bool value = false; };
    template <> struct is_slice_size<SliceSizeValidator> { static constexpr bool value = true; };
    template <typename T> concept IsSliceSize = is_slice_size<used_std::decay_t<T>>::value;

    template <typename T> struct is_bitwise { static constexpr bool value = false; };
    template <BitPolicy P, typename T> struct is_bitwise<BitwisePredicate<P, T>> { static constexpr bool value = true; };
    template <typename T> concept IsBitwise = is_bitwise<used_std::decay_t<T>>::value;

    // Check if a single type is either a reference or a pointer
    template <typename T>
    concept IsReferenceOrPointer = std::is_reference_v<T> || std::is_pointer_v<T>;

    // Primary helper trait to unpack tuple elements
    template <typename Tuple>
    struct IsTupleOfRefsOrPointers : std::false_type {};

    template <typename... Elements>
    struct IsTupleOfRefsOrPointers<used_std::tuple<Elements...>> 
        : std::bool_constant<(IsReferenceOrPointer<Elements> && ...)> {};

    // Concept to enforce that a Tuple-like type contains ONLY references or pointers
    template <typename Tuple> 
    concept TupleOfRefsOrPointers = IsTupleOfRefsOrPointers<std::remove_cvref_t<Tuple>>::value;

   template <typename K, typename T>
    concept IsCallablePredicate = requires(K k, T t) {
        // Check if directly callable or via invoke without triggering instantiation errors
        { k(t) } -> convertible_to<bool>;
    };  
    
}

// Global Core Match Evaluator Implementation
template <typename TargetType, typename KeyType>
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept {
    using TargetDecay = used_std::decay_t<TargetType>;
    using KeyDecay    = used_std::decay_t<KeyType>;

    // 1. Wildcard / Catch-all
    if constexpr (mini_concepts::IsWildcard<KeyType>) {
        return true;
    }
    // 2. Type-Level Trait Matching on Tuples/Types
    else if constexpr (mini_concepts::IsTypePredicate<KeyType>) {
        if constexpr (used_std::is_tuple<TargetDecay>) {
            auto evaluator = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) noexcept {
                if constexpr (KeyDecay::policy_value == TypePolicy::Any) {
                    return (KeyDecay::template Trait<typename used_std::tuple_element<Is, TargetDecay>::type>::value || ...);
                } else {
                    return (KeyDecay::template Trait<typename used_std::tuple_element<Is, TargetDecay>::type>::value && ...);
                }
            };
            return evaluator(used_std::make_index_sequence<used_std::tuple_size_v<TargetDecay>>{});
        } else {
            return KeyDecay::template Trait<TargetDecay>::value;
        }
    }
    // 3. Tuple Unrolling
    else if constexpr (mini_concepts::IsTupleIterator<KeyType>) {
        static_assert(used_std::is_tuple<TargetDecay>, "IsTupleIterator target must be a tuple-like type");
        auto unroller = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) noexcept {
            if constexpr (KeyDecay::policy == MatchPolicy::Any) {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) || ...);
            } else {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) && ...);
            }
        };
        return unroller(used_std::make_index_sequence<used_std::tuple_size_v<TargetDecay>>{});
    }
    // 4. Container Element Predicates (Any / All)
    else if constexpr (mini_concepts::IsAnyElement<KeyType>) {
        used_std::UniversalView view(target);
        for (used_std::size_t i = 0; i < view.size(); ++i) {
            if (evaluate_match(view.data()[i], key.expected_value)) return true;
        }
        return false;
    }
    else if constexpr (mini_concepts::IsAllElement<KeyType>) {
        used_std::UniversalView view(target);
        for (used_std::size_t i = 0; i < view.size(); ++i) {
            if (!evaluate_match(view.data()[i], key.expected_value)) return false;
        }
        return true;
    }
    // 5. Sequence Slice / Sub-range Matching
    else if constexpr (mini_concepts::IsSeqOffset<KeyType>) {
        used_std::UniversalView view(target);
        constexpr used_std::size_t Offset = KeyDecay::offset_val;
        constexpr used_std::size_t PatternLen = KeyDecay::pattern_len;
        
        if (Offset + PatternLen > view.size()) return false;
        for (used_std::size_t i = 0; i < PatternLen; ++i) {
            if (!(key.expected_pattern[i] == view.data()[Offset + i])) return false;
        }
        return true;
    }
    else if constexpr (mini_concepts::IsSliceSize<KeyType>) {
        used_std::UniversalView view(target);
        return view.size() == key.expected_size;
    }
    // 6. Bitwise Operations
    else if constexpr (mini_concepts::IsBitwise<KeyType>) {
        if constexpr (KeyDecay::policy_value == BitPolicy::AnySet)   { return (target & key.bit_mask) != 0; }
        if constexpr (KeyDecay::policy_value == BitPolicy::AllSet)   { return (target & key.bit_mask) == key.bit_mask; }
        if constexpr (KeyDecay::policy_value == BitPolicy::AllClear) { return (target & key.bit_mask) == 0; }
        return false;
    }
    // 7. Ranges
    else if constexpr (mini_concepts::ContainsRange<KeyType, TargetType>) {
        return key.contains(target);
    }
    else if constexpr (mini_concepts::MatchesPredicate<KeyType, TargetType>) {
        return used_std::invoke(key.matches, target);
    } 
    else if constexpr (mini_concepts::IsCallablePredicate<KeyType, TargetType>) {
        return used_std::invoke(key, target);
    } 
    // else if constexpr (is_fnpredicate<KeyType>) {
    //     return used_std::invoke(key, target);
    // } 
    // else if constexpr (is_boundfn_predicate<TargetType,KeyType>) {
    //     return used_std::invoke(key, target);
    // } 
    // else if constexpr (requires {target.mem_fn && target.pattern;}) {
    //     decltype(auto) extracted_val = used_std::invoke(target.mem_fn, target.pattern);
    //     return evaluate_match(target,extracted_val);
    //     // return used_std::invoke(key, target);
    // } 
    else if constexpr (requires { { target == key } -> mini_concepts::convertible_to<bool>; }) {
        return (target == key);
    } 
    else {
        return false;
    }
}

// ============================================================================
// 7. UNIFIED CASING LAYOUT STORAGE WITH HINT PARAMETERS
// ============================================================================
template <StaticLabel LabelID, typename KeyType, typename ActionType, BranchHint HintValue>
struct ImplCase {
    KeyType key;
    ActionType action;
    static constexpr auto label = LabelID;
    static constexpr BranchHint hint = HintValue;
};

// ============================================================================
// 8. OVERLOADED PIPELINE SUGAR GENERATORS (operator>>)
// ============================================================================
template <StaticLabel LabelID, BranchHint Hint, typename KeyType>
struct SugarProxyKey {
    KeyType key;
    template <typename ActionType>
    constexpr auto operator>>(ActionType&& action) && noexcept {
        return ImplCase<LabelID, KeyType, used_std::decay_t<ActionType>, Hint>{
            used_std::forward<KeyType>(key), used_std::forward<ActionType>(action)
        };
    }
};

// Consolidated syntactic sugar match nodes (Default Label ID is set to 0)
template <BranchHint Hint = BranchHint::None,typename T> constexpr auto Case(T&& val) noexcept { return SugarProxyKey<0, Hint, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto likely_Case(T&& val) noexcept { return SugarProxyKey<0, BranchHint::Likely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto unlikely_Case(T&& val) noexcept { return SugarProxyKey<0, BranchHint::Unlikely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <StaticLabel LabelID, typename T> constexpr auto label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::None, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <StaticLabel LabelID, typename T> constexpr auto likely_label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::Likely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <StaticLabel LabelID, typename T> constexpr auto unlikely_label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::Unlikely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

// Hardware Prediction Branch Optimizer Hints Primitive Mapping
template <BranchHint Hint>
[[nodiscard]] constexpr bool apply_hardware_hint(bool condition) noexcept {
    if constexpr (Hint == BranchHint::Likely) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_expect(static_cast<bool>(condition), 1);
#else
        if (condition) [[likely]] {
            return true;
        } else [[unlikely]] {
            return false;
        }
#endif
    } 
    else if constexpr (Hint == BranchHint::Unlikely) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_expect(static_cast<bool>(condition), 0);
#else
        if (condition) [[unlikely]] {
            return true;
        } else [[likely]] {
            return false;
        }
#endif
    } 
    else {
        return condition;
    }
}

template <typename ActionType, typename ContextType> 
constexpr decltype(auto) execute_action(ActionType&& action, ContextType& ctx) {
    using ActionDecay  = used_std::remove_cvref_t<ActionType>;
    using CleanContext = used_std::remove_cvref_t<ContextType>;
    
    // Resolve traits dynamically based on whether it is a function pointer or a functor object
    using FnTrait = used_std::callable_traits_t<ActionType>;

    // 1. Passive signals and primitives
    if constexpr (mini_concepts::IsGotoSignal<ActionDecay> || 
                  mini_concepts::IsFallthroughSignal<ActionDecay> || 
                  mini_concepts::Primitive<ActionDecay>) 
    {
        return used_std::forward<ActionType>(action);
    } 
    else if constexpr (mini_concepts::IsWildcard<ActionDecay>) {
        return true;
    } 
    // 2. Zero-argument lambdas or functions [] {}
    else if constexpr (FnTrait::args == 0) {
        return used_std::forward<ActionType>(action)();
    }
    // 3. Partial / Matching Tuple Unpack Strategy
    else if constexpr (FnTrait::args > 0) {
        // Extract the explicit std::tuple of the function parameters
        using FnArgsTuple = typename FnTrait::args_tuple;
        
        // Fix 2: Compare Tuple to Tuple (FnArgsTuple vs CleanContext)
        using ResultSequence = used_std::get_matching_indices_safe_t<FnArgsTuple, CleanContext>;
        
        // Pass the calculated compile-time matching indices down to invoke std::get
        return []<used_std::size_t... Is>(auto&& act, auto& context, used_std::index_sequence<Is...>) -> decltype(auto) {
            return used_std::forward<ActionType>(act)(used_std::get<Is>(context)...);
        }(used_std::forward<ActionType>(action), ctx, ResultSequence{});
    }
    // 4. Pass entire context tuple directly if nothing else matched
    else if constexpr (mini_concepts::TupleInvocable<ActionDecay, CleanContext>) {
        return used_std::forward<ActionType>(action)(ctx);
    } 
}

template <typename T>
struct UnwrapReturnType { using type = used_std::remove_cvref_t<T>;};

template <>
struct UnwrapReturnType<fallthrough_t> {
    using type = void;
};

template <typename T>
struct UnwrapReturnType<FallthroughValue<T>> {
    using type = T;
};

template <auto LabelID>
struct UnwrapReturnType<goto_case_t<LabelID>> {
    using type = void;
};

template <auto LabelID, typename T>
struct UnwrapReturnType<GotoValue<LabelID, T>> {
    using type = T;
};

template <typename T>
using unwrap_return_type_t = typename UnwrapReturnType<used_std::decay_t<T>>::type;

// Unrolled Matrix State Router Engine Loop
template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
    constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;
    
    // Updated: Pass target to resolve correct return type
    using CoreReturnType  = decltype(execute_action(default_action, ctx));
    using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;
    
    auto unrolled_matrix_router = []<used_std::size_t... Is>(const TargetType& target, DefaultType&& default_action, 
        ContextTuple& ctx, CasesTuple&& cases,used_std::index_sequence<Is...>) -> CleanReturnType {
        using RawFirstCase = used_std::remove_cvref_t<decltype(used_std::get<0>(cases))>;
        using LabelType    = used_std::remove_cvref_t<decltype(RawFirstCase::label)>;
        
        LabelType target_jump_label{};
        bool matched = false;
        bool force_execute_next = false;
        bool jump_requested = false;
        
        used_std::size_t loop_guard = 0;
        constexpr used_std::size_t MaxAllowedJumps = TotalCases * 2;

        // Separate void and non-void return type pathways
        if constexpr (used_std::is_same_v<CleanReturnType, void>) {
            while (loop_guard++ < MaxAllowedJumps) {
                bool current_iteration_jumped = false;
                
                ([&]() {
                    if (matched && !force_execute_next && !jump_requested) return;
                    
                    auto&& current_case = used_std::get<Is>(cases);
                    using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
                    
                    bool should_execute = false;
                    
                    if (jump_requested) {
                        if (current_case.label == target_jump_label) {
                            should_execute = true;
                        }
                    } else if (force_execute_next) {
                        should_execute = true;
                    } else if (!matched) {
                        should_execute = apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key));
                    }

                    if (should_execute) {
                        matched = true;
                        force_execute_next = false;
                        jump_requested = false;
                        
                        execute_action(current_case.action, ctx);
                        using ActionDecay = used_std::decay_t<CleanReturnType>;

                        if constexpr (mini_concepts::IsPureFallthrough<ActionDecay> || mini_concepts::IsValueFallthrough<ActionDecay>) {
                            force_execute_next = true;
                        }
                        else if constexpr (mini_concepts::IsPureGoto<ActionDecay>) {
                            target_jump_label = mini_concepts::is_goto_case<ActionDecay>::label;
                            jump_requested = true;
                            current_iteration_jumped = true;
                        }
                        else if constexpr (mini_concepts::IsValueGoto<ActionDecay>) {
                            target_jump_label = mini_concepts::is_goto_value<ActionDecay>::label;
                            jump_requested = true;
                            current_iteration_jumped = true;
                        }
                    }
                }(), ...);
                if (!current_iteration_jumped && !force_execute_next) {
                    break;
                }
            }
            
            if (!matched || force_execute_next || jump_requested) {
                // Updated: Passed target to default execute_action
                execute_action(default_action, ctx);
            }
        } 
        else {
            CleanReturnType result{};

            while (loop_guard++ < MaxAllowedJumps) {
                bool current_iteration_jumped = false;
                
                ([&]() {
                    if (matched && !force_execute_next && !jump_requested) return;

                    auto&& current_case = used_std::get<Is>(cases);
                    using RawCaseType = used_std::remove_cvref_t<decltype(current_case)>;
                    
                    bool should_execute = false;
                    
                    if (jump_requested) {
                        if (current_case.label == target_jump_label) {
                            should_execute = true;
                        }
                    } else if (force_execute_next) {
                        should_execute = true;
                    } else if (!matched) {
                        should_execute = apply_hardware_hint<RawCaseType::hint>(evaluate_match(target, current_case.key));
                    }

                    if (should_execute) {
                        matched = true;
                        force_execute_next = false;
                        jump_requested = false;
                        
                        // Updated: Passed target to execute_action
                        decltype(auto) action_res = execute_action(current_case.action, ctx);
                        using ActionDecay = used_std::decay_t<decltype(action_res)>;

                        if constexpr (mini_concepts::IsPureFallthrough<ActionDecay>) {
                            force_execute_next = true;
                        }
                        else if constexpr (mini_concepts::IsValueFallthrough<ActionDecay>) {
                            result = action_res.value;
                            force_execute_next = true;
                        }
                        else if constexpr (mini_concepts::IsPureGoto<ActionDecay>) {
                            target_jump_label = mini_concepts::is_goto_case<ActionDecay>::label;
                            jump_requested = true;
                            current_iteration_jumped = true;
                        }
                        else if constexpr (mini_concepts::IsValueGoto<ActionDecay>) {
                            result = action_res.value;
                            target_jump_label = mini_concepts::is_goto_value<ActionDecay>::label;
                            jump_requested = true;
                            current_iteration_jumped = true;
                        }
                        else {
                            result = action_res;
                        }
                    }
                }(), ...);

                if (!current_iteration_jumped && !force_execute_next) {
                    break;
                }
            }
            
            if (matched && !force_execute_next && !jump_requested) {
                return result;
            }
            
            // Updated: Passed target to default execute_action
            return execute_action(default_action, ctx);
        }
    };

    return unrolled_matrix_router(target, std::forward<DefaultType>(default_action), ctx, used_std::forward<CasesTuple>(cases),used_std::make_index_sequence<TotalCases>{});
}

// ============================================================================
// 9. PACK SEPARATION & DELAYED UNIVERSAL INITIALIZATION
// ============================================================================
namespace mini_pack {
    template <used_std::size_t Index, typename... Ts> struct pack_element;

    template <typename Head, typename... Tail> struct pack_element<0, Head, Tail...> {
        using type = Head;
        constexpr static decltype(auto) extract(Head&& head, Tail&&...) noexcept { 
            return used_std::forward<Head>(head); 
        }
    };

    template <used_std::size_t Index, typename Head, typename... Tail> struct pack_element<Index, Head, Tail...> {
        using type = typename pack_element<Index - 1, Tail...>::type;
        constexpr static decltype(auto) extract(Head&&, Tail&&... tail) noexcept { 
            return pack_element<Index - 1, Tail...>::extract(used_std::forward<Tail>(tail)...); 
        }
    };

    template <typename Tuple, used_std::size_t... Is> 
    requires (mini_concepts::TupleLike<used_std::remove_cvref_t<Tuple>>)
    constexpr auto make_cases_tuple_impl(Tuple&& full_tuple, used_std::index_sequence<Is...>) {
        return used_std::make_tuple(
            used_std::get<Is>(used_std::forward<Tuple>(full_tuple))...
        );
    }

    template <used_std::size_t... Is, typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> 
    requires (
        mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>> && 
        mini_concepts::TupleLike<used_std::remove_cvref_t<CasesTuple>>)
    constexpr auto evaluate_sorted_matrix(used_std::index_sequence<Is...>, const TargetType& target, DefaultType&& default_action, ContextTuple&& ctx, CasesTuple&& cases) {
        if constexpr (sizeof...(Is) > 0) {
            return universal_switch_matrix(
                target, 
                used_std::forward<DefaultType>(default_action), 
                ctx,
                used_std::forward<CasesTuple>(cases)
            );
        } else {
            return execute_action(used_std::forward<DefaultType>(default_action), ctx, target);
        }
    }
}

template <typename TargetType, typename ContextTuple, typename... AllTrailingArgs> 
requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>>)
constexpr auto universal_switch(const TargetType& target, ContextTuple&& ctx, AllTrailingArgs&&... args) {
    constexpr used_std::size_t TotalArgs = sizeof...(AllTrailingArgs);
    static_assert(TotalArgs >= 1, "Library Error: You must supply a terminal fallback default action.");
    
    constexpr used_std::size_t CaseCount = TotalArgs - 1;
    
    return [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> decltype(auto) {
        auto args_tuple = used_std::forward_as_tuple(used_std::forward<AllTrailingArgs>(args)...);

        decltype(auto) default_action = used_std::get<CaseCount>(args_tuple);

        if constexpr (CaseCount == 0) {
            return execute_action(used_std::forward<decltype(default_action)>(default_action), ctx, target);
        } else {
            auto cases_tuple = used_std::forward_as_tuple(
                used_std::get<Is>(used_std::forward<decltype(args_tuple)>(args_tuple))...
            );

            return mini_pack::evaluate_sorted_matrix(
                used_std::make_index_sequence<CaseCount>{}, 
                target, 
                used_std::forward<decltype(default_action)>(default_action), 
                used_std::forward<ContextTuple>(ctx), 
                cases_tuple
            );
        }
    }(used_std::make_index_sequence<CaseCount>{});
}
// ============================================================================
// 10. HIGH-UTILITY PROXY WRAPPERS (uswitch DSL FRONTEND ENTRY)
// ============================================================================
template <typename TargetType, typename ContextTuple> 
requires (mini_concepts::TupleLike<used_std::remove_cvref_t<ContextTuple>>)
struct SwitchPipelineProxy {
    const TargetType& target;
    ContextTuple ctx;

    template <typename... CaseTypes>
    constexpr decltype(auto) operator()(CaseTypes&&... cases) && {
        return universal_switch(target, used_std::forward<ContextTuple>(ctx), used_std::forward<CaseTypes>(cases)...);
    }
};

template <typename TargetType>
struct SwitchTargetProxy {
    const TargetType& target;

    template <typename ContextArgs> requires (mini_concepts::IsWildcard<ContextArgs>)
    constexpr auto operator[](ContextArgs& arg) && noexcept {
        using EmptyTuple = used_std::tuple<>;
        return SwitchPipelineProxy<TargetType, EmptyTuple>{ target, EmptyTuple{} };
    }

    template <typename... ContextArgs>
    requires (sizeof...(ContextArgs) > 0) 
    constexpr auto operator[](ContextArgs&&... args) && noexcept {
        // FIX: Capture exact lvalue/rvalue reference categories to prevent dangling references
        auto ctx_tuple = used_std::forward_as_tuple(used_std::forward<ContextArgs>(args)...);
        using TupleType = decltype(ctx_tuple);
        static_assert(
            mini_concepts::TupleOfRefsOrPointers<TupleType>,
            "DSL Error: Context parameters must strictly be references (&) or pointers (*). Value types are forbidden."
        );
        return SwitchPipelineProxy<TargetType, TupleType>{ target, ctx_tuple };
    }
    template <typename... ContextArgs>
    requires (sizeof...(ContextArgs) > 0) 
    constexpr auto operator()(ContextArgs&&... args) && noexcept {
        // FIX: Capture exact lvalue/rvalue reference categories to prevent dangling references
        auto ctx_tuple = used_std::forward_as_tuple(used_std::forward<ContextArgs>(args)...);
        using TupleType = decltype(ctx_tuple);
        static_assert(
            mini_concepts::TupleOfRefsOrPointers<TupleType>,
            "DSL Error: Context parameters must strictly be references (&) or pointers (*). Value types are forbidden."
        );
        return SwitchPipelineProxy<TargetType, TupleType>{ target, ctx_tuple };
    }
};

template <typename TargetType>
constexpr auto Match(const TargetType& target) noexcept {
    return SwitchTargetProxy<TargetType>{ target };
}
// ============================================================================
//                                    END
// ============================================================================



// --- A. Numeric & Range Matching ---
void showcase_numeric_and_ranges(int score) {
    std::cout << "\n=== 1. Numeric & Range Pattern Matching ===" << std::endl;
    // std::size_t score2 = 2;
    std::string_view result = Match(score)[score]  (
        Case(100)                      >> [] { return "Perfect Score!"; },
        Case(make_range(90, 99))       >> [] { return "Grade: A"; },
        Case(make_range(80, 89))       >> [] { return "Grade: B"; },
        Case(make_range(70, 79))       >> [] { return "Grade: C"; },
        [](int& s) { 
            // auto gett = s;
            return s < 70 ? "Grade: Fail" : "Grade: Invalid"; 
            // return "Invalid"; 
        }
    );

    std::cout << "Score [" << score << "] -> " << result << std::endl;
}

// --- B. StaticLabel / FNV-1a Hash Matching ---
void showcase_hash_labels(std::string_view command) {
    std::cout << "\n=== 2. StaticLabel Hash Matching ===" << std::endl;

    std::size_t cmd_hash = used_std::strHash::fnv1a_hash(command.data(), command.size());

    std::string_view response = Match(command)(command,cmd_hash) (
        Case("start")  >> [] { return "System Starting..."; },
        Case("stop")   >> [] { return "System Stopping..."; },
        Case("pause")  >> [] { return "System Paused."; },
        Case(__)       >> [](std::string_view& s) { return s.data(); },
        []{return "UNDEFINED!";}
    );

    std::cout << "Command [\"" << command << "\"] (Hash: " << cmd_hash << ") -> " << response << std::endl;
}

// --- C. Branch Prediction Hints ---
void showcase_branch_hints(int http_code) {
    std::cout << "\n=== 3. Branch Hint Guided Dispatch ===" << std::endl;

    std::string_view status = Match(http_code)[__] (
        // Common paths marked as likely
        Case<BranchHint::Likely>(200)   >> []() { return "200 OK (Fast Path)"; },
        Case<BranchHint::Likely>(404)   >> []() { return "404 Not Found"; },
        
        // Exceptional paths marked as unlikely
        Case<BranchHint::Unlikely>(500) >> []() { return "500 Internal Server Error"; },
        []() { return "Other HTTP Status"; }
    );

    std::cout << "HTTP [" << http_code << "] -> " << status << std::endl;
}

constexpr bool is_even(int val) { return val % 2 == 0; }
bool is_positive(int val) { return val > 0; }

// Class for testing Member Functions
struct User {
    std::string_view name;
    int age;
    bool active;

    bool is_adult() const { return age >= 18; }
    bool is_active() const { return active; }
};

// Class with validator methods
struct Validator {
    int min_threshold = 50;

    bool exceeds_threshold(int val) const {
        return val > min_threshold;
    }
};

int main () {
    std::cout << "=================================================" << std::endl;
    std::cout << "       PATTERN MATCHING LIBRARY SHOWCASE         " << std::endl;
    std::cout << "=================================================" << std::endl;

    std::cout << "=== Free Function & Member Function Matching ===\n";

    // -------------------------------------------------------------
    // 1. Standalone / Free Function Evaluation
    // -------------------------------------------------------------
    int number = -43;
    std::string_view num_res = Match(number)[__](
        Case(Predicate(is_even))     >> [] { return "Even Number"; },
        Case(Predicate(is_positive)) >> [] { return "Positive Odd Number"; },
        [] { return "Other"; }
    );
    std::cout << "Number " << number << " -> " << num_res << "\n";

    // -------------------------------------------------------------
    // 2. Unbound Member Function Evaluation (Target is the Instance)
    // -------------------------------------------------------------
    User u1{"Alice", 22, true};
    
    std::string_view user_res = Match(u1)[__](
        Case(Predicate(&User::is_adult))  >> [] { return "Adult User"; },
        Case(Predicate(&User::is_active)) >> [] { return "Active Minor"; },
        [] { return "Inactive Minor"; }
    );
    std::cout << u1.name << " -> " << user_res << "\n";

    // -------------------------------------------------------------
    // 3. Bound Member Function Evaluation (External Instance)
    // -------------------------------------------------------------
    Validator validator{30};
    int score = 75;

    std::string_view val_res = Match(score)[__](
        Case(BoundPredicate(validator, &Validator::exceeds_threshold)) >> [] {
            return "Passed Validation";
        },
        [] { return "Failed Validation"; }
    );
    std::cout << "Score " << score << " -> " << val_res << "\n";

    // 4. Numeric Range Showcase
    showcase_numeric_and_ranges(95);
    showcase_numeric_and_ranges(72);
    showcase_numeric_and_ranges(45);

    // 4. Compile-Time Hash Labels Showcase
    showcase_hash_labels("start");
    showcase_hash_labels("pause");
    showcase_hash_labels("reboot");
    showcase_hash_labels("rebootss");

    // 4. Branch Prediction Showcase
    showcase_branch_hints(200);
    showcase_branch_hints(500);
    // std::printf("%d %d" , ret , num);

    constexpr struct s {
        int i;
        constexpr s(int i) : i(i){}
        constexpr int get(int s) {
            return i + s;
        }
    } t(10);
    
    constexpr int num = 20;
    static_assert(Match(num)[__] (
        Case(RangeCompound{make_range(0,10),make_range(20,30)}) >> []{return true;},
        []{return false;}), "" );
        
    Match(num)[__] (
        Case(ProjectionCase(10,&s::get)) >> []{
            std::cout << "is in range";
        },
        []{
            std::cout << "is not range";
        }
    );
    
    return 1;
}