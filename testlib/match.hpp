
#ifndef MATCH_H
#define MATCH_H


    #include <type_traits>
    #include <tuple>
    #include <utility>
    // #include "mini_std.hpp"


// ============================================================================
// BEGIN. USED DEFINITIONS
// ============================================================================

namespace used_std {
    using namespace std; 

    template <typename T, typename = void>
    struct is_tuple_like : used_std::false_type {};
    template <typename T>
    struct is_tuple_like<T, used_std::void_t<decltype(used_std::tuple_size<used_std::remove_cvref_t<T>>::value)>> 
        : used_std::true_type {};

    template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

    template <typename T>
    concept is_tuple = requires { used_std::is_tuple_like_v<T>; };

    template <typename T, typename U>
    struct is_same_template : used_std::false_type {};

    template <template <typename...> class TemplateClass, typename... Args1, typename... Args2>
    struct is_same_template<TemplateClass<Args1...>, TemplateClass<Args2...>> : used_std::true_type {};

    template <typename T, typename U>
    inline constexpr bool is_same_template_v = used_std::is_same_template<T, U>::value;

    // Base helper for non-member argument traits
    template <typename Ret, typename... Args>
    struct function_traits_base {
        using return_type     = Ret;
        using args_tuple      = used_std::tuple<Args...>;
        using args_tuple_temp = used_std::tuple<used_std::decay_t<Args>...>; // Value types to prevent dangling refs
        using args_tuple_ptr  = used_std::tuple<used_std::add_pointer_t<used_std::decay_t<Args>>...>;
        using Idx_seq         = used_std::make_index_sequence<sizeof...(Args)>;

        static constexpr used_std::size_t args = sizeof...(Args);

        template <used_std::size_t N>
        using arg_type = used_std::tuple_element_t<N, args_tuple>;
    };

    // 1. Correct Primary Template (Single parameter T)
    template <typename T, typename = void>
    struct function_traits : function_traits_base<T> {
        static constexpr bool is_function          = false;
        static constexpr bool is_member_function   = false;
        static constexpr bool is_noexcept          = false;
    };
    #define DEFINE_FUNCTION_TRAITS(ACCESSOR,QUALIFIERS,NOEXCEPT_VAL) \
    template <typename Ret, typename... Args>                \
    struct function_traits<Ret ACCESSOR (Args...) QUALIFIERS>\
    : function_traits_base<Ret, Args...> {                   \
        using fn_type = Ret ACCESSOR (Args...) QUALIFIERS ;  \
        static constexpr bool is_function         = true;        \
        static constexpr bool is_member_function  = false;       \
        static constexpr bool is_noexcept         = NOEXCEPT_VAL;\
    };                                                       
    DEFINE_FUNCTION_TRAITS(,,false)
    DEFINE_FUNCTION_TRAITS(,volatile,false)
    DEFINE_FUNCTION_TRAITS(,noexcept,true)
    // DEFINE_FUNCTION_TRAITS(,noexcept volatile,true)

    DEFINE_FUNCTION_TRAITS((*),,false)
    DEFINE_FUNCTION_TRAITS((*),noexcept,true)

    DEFINE_FUNCTION_TRAITS((&),,false)
    DEFINE_FUNCTION_TRAITS((&),noexcept,true)

    #undef DEFINE_FUNCTION_TRAITS
    #undef EMPTY

    // Macro helper to generate member function pointer specializations cleanly
    #define DEFINE_MEMBER_FUNCTION_TRAITS(QUALIFIERS, NOEXCEPT_VAL)                         \
    template <typename Ret, typename Class, typename... Args>                               \
    struct function_traits<Ret (Class::*)(Args...) QUALIFIERS>                              \
        : function_traits_base<Ret, Args...> {                                              \
        using class_type = Class;                                                           \
        using fn_type    = Ret (Class::*)(Args...) QUALIFIERS;                              \
        using args_tuple_class_temp = used_std::tuple<Class*, used_std::decay_t<Args>...>;  \
        static constexpr bool is_function        = false;                                   \
        static constexpr bool is_member_function = true;                                    \
        static constexpr bool is_noexcept        = NOEXCEPT_VAL;                            \
    };

    // Generate for standard combinations of const / volatile / ref / noexcept
    DEFINE_MEMBER_FUNCTION_TRAITS(, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(const, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(volatile, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(const volatile, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(&, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(const &, false)
    DEFINE_MEMBER_FUNCTION_TRAITS(&&, false)

    DEFINE_MEMBER_FUNCTION_TRAITS(noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(const noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(volatile noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(const volatile noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(& noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(const & noexcept, true)
    DEFINE_MEMBER_FUNCTION_TRAITS(&& noexcept, true)

    #undef DEFINE_MEMBER_FUNCTION_TRAITS

    // 5. Callable traits dispatcher
    template <typename T, typename = void>
    struct callable_traits {
        using type = function_traits<T>;
    };

    // Specialization for Lambdas / Functors with operator()
    template <typename T>
    struct callable_traits<T, used_std::void_t<decltype(&used_std::remove_cvref_t<T>::operator())>> {
        using type = typename callable_traits<decltype(&used_std::remove_cvref_t<T>::operator())>::type;
    };

    template <typename T>
    using callable_traits_t = typename callable_traits<used_std::remove_cvref_t<T>>::type;

    template<typename Fn>
    concept callable_type = 
    callable_traits_t<Fn>::is_function ||
    callable_traits_t<Fn>::is_member
    ;

   template <typename Fn, typename... Args>
    concept invocable = requires (Fn&& a,Args&&... args) {
        used_std::invoke(used_std::forward<Fn>(a),used_std::forward<Args>(args)...);
    };

    template <typename ContextTuple>
    using CleanContextDecayed = decltype([]<typename... Ts>(used_std::tuple<Ts...>*) {
        return used_std::tuple<used_std::decay_t<Ts>...>{};
    }(static_cast<used_std::remove_cvref_t<ContextTuple>*>(nullptr)));

    struct string_view {
            const char* data = nullptr;
            used_std::size_t len = 0;
            constexpr string_view() = default;
            constexpr string_view(const char* str) noexcept : data(str) { while (str[len] != '\0') { ++len; } }
            constexpr bool operator==(const string_view& other) const noexcept {
                if (len != other.len) return false;
                for (used_std::size_t i = 0; i < len; ++i) { if (data[i] != other.data[i]) return false; }
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
        used_std::size_t length = 0;

        constexpr UniversalView() = default;

        template <used_std::size_t N>
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

    //========================================
    //              TUPLE STUFF
    //========================================
    
    
    namespace detail {

        // Equivalent operator version
        template <used_std::size_t... Is1, used_std::size_t... Is2>
        constexpr auto operator+(index_sequence<Is1...>, index_sequence<Is2...>) {
            return index_sequence<Is1..., Is2...>{};
        }

        // Helper to extract the size of an index_sequence
        template <typename Seq>
        struct index_sequence_size;

        template <used_std::size_t... Is>
        struct index_sequence_size<used_std::index_sequence<Is...>> {
            static constexpr used_std::size_t value = sizeof...(Is);
        };
        
        // 1. Count how many times TargetType appeared in TupleA BEFORE position UpToIdx
        template <typename TargetType, typename TupleA, used_std::size_t UpToIdx>
        constexpr used_std::size_t count_previous_occurrences() {
            return []<used_std::size_t... Is>(used_std::index_sequence<Is...>) {
                return ((used_std::is_same_v<
                    TargetType,
                    used_std::remove_cvref_t<used_std::tuple_element_t<Is, TupleA>>
                > ? 1 : 0) + ... + 0);
            }(used_std::make_index_sequence<UpToIdx>{});
        }

        template <typename TargetType, typename Tuple, used_std::size_t TargetOccurrence, 
        used_std::size_t CurrentIdx = 0>
        constexpr used_std::size_t find_matching_context_index() {
            constexpr used_std::size_t TupleSize = used_std::tuple_size_v<Tuple>;
            
            if constexpr (CurrentIdx >= TupleSize) {
                return static_cast<used_std::size_t>(-1);
            } 
            else {
                using ElementType = used_std::remove_cvref_t<used_std::tuple_element_t<CurrentIdx, Tuple>>;
                
                if constexpr (used_std::is_same_v<TargetType, ElementType>) {
                    // if constexpr (FoundCount == TargetOccurrence) {
                        return CurrentIdx; // FOUND MATCH
                    // } else {
                    //     return find_matching_context_index<
                    //         TargetType, Tuple, TargetOccurrence, CurrentIdx + 1>();
                    // }
                } else {
                    return find_matching_context_index<
                        TargetType, Tuple, TargetOccurrence, CurrentIdx + 1>();
                }
            }
        }
        template <typename TargetType, typename Tuple, used_std::size_t TargetOccurrence, 
        used_std::size_t CurrentIdx = 0, used_std::size_t FoundCount = 0>
        constexpr used_std::size_t find_nth_matching_context_index() {
            constexpr used_std::size_t TupleSize = used_std::tuple_size_v<Tuple>;
            
            if constexpr (CurrentIdx >= TupleSize) {
                return static_cast<used_std::size_t>(-1);
            } 
            else {
                using ElementType = used_std::remove_cvref_t<used_std::tuple_element_t<CurrentIdx, Tuple>>;
                
                if constexpr (used_std::is_same_v<TargetType, ElementType>) {
                    if constexpr (FoundCount == TargetOccurrence) {
                        return CurrentIdx; // FOUND MATCH
                    } else {
                        return find_nth_matching_context_index<
                            TargetType, Tuple, TargetOccurrence, CurrentIdx + 1, FoundCount + 1>();
                    }
                } else {
                    return find_nth_matching_context_index<
                        TargetType, Tuple, TargetOccurrence, CurrentIdx + 1, FoundCount>();
                }
            }
        }

        template <typename TupleA, typename TupleB>
        constexpr auto cross_index_type() {
            constexpr used_std::size_t SizeA = used_std::tuple_size_v<TupleA>;

            return []<used_std::size_t... IsA>(used_std::index_sequence<IsA...>) {
                
                auto map_parameter = []<used_std::size_t IdxA>() {
                    using CleanTypeA = used_std::remove_cvref_t<used_std::tuple_element_t<IdxA, TupleA>>;
                    
                    // Determine occurrence index for duplicate parameters in TupleA
                    constexpr used_std::size_t Occurrence = count_previous_occurrences<CleanTypeA, TupleA, IdxA>();
                    constexpr used_std::size_t MatchedIdxB = find_nth_matching_context_index<CleanTypeA, TupleB, Occurrence>();
                    // static_assert(MatchedIdxB != static_cast<used_std::size_t>(-1), "Out of bound" );
                    return used_std::conditional_t<
                    MatchedIdxB != static_cast<used_std::size_t>(-1), 
                    used_std::index_sequence<MatchedIdxB>, 
                    used_std::index_sequence<>>{};
                };

                return (map_parameter.template operator()<IsA>() + ... + used_std::index_sequence<>{});

            }(used_std::make_index_sequence<SizeA>{});
        }
        template <typename TupleA, typename TupleB>
        constexpr auto cross_index_type2() {
            constexpr used_std::size_t SizeA = used_std::tuple_size_v<TupleA>;

            return []<used_std::size_t... IsA>(used_std::index_sequence<IsA...>) {
                
                auto map_parameter = []<used_std::size_t IdxA>() {
                    using CleanTypeA = used_std::remove_cvref_t<used_std::tuple_element_t<IdxA, TupleA>>;
                    
                    // Determine occurrence index for duplicate parameters in TupleA
                    // constexpr used_std::size_t Occurrence = count_previous_occurrences<CleanTypeA, TupleA, IdxA>();
                    constexpr used_std::size_t MatchedIdxB = find_matching_context_index<CleanTypeA, TupleB, IdxA>();
                    // static_assert(MatchedIdxB != static_cast<used_std::size_t>(-1), "Out of bound" );
                    return used_std::conditional_t<
                    MatchedIdxB != static_cast<used_std::size_t>(-1), 
                    used_std::index_sequence<MatchedIdxB>, 
                    used_std::index_sequence<>>{};
                };

                return (map_parameter.template operator()<IsA>() + ... + used_std::index_sequence<>{});
 
            }(used_std::make_index_sequence<SizeA>{});
        }
    }

    // Public alias template
    template <typename TupleA, typename TupleB>
    using get_matching_indices2_t = decltype(
        detail::cross_index_type2<TupleA, TupleB>()
    );
    template <typename TupleA, typename TupleB>
    using get_matching_indices_t = decltype(
        detail::cross_index_type<TupleA, TupleB>()
    );
    
    template <typename TupleA, typename TupleB>
    constexpr auto count_total_matches_t = detail::index_sequence_size<get_matching_indices_t<TupleA, TupleB>>::value;
    
    template <typename TupleA, typename TupleB>
    constexpr bool is_one_matching_index_t = (count_total_matches_t<TupleA, TupleB> == 1);

    
    using Tuple1 = used_std::tuple<int, short, char, float, double,short,int>;
    using Tuple2 = used_std::tuple<int>;
    using test_matchCoord = get_matching_indices_t<Tuple2,Tuple1>;
    static_assert(used_std::is_same_v<test_matchCoord, used_std::index_sequence<0>>,"");

    template <auto Accessor, auto Value, typename Tuple>
    constexpr used_std::size_t find_index_v = []<used_std::size_t... Is>(used_std::index_sequence<Is...>) {
        using CleanTuple = used_std::remove_cvref_t<Tuple>;
        used_std::size_t found_index = used_std::tuple_size_v<CleanTuple>;

        ((void)((Accessor.template operator()<used_std::remove_cvref_t<used_std::tuple_element_t<Is, CleanTuple>>>() == Value)
                ? (found_index = Is)
                : 0
        )or ...);

        return found_index;
    }(used_std::make_index_sequence<used_std::tuple_size_v<used_std::remove_cvref_t<Tuple>>>{});

    template <auto Accessor,typename T,typename CasesTuple, used_std::size_t... Is>
    constexpr used_std::size_t find_by_value(T target_hash, used_std::index_sequence<Is...>) {
        using CleanTuple = used_std::remove_cvref_t<CasesTuple>;
        used_std::size_t found_index = static_cast<used_std::size_t>(-1);
        auto getWithLabel = [&]<used_std::size_t Idx>(){
            using element = used_std::remove_cvref_t<used_std::tuple_element_t<Idx, CleanTuple>>;
            if constexpr ( requires (element tt) { tt.label;}) {
                if (Accessor.template operator()<element>() == target_hash) {
                    found_index = Idx;
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        };
        (getWithLabel.template operator()<Is>() || ...);
        // ((Accessor.template operator()<used_std::remove_cvref_t<used_std::tuple_element_t<Is, CleanTuple>>>() == target_hash ? (found_index = Is) : 0) or ...);
        
        return found_index;
    }

    template<typename Fn,typename Tuple ,used_std::size_t... I>
    inline constexpr auto apply_index (Fn&& fn,Tuple& t,used_std::index_sequence<I...>) 
    noexcept(noexcept(used_std::forward<Fn>(fn)(used_std::get<I>(t)...))) 
    {
        return used_std::forward<Fn>(fn)(used_std::get<I>(t)...);
    }

}
template <used_std::size_t N>
struct StaticString {
    char data[N]{};

    constexpr StaticString(const char (&str)[N]) {
        for (used_std::size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
    }

    constexpr operator const char*() const { return data; }
};

template <used_std::size_t N>
StaticString(const char (&)[N]) -> StaticString<N>;

struct StaticLabel {
    const unsigned int hash{0};
    
    // 1. String Literal Constructor (auto-hashes and stores string)
    template <used_std::size_t N>
    constexpr StaticLabel(StaticString<N> ss) noexcept 
    : hash(used_std::strHash::fnv1a_hash(ss.data, N)) 
    {}
    
    template <used_std::size_t N>
    constexpr StaticLabel(const char (&s)[N]) noexcept 
    : hash(used_std::strHash::fnv1a_hash(s, N)) 
    {}
    template <used_std::size_t N>
    constexpr StaticLabel(const StaticLabel& s) noexcept 
    : hash(s.hash) 
    {}

    // 2. Integer Constructor (stores int value and hashes the int)
    constexpr StaticLabel(int val) noexcept 
    : hash(itoHash(val))
    {}
    
    constexpr StaticLabel() = default;
    constexpr auto itoHash(int val) noexcept -> unsigned int {
        unsigned int ret;
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
        return ret  = used_std::strHash::fnv1a_hash(buf,static_cast<used_std::size_t>(len));
    }
    constexpr auto slen(const char* s) noexcept -> unsigned int {
        size_t ret{0};
        while (*s != '\0') {
            ++ret;
        }
        return ret;
    }
    
    constexpr bool operator==(const StaticLabel& other) const noexcept {  
        return hash == other.hash;
    }
    
    constexpr bool operator==(const unsigned int& other) const noexcept {  
        return hash == other;
    }

};

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
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, KeyType& key) noexcept;


// ============================================================================
// FLOW CONTROL STATE TRACE SIGNALS AND ENUMS
// ============================================================================
enum class FlowKind : int {
    Terminal,   
    Fallthrough, 
    Goto,
    Composite,
};


struct DefaultState{};
struct Wildcard {};
[[maybe_unused]] inline constexpr Wildcard __{};

template <FlowKind Kind>
struct SignalBase {
    static constexpr FlowKind static_flow_kind = Kind;
};

// 1. Fallthrough signals
struct fallthrough_t : SignalBase<FlowKind::Fallthrough> {};

// 2. Dynamic Hash Goto
struct goto_hash_t : SignalBase<FlowKind::Goto> { 
    const StaticLabel label {0}; 

    constexpr goto_hash_t() = default;
    constexpr explicit goto_hash_t(StaticLabel h) noexcept : label(h) {}
};

// 3. Static Label Goto (adds compile-time label metadata directly)
template <StaticLabel LabelID>
struct goto_case_t : SignalBase<FlowKind::Goto> {
    static constexpr bool is_static_label = true;
    static constexpr auto label = LabelID;
};


inline constexpr fallthrough_t fallthrough = fallthrough_t{};
template <StaticLabel LabelID> inline constexpr goto_hash_t Goto = goto_hash_t{LabelID};
template <StaticLabel LabelID> inline constexpr goto_case_t<LabelID> Goto_v = goto_case_t<LabelID>{};

namespace concepts {
    template <typename T>
    concept IsSignal = requires { T::static_flow_kind; };
    
    template <typename T>
    concept IsGotoSignal = used_std::derived_from<used_std::decay_t<T>, SignalBase<FlowKind::Goto>>;
    // template <typename T>
    // concept IsGotoValue = requires (T t) {
    //     requires IsGotoSignal<T> and t.value;
    // };
    template <typename T>
    concept IsFallthroughSignal = used_std::derived_from<used_std::decay_t<T>, SignalBase<FlowKind::Fallthrough>>;
    // template <typename T>
    // concept IsFallthroughValue = requires (T t) {
    //     requires IsFallthroughSignal<T> and t.value;
    // };
    
    // 2. Static vs Dynamic Goto Differentiation
    template <typename T>
    concept IsStaticGotoSignal = IsGotoSignal<T> and requires {
        requires used_std::decay_t<T>::is_static_label;
    };
    
    template <typename T>
    concept IsDynamicGotoSignal = IsGotoSignal<T> and !IsStaticGotoSignal<T>;
}

// ============================================================================
//  FUNCTION PREDICATES DEFINITIONS
// ============================================================================
// --- Free Function / Lambda Predicate Wrapper / Unbound Member Function Predicate ---

template <typename Fn>
concept IsCallableType = 
used_std::callable_traits_t<used_std::remove_pointer_t<Fn>>::is_function ||
used_std::callable_traits_t<used_std::remove_pointer_t<Fn>>::is_member_function;

template <typename T, typename = void>
struct Function_Traits_helper {
    using type       = used_std::callable_traits_t<T>;
    using fn_type    = Wildcard;
    using class_type = Wildcard;
};

template <typename T>
requires (used_std::callable_traits_t<T>::is_function)
struct Function_Traits_helper<T> {
    using type       = used_std::callable_traits_t<T>;
    using fn_type    = typename type::fn_type;
    using class_type = Wildcard;
};

// Specialization for Member Function Pointers and Lambda operator()s
template <typename T>
requires (used_std::callable_traits_t<T>::is_member_function)
struct Function_Traits_helper<T> {
    using type       = used_std::callable_traits_t<T>;
    using fn_type    = typename type::fn_type;
    using class_type = typename type::class_type;
};

template <IsCallableType Fn>
struct FnPredicate {
    using Helper     = Function_Traits_helper<Fn>;
    using Traits     = Helper::type;
    using class_type = Helper::class_type;
    using fn_type    = Helper::fn_type;
    static constexpr bool is_member_function = Traits::is_member_function;
    using instance_type = used_std::conditional_t<is_member_function, const class_type*, Wildcard>;
    const fn_type fn;
    instance_type instance;
    FnPredicate(Fn&& f) noexcept : fn(used_std::forward<Fn>(f)) {}
    FnPredicate(Fn&& f, class_type* inst) noexcept requires(is_member_function) 
    : fn(used_std::forward<Fn>(f)),instance(inst) {}

    template <typename Target>
    constexpr bool operator()(const Target& target) const noexcept {
        if constexpr (is_member_function) {
            if constexpr (used_std::is_convertible_v<Target,class_type>) {
                return used_std::invoke(fn, target);
            } else {
                return used_std::invoke(fn, instance ,target);
            }
        } else{
            return used_std::invoke(fn, target);
        }
    }
};


template <IsCallableType Fn>
constexpr auto Predicate(Fn&& fn) {
    return FnPredicate<Fn>(used_std::forward<Fn>(fn));
}
template <IsCallableType Fn,typename Class> 
constexpr auto Predicate(Fn&& fn,Class* obj) {
    return FnPredicate<Fn>(used_std::forward<Fn>(fn) ,obj);
}

// --- Projection Case (Pattern + Member Function Extractor) ---
template <IsCallableType Fn, typename ExpectedPattern>
struct ProjectionCaseimpl {
    using Helper = Function_Traits_helper<Fn>;
    using fnTraits = Helper::type;
    using fn_type = Helper::fn_type;
    using class_type = Helper::class_type;
    using args_type = typename fnTraits::args_tuple_temp;

    fn_type fn;
    args_type args;
    ExpectedPattern pattern;

    // Single-argument constructor (no projection args)
    ProjectionCaseimpl(Fn&& f, ExpectedPattern&& p) noexcept
        : fn(used_std::forward<Fn>(f)), 
          pattern(used_std::forward<ExpectedPattern>(p)) {}

    // Constructor accepting fn, tuple of args, and pattern
    template<typename TupleArgs>
    ProjectionCaseimpl(Fn&& f, TupleArgs&& a, ExpectedPattern&& p) noexcept 
        : fn(used_std::forward<Fn>(f)), 
          args(used_std::forward<TupleArgs>(a)),
          pattern(used_std::forward<ExpectedPattern>(p)) {}

    template<typename T = void>
    constexpr bool operator()() const noexcept {
        // std::apply handles both regular functions and member function pointers 
        // when arguments (including instance pointer/ref) are packed in a tuple.
        decltype(auto) extracted_val = used_std::apply(fn, args);
        return evaluate_match(extracted_val, pattern);
    }
};

// Case 1: No extra arguments
template <typename ExpectedPattern, IsCallableType Fn>
constexpr auto ProjectionCase(ExpectedPattern&& pattern, Fn&& fn) noexcept {
    using fn_t = used_std::remove_cvref_t<Fn>;
    return ProjectionCaseimpl<fn_t, used_std::decay_t<ExpectedPattern>>(
        used_std::forward<Fn>(fn),
        used_std::forward<ExpectedPattern>(pattern)
    );
}

// Case 2: General arguments
template <typename ExpectedPattern, IsCallableType Fn, typename... Args>
constexpr auto ProjectionCase(ExpectedPattern&& pattern, Fn&& fn, Args&&... args) noexcept {
    using fnTraits = used_std::callable_traits_t<used_std::remove_cvref_t<Fn>>;
    static_assert(sizeof...(Args) <= fnTraits::args, "Too many arguments provided for projection function");

    using fn_t = typename fnTraits::fn_type;
    return ProjectionCaseimpl<fn_t, used_std::decay_t<ExpectedPattern>>(
        used_std::forward<Fn>(fn), 
        used_std::make_tuple(used_std::forward<Args>(args)...),
        used_std::forward<ExpectedPattern>(pattern)
    );
}

// Case 3: Explicit object instance + member arguments
template <typename ExpectedPattern, IsCallableType Fn, typename Class, typename... Args>
constexpr auto ProjectionCase(ExpectedPattern&& pattern, Fn&& fn, Class* instance, Args&&... args) noexcept {
    using fnTraits = used_std::callable_traits_t<used_std::remove_cvref_t<Fn>>;
    using fn_t = typename fnTraits::fn_type;

    return ProjectionCaseimpl<fn_t, used_std::decay_t<ExpectedPattern>>(
        used_std::forward<Fn>(fn), 
        used_std::make_tuple(instance, used_std::forward<Args>(args)...),
        used_std::forward<ExpectedPattern>(pattern)
    );
}

template <typename T> struct is_fn_predicate : used_std::false_type {};
template <typename Fn> struct is_fn_predicate<FnPredicate<Fn>> : used_std::true_type {};
template <typename T> concept is_fnpredicate = is_fn_predicate<used_std::remove_cvref_t<T>>::value;

template <typename Fn>
struct is_projection_caseimpl : used_std::false_type {};
template <typename MemFn, typename ExpectedPattern>
struct is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>> : used_std::true_type {};
template <typename MemFn, typename ExpectedPattern>
concept is_projection_case = is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>>::value;
template<typename T>
struct TypePred {
    using type = T;
};

template<typename T>
inline constexpr auto Type_is = TypePred<T>{};

template <typename T>
struct is_type_pred : used_std::false_type {};

template <typename T>
struct is_type_pred<TypePred<T>> : used_std::true_type {};

template <typename T>
concept isTypePred = is_type_pred<T>::value;

// ============================================================================
// MATH INTERVAL DEFINITIONS
// ============================================================================
enum class RangeType { None, eq, neq, lt, lte, gt, gte, OneSize, Closed, Open, HalfOpenLeft, HalfOpenRight, Or, And };

template <RangeType iType = RangeType::None, typename T = DefaultState> 
requires (used_std::is_arithmetic_v<T> || used_std::is_same_v<T, DefaultState>)
struct Range_t {
    using enum RangeType;
    static constexpr auto op = iType;
    static constexpr used_std::size_t N = (op <= RangeType::OneSize) ? 1 : 2;  
    T data[N];

    constexpr Range_t() requires (N == 1) : data{} {}
    constexpr Range_t(T val) requires (N == 1) : data{val} {}
    constexpr Range_t(T lhs, T rhs) requires (N > 1) : data{lhs, rhs} {}

    template<typename Target>
    constexpr bool contains(Target target) const noexcept {
        if constexpr (op == None) {
            return false;
        }
        else if constexpr (op == eq || op == OneSize) {
            return target == data[0];
        }
        else if constexpr (op == neq) {
            return target != data[0];
        }
        else if constexpr (op == lt) {
            return target < data[0];
        }
        else if constexpr (op == lte) {
            return target <= data[0];
        }
        else if constexpr (op == gt) {
            return target > data[0];
        }
        else if constexpr (op == gte) {
            return target >= data[0];
        }
        else if constexpr (op == Closed) {
            return (target >= data[0]) && (target <= data[1]);
        } 
        else if constexpr (op == Open) {
            return (target > data[0]) && (target < data[1]);
        }
        else if constexpr (op == HalfOpenLeft) { // (min, max]
            return (target > data[0]) && (target <= data[1]);
        }
        else if constexpr (op == HalfOpenRight) { // [min, max)
            return (target >= data[0]) && (target < data[1]);
        }
        else if constexpr (op == Or) { // Outside (min, max)
            return (target < data[0]) || (target > data[1]);
        } 
        else {
            return false;
        }
    }

    // Friend operators for initial left-hand bounds (e.g., 1 < x)
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator <(R lhs, Range_t) noexcept {
        return Range_t<RangeType::lt, R>(lhs);
    }
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator <=(R lhs, Range_t) noexcept {
        return Range_t<RangeType::lte, R>(lhs);
    }
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator >(R lhs, Range_t) noexcept {
        return Range_t<RangeType::gt, R>(lhs);
    }
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator >=(R lhs, Range_t) noexcept {
        return Range_t<RangeType::gte, R>(lhs);
    }
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator ==(R lhs, Range_t) noexcept {
        return Range_t<RangeType::eq, R>(lhs);
    }
    template<typename R> requires (used_std::is_arithmetic_v<R>)
    friend constexpr auto operator !=(R lhs, Range_t) noexcept {
        return Range_t<RangeType::neq, R>(lhs);
    }

    // Member operators for chaining ranges (e.g., 1 < x < 5)
    constexpr auto operator <(T rhs) const noexcept {
        if constexpr (op == None) {
            return Range_t<RangeType::lt, T>(rhs);
        } 
        else if constexpr (op == gt || op == lt) {
            return Range_t<RangeType::Open, T>(data[0], rhs);
        }
        else if constexpr (op == gte) {
            return Range_t<RangeType::HalfOpenRight, T>(data[0], rhs);
        }
        else if constexpr (op == lte) {
            return Range_t<RangeType::HalfOpenLeft, T>(data[0], rhs);
        } else {
            return Range_t<RangeType::Open, T>(data[0], rhs);
        }
    }

    constexpr auto operator <=(T rhs) const noexcept {
        if constexpr (op == None) {
            return Range_t<RangeType::lte, T>(rhs);
        }
        else if constexpr (op == gt) {
            return Range_t<RangeType::HalfOpenLeft, T>(data[0], rhs);
        }
        else if constexpr (op == gte && op == lte) {
            return Range_t<RangeType::Closed, T>(data[0], rhs);
        }
        else if constexpr (op == lt) {
            return Range_t<RangeType::HalfOpenRight, T>(data[0], rhs);
        } 
        else {
            return Range_t<RangeType::Closed, T>(data[0], rhs);
        }
    }

    constexpr auto operator >(T rhs) const noexcept requires (op == None) {
        return Range_t<RangeType::gt, T>(rhs);
    }

    constexpr auto operator >=(T rhs) const noexcept requires (op == None) {
        return Range_t<RangeType::gte, T>(rhs);
    }

    constexpr auto operator ==(T rhs) const noexcept {
        return contains(rhs);
    }

    constexpr auto operator !=(T rhs) const noexcept {
        return !contains(rhs);
    }
};

template <RangeType iType = RangeType::Closed, typename T> 
requires (used_std::is_arithmetic_v<T>)
Range_t(T, T) -> Range_t<iType, T>;
namespace concepts {
    template <typename T>
    struct is_range_inst : used_std::false_type {};
    
    template <RangeType iType,typename T>
    struct is_range_inst<Range_t<iType,T>> : used_std::true_type {};
    
    template <typename T>
    concept is_range_instance = is_range_inst<used_std::remove_cvref_t<T>>::value;
}

inline constexpr auto Range = Range_t{}; 
template <used_std::size_t I, typename T>
struct RangeNode {
    T range;
};

// Helper implementation using index sequence expansion
template <RangeType Op, typename IndexSeq, typename... Ranges>
struct RangeCompoundImpl;

template <RangeType Op, used_std::size_t... Is, typename... Ranges>
struct RangeCompoundImpl<Op, used_std::index_sequence<Is...>, Ranges...> : private RangeNode<Is, Ranges>... {
    
    constexpr explicit RangeCompoundImpl(Ranges... r) 
        : RangeNode<Is, Ranges>{used_std::move(r)}... {}

    template <typename Target>
    constexpr bool contains(Target target) const noexcept {
        if constexpr (Op == RangeType::Or) {
            // Short-circuits on the FIRST 'true'
            return (RangeNode<Is, Ranges>::range.contains(target) || ...); 
        } else {
            // Short-circuits on the FIRST 'false'
            return (RangeNode<Is, Ranges>::range.contains(target) && ...); 
        }
    }
};

// Public user-facing struct inheriting from the implementation pack
template <RangeType Op = RangeType::Or, typename... Ranges> 
requires (concepts::is_range_instance<Ranges> && ...)
struct RangeCompound : public RangeCompoundImpl<Op, used_std::make_index_sequence<sizeof...(Ranges)>, Ranges...> {
    
    using Base = RangeCompoundImpl<Op, used_std::make_index_sequence<sizeof...(Ranges)>, Ranges...>;
    using Base::Base; // Inherit constructors

    static_assert(Op == RangeType::Or || Op == RangeType::And, "Requires RangeType And / Or");
};

template <RangeType Op = RangeType::Or, typename... Ranges> 
requires (concepts::is_range_instance<Ranges> && ...)
RangeCompound(Ranges...) -> RangeCompound<Op, Ranges...>;

template <RangeType Op = RangeType::Or,typename... Ranges>
constexpr auto make_compound_range(Ranges&&... ranges) noexcept {
    return RangeCompound<Op,used_std::decay_t<Ranges>...>{used_std::forward<Ranges>(ranges)...};
}
// ============================================================================
// FIELD PREDICATES
// ============================================================================
enum class Op { Eq, Neq, Gt, Gte, Lt, Lte };

template <typename ClassType, typename MemberType> 
requires (used_std::is_class_v<ClassType> && used_std::is_member_object_pointer_v<MemberType ClassType::*>)
struct FieldRule {
    const MemberType ClassType::*member_ptr;
    const Op op_tag = Op::Eq;
    const MemberType value;

    constexpr bool eval(const ClassType& obj) const noexcept {
        const auto& target_field = obj.*member_ptr;
        switch (op_tag) {
            case Op::Eq:  return target_field == value;
            case Op::Neq: return target_field != value;
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

template <typename ClassType, typename... Rules> 
requires ((is_field_rule<Rules>::value && ...))
struct MultiFieldPredicate {
    const used_std::tuple<const Rules...> rules;

    constexpr bool matches(const ClassType& obj) const noexcept {
        return evaluate_all(obj, used_std::make_index_sequence<sizeof...(Rules)>{});
    }

    // Making predicate callable as std::predicate / functor
    constexpr bool operator()(const ClassType& obj) const noexcept {
        return matches(obj);
    }

private:
    template <used_std::size_t... Is>
    constexpr bool evaluate_all(const ClassType& obj, used_std::index_sequence<Is...>) const noexcept {
        return (used_std::get<Is>(rules).eval(obj) && ...);
    }
};

// Deduction Guide for MultiFieldPredicate
template <typename HeadRule, typename... TailRules>
requires (is_field_rule<used_std::decay_t<HeadRule>>::value && 
         (is_field_rule<used_std::decay_t<TailRules>>::value && ...))
MultiFieldPredicate(HeadRule, TailRules...) 
    -> MultiFieldPredicate<typename is_field_rule<used_std::decay_t<HeadRule>>::class_type, HeadRule, TailRules...>;

// FIX 2: Accept const MemberType& to allow passing both lvalues and rvalues
template <typename ClassType, typename MemberType>
constexpr auto field(MemberType ClassType::*member, Op op, const MemberType& val) noexcept {
    return FieldRule<ClassType, MemberType>{member, op, val};
}

template <typename ClassType, typename MemberType>
constexpr auto field(MemberType ClassType::*member, const MemberType& val) noexcept {
    return FieldRule<ClassType, MemberType>{member, Op::Eq, val};
}

// FIX 3: Deduce ClassType directly from the HeadRule using CTAD
template <typename HeadRule, typename... TailRules>
constexpr auto fields_match(HeadRule&& head, TailRules&&... tail) noexcept {
    using Class = typename is_field_rule<used_std::decay_t<HeadRule>>::class_type;
    return MultiFieldPredicate<Class, used_std::decay_t<HeadRule>, used_std::decay_t<TailRules>...>{
        used_std::tuple{used_std::forward<HeadRule>(head), used_std::forward<TailRules>(tail)...}
    };
}

// ============================================================================
// TUPLES, VIEWS, BITS
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
template <BitPolicy Policy, typename T> struct BitwisePredicate { 
    T bit_mask; 
    constexpr bool operator==(const T& target) const noexcept {
        if constexpr (Policy == BitPolicy::AnySet)   { return (target & bit_mask) != 0; }
        if constexpr (Policy == BitPolicy::AllSet)   { return (target & bit_mask) == bit_mask; }
        if constexpr (Policy == BitPolicy::AllClear) { return (target & bit_mask) == 0; }
        return false;
    }
};

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
// C++20 CONCEPT CONSTRAINTS IDENTIFICATION MATRIX
// ============================================================================
namespace concepts {
    
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

        template <typename F, typename Tuple, used_std::size_t... Is>
        struct is_tuple_invocable_impl<F, Tuple, used_std::index_sequence<Is...>> 
            : used_std::is_invocable<F, used_std::remove_cvref_t<used_std::tuple_element_t<Is, Tuple>>...> {};

    }
    template <typename Action, typename Tuple>
    concept TupleInvocable = detail::is_tuple_invocable_impl<
        Action, 
        Tuple, 
        used_std::make_index_sequence<used_std::tuple_size_v<used_std::decay_t<Tuple>>>
    >::value;

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

    template <typename T>
    concept IsReferenceOrPointer = used_std::is_reference_v<T> || used_std::is_pointer_v<T>;

    template <typename Tuple>
    struct IsTupleOfRefsOrPointers : used_std::false_type {};

    template <typename... Elements>
    struct IsTupleOfRefsOrPointers<used_std::tuple<Elements...>> 
        : used_std::bool_constant<(IsReferenceOrPointer<Elements> && ...)> {};

    template <typename Tuple> 
    concept TupleOfRefsOrPointers = IsTupleOfRefsOrPointers<used_std::remove_cvref_t<Tuple>>::value;

   template <typename K, typename T>
    concept IsCallablePredicate = requires(K k, T t) {
        { used_std::invoke(k,t) } -> convertible_to<bool>;
    };  
    template <typename T>
    using primitive_param_t = used_std::conditional_t<
        used_std::is_fundamental_v<used_std::decay_t<T>>, 
        used_std::decay_t<T>, 
        used_std::decay_t<T>&
    >;
}

// Global Core Match Evaluator Implementation
template <typename TargetType, typename KeyType>
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, KeyType& key) noexcept {
    using TargetDecay = used_std::decay_t<TargetType>;
    using KeyDecay    = used_std::decay_t<KeyType>;

    // 1. Wildcard / Catch-all
    if constexpr (concepts::IsWildcard<KeyType>) {
        return true;
    }
    else if constexpr (isTypePred<KeyDecay>) {
        return used_std::is_same_v<typename KeyDecay::type, TargetDecay>;
    } 
    // 2. Type-Level Trait Matching on Tuples/Types
    else if constexpr (concepts::IsTypePredicate<KeyType>) {
        if constexpr (used_std::is_tuple<TargetDecay>) {
            auto evaluator = []<used_std::size_t... Is>(used_std::index_sequence<Is...>) noexcept {
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
    else if constexpr (concepts::IsTupleIterator<KeyType>) {
        static_assert(used_std::is_tuple<TargetDecay>, "IsTupleIterator target must be a tuple-like type");
        auto unroller = []<used_std::size_t... Is>(const TargetType& target, const KeyType& key,used_std::index_sequence<Is...>) noexcept {
            if constexpr (KeyDecay::policy == MatchPolicy::Any) {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) or ...);
            } else {
                return (evaluate_match(used_std::get<Is>(target), key.expected_value) and ...);
            }
        };
        return unroller(target,key,used_std::make_index_sequence<used_std::tuple_size_v<TargetDecay>>{});
    }
    // 4. Container Element Predicates (Any / All)
    else if constexpr (concepts::IsAnyElement<KeyType>) {
        used_std::UniversalView view(target);
        for (used_std::size_t i = 0; i < view.size(); ++i) {
            if (view.data()[i] == key.expected_value) return true;
        }
        return false;
    }
    else if constexpr (concepts::IsAllElement<KeyType>) {
        used_std::UniversalView view(target);
        for (used_std::size_t i = 0; i < view.size(); ++i) {
            if (view.data()[i] != key.expected_value) return false;
        }
        return true;
    }
    // 5. Sequence Slice / Sub-range Matching
    else if constexpr (concepts::IsSeqOffset<KeyType>) {
        used_std::UniversalView view(target);
        constexpr used_std::size_t Offset = KeyDecay::offset_val;
        constexpr used_std::size_t PatternLen = KeyDecay::pattern_len;
        
        if (Offset + PatternLen > view.size()) return false;
        for (used_std::size_t i = 0; i < PatternLen; ++i) {
            if (!(key.expected_pattern[i] == view.data()[Offset + i])) return false;
        }
        return true;
    }
    else if constexpr (concepts::IsSliceSize<KeyType>) {
        used_std::UniversalView view(target);
        return view.size() == key.expected_size;
    }
    // 6. Ranges
    else if constexpr (concepts::ContainsRange<KeyType, TargetType>) {
        return key.contains(target);
    }
    // 7. Function
    else if constexpr (concepts::MatchesPredicate<KeyType, TargetType>) {
        return used_std::invoke(key.matches, target);
    } 
    else if constexpr (concepts::IsCallablePredicate<KeyType, TargetType>) {
        return used_std::invoke(key, target);
    } 
    
    else if constexpr (requires { { target == key } -> concepts::convertible_to<bool>; }) {
        return (target == key);
    } 
    else {
        return false;
    }
}

// ============================================================================
// CASE STORAGE
// ============================================================================

template <typename ActionType, typename ContextType> 
inline constexpr decltype(auto) execute_action_impl(ActionType&& action, const ContextType& ctx) noexcept {
    using ActionDecay  = used_std::remove_cvref_t<ActionType>;
    using CleanContext = used_std::remove_cvref_t<ContextType>;
    using FnTrait      = used_std::callable_traits_t<ActionType>;

    if constexpr (concepts::IsWildcard<ActionDecay>) {
        return true;
    } else if constexpr (FnTrait::is_function || FnTrait::is_member_function) {
        if constexpr (FnTrait::args == 0) {
            return used_std::forward<ActionType>(action)();
        }
        else if constexpr (FnTrait::args == 1) {
            return used_std::forward<ActionType>(action)(ctx);
        }
        else if constexpr (FnTrait::args > 1) {
            using FnArgsTuple = typename FnTrait::args_tuple;
            using ResultSequence = used_std::get_matching_indices_t<FnArgsTuple, CleanContext>;
            
            return used_std::apply_index(used_std::forward<ActionType>(action), ctx, ResultSequence{});
        }
        else if constexpr (concepts::TupleInvocable<ActionDecay, CleanContext>) {
            return used_std::forward<ActionType>(action)(ctx);
        } 
    } else {
        return used_std::forward<ActionType>(action);
    }
}

// 2. Public wrapper function with automatic conditional noexcept deduction
template <typename ActionType, typename ContextType> 
inline constexpr decltype(auto) execute_action(ActionType&& action, const ContextType& ctx) 
    noexcept(noexcept(execute_action_impl(used_std::forward<ActionType>(action), ctx))) 
{
    return execute_action_impl(used_std::forward<ActionType>(action), ctx);
}

struct baseCase {};

template <typename KeyType = DefaultState, typename ActionType = DefaultState>
struct ImplCase final : baseCase {
    static constexpr bool isDefaultKey    = !used_std::is_same_v<KeyType, DefaultState>;
    static constexpr bool isDefaultAction = !used_std::is_same_v<ActionType, DefaultState>;

    const KeyType key;
    const ActionType action;
    
    constexpr ImplCase() = delete;

    constexpr ImplCase(const KeyType& k) noexcept requires (isDefaultKey)
    : key(k), 
    action(DefaultState{}) 
    {}
    constexpr ImplCase(KeyType&& k) noexcept requires (isDefaultKey)
    : key(used_std::forward<KeyType>(k)),
    action(DefaultState{}) 
    {}
    
    template <typename K, typename A>
    constexpr ImplCase(K&& k, A&& a) noexcept requires (isDefaultKey)
    : key(used_std::forward<K>(k)), action(used_std::forward<A>(a)) {}

    template <typename NewAction>
    constexpr auto operator>>(NewAction&& new_action) noexcept requires (isDefaultKey && !isDefaultAction) 
    {
        using Action_t = used_std::decay_t<NewAction>;
        return ImplCase<KeyType, Action_t>(
            used_std::move(key), 
            used_std::forward<const Action_t>(new_action)
        );
    }
};
template<typename Key>
ImplCase(Key) -> ImplCase<Key>;

template <StaticLabel LabelID, typename KeyType = DefaultState, typename ActionType = DefaultState>
struct ImplLabelCase final : baseCase {
    static constexpr bool isDefaultKey = !used_std::is_same_v<KeyType, DefaultState>;
    static constexpr bool isDefaultAction = !used_std::is_same_v<ActionType, DefaultState>;

    const KeyType key;
    const ActionType action;
    
    static constexpr auto label = LabelID;
    
    constexpr ImplLabelCase(const KeyType& k) noexcept requires (isDefaultKey)
    : key(k),
    action(DefaultState{}) 
    {}
    constexpr ImplLabelCase(KeyType&& k) noexcept requires (isDefaultKey)
    : key(used_std::forward<KeyType>(k)), 
    action(DefaultState{}) 
    {}
    
    template <typename K, typename A>
    constexpr ImplLabelCase(K&& k, A&& a) noexcept requires (isDefaultKey)
    : key(used_std::forward<K>(k)), action(used_std::forward<A>(a)) {}


    template <typename NewAction>
    constexpr auto operator>>(NewAction&& new_action) noexcept requires (isDefaultKey && !isDefaultAction)
    {
        using Action_t = used_std::decay_t<NewAction> ;
        return ImplLabelCase<LabelID,KeyType, Action_t>(
            used_std::move(key), 
            used_std::forward<const Action_t>(new_action)
        );
    }
};

template <typename T> 
constexpr auto Case(T&& __val) noexcept {
    using Key_t = used_std::decay_t<T>;
    return ImplCase<Key_t>(used_std::forward<T>(__val)); 
}
template <StaticLabel LabelID,typename T> 
constexpr auto Case(T&& __val) noexcept { 
    using Key_t = used_std::decay_t<T>;
    return ImplLabelCase<LabelID, Key_t>(used_std::forward<T>(__val)); 
}

template <typename T>
struct UnwrapReturnType { using type = used_std::remove_cvref_t<T>;};

template <>
struct UnwrapReturnType<fallthrough_t> {
    using type = Wildcard;
};
template <>
struct UnwrapReturnType<goto_hash_t> {
    using type = goto_hash_t;
};
template <>
struct UnwrapReturnType<void> {
    using type = void;
};

template <auto LabelID>
struct UnwrapReturnType<goto_case_t<LabelID>> {
    using type = goto_case_t<LabelID>;//decltype(LabelID);
};

template <typename T, typename = void>
struct get_action_type {
    using type = T;
};

template <typename T>
struct get_action_type<T, used_std::void_t<decltype(used_std::declval<T>().action)>> {
    using type = decltype(used_std::declval<T>().action);
};

template <bool Configured, typename CaseType, typename TargetType>
struct ReturnTypeHelper {
    using type = DefaultState;
};

template <typename CaseType, typename TargetType>
struct ReturnTypeHelper<true, CaseType, TargetType> {
    using ActionType = typename get_action_type<CaseType>::type;
    using type = 
    // used_std::decay_t<ActionType>; 
    decltype(execute_action(
        used_std::declval<ActionType>(), 
        used_std::declval<TargetType>()
    ));
};


// ============================================================================
//                               WRAPPERS 
// ============================================================================
template <typename TargetType = DefaultState, typename... Cases>
struct match_t {
    static constexpr used_std::size_t TotalCases    = sizeof...(Cases);
    
    static constexpr bool defaultTarget  = used_std::is_same_v<TargetType, DefaultState>;
    static constexpr bool defaultCases   = TotalCases == 0;
    static constexpr bool is_configured  = !defaultTarget && !defaultCases;

    static constexpr used_std::size_t TotalCasesIndex = is_configured ? TotalCases - 1 : 0;
    static constexpr used_std::size_t LastCaseIndex = TotalCasesIndex;
    
    using StoreTarget = TargetType;
    using StoreCases = used_std::conditional_t<is_configured, used_std::tuple<used_std::remove_cvref_t<Cases>...>, used_std::tuple<Wildcard>>;
    
    using CasesIndex = used_std::make_index_sequence<TotalCases>;
    using LastCaseType  = used_std::tuple_element_t<LastCaseIndex, StoreCases>;

    using RawReturnType = typename ReturnTypeHelper<is_configured, used_std::tuple_element_t<LastCaseIndex,StoreCases>, StoreTarget>::type;
    using ReturnType = typename UnwrapReturnType<RawReturnType>::type;
    using CleanReturnType = used_std::conditional_t<used_std::is_same_v<ReturnType, void>, Wildcard, ReturnType>;
    struct State {
        used_std::size_t next_idx;
        TargetType current_target;
        bool jump_signal;
        bool matched;
        CleanReturnType result;
    };
    
    StoreTarget target;
    StoreCases cases;
    
    constexpr match_t(StoreTarget t) noexcept : target(t) 
    {}
    constexpr match_t() noexcept : target(DefaultState{})
    {}
    constexpr match_t(match_t&& other) noexcept 
    : target(other.target),
      cases(other.cases)
    {}
    constexpr match_t(StoreTarget&& t,Cases&&... ca) noexcept requires (is_configured)
        : target(used_std::forward<StoreTarget>(t)),  
          cases(used_std::forward<Cases>(ca)...)
    {}

    template <typename cT>
    using case_t = used_std::decay_t<cT>;
    template <typename T> requires (defaultTarget)
    constexpr auto operator()(T&& target) const noexcept {
        // static_assert(sizeof...(ContextArgs) > 0, "operator() requires at least one argument (the default action).");
        // static_assert( == sizeof(match), "eq size"); 
        return match_t<used_std::remove_cvref_t<T>>(
            used_std::forward<T>(target)
        );
    }
    template <typename... ContextArgs> requires (!defaultTarget && defaultCases)
    constexpr auto operator()(ContextArgs&&... args) && noexcept {
        static_assert(sizeof...(ContextArgs) > 0, "operator() requires at least one argument (the default action).");
        // static_assert( == sizeof(match), "eq size"); 
        return match_t<TargetType, case_t<ContextArgs>...>(
            used_std::move(target),
            used_std::forward<ContextArgs>(args)...
        ).run();
    }
    template <typename... ContextArgs> requires (!defaultTarget && defaultCases)
    constexpr auto when(ContextArgs&&... args) && noexcept {
        static_assert(sizeof...(ContextArgs) > 0, "operator() requires at least one argument (the default action).");
        // static_assert( == sizeof(match), "eq size"); 
        return match_t<TargetType, case_t<ContextArgs>...>(
            used_std::move(target),
            used_std::forward<ContextArgs>(args)...
        ).to_fn();
    }

    constexpr void Dispatch(auto& action,State& current_state) noexcept requires(is_configured) {
        auto& [next_idx, current_target,jump_signal,matched,result] = current_state;
        
        using RawActionResult = typename ReturnTypeHelper<true,decltype(action),StoreTarget>::type;

        if constexpr (used_std::is_same_v<RawActionResult, void> || used_std::is_same_v<RawActionResult, Wildcard>) {
            execute_action(action, target);
            matched=true;
            return;
        } else {
            decltype(auto) action_result = execute_action(action, target);
            using CaseActionDecay = UnwrapReturnType<decltype(action_result)>::type;

            // Signal 1: Static Goto
            if constexpr (concepts::IsStaticGotoSignal<CaseActionDecay>) {
                constexpr used_std::size_t comptime_index = used_std::find_index_v<[]<typename T>{return T::label;}, CaseActionDecay::label, StoreCases>;
                
                if constexpr (comptime_index >= TotalCases) {
                    __builtin_unreachable();
                    return;
                }

                next_idx = comptime_index;
                jump_signal = true;
                return;
            } 
            // Signal 2: Dynamic Goto
            else if constexpr (concepts::IsDynamicGotoSignal<CaseActionDecay>) {
                used_std::size_t active_index = used_std::find_by_value<[]<typename T>{return T::label;}, StaticLabel, StoreCases>(
                    action_result.label, 
                    CasesIndex{}
                );
                if (active_index >= TotalCases) {
                    __builtin_unreachable();
                    return;
                }
                
                next_idx = active_index;
                jump_signal = true;
                return;
            }
            // Signal 3: Fallthrough
            else if constexpr (concepts::IsFallthroughSignal<CaseActionDecay>) {
                next_idx = next_idx + 1;
                jump_signal = true;
                return;
            }
            // Terminal Return Value
            else {
                if constexpr (
                    !concepts::IsGotoSignal<CaseActionDecay> && !concepts::IsFallthroughSignal<CaseActionDecay> && 
                    !concepts::IsDynamicGotoSignal<CaseActionDecay> &&
                    !used_std::is_same_v<CaseActionDecay, void> && !used_std::is_same_v<CaseActionDecay, Wildcard>
                ) {
                    matched = true;
                    result = used_std::move(action_result);
                } else {
                    matched = true;
                    return;
                }
            }
        }
    }

    template<used_std::size_t I> 
    constexpr void evaluateCase(State& state) noexcept requires(is_configured) {
        if constexpr (I >= LastCaseIndex) __builtin_unreachable();
        auto& [key, action] = std::get<I>(cases);
        const bool matched = state.jump_signal +
        evaluate_match(state.current_target, key);
        return matched ? (Dispatch(action,state)) :
        (state.next_idx = I + 1,void(0));
    }

    template<used_std::size_t Low,used_std::size_t High>
    constexpr void DispatchCase (State& state) noexcept requires(is_configured) {
        constexpr used_std::size_t Range = High - Low;      
        #define DCASE(IDX) \
        case(Low + IDX):   \
        if constexpr (Low + IDX < High) {           \
            return evaluateCase<Low + IDX>(state);  \
            break;                                  \
        } else __builtin_unreachable();
        switch(state.next_idx) {
            DCASE(0)
            DCASE(1)
            DCASE(2)
            DCASE(3)
            DCASE(4)
            DCASE(5)
            DCASE(6)
            DCASE(7)
            default: 
            if constexpr (Range >= 8) {
                constexpr used_std::size_t Mid = Low + (High - Low) / 2;
                if (state.next_idx < Mid) {
                    return DispatchCase<Low, Mid>(state);
                } else {
                    return DispatchCase<Mid, High>(state);
                }
            } else {
                __builtin_unreachable();
            }
        }
        #undef DCASE
    }

    constexpr ReturnType run() noexcept requires (is_configured && TotalCases > 1) {
        State state{.next_idx= 0 ,.current_target= target ,.jump_signal= false ,.matched= false,.result={}};
        while (state.next_idx < TotalCasesIndex) {
            DispatchCase<0,TotalCasesIndex>(state);
            if (state.matched) break;
        }
        
        if constexpr (used_std::is_same_v<CleanReturnType, void> || used_std::is_same_v<CleanReturnType, Wildcard>) {
            return state.matched ? 
            void(0) : 
            execute_action(used_std::get<LastCaseIndex>(cases), target);
        } else {
            return state.matched ? 
            state.result : 
            execute_action(used_std::get<LastCaseIndex>(cases), target);
        }
    }
    
    constexpr ReturnType run() noexcept requires (is_configured && TotalCases <= 1) {
            return used_std::is_same_v<CleanReturnType, void> || used_std::is_same_v<CleanReturnType, Wildcard> ? 
            ReturnType(0) : 
            execute_action(used_std::get<LastCaseIndex>(cases), target);
    }
    
    constexpr auto to_fn() {
        return [self = used_std::move(*this)](const StoreTarget& elem) mutable {
            State state{.next_idx= 0 ,.current_target=elem ,.jump_signal= false ,.matched= false,.result={}};
            while (state.next_idx < TotalCasesIndex) {
                self.template DispatchCase<0,TotalCasesIndex>(state);
                if (state.matched) break;
            }
            return state.matched ? 
            state.result : 
            execute_action(used_std::get<LastCaseIndex>(self.cases), self.target);
        };
    }
    // constexpr auto to_predicate() && requires (is_configured) {
    //     return [self = used_std::move(*this)](const auto& elem) mutable {
    //         constexpr auto is = used_std::make_index_sequence<TotalCases::value>{};
    //         struct State {
    //             used_std::size_t next_idx;
    //             used_std::remove_cvref_t<decltype(elem)> current_target;
    //             bool jump_signal;
    //             bool matched;
    //             bool result;
    //         };
    //         State state{0 , elem , false , false};
    //         return [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) {
    //             ((state = used_std::get<Is>(self.cases).caseDispatch(state,__,self.cases,used_std::integral_constant<used_std::size_t, Is>{})), ...);
    //         if constexpr (used_std::is_same_v<CleanReturnType, void> or used_std::is_same_v<CleanReturnType, Wildcard>) {
    //             if (state.matched) {
    //                 return;
    //             } else {
    //                 execute_action(self.default_action, self.ctx);
    //                 return;
    //             }
    //         } else {
    //             return state.matched ? state.result : execute_action(self.default_action, self.ctx);
    //         } 
    //         }(is);
    //     };
    // }
    
};
template <typename TargetType>
match_t(TargetType) -> match_t<TargetType>;

inline constexpr auto match = match_t();

#endif
// ============================================================================
//                                GRAVEYARD
// ============================================================================

// template <used_std::size_t Low, used_std::size_t High, typename Fn>
// static constexpr auto dispatch(Fn&& fn, used_std::size_t idx) 
// noexcept(noexcept(used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 0>{}))) 
// {
//     constexpr used_std::size_t Range = High - Low;
//     if constexpr (Range <= 8) {
//         switch(idx) {
//             case(Low + 0): if constexpr ((Low + 0) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 0>{});}
//             case(Low + 1): if constexpr ((Low + 1) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 1>{});}
//             case(Low + 2): if constexpr ((Low + 2) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 2>{});}
//             case(Low + 3): if constexpr ((Low + 3) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 3>{});}
//             case(Low + 4): if constexpr ((Low + 4) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 4>{});}
//             case(Low + 5): if constexpr ((Low + 5) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 5>{});}
//             case(Low + 6): if constexpr ((Low + 6) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 6>{});}
//             case(Low + 7): if constexpr ((Low + 7) < High) {return used_std::forward<Fn>(fn)(used_std::integral_constant<used_std::size_t, Low + 7>{});}
//             default: __builtin_unreachable();
//         }
//     } else {
//         constexpr used_std::size_t Mid = Low + (High - Low) / 2;
//         if (idx < Mid) {
//             return dispatch<Low, Mid>(used_std::forward<Fn>(fn), idx);
//         } else {
//             return dispatch<Mid, High>(used_std::forward<Fn>(fn), idx);
//         }
//     }
// }
// template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple>
// inline constexpr auto universal_switch_matrix(TargetType target, DefaultType&& default_action, ContextTuple&& ctx, CasesTuple&& cases) noexcept {
//     using RawCases = used_std::remove_cvref_t<CasesTuple>;
//     constexpr used_std::size_t TotalCases = used_std::tuple_size_v<RawCases>;
//     // constexpr auto CasesIndex = used_std::make_index_sequence<TotalCases>{};
//     using CoreReturnType  = decltype(execute_action(default_action, ctx));
//     using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;

//     struct State {
//         used_std::size_t Guard;
//         used_std::size_t next_idx;
//         TargetType current_target;
//         bool jump_signal;
//         bool matched;
//         used_std::conditional_t<used_std::is_same_v<CleanReturnType, void>, Wildcard, CleanReturnType> result;
//     };
//     // Storage for return value without default-constructor penalties
//     State state{0, 0 , target , false , false , {}};

//     auto dispatch_case = [&]<size_t in>(used_std::integral_constant<used_std::size_t, in>) {
//         auto* current_case = &used_std::get<in>(cases);
//         return current_case->caseDispatch(state,ctx,cases,used_std::integral_constant<used_std::size_t, in>{});
        
//     };

//     while (state.next_idx < TotalCases) {
//         state = dispatch<0, TotalCases>(dispatch_case,state.next_idx);
//         if (state.matched) break;
//     }
    
//     if constexpr (used_std::is_same_v<CleanReturnType, void>) {
//         if (state.matched) {
//             return;
//         } else {
//             execute_action(used_std::forward<DefaultType>(default_action), ctx);
//         }
//     } else {
//         if (state.matched) { 
//             return state.result;
//         } else {
//             return execute_action(used_std::forward<DefaultType>(default_action), ctx);
//         }
//     }
// }
// template <typename TargetType, typename ContextTuple, typename DefaultAction, typename CasesTuple, used_std::size_t... Is>
// inline constexpr decltype(auto) universal_switch_helper(TargetType target, DefaultAction&& default_action, ContextTuple&& ctx, CasesTuple&& cases, used_std::index_sequence<Is...>) 
// {
//     return universal_switch_matrix(
//         target,
//         used_std::forward<DefaultAction>(default_action),
//         used_std::forward<ContextTuple>(ctx),
//         used_std::forward_as_tuple(used_std::get<Is>(used_std::forward<CasesTuple>(cases))...)
//     );
// }

// template <typename TargetType, typename ContextTuple, typename... AllTrailingArgs>
// inline constexpr decltype(auto) universal_switch(TargetType target, ContextTuple&& ctx, AllTrailingArgs&&... args) {
//     constexpr used_std::size_t TotalArgs = sizeof...(AllTrailingArgs);
//     static_assert(TotalArgs >= 1, "Library Error: You must supply a terminal fallback default action.");
    
//     constexpr used_std::size_t CaseCount = TotalArgs - 1;

//     // Direct pack access to the default action (last argument) without tuple wrapping
//     auto args_tuple = used_std::forward_as_tuple(used_std::forward<AllTrailingArgs>(args)...);
//     decltype(auto) default_action = used_std::get<CaseCount>(args_tuple);

//     if constexpr (CaseCount == 0) {
//         return execute_action(used_std::forward<decltype(default_action)>(default_action), ctx, target);
//     } else {
//         return universal_switch_helper(
//             target, 
//             used_std::forward<decltype(default_action)>(default_action), 
//             used_std::forward<ContextTuple>(ctx), 
//             used_std::move(args_tuple),
//             used_std::make_index_sequence<CaseCount>{}
//         );
//     }
// }
// static_assert(match(__)()(Case(__) >> true,true), "");
// template <typename TargetType, typename ContextTuple>
// struct SwitchPipelineProxy {
//     TargetType target;
//     ContextTuple ctx;

//     template <typename... CaseTypes>
//     inline constexpr decltype(auto) operator()(CaseTypes&&... cases) && {
//         return universal_switch(target,used_std::forward<ContextTuple>(ctx), used_std::forward<CaseTypes>(cases)...);
//     }
// };
// template <typename TargetType>
// struct SwitchTargetProxy {
//     TargetType target;
 
//     inline constexpr auto operator()() && noexcept {
//         return SwitchPipelineProxy<TargetType, Wildcard>{ target, __ };
//     }
//     template <typename... ContextArgs> 
//     inline constexpr auto operator()(ContextArgs&&... args) && noexcept {
//         auto ctx_tuple = used_std::forward_as_tuple(used_std::forward<ContextArgs>(args)...);
//         using TupleType = decltype(ctx_tuple);
//         return SwitchPipelineProxy<TargetType, TupleType>{ target, used_std::move(ctx_tuple) };
//     }
// };

// inline constexpr auto Match() noexcept {
//     return SwitchTargetProxy<Wildcard>{ __ };
// }

// template <typename TargetType>
// inline constexpr auto Match(TargetType&& target) noexcept {
//     using StoreType = concepts::primitive_param_t<TargetType>;
//     return SwitchTargetProxy<StoreType>{ target };
// }
// template <typename... Cases>
// inline constexpr auto MatchPredicate(Cases&&... target) noexcept {
//     return [... cases = used_std::forward<Cases>(target)](const auto& Element) {
//         return static_cast<bool>(universal_switch(Element,__,cases...));
//     };
// }
//