
#ifndef MATCH_H
#define MATCH_H


    // #include <type_traits>
    // #include <tuple>
    // #include <utility>
    #include "mini_std.hpp"


// ============================================================================
// BEGIN. USED DEFINITIONS
// ============================================================================

namespace used_std {
    using namespace mini_std; 

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

    template <typename Ret, typename... Args>
    struct function_traits_base {
        using return_type = Ret;
        using args_tuple  = used_std::tuple<Args...>;
        using args_tuple_temp = used_std::tuple<used_std::decay_t<Args>&&...>;
        using args_tuple_ptr = used_std::tuple<used_std::add_pointer_t<used_std::decay_t<Args>>...>;
        using Idx_seq = used_std::make_index_sequence<sizeof...(Args)>;

        static constexpr used_std::size_t args = sizeof...(Args);

        template <used_std::size_t N>
        using arg_type = used_std::tuple_element_t<N, args_tuple>;
    };

    // 1. Primary template takes EXACTLY ONE type (the function signature or pointer)
    template <typename Ret>
    struct function_traits;

    // 3. Free function signature: Ret(Args...)
    template <typename Ret, typename... Args>
    struct function_traits<Ret(Args...) &> : function_traits_base<Ret, Args...> {
        using fn_type = Ret(Args...);
    };
    template <typename Ret, typename... Args>
    struct function_traits<Ret(Args...) &&> : function_traits_base<Ret, Args...> {
        using fn_type = Ret(Args...);
    };

    template <typename Ret, typename... Args>
    struct function_traits<Ret(Args...) const> : function_traits_base<Ret, Args...> {
        using fn_type = Ret(Args...);
    };

    // 4. Function pointer: Ret(*)(Args...)
    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> : function_traits_base<Ret, Args...> {
        using fn_type = Ret(*)(Args...);
    };
    // 5. Function reference: Ret(&)(Args...)
    template <typename Ret, typename... Args>
    struct function_traits<Ret(&)(Args...)> : function_traits_base<Ret, Args...> {
        using fn_type = Ret(&)(Args...);
    };
    // 6. Member function: Ret(Class::*)(Args...)
    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> : function_traits_base<Ret, Args...> {
        using class_type = Class;
        using fn_type    = Ret(Class::*)(Args...);
        using args_tuple_class_temp = used_std::tuple<used_std::add_pointer_t<used_std::decay_t<class_type>>,used_std::decay_t<Args>&&...>;
        
        static constexpr bool is_member   = true;
        static constexpr bool is_noexcept = true;
    };
    // Non-const Member Function (noexcept)
    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) noexcept> 
    : function_traits_base<Ret, Args...> {
        using class_type = Class;
        using fn_type    = Ret(Class::*)(Args...) noexcept;
        using args_tuple_class_temp = used_std::tuple<Class*, used_std::decay_t<Args>...>;
        static constexpr bool is_member   = true;
        static constexpr bool is_noexcept = true;
    };

    // 7. Const member function: Ret(Class::*)(Args...) const
    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> : function_traits_base<Ret, Args...> {
        using class_type = const Class;
        using fn_type    = Ret(Class::*)(Args...) const;
        using args_tuple_class_temp = used_std::tuple<used_std::add_pointer_t<const used_std::decay_t<class_type>>,used_std::decay_t<Args>&&...>;
        
        static constexpr bool is_member   = true;
        static constexpr bool is_noexcept = true;
    };


    // Const Member Function (noexcept)
    template <typename Ret, typename Class, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const noexcept> 
    : function_traits_base<Ret, Args...> {
        using class_type = const Class;
        using fn_type    = Ret(Class::*)(Args...) const noexcept;
        using args_tuple_class_temp = used_std::tuple<const Class*, used_std::decay_t<Args>...>;
        static constexpr bool is_member   = true;
        static constexpr bool is_noexcept = true;
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

    template <typename ContextTuple>
    using CleanContextDecayed = decltype([]<typename... Ts>(used_std::tuple<Ts...>*) {
        return used_std::tuple<used_std::decay_t<Ts>...>{};
    }(static_cast<used_std::remove_cvref_t<ContextTuple>*>(nullptr)));

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
        template <typename Seq1, typename Seq2>
        struct concat_index_sequence;
    
        template <used_std::size_t... Is1, used_std::size_t... Is2>
        struct concat_index_sequence<used_std::index_sequence<Is1...>, used_std::index_sequence<Is2...>> {
            using type = used_std::index_sequence<Is1..., Is2...>;
        };
    
        template <typename Seq1, typename Seq2>
        using concat_index_sequence_t = typename concat_index_sequence<Seq1, Seq2>::type;
    
        template <used_std::size_t... Is1, used_std::size_t... Is2>
        constexpr auto operator+(used_std::index_sequence<Is1...>, used_std::index_sequence<Is2...>) {
            return concat_index_sequence_t<used_std::index_sequence<Is1...>, used_std::index_sequence<Is2...>>{};
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
    }

    // Public alias template
    template <typename TupleA, typename TupleB>
    using get_matching_indices_t = decltype(
        detail::cross_index_type<TupleA, TupleB>()
    );
    
    template <typename TupleA, typename TupleB>
    constexpr auto count_total_matches_t = detail::index_sequence_size<get_matching_indices_t<TupleA, TupleB>>::value;
    
    template <typename TupleA, typename TupleB>
    constexpr bool is_one_matching_index_t = (count_total_matches_t<TupleA, TupleB> == 1);

    
    using Tuple1 = used_std::tuple<int, short, char, float, double,short>;
    using Tuple2 = used_std::tuple<short,double,short>;
    using test_matchCoord = get_matching_indices_t<Tuple2,Tuple1>;
    static_assert(used_std::is_same_v<test_matchCoord, used_std::index_sequence<1,4,5>>,"");

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
        ((Accessor.template operator()<used_std::remove_cvref_t<used_std::tuple_element_t<Is, CleanTuple>>>() == target_hash ? (found_index = Is) : 0) or ...);
        
        return found_index;
    }

    template<typename Fn,typename Tuple ,used_std::size_t... I>
    constexpr auto apply_index (Fn&& fn,Tuple& t,used_std::index_sequence<I...>) noexcept {
        return used_std::invoke(used_std::forward<Fn>(fn),used_std::get<I>(t)...);
    }

    constexpr auto unreachable = mini_std::unreachable;
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
    unsigned int hash{0};
    
    // 1. String Literal Constructor (auto-hashes and stores string)
    template <used_std::size_t N>
    constexpr StaticLabel(StaticString<N> string) {
        hash = used_std::strHash::fnv1a_hash(string.data, N);
    }
    template <used_std::size_t N>
    constexpr StaticLabel(const char (&string)[N] ) {
        hash = used_std::strHash::fnv1a_hash(string, N);
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

    constexpr bool operator==(const StaticLabel& other) const noexcept {  
        return hash == other.hash;
    }
    
    constexpr bool operator==(const unsigned int& other) const noexcept {  
        return hash == other;
    }

    constexpr StaticLabel& operator=(int val) noexcept {
        *this = StaticLabel(val);
        return *this;
    }
    constexpr StaticLabel& operator=(const StaticLabel& other) noexcept {
        this->hash = other.hash;
        return *this;
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
[[nodiscard]] constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept;


// ============================================================================
// FLOW CONTROL STATE TRACE SIGNALS AND ENUMS
// ============================================================================
enum class FlowKind : int {
    Terminal,   
    Fallthrough, 
    Goto,
    Composite,
};


struct Wildcard {};
[[maybe_unused]] inline constexpr Wildcard __{};

enum class BranchHint { None, Likely, Unlikely };

template <FlowKind Kind>
struct SignalBase {
    static constexpr FlowKind static_flow_kind = Kind;
};

// 1. Fallthrough signals
struct fallthrough_t : SignalBase<FlowKind::Fallthrough> {};

template <typename T>
struct FallthroughValue : SignalBase<FlowKind::Fallthrough> { 
    T value; 
};

// 2. Dynamic Hash Goto
struct goto_hash_t : SignalBase<FlowKind::Goto> { 
    unsigned int hash {0}; 

    constexpr goto_hash_t() = default;
    constexpr explicit goto_hash_t(unsigned int h) noexcept : hash(h) {}
};

// 3. Static Label Goto (adds compile-time label metadata directly)
template <StaticLabel LabelID>
struct goto_case_t : SignalBase<FlowKind::Goto> {
    static constexpr bool is_static_label = true;
    static constexpr auto static_label = LabelID;
};

// 4. Static Label Goto with Payload
template <StaticLabel LabelID, typename T>
struct GotoValue : SignalBase<FlowKind::Goto> {
    static constexpr bool is_static_label = true;
    static constexpr auto static_label = LabelID;
    T value;
};

// 5. Composite Dynamic Signal
struct CompositeSignalBase : SignalBase<FlowKind::Composite> {};

constexpr fallthrough_t fallthrough_to_next() noexcept { return fallthrough_t{}; }
template <typename T> constexpr auto pass_and_fallthrough(T&& val) noexcept { return FallthroughValue<used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <StaticLabel LabelID> constexpr auto Goto() noexcept { return goto_case_t<LabelID>{}; }
template <used_std::size_t N>
constexpr goto_hash_t Goto(const char (&input)[N]) {
    return goto_hash_t(used_std::strHash::fnv1a_hash(input, N));
}
constexpr goto_hash_t Goto(StaticLabel label) {
    return goto_hash_t{ label.hash };
}
template <StaticLabel LabelID, typename T> constexpr auto pass_and_goto(T&& val) noexcept { return GotoValue<LabelID, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

// Static compile-time label jump
namespace concepts {
    template <typename T>
    concept IsSignal = requires { T::static_flow_kind; };
    
    template <typename T>
    concept IsGotoSignal = used_std::derived_from<used_std::decay_t<T>, SignalBase<FlowKind::Goto>>;
    template <typename T>
    concept IsGotoValue = requires (T t) {
        requires IsGotoSignal<T> and t.value;
    };
    template <typename T>
    concept IsFallthroughSignal = used_std::derived_from<used_std::decay_t<T>, SignalBase<FlowKind::Fallthrough>>;
    template <typename T>
    concept IsFallthroughValue = requires (T t) {
        requires IsFallthroughSignal<T> and t.value;
    };
    
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
used_std::is_function_v<used_std::remove_pointer_t<Fn>> or  
used_std::is_member_pointer_v<used_std::remove_pointer_t<Fn>> or                
requires (used_std::remove_cvref_t<Fn> f) {
    f;
};
template <IsCallableType T>
struct Free_Function_helper {
    using type = Wildcard;
};

// Only instantiate callable_traits_t if T is a member function pointer
template <IsCallableType T>
requires used_std::is_member_function_pointer_v<T>
struct Free_Function_helper<T> {
    using type = typename used_std::callable_traits_t<T>::class_type;
};

template <IsCallableType Fn>
struct FnPredicate {
    Fn fn;
    typename Free_Function_helper<Fn>::type* instance;
    template <typename Target>
    constexpr bool operator()(const Target& target) const {
        if constexpr (used_std::is_member_function_pointer_v<used_std::remove_pointer_t<Fn>>) {
            if constexpr (used_std::is_convertible_v<Target,typename Free_Function_helper<Fn>::type>) {
                return used_std::invoke(fn, target);
            } else {
                return used_std::invoke(fn, instance ,target);
            }
        } else{
            return used_std::invoke(fn, target);
        }
    }
};

template <IsCallableType T>
struct Projection_Function_helper {
    using type = typename used_std::callable_traits_t<T>::args_tuple_temp;
};

// Only instantiate callable_traits_t if T is a member function pointer
template <IsCallableType T>
requires used_std::is_member_function_pointer_v<T>
struct Projection_Function_helper<T> {
    using type = typename used_std::callable_traits_t<T>::args_tuple_class_temp;
};
// --- Projection Case (Pattern + Member Function Extractor) ---
template <IsCallableType Fn, typename ExpectedPattern>
struct ProjectionCaseimpl {
    using fnTraits = used_std::callable_traits_t<Fn>;
    fnTraits::fn_type fn;
    Projection_Function_helper<Fn>::type args;
    ExpectedPattern pattern;
    template <typename Target>
    constexpr bool operator()(const Target& target) const {
        if constexpr (fnTraits::args > 0) {
            decltype(auto) extracted_val = used_std::apply(fn,args);
            return evaluate_match(extracted_val, pattern);
        } else {
            if constexpr (used_std::is_member_function_pointer_v<Fn>) {
                decltype(auto) extracted_val = used_std::apply(fn,args);
                return evaluate_match(extracted_val, pattern);
            } else {
                decltype(auto) extracted_val = used_std::invoke(fn);
                return evaluate_match(extracted_val, pattern);
            }
        }
    }
    private:
    enum __ {
        Instance = 0
    };
};

template <typename T> struct is_fn_predicate : used_std::false_type {};
template <typename Fn> struct is_fn_predicate<FnPredicate<Fn>> : used_std::true_type {};
template <typename T> concept is_fnpredicate = is_fn_predicate<used_std::remove_cvref_t<T>>::value;

template <typename Fn>
struct is_projection_caseimpl : used_std::false_type {};
template <typename MemFn, typename ExpectedPattern>
struct is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>> : used_std::true_type {};
template <typename MemFn, typename ExpectedPattern>
concept is_projection_case = is_projection_caseimpl<ProjectionCaseimpl<MemFn,ExpectedPattern>>::value;


template <IsCallableType Fn>
constexpr auto Predicate(Fn&& fn) {
    return FnPredicate<Fn>{ used_std::forward<Fn>(fn) };
}
template <IsCallableType Fn,typename Class> 
constexpr auto Predicate(Fn&& fn,Class* obj) {
    return FnPredicate<Fn>{ .fn=used_std::forward<Fn>(fn) ,.instance=obj};
}


// Helper builder function for Case(Pattern, &fn)
template <typename ExpectedPattern, IsCallableType Fn,typename... Args>
constexpr auto ProjectionCase(ExpectedPattern&& pattern, Fn fn,Args&&... args) {
    using fnTraits = used_std::callable_traits_t<used_std::remove_cvref_t<Fn>>;
    static_assert(sizeof...(Args) <= fnTraits::args, "Too many arguments provided for projection function");
    // static_assert(sizeof...(Args) < fnTraits::args, "Not Enough Arguments");
    return ProjectionCaseimpl<typename fnTraits::fn_type, used_std::decay_t<ExpectedPattern>>{
        .fn = fn, 
        .args = used_std::forward_as_tuple(used_std::forward<Args>(args)...),
        .pattern = used_std::forward<ExpectedPattern>(pattern)
    };
}
// Helper builder function for Case(Pattern, &Class::member_fn)
template <typename ExpectedPattern, IsCallableType Fn,typename Class,typename... Args>
constexpr auto ProjectionCase(ExpectedPattern&& pattern, Fn fn,Class* instance,Args&&... args) {
    using fnTraits = used_std::callable_traits_t<used_std::remove_cvref_t<Fn>>;
    if constexpr (sizeof...(Args) == 0) {
        using classType = Class*;
        return ProjectionCaseimpl<typename fnTraits::fn_type, used_std::decay_t<ExpectedPattern>>{
            .fn = fn, 
            .args = used_std::tuple<classType>(instance),
            .pattern = used_std::forward<ExpectedPattern>(pattern)
        };
    } else {
        return ProjectionCaseimpl<typename fnTraits::fn_type, used_std::decay_t<ExpectedPattern>>{
            .fn = fn, 
            .args = used_std::forward_as_tuple(instance,used_std::forward<Args>(args)...),
            .pattern = used_std::forward<ExpectedPattern>(pattern)
        };

    }
}

// ============================================================================
// MATH INTERVAL DEFINITIONS
// ============================================================================
enum class RangeType  { Closed, Open, HalfOpenLeft, HalfOpenRight , Or , And };

template <RangeType  iType = RangeType ::Closed,typename T = int> requires (used_std::is_arithmetic_v<T>)
struct Range {
    T lhs;
    T rhs;
    
    template<typename Target>
    constexpr bool contains(Target target) const noexcept {
        if constexpr (iType == RangeType::Closed) {
            return (target >= lhs) && (target <= rhs);
        } 
        else if constexpr (iType == RangeType::Open) {
            return (target > lhs) && (target < rhs);
        }
        else if constexpr (iType == RangeType::HalfOpenLeft) { // (min, max]
            return (target > lhs) && (target <= rhs);
        }
        else if constexpr (iType == RangeType::HalfOpenRight) { // [min, max)
            return (target >= lhs) && (target < rhs);
        }
        else if constexpr (iType == RangeType::Or) { // Outside (min, max)
            return (target < lhs) || (target > rhs);
        } 
        else {
            return false;
        }
    }
};
template <RangeType  iType = RangeType ::Closed,typename T> requires (used_std::is_arithmetic_v<T>)
Range(T,T) -> Range<iType,T>;
namespace concepts {
    template <typename T>
    struct is_range_inst : used_std::false_type {};
    
    template <RangeType  iType,typename T>
    struct is_range_inst<Range<iType,T>> : used_std::true_type {};
    
    template <typename T>
    concept is_range_instance = is_range_inst<used_std::remove_cvref_t<T>>::value;
}

template <typename T,RangeType iType = RangeType::Closed> constexpr auto make_range(T min, T max) noexcept { return Range<iType,T>{min, max}; }
template <typename T,RangeType iType = RangeType::Open> constexpr auto make_range_exclusive(T min, T max) noexcept { return Range<iType,T>{min, max}; }
template <typename T,RangeType iType = RangeType::HalfOpenLeft> constexpr auto make_range_left_open(T min, T max) noexcept { return Range<iType,T>{min, max}; }
template <typename T,RangeType iType = RangeType::HalfOpenRight> constexpr auto make_range_right_open(T min, T max) noexcept { return Range<iType,T>{min, max}; }

template <RangeType Op = RangeType::Or,typename... Ranges> requires (concepts::is_range_instance<Ranges> && ...)
struct RangeCompound {
    used_std::tuple<Ranges...> ranges;

    constexpr explicit RangeCompound(Ranges... r) : ranges(used_std::move(r)...) {}

    template <typename Target>
    constexpr bool contains(Target target) const noexcept {
        return used_std::apply([&target](const auto&... r) {
            if constexpr (Op == RangeType::Or) {
                // Short-circuits on the FIRST 'true'
                return (r.contains(target) or ...); 
            } else {
                // Short-circuits on the FIRST 'false'
                return (r.contains(target) and ...); 
            }
        }, ranges);
    }
    static_assert(Op == RangeType::Or or Op == RangeType::And,"Requires RangeType And / Or");
};
template <RangeType Op = RangeType::Or,typename... Ranges> 
requires (concepts::is_range_instance<Ranges> && ...)
RangeCompound(Ranges...) -> RangeCompound<Op,Ranges...>;

template <RangeType Op = RangeType::Or,typename... Ranges>
constexpr auto make_compound_range(Ranges&&... ranges) noexcept {
    return RangeCompound<Op,used_std::decay_t<Ranges>...>{used_std::forward<Ranges>(ranges)...};
}
// ============================================================================
// FIELD PREDICATES
// ============================================================================
enum class Op { Eq, Neq, Gt, Gte, Lt, Lte };


template <typename ClassType, typename MemberType> requires 
(used_std::is_class<ClassType>::value and (used_std::is_member_object_pointer_v<MemberType> || used_std::is_member_pointer<class Tp>::value))
struct FieldRule {
    MemberType ClassType::*member_ptr;
    Op op_tag = Op::Eq;
    MemberType value;

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

template <typename ClassType, typename... Rules> requires ((is_field_rule<Rules>::value and ...))
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
            // Strip inner cv-qualifiers and references from every single element type!
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
        { k(t) } -> convertible_to<bool>;
    };  
    template <typename T>
    using primitive_param_t = used_std::conditional_t<
        used_std::is_fundamental_v<used_std::decay_t<T>>, 
        used_std::decay_t<T>, 
        const used_std::decay_t<T>&
    >;
}

// Global Core Match Evaluator Implementation
template <typename TargetType, typename KeyType>
[[nodiscard]] inline constexpr bool evaluate_match(const TargetType& target, const KeyType& key) noexcept {
    using TargetDecay = used_std::decay_t<TargetType>;
    using KeyDecay    = used_std::decay_t<KeyType>;

    // 1. Wildcard / Catch-all
    if constexpr (concepts::IsWildcard<KeyType>) {
        return true;
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
// CASE STORAGE WITH HINT PARAMETERS
// ============================================================================
struct SelfBase {};

template <typename Derived>
struct Self : public SelfBase {
    using ConcreteType = Derived;
    constexpr Derived& self() noexcept { 
        return static_cast<Derived&>(*this); 
    }
    
    constexpr const Derived& self() const noexcept { 
        return static_cast<const Derived&>(*this); 
    }
};

template <StaticLabel LabelID,BranchHint HintValue = BranchHint::None, typename KeyType = Wildcard, typename ActionType = Wildcard>
struct ImplCase {
    KeyType key;
    ActionType action;
    static constexpr auto label = LabelID;
    static constexpr BranchHint hint = HintValue;
    template <typename NewAction>
    inline constexpr auto operator>>(NewAction&& action) && noexcept {
        return ImplCase<LabelID,HintValue, KeyType, used_std::decay_t<NewAction>>{
            .key=used_std::move(key), .action=used_std::forward<NewAction>(action)
        };
    }
};


template <StaticLabel LabelID = 0,BranchHint Hint = BranchHint::None,typename T> 
inline constexpr auto Case(T&& val) noexcept { return ImplCase<LabelID, Hint, used_std::decay_t<T>>{ .key=used_std::forward<T>(val) }; }
template <StaticLabel LabelID = 0,typename T> 
inline constexpr auto likely_Case(T&& val) noexcept { return ImplCase<LabelID, BranchHint::Likely, used_std::decay_t<T>>{ .key=used_std::forward<T>(val) }; }
template <StaticLabel LabelID = 0,typename T> 
inline constexpr auto unlikely_Case(T&& val) noexcept { return ImplCase<LabelID, BranchHint::Unlikely, used_std::decay_t<T>>{ .key=used_std::forward<T>(val) }; }

// Hardware Prediction Branch Optimizer Hints Primitive Mapping
template <BranchHint Hint>
[[nodiscard]] inline constexpr bool apply_hardware_hint(bool condition) noexcept {
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
inline constexpr decltype(auto) execute_action(ActionType&& action, ContextType& ctx) {
    using ActionDecay  = used_std::remove_cvref_t<ActionType>;
    using CleanContext = used_std::remove_cvref_t<ContextType>;
    
    // Resolve traits dynamically based on whether it is a function pointer or a functor object
    using FnTrait = used_std::callable_traits_t<ActionType>;

    // 1. Passive signals and primitives
    if constexpr (concepts::IsGotoSignal<ActionDecay> || 
                  concepts::IsFallthroughSignal<ActionDecay> || 
                  concepts::Primitive<ActionDecay>) 
    {
        return used_std::forward<ActionType>(action);
    } 
    else if constexpr (concepts::IsWildcard<ActionDecay>) {
        return true;
    } 
    // 2. Zero-argument lambdas or functions [] {}
    else if constexpr (FnTrait::args == 0) {
        return used_std::forward<ActionType>(action)();
    }
    // 3. Partial / Matching Tuple Unpack Strategy
    else if constexpr (FnTrait::args > 0) {
        using FnArgsTuple = typename FnTrait::args_tuple;
        using ResultSequence = used_std::get_matching_indices_t<FnArgsTuple, CleanContext>;
        
        return used_std::apply_index(used_std::forward<ActionType>(action),ctx,ResultSequence{});
    }
    // 4. Pass entire context tuple directly if nothing else matched
    else if constexpr (concepts::TupleInvocable<ActionDecay, CleanContext>) {
        return used_std::forward<ActionType>(action)(ctx);
    } 
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
    using type = Wildcard;
};

template <typename T>
struct UnwrapReturnType<FallthroughValue<T>> {
    using type = T;
};

template <auto LabelID>
struct UnwrapReturnType<goto_case_t<LabelID>> {
    using type = goto_case_t<LabelID>;//decltype(LabelID);
};

template <auto LabelID, typename T>
struct UnwrapReturnType<GotoValue<LabelID, T>> {
    using type = T;
};
template <used_std::size_t Low, used_std::size_t High, typename Fn>
static constexpr auto dispatch(auto& State, Fn&& fn, used_std::size_t idx) noexcept {
    constexpr used_std::size_t Range = High - Low;
    if constexpr (Range <= 8) {
        switch(idx) {
            case(Low + 0): if constexpr ((Low + 0) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 0>{});}
            case(Low + 1): if constexpr ((Low + 1) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 1>{});}
            case(Low + 2): if constexpr ((Low + 2) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 2>{});}
            case(Low + 3): if constexpr ((Low + 3) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 3>{});}
            case(Low + 4): if constexpr ((Low + 4) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 4>{});}
            case(Low + 5): if constexpr ((Low + 5) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 5>{});}
            case(Low + 6): if constexpr ((Low + 6) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 6>{});}
            case(Low + 7): if constexpr ((Low + 7) < High) {return fn(used_std::forward<decltype(State)>(State), used_std::integral_constant<used_std::size_t, Low + 7>{});}
            default: used_std::unreachable();
        }
    } else {
        constexpr used_std::size_t Mid = Low + (High - Low) / 2;
        if (idx < Mid) {
            return dispatch<Low, Mid>(used_std::forward<decltype(State)>(State), fn, idx);
        } else {
            return dispatch<Mid, High>(used_std::forward<decltype(State)>(State), fn, idx);
        }
    }
}

template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple>
inline constexpr auto universal_switch_matrix(TargetType target, DefaultType&& default_action, ContextTuple&& ctx, CasesTuple&& cases) noexcept {
    using RawCases = used_std::remove_cvref_t<CasesTuple>;
    constexpr used_std::size_t TotalCases = used_std::tuple_size_v<RawCases>;
    constexpr auto CasesIndex = used_std::make_index_sequence<TotalCases>{};
    using CoreReturnType  = decltype(execute_action(default_action, ctx));
    using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;

    struct State {
        used_std::size_t Guard;
        used_std::size_t next_idx;
        TargetType current_target;
        bool jump_signal;
        bool matched;
    };
    // Storage for return value without default-constructor penalties
    CleanReturnType result{};
    State state{0, 0 , target , false , false };

    auto step_lambda = [&]<used_std::size_t Is>(State& current_state, used_std::integral_constant<used_std::size_t, Is>) noexcept -> State {
        if constexpr (Is >= TotalCases) {
            used_std::unreachable();
            return State{current_state.Guard + 1,Is, current_state.current_target, false, false};
        } else {
            auto& current_case = used_std::get<Is>(cases);
            using RawCaseType = typename used_std::decay<decltype(current_case)>::type;

            if (current_state.jump_signal or apply_hardware_hint<RawCaseType::hint>(evaluate_match(current_state.current_target, current_case.key))) {
                using RawActionResult = decltype(execute_action(current_case.action, ctx));

                if constexpr (used_std::is_same_v<RawActionResult, void> || used_std::is_same_v<RawActionResult, Wildcard>) {
                    execute_action(current_case.action, ctx);
                    return State{current_state.Guard + 1,Is, current_state.current_target, false, true};
                } else {
                    decltype(auto) action_result = execute_action(current_case.action, ctx);
                    using CaseActionDecay = used_std::decay_t<decltype(action_result)>;

                    // Signal 1: Static Goto
                    if constexpr (concepts::IsStaticGotoSignal<CaseActionDecay>) {
                        constexpr used_std::size_t comptime_index = used_std::find_index_v<[]<typename T>{return T::label;}, CaseActionDecay::label, RawCases>;
                        
                        if constexpr (comptime_index >= TotalCases) {
                            used_std::unreachable();
                            return State{current_state.Guard + 1,Is, current_state.current_target, false, false};
                        } else {
                            if constexpr (concepts::IsGotoValue<CaseActionDecay>) {
                                return State{current_state.Guard + 1,comptime_index, used_std::move(action_result.value), true, false}; 
                            } else {
                                return State{current_state.Guard + 1,comptime_index, current_state.current_target, true, false}; 
                            }
                        }
                    } 
                    // Signal 2: Dynamic Goto
                    else if constexpr (concepts::IsDynamicGotoSignal<CaseActionDecay>) {
                        used_std::size_t active_index = used_std::find_by_value<[]<typename T>{return T::label;}, unsigned int, RawCases>(
                            action_result.hash, 
                            CasesIndex
                        );
                        if (current_state.Guard > TotalCases) {
                            used_std::unreachable();
                            return State{current_state.Guard + 1,active_index, current_state.current_target, true, false};
                        } else {
                            return State{current_state.Guard + 1,active_index, current_state.current_target, true, false};
                        }
                    }
                    // Signal 3: Fallthrough
                    else if constexpr (concepts::IsFallthroughSignal<CaseActionDecay>) {
                        if constexpr (concepts::IsFallthroughValue<CaseActionDecay>) {
                            return State{current_state.Guard + 1,Is + 1, used_std::move(action_result.value), true, false};
                        } else {
                            return State{current_state.Guard + 1,Is + 1, current_state.current_target, true, false};
                        }
                    }
                    // Terminal Return Value
                    else {
                        if constexpr (!concepts::IsGotoSignal<CaseActionDecay> && 
                                      !concepts::IsFallthroughSignal<CaseActionDecay> && 
                                      !used_std::is_same_v<CaseActionDecay, void> && 
                                      !used_std::is_same_v<CaseActionDecay, Wildcard>) {
                            result = used_std::move(action_result);
                        }
                        return State{current_state.Guard + 1,Is, current_state.current_target, false, true};
                    }
                }
            } else {
                return State{current_state.Guard + 1,Is + 1, current_state.current_target, false, false};
            }
        }
    };


    while (state.next_idx < TotalCases) {
        state = dispatch<0, TotalCases>(state,step_lambda,state.next_idx);
        if (state.matched) break;
    }
    
    if constexpr (used_std::is_same_v<CleanReturnType, void> || used_std::is_same_v<CleanReturnType, Wildcard>) {
        if (state.matched) {
            return;
        } else {
            execute_action(used_std::forward<DefaultType>(default_action), ctx);
        }
    } else {
        if (state.matched) { 
            return result;
        } else {
            return execute_action(used_std::forward<DefaultType>(default_action), ctx);
        }
    }
}
// ============================================================================
// 9. PACK SEPARATION & DELAYED UNIVERSAL INITIALIZATION
// ============================================================================
template <typename TargetType, typename ContextTuple, typename DefaultAction, typename CasesTuple, used_std::size_t... Is>
inline constexpr decltype(auto) universal_switch_helper(TargetType target, DefaultAction&& default_action, ContextTuple&& ctx, CasesTuple&& cases, used_std::index_sequence<Is...>) 
{
    return universal_switch_matrix(
        target,
        used_std::forward<DefaultAction>(default_action),
        used_std::forward<ContextTuple>(ctx),
        used_std::forward_as_tuple(used_std::get<Is>(used_std::forward<CasesTuple>(cases))...)
    );
}

template <typename TargetType, typename ContextTuple, typename... AllTrailingArgs>
inline constexpr decltype(auto) universal_switch(TargetType target, ContextTuple&& ctx, AllTrailingArgs&&... args) {
    constexpr used_std::size_t TotalArgs = sizeof...(AllTrailingArgs);
    static_assert(TotalArgs >= 1, "Library Error: You must supply a terminal fallback default action.");
    
    constexpr used_std::size_t CaseCount = TotalArgs - 1;

    // Direct pack access to the default action (last argument) without tuple wrapping
    auto args_tuple = used_std::forward_as_tuple(used_std::forward<AllTrailingArgs>(args)...);
    decltype(auto) default_action = used_std::get<CaseCount>(args_tuple);

    if constexpr (CaseCount == 0) {
        return execute_action(used_std::forward<decltype(default_action)>(default_action), ctx, target);
    } else {
        return universal_switch_helper(
            target, 
            used_std::forward<decltype(default_action)>(default_action), 
            used_std::forward<ContextTuple>(ctx), 
            used_std::move(args_tuple),
            used_std::make_index_sequence<CaseCount>{}
        );
    }
}
// ============================================================================
// 10. HIGH-UTILITY PROXY WRAPPERS (uswitch DSL FRONTEND ENTRY)
// ============================================================================
template <typename TargetType, typename ContextTuple>
struct SwitchPipelineProxy {
    TargetType target;
    ContextTuple ctx;

    template <typename... CaseTypes>
    inline constexpr decltype(auto) operator()(CaseTypes&&... cases) && {
        return universal_switch(target,used_std::forward<ContextTuple>(ctx), used_std::forward<CaseTypes>(cases)...);
    }
};

template <typename TargetType>
struct SwitchTargetProxy {
    TargetType target;
 
    inline constexpr auto operator()() && noexcept {
        return SwitchPipelineProxy<TargetType, Wildcard>{ target, __ };
    }
    template <typename... ContextArgs> 
    inline constexpr auto operator()(ContextArgs&&... args) && noexcept {
        auto ctx_tuple = used_std::forward_as_tuple(used_std::forward<ContextArgs>(args)...);
        using TupleType = decltype(ctx_tuple);
        return SwitchPipelineProxy<TargetType, TupleType>{ target, used_std::move(ctx_tuple) };
    }
};

inline constexpr auto Match() noexcept {
    return SwitchTargetProxy<Wildcard>{ __ };
}

template <typename TargetType>
inline constexpr auto Match(TargetType&& target) noexcept {
    using StoreType = concepts::primitive_param_t<TargetType>;
    return SwitchTargetProxy<StoreType>{ target };
}
template <typename... Cases>
inline constexpr auto MatchPredicate(Cases&&... target) noexcept {
    return [... cases = used_std::forward<Cases>(target)](const auto& Element) {
        return static_cast<bool>(universal_switch(Element,__,cases...));
    };
}
// ============================================================================
//                                    END
// ============================================================================
#endif
