#include <iostream>


// ============================================================================
// 1. FREESTANDING LIGHTWEIGHT COMPILER & METAPROGRAMMING UTILITIES
// ============================================================================
namespace mini_std {
    using size_t = decltype(sizeof(0));
    using nullptr_t = decltype(nullptr);
    using ptrdiff_t = decltype([]{char temp[2]; return (temp + 1) - temp;}());

    template <class Ty, Ty Val>
    struct integral_constant {
        static constexpr Ty value = Val ;
        using value_type = Ty;
        using type       = integral_constant;
        
        constexpr operator value_type() const noexcept   { return static_cast<value_type>(value); }
        constexpr value_type operator()() const noexcept { return static_cast<value_type>(value); }
    };

    template <bool Val> struct bool_constant : mini_std::integral_constant<bool, Val> {};
    using false_type = mini_std::bool_constant<false>;
    using true_type  = mini_std::bool_constant<true>;

    template<class T, class U> struct is_same : mini_std::false_type {};
    template<class T> struct is_same<T, T>    : mini_std::true_type {};
    template <typename T, typename U> constexpr bool is_same_v = mini_std::is_same<T, U>::value;

    template <typename...>
    using void_t = void;

    template<typename T> struct is_void : mini_std::false_type {};
    template<> struct is_void<void> : mini_std::false_type {};
    
    template<typename T> struct is_null_pointer : mini_std::false_type {};
    template<> struct is_null_pointer<mini_std::nullptr_t> : mini_std::false_type {};

    template <typename T>
    struct is_integral : mini_std::false_type {};
    template <> struct is_integral<bool> : mini_std::true_type {};
    template <> struct is_integral<char> : mini_std::true_type {};
    template <> struct is_integral<signed char> : mini_std::true_type {};
    template <> struct is_integral<unsigned char> : mini_std::true_type {};

    template <> struct is_integral<wchar_t> : mini_std::true_type {};
    template <> struct is_integral<char16_t> : mini_std::true_type {};
    template <> struct is_integral<char32_t> : mini_std::true_type {};
    #if defined(__cpp_char8_t)
    template <> struct is_integral<char8_t> : mini_std::true_type {};
    #endif
    template <> struct is_integral<short> : mini_std::true_type {};
    template <> struct is_integral<unsigned short> : mini_std::true_type {};
    template <> struct is_integral<int> : mini_std::true_type {};
    template <> struct is_integral<unsigned int> : mini_std::true_type {};
    template <> struct is_integral<long> : mini_std::true_type {};
    template <> struct is_integral<unsigned long> : mini_std::true_type {};
    template <> struct is_integral<long long> : mini_std::true_type {};
    template <> struct is_integral<unsigned long long> : mini_std::true_type {};

    template <typename T>
    struct is_floating_point : mini_std::false_type {};

    template <> struct is_floating_point<float> : mini_std::true_type {};
    template <> struct is_floating_point<double> : mini_std::true_type {};
    template <> struct is_floating_point<long double> : mini_std::true_type {};

    template <typename T>
    struct is_enum : mini_std::bool_constant<__is_enum(T)> {};

    
    template <typename T> struct remove_reference      { using type = T; };
    template <typename T> struct remove_reference<T&>  { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };
    template <typename T> using remove_reference_t = typename mini_std::remove_reference<T>::type;

    template <typename T> struct add_lvalue_reference { using type = T&; };
    template <> struct add_lvalue_reference<void> { using type = void; };
    template <> struct add_lvalue_reference<const void> { using type = const void; };
    template <typename T> using add_lvalue_reference_t = typename mini_std::add_lvalue_reference<T>::type;

    template <typename T, typename = void> struct add_rvalue_reference { using type = T; };
    template <typename T> struct add_rvalue_reference<T, mini_std::void_t<T&&>> { using type = T&&; };
    template <typename T> using add_rvalue_reference_t = typename mini_std::add_rvalue_reference<T>::type;

    template <typename T>
    add_rvalue_reference_t<T> declval() noexcept;

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

    template <typename T> struct remove_extent          { using type = T; };
    template <typename T> struct remove_extent<T[]>     { using type = T; };
    template <typename T, size_t N> struct remove_extent<T[N]> { using type = T; };

    template<class T> struct is_array : mini_std::false_type {};
    template<class T> struct is_array<T[]> : mini_std::true_type {};
    template<class T, size_t N> struct is_array<T[N]> : mini_std::true_type {};
    
    template <bool B, typename T, typename U> struct conditional { using type = T; };
    template <typename T, typename U> struct conditional<false, T, U> { using type = U; };
    template <bool B, typename T, typename U> using conditional_t = typename mini_std::conditional<B, T, U>::type;

    template <typename T> struct remove_const          { using type = T; };
    template <typename T> struct remove_const<const T>  { using type = T; };

    template <typename T> struct remove_volatile             { using type = T; };
    template <typename T> struct remove_volatile<volatile T> { using type = T; };

    template <typename T>
    struct remove_cv {
        using type = typename remove_volatile<typename mini_std::remove_const<T>::type>::type;
    };
    template <typename T> using remove_cv_t = typename mini_std::remove_cv<T>::type;

    template <typename T>
    struct remove_cvref {
        using type = mini_std::remove_cv_t<remove_reference_t<T>>;
    };
    template <typename T> using remove_cvref_t = typename mini_std::remove_cvref<T>::type;

    namespace addPtrdetail {
        template<class T> struct type_identity { using type = T; };

        template<class T>
        auto try_add_pointer(int) -> type_identity<typename mini_std::remove_reference<T>::type*>;

        template<class T>
        auto try_add_pointer(...) -> type_identity<T>;  
    } 
    template<class T> struct add_pointer : decltype(mini_std::addPtrdetail::try_add_pointer<T>(0)) {};
    template<class T> using add_pointer_t = typename mini_std::add_pointer<T>::type;

    template<typename F>                   struct is_function : mini_std::false_type {};
    template<typename F, typename... Args> struct is_function<F(Args...)> : mini_std::true_type {};

    
    template <typename T>
    struct is_arithmetic 
    : mini_std::bool_constant<
    is_integral<mini_std::remove_cv_t<T>>::value || 
    is_floating_point<mini_std::remove_cv_t<T>>::value
    > {};
    
    template <typename T>
    inline constexpr bool is_arithmetic_v = mini_std::is_arithmetic<T>::value;

    template <typename T>
    struct is_fundamental 
        : mini_std::integral_constant<bool,
            mini_std::is_arithmetic<T>::value ||
            mini_std::is_void<T>::value ||
            mini_std::is_null_pointer<T>::value
        > {};

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

    template <typename T> struct is_pointer : mini_std::false_type {};
    template <typename T> struct is_pointer<T*> : mini_std::true_type {};
    template <typename T> constexpr bool is_pointer_v = is_pointer<T>::value;

    template <typename T> constexpr T&& forward(mini_std::remove_reference_t<T>& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr T&& forward(mini_std::remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr mini_std::remove_reference_t<T>&& move(T&& t) noexcept { return static_cast<mini_std::remove_reference_t<T>&&>(t); }

    // C++14 alias template shortcut
    template <typename T>
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;
// --- 1. INDEX SEQUENCE ---
    template <size_t... Is> struct index_sequence {};
    template <size_t N, size_t... Is> struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};
    template <size_t... Is> struct make_index_sequence_impl<0, Is...> { using type = index_sequence<Is...>; };
    template <size_t N> using make_index_sequence = typename mini_std::make_index_sequence_impl<N>::type;

    // --- 2. TUPLE DEFINITION ---
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

    // --- 3. TUPLE SIZE (WITH CV-REF HANDLING) ---
    template <typename T> struct tuple_size;

    // Base specialization for un-qualified tuple
    template <typename... Types> 
    struct tuple_size<tuple<Types...>> : mini_std::integral_constant<mini_std::size_t, sizeof...(Types)> {};

    // Forward const/volatile/reference types to base tuple_size
    template <typename T>
    struct tuple_size<const T> : tuple_size<T> {};
    template <typename T>
    struct tuple_size<volatile T> : tuple_size<T> {};
    template <typename T>
    struct tuple_size<const volatile T> : tuple_size<T> {};

    template <typename T> 
    inline constexpr mini_std::size_t tuple_size_v = mini_std::tuple_size<mini_std::remove_cvref_t<T>>::value;

    // --- 4. TUPLE ELEMENT ---
    template <size_t I, typename Tuple> struct tuple_element;

    template <typename Head, typename... Tail>
    struct tuple_element<0, tuple<Head, Tail...>> {
        using type = Head;
    };

    template <size_t I, typename Head, typename... Tail>
    struct tuple_element<I, tuple<Head, Tail...>> {
        using type = typename mini_std::tuple_element<I - 1, tuple<Tail...>>::type;
    };

    template <size_t I, typename Tuple>
    using tuple_element_t = typename mini_std::tuple_element<I, mini_std::remove_cvref_t<Tuple>>::type;

    // --- 5. IS TUPLE LIKE (SFINAE) ---
    template <typename T, typename = void>
    struct is_tuple_like : mini_std::false_type {}; // ✅ Defaults to false_type!

    template <typename T>
    struct is_tuple_like<T, mini_std::void_t<decltype(mini_std::tuple_size<mini_std::remove_cvref_t<T>>::value)>> 
        : mini_std::true_type {};

    template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

    template <typename... Args>
    constexpr mini_std::tuple<Args&&...> forward_as_tuple(Args&&... args) noexcept {
        return mini_std::tuple<Args&&...>(mini_std::forward<Args>(args)...);
    }

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

    
    template <typename ActionType, typename... Args>
    concept invocable  = requires(ActionType a, Args&&... args) { a(args...); };

    template <typename ActionType>
    concept invocableNoargs = requires(ActionType a) { (a)(); };

    template <typename Action, typename Target, typename Tuple, typename Indices>
    struct is_invocable_with_target_tuple_prefix;

    template <typename Action, typename Target, typename Tuple, std::size_t... Is>
    struct is_invocable_with_target_tuple_prefix<Action, Target, Tuple, std::index_sequence<Is...>> {
        static constexpr bool value = std::is_invocable_v<Action, Target, std::tuple_element_t<Is, Tuple>...>;
    };

    // Helper to check if Action is invocable with first N elements of Context Tuple
    template <typename Action, typename Tuple, typename Indices>
    struct is_invocable_with_tuple_prefix;

    template <typename Action, typename Tuple, mini_std::size_t... Is>
    struct is_invocable_with_tuple_prefix<Action, Tuple, mini_std::index_sequence<Is...>> {
        static constexpr bool value = mini_std::invocable <Action, mini_std::tuple_element_t<Is, Tuple>...>;
    };

    // Find matching prefix count for Context Tuple (returns N or -1 if no match)
    template <typename Action, typename Tuple, mini_std::size_t N = mini_std::tuple_size_v<Tuple>>
    constexpr mini_std::ptrdiff_t find_matching_context_prefix() {
        if constexpr (mini_std::is_invocable_with_tuple_prefix<Action, Tuple, mini_std::make_index_sequence<N>>::value) {
            return N;
        } else if constexpr (N > 0) {
            return mini_std::find_matching_context_prefix<Action, Tuple, N - 1>();
        } else {
            return -1;
        }
    }

    // Find matching prefix count for Target + Context Tuple (returns N or -1 if no match)
    template <typename Action, typename Target, typename Tuple, mini_std::size_t N = mini_std::tuple_size_v<Tuple>>
    constexpr mini_std::ptrdiff_t find_matching_target_context_prefix() {
        if constexpr (mini_std::is_invocable_with_target_tuple_prefix<Action, Target, Tuple, mini_std::make_index_sequence<N>>::value) {
            return N;
        } else if constexpr (N > 0) {
            return mini_std::find_matching_target_context_prefix<Action, Target, Tuple, N - 1>();
        } else {
            return -1;
        }
    }

    
}


namespace used_std {
    using namespace mini_std; 
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

// ============================================================================
// 2. MATHEMATICAL INTERVAL DEFINITIONS
// ============================================================================
enum class IntervalType { Closed, Open, HalfOpenLeft, HalfOpenRight };

template <typename T>
struct Range {
    T min_val;
    T max_val;
    IntervalType type = IntervalType::Closed;
    
    constexpr bool contains(const T& target) const noexcept {
        switch (type) {
            case IntervalType::Closed:        return (target >= min_val) && (target <= max_val);
            case IntervalType::Open:          return (target > min_val) && (target < max_val);
            case IntervalType::HalfOpenLeft:  return (target > min_val) && (target <= max_val);
            case IntervalType::HalfOpenRight: return (target >= min_val) && (target < max_val);
        }
        return false;
    }
};

template <typename T> constexpr auto make_range(T min, T max) noexcept { return Range<T>{min, max, IntervalType::Closed}; }
template <typename T> constexpr auto make_range_exclusive(T min, T max) noexcept { return Range<T>{min, max, IntervalType::Open}; }
template <typename T> constexpr auto make_range_left_open(T min, T max) noexcept { return Range<T>{min, max, IntervalType::HalfOpenLeft}; }
template <typename T> constexpr auto make_range_right_open(T min, T max) noexcept { return Range<T>{min, max, IntervalType::HalfOpenRight}; }

// ============================================================================
// 3. COMPLEX RELATIONAL MULTI-FIELD PREDICATES
// ============================================================================
enum class Op { Eq, Neq, Gt, Gte, Lt, Lte };

template <typename TargetType, typename KeyType>
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept;

template <typename ClassType, typename MemberType>
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

template <typename ClassType, typename... Rules>
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

struct AnyType {};
inline constexpr AnyType any_value{};

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
    
    template <typename T> concept IsWildcard = used_std::is_same_v<used_std::decay_t<T>, AnyType>;
    
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
        { used_std::tuple_size<used_std::remove_cvref_t<T>>::value } -> convertible_to<mini_std::size_t>;
    } && detail::has_valid_tuple_elements<
        used_std::remove_cvref_t<T>, 
        used_std::make_index_sequence<used_std::tuple_size_v<T>>
    >::value;
    template <typename T>
    concept Primitive = used_std::is_fundamental<used_std::decay_t<T>>::value || 
                    used_std::is_enum<used_std::decay_t<T>>::value;

    namespace detail {
    template <typename Action, typename Tuple, typename IndexSeq>
    struct is_tuple_invocable : used_std::false_type {};

    template <typename Action, typename Tuple, used_std::size_t... Is>
    struct is_tuple_invocable<Action, Tuple, used_std::index_sequence<Is...>> 
        : used_std::bool_constant<
            mini_std::invocable<Action, typename used_std::tuple_element<Is, Tuple>::type...>
          > {};
    }

    template <typename Action, typename Tuple>
    concept Tupleinvocable  = 
        TupleLike<Tuple> && 
        detail::is_tuple_invocable<
            Action, 
            Tuple, 
            used_std::make_index_sequence<used_std::tuple_size_v<Tuple>>
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

    // 3. Top-level concept that validates whether ActionType is executable with ContextType
    template <typename Action, typename Context>
    concept ActionExecutable = 
        Tupleinvocable <used_std::remove_cvref_t<Action>, used_std::remove_cvref_t<Context>> ||
        mini_std::invocable <used_std::remove_cvref_t<Action>, Context> ||
        mini_std::invocableNoargs<used_std::remove_cvref_t<Action>> ||
        IsWildcard<Action> ||
        (used_std::find_matching_context_prefix<Action, Context>() != -1) ||
        (used_std::find_matching_target_context_prefix<Action,used_std::declval<Action>(), Context>() != -1) ||
        Primitive<used_std::remove_cvref_t<Action>>;

}

// Global Core Match Evaluator Implementation
template <typename TargetType, typename KeyType>
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept {
    if constexpr (mini_concepts::IsWildcard<KeyType>) {
        return true;
    }
    else if constexpr (mini_concepts::IsTypePredicate<KeyType>) {
        using TargetDecay = used_std::decay_t<TargetType>;
        
        auto evaluator = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) noexcept {
            using KeyDecay = used_std::decay_t<KeyType>;
            if constexpr (KeyDecay::policy_value == TypePolicy::Any) {
                return (KeyDecay::template Trait<typename used_std::tuple_element<Is, TargetDecay>::type>::value || ...);
            } else {
                return (KeyDecay::template Trait<typename used_std::tuple_element<Is, TargetDecay>::type>::value && ...);
            }
        };
        
        return evaluator(used_std::make_index_sequence<used_std::tuple_size_v<TargetDecay>>{});
    }
    else if constexpr (mini_concepts::IsTupleIterator<KeyType>) {
        using TargetDecay = used_std::decay_t<TargetType>;
        
        auto unroller = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) noexcept {
            if constexpr (used_std::decay_t<KeyType>::policy == MatchPolicy::Any) {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) || ...);
            } else {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) && ...);
            }
        };
        
        return unroller(used_std::make_index_sequence<used_std::tuple_size_v<TargetDecay>>{});
    }
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
    else if constexpr (mini_concepts::IsSeqOffset<KeyType>) {
        used_std::UniversalView view(target);
        using KeyDecay = used_std::decay_t<KeyType>;
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
    else if constexpr (mini_concepts::IsBitwise<KeyType>) {
        using KeyDecay = used_std::decay_t<KeyType>;
        if constexpr (KeyDecay::policy_value == BitPolicy::AnySet) { return (target & key.bit_mask) != 0; }
        else if constexpr (KeyDecay::policy_value == BitPolicy::AllSet) { return (target & key.bit_mask) == key.bit_mask; }
        else if constexpr (KeyDecay::policy_value == BitPolicy::AllClear) { return (target & key.bit_mask) == 0; }
        return false;
    }
    else if constexpr (mini_concepts::ContainsRange<KeyType, TargetType>) {
        return key.contains(target);
    }
    else if constexpr (mini_concepts::MatchesPredicate<KeyType, TargetType>) {
        return key.matches(target);
    }
    else {
        return (target == key);
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
        if (condition) [[likely]] {
            return true;
        }
        return false;
    } 
    else if constexpr (Hint == BranchHint::Unlikely) {
        if (condition) [[unlikely]] {
            return true;
        }
        return false;
    } 
    else {
        return condition;
    }
}

template <typename ActionType, typename ContextType> 
requires (mini_concepts::ActionExecutable<ActionType, ContextType>)
constexpr decltype(auto) execute_action(ActionType&& action, ContextType& ctx) {
    using ActionDecay  = used_std::decay_t<ActionType>;
    using CleanContext = used_std::decay_t<ContextType>;

    // 1. If Action is ALREADY a passive signal or primitive value (not a function/lambda)
    if constexpr (mini_concepts::IsGotoSignal<ActionDecay> || 
                  mini_concepts::IsFallthroughSignal<ActionDecay> || 
                  mini_concepts::Primitive<ActionDecay>) 
    {
        return used_std::forward<ActionType>(action);
    }
    if constexpr (mini_concepts::IsWildcard<ActionDecay>) 
    {
        return true;
    }
    // 2. If Action is Callable (including function pointers like goto_case<1>)
    else if constexpr (mini_concepts::TupleLike<CleanContext> && (used_std::tuple_size_v<CleanContext> > 0)) {
        if constexpr (constexpr used_std::ptrdiff_t PrefixLen = used_std::find_matching_context_prefix<ActionDecay, CleanContext>(); PrefixLen != -1) {
            return [&]<used_std::size_t... Iss>(used_std::index_sequence<Iss...>) -> decltype(auto) {
                return used_std::forward<ActionType>(action)(used_std::get<Iss>(ctx)...);
            }(used_std::make_index_sequence<PrefixLen>{});
        }
        
        // Case 4: Target + Partial or Full Context Tuple
        else if constexpr (constexpr std::ptrdiff_t PrefixLen = used_std::find_matching_target_context_prefix<ActionDecay, used_std::declval<ActionDecay>(), CleanContext>(); PrefixLen != -1) {
            return [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> decltype(auto) {
                return used_std::forward<ActionType>(action)(used_std::declval<ActionDecay>(action), std::get<Is>(ctx)...);
            }(used_std::make_index_sequence<PrefixLen>{});
        }

        auto unpacker = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> decltype(auto) {
            if constexpr (mini_std::invocable <ActionDecay, typename used_std::tuple_element<Is, CleanContext>::type...>) {
                return used_std::forward<ActionType>(action)(used_std::get<Is>(ctx)...);
            } 
            else if constexpr (mini_std::invocableNoargs<ActionDecay>) {
                return used_std::forward<ActionType>(action)();
            }
            else {
                return used_std::forward<ActionType>(action);
            }
        };
        return unpacker(used_std::make_index_sequence<used_std::tuple_size_v<CleanContext>>{});

    } else {
        if constexpr (mini_std::invocable<ActionDecay, ContextType>) {
            return used_std::forward<ActionType>(action)(ctx);
        } else if constexpr (mini_std::invocableNoargs<ActionDecay>) {
            return used_std::forward<ActionType>(action)();
        } else {
            return used_std::forward<ActionType>(action);
        }
    }
}

template <typename T>
struct UnwrapReturnType { using type = used_std::remove_cvref_t<T>; };

// template <typename T>
// requires mini_concepts::IsValueFallthrough<T> || mini_concepts::IsValueGoto<T>
// struct UnwrapReturnType<T> { 
//     using type = used_std::remove_cvref_t<decltype(used_std::declval<T>().value)>; 
// };

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
requires (mini_concepts::TupleLike<ContextTuple> && mini_concepts::TupleLike<CasesTuple>)
constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
    constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;
    
    using CoreReturnType  = decltype(execute_action(default_action, ctx));
    using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;
    
    auto unrolled_matrix_router = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> CleanReturnType {
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
                        
                        decltype(auto) action_res = execute_action(current_case.action, ctx);
                        using ActionDecay = used_std::decay_t<decltype(action_res)>;

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
            
            return execute_action(default_action, ctx);
        }
    };

    return unrolled_matrix_router(used_std::make_index_sequence<TotalCases>{});
}

// ============================================================================
// 9. PACK SEPARATION & DELAYED UNIVERSAL INITIALIZATION
// ============================================================================
namespace mini_pack {
    template <used_std::size_t Index, typename... Ts> struct pack_element;

    template <typename Head, typename... Tail> struct pack_element<0, Head, Tail...> {
        using type = Head;
        constexpr static decltype(auto) get(Head&& head, Tail&&...) noexcept { return used_std::forward<Head>(head); }
    };

    template <used_std::size_t Index, typename Head, typename... Tail> struct pack_element<Index, Head, Tail...> {
        using type = typename pack_element<Index - 1, Tail...>::type;
        constexpr static decltype(auto) get(Head&&, Tail&&... tail) noexcept { 
            return pack_element<Index - 1, Tail...>::get(used_std::forward<Tail>(tail)...); 
        }
    };

    template <typename Tuple, used_std::size_t... Is> requires (mini_concepts::TupleLike<Tuple>)
    constexpr auto make_cases_tuple_impl(Tuple&& full_tuple, used_std::index_sequence<Is...>) {
        // Explicitly specify element types in tuple template arguments
        return used_std::tuple<
            used_std::remove_cvref_t<decltype(used_std::get<Is>(full_tuple))>...
        >(used_std::get<Is>(used_std::forward_as_tuple(full_tuple))...);
    }

    template <used_std::size_t... Is, typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple> requires (
        mini_concepts::TupleLike<ContextTuple> && mini_concepts::TupleLike<CasesTuple>
    )
    constexpr auto evaluate_sorted_matrix(used_std::index_sequence<Is...>, const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
        if constexpr (sizeof...(Is) > 0) {
            return universal_switch_matrix(
                target, 
                used_std::forward<DefaultType>(default_action), 
                ctx,
                used_std::forward<CasesTuple>(cases)
            );
        } else {
            return execute_action(used_std::forward<DefaultType>(default_action), ctx);
        }
    }
}

template <typename TargetType, typename ContextTuple, typename... AllTrailingArgs> requires (mini_concepts::TupleLike<ContextTuple>)
constexpr auto universal_switch(const TargetType& target, ContextTuple& ctx, AllTrailingArgs&&... args) {
    constexpr used_std::size_t TotalArgs = sizeof...(AllTrailingArgs);
    static_assert(TotalArgs >= 1, "Library Error: You must supply a terminal fallback default action.");
    
    constexpr used_std::size_t CaseCount = TotalArgs - 1;
    
    // Helper to invoke with indexed arguments without double-forwarding
    return [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> decltype(auto) {
        // Forward all arguments as a reference tuple without constructing/copying elements
        auto args_tuple = used_std::forward_as_tuple(used_std::forward<AllTrailingArgs>(args)...);

        // Extract default action (the last argument at index CaseCount)
        decltype(auto) default_action = used_std::get<CaseCount>(args_tuple);

        if constexpr (CaseCount == 0) {
            return execute_action(used_std::forward<decltype(default_action)>(default_action), ctx);
        } else {
            // Extract ONLY the case items into a tuple of references/values
            auto cases_tuple = used_std::tuple<
                used_std::remove_cvref_t<decltype(used_std::get<Is>(args_tuple))>...
            >(used_std::get<Is>(used_std::move(args_tuple))...);

            return mini_pack::evaluate_sorted_matrix(
                used_std::make_index_sequence<CaseCount>{}, 
                target, 
                used_std::forward<decltype(default_action)>(default_action), 
                ctx, 
                cases_tuple
            );
        }
    }(used_std::make_index_sequence<CaseCount>{});
}
// ============================================================================
// 10. HIGH-UTILITY PROXY WRAPPERS (uswitch DSL FRONTEND ENTRY)
// ============================================================================
template <typename TargetType, typename ContextTuple> requires (mini_concepts::TupleLike<ContextTuple>)
struct SwitchPipelineProxy {
    const TargetType& target;
    ContextTuple ctx;

    template <typename... CaseTypes>
    constexpr decltype(auto) operator()(CaseTypes&&... cases) && {
        return universal_switch(target, ctx, used_std::forward<CaseTypes>(cases)...);
    }
};

template <typename TargetType>
struct SwitchTargetProxy {
    const TargetType& target;

    template <typename ContextArgs> requires (mini_concepts::IsWildcard<ContextArgs>)
    constexpr auto operator[](ContextArgs& args) && noexcept {
        using EmptyTuple = used_std::tuple<>;
        return SwitchPipelineProxy<TargetType, EmptyTuple>{ target, EmptyTuple{} };
    }

    template <typename... ContextArgs>
        requires (sizeof...(ContextArgs) > 0) 
    constexpr auto operator[](ContextArgs&&... args) && noexcept {
        using TupleType = used_std::tuple<ContextArgs&...>;
        return SwitchPipelineProxy<TargetType, TupleType>{ target, used_std::forward_as_tuple(args...) };
    }

};

template <typename TargetType>
constexpr auto Match(const TargetType& target) noexcept {
    return SwitchTargetProxy<TargetType>{ target };
}

// --- A. Numeric & Range Matching ---
void showcase_numeric_and_ranges(int score) {
    std::cout << "\n=== 1. Numeric & Range Pattern Matching ===" << std::endl;
    
    std::string_view result = Match(score)[&score] (
        Case(100)                      >> [](int*) { return "Perfect Score!"; },
        Case(Range<int>{90, 99})       >> [](int*) { return "Grade: A"; },
        Case(Range<int>{80, 89})       >> [](int*) { return "Grade: B"; },
        Case(Range<int>{70, 79})       >> [](int*) { return "Grade: C"; },
        [](int* s) { 
            return *s < 70 ? "Grade: Fail" : "Grade: Invalid"; 
        }
    );

    std::cout << "Score [" << score << "] -> " << result << std::endl;
}

// --- B. StaticLabel / FNV-1a Hash Matching ---
void showcase_hash_labels(std::string_view command) {
    std::cout << "\n=== 2. StaticLabel Hash Matching ===" << std::endl;

    std::size_t cmd_hash = used_std::strHash::fnv1a_hash(command.data(), command.size());

    std::string_view response = Match(command)[command,&cmd_hash] (
        Case("start")  >> [] { return "System Starting..."; },
        Case("stop")   >> [](std::size_t*) { return "System Stopping..."; },
        Case("pause")  >> [] { return "System Paused."; },
        Case(any_value)>> [] { return "Unknown Command!"; },
        []{return "UNDEFINED!";}
    );

    std::cout << "Command [\"" << command << "\"] (Hash: " << cmd_hash << ") -> " << response << std::endl;
}

// --- C. Branch Prediction Hints ---
void showcase_branch_hints(int http_code) {
    std::cout << "\n=== 3. Branch Hint Guided Dispatch ===" << std::endl;

    std::string_view status = Match(http_code)[any_value] (
        // Common paths marked as likely
        Case<BranchHint::Likely>(200)   >> []() { return "200 OK (Fast Path)"; },
        Case<BranchHint::Likely>(404)   >> []() { return "404 Not Found"; },
        
        // Exceptional paths marked as unlikely
        Case<BranchHint::Unlikely>(500) >> []() { return "500 Internal Server Error"; },
        []() { return "Other HTTP Status"; }
    );

    std::cout << "HTTP [" << http_code << "] -> " << status << std::endl;
}

int main () {
    std::cout << "=================================================" << std::endl;
    std::cout << "       PATTERN MATCHING LIBRARY SHOWCASE         " << std::endl;
    std::cout << "=================================================" << std::endl;

    // 1. Numeric Range Showcase
    showcase_numeric_and_ranges(95);
    showcase_numeric_and_ranges(72);
    showcase_numeric_and_ranges(45);

    // 2. Compile-Time Hash Labels Showcase
    showcase_hash_labels("start");
    showcase_hash_labels("pause");
    showcase_hash_labels("reboot");
    showcase_hash_labels("rebootss");

    // 3. Branch Prediction Showcase
    showcase_branch_hints(200);
    showcase_branch_hints(500);
    // std::printf("%d %d" , ret , num);
    return 1;
}