
// ============================================================================
// FREESTANDING LIGHTWEIGHT COMPILER & METAPROGRAMMING UTILITIES
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
