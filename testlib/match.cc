#include <iostream>
#include <type_traits>
#include <utility>


// ============================================================================
// 1. FREESTANDING LIGHTWEIGHT COMPILER & METAPROGRAMMING UTILITIES
// ============================================================================
namespace mini_std {
    using size_t = decltype(sizeof(0));

    template <class Ty, Ty Val>
    struct integral_constant {
        enum : Ty { value = Val };
        using value_type = Ty;
        using type       = integral_constant;
        
        consteval operator value_type() const noexcept   { return static_cast<value_type>(value); }
        consteval value_type operator()() const noexcept { return static_cast<value_type>(value); }
    };

    template <bool Val> struct bool_constant : integral_constant<bool, Val> {};
    using false_t = bool_constant<false>;
    using true_t  = bool_constant<true>;

    template<class T, class U> struct is_same : false_t {};
    template<class T> struct is_same<T, T>    : true_t {};
    template <typename T, typename U> constexpr bool is_same_v = is_same<T, U>::value;

    template <typename T> struct remove_reference      { using type = T; };
    template <typename T> struct remove_reference<T&>  { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };
    template <typename T> using remove_reference_t = typename remove_reference<T>::type;

    template <typename T> struct add_lvalue_reference { using type = T&; };
    template <> struct add_lvalue_reference<void> { using type = void; };
    template <> struct add_lvalue_reference<const void> { using type = const void; };
    template <typename T> using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    template <typename T> struct add_rvalue_reference { using type = T&&; };
    template <> struct add_rvalue_reference<void> { using type = void; };
    template <> struct add_rvalue_reference<const void> { using type = const void; };
    template <typename T> using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

    template <typename T>
    add_rvalue_reference_t<T> declval() noexcept;

    template <typename T> struct remove_extent          { using type = T; };
    template <typename T> struct remove_extent<T[]>     { using type = T; };
    template <typename T, size_t N> struct remove_extent<T[N]> { using type = T; };

    template<class T> struct is_array : false_t {};
    template<class T> struct is_array<T[]> : true_t {};
    template<class T, size_t N> struct is_array<T[N]> : true_t {};
    
    template <bool B, typename T, typename U> struct conditional { using type = T; };
    template <typename T, typename U> struct conditional<false, T, U> { using type = U; };
    template <bool B, typename T, typename U> using conditional_t = typename conditional<B, T, U>::type;

    template <typename T> struct remove_const          { using type = T; };
    template <typename T> struct remove_const<const T>  { using type = T; };

    template <typename T> struct remove_volatile             { using type = T; };
    template <typename T> struct remove_volatile<volatile T> { using type = T; };

    template <typename T>
    struct remove_cv {
        using type = typename remove_volatile<typename remove_const<T>::type>::type;
    };
    template <typename T> using remove_cv_t = typename remove_cv<T>::type;

    template <typename T>
    struct remove_cvref {
        using type = remove_cv_t<remove_reference_t<T>>;
    };
    template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

    namespace addPtrdetail {
        template<class T> struct type_identity { using type = T; };

        template<class T>
        auto try_add_pointer(int) -> type_identity<typename remove_reference<T>::type*>;

        template<class T>
        auto try_add_pointer(...) -> type_identity<T>;  
    } 
    template<class T> struct add_pointer : decltype(addPtrdetail::try_add_pointer<T>(0)) {};
    template<class T> using add_pointer_t = typename add_pointer<T>::type;

    template<typename F>                   struct is_function : false_t {};
    template<typename F, typename... Args> struct is_function<F(Args...)> : true_t {};

    template <typename T>
    struct decay {
    private:
        using U = remove_reference_t<T>;
    public:
        using type = conditional_t<
            is_array<U>::value,
            typename add_pointer<typename remove_extent<U>::type>::type,
            conditional_t<
                is_function<U>::value,
                typename add_pointer<U>::type,
                remove_cv_t<U>
            >
        >;
    };
    template <typename T> using decay_t = typename decay<T>::type;

    template <typename T> struct is_pointer : false_t {};
    template <typename T> struct is_pointer<T*> : true_t {};
    template <typename T> constexpr bool is_pointer_v = is_pointer<T>::value;

    template <typename T> constexpr T&& forward(remove_reference_t<T>& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr T&& forward(remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }
    template <typename T> constexpr remove_reference_t<T>&& move(T&& t) noexcept { return static_cast<remove_reference_t<T>&&>(t); }

    template <size_t... Is> struct index_sequence {};
    template <size_t N, size_t... Is> struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};
    template <size_t... Is> struct make_index_sequence_impl<0, Is...> { using type = index_sequence<Is...>; };
    template <size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;

    template <typename... Args> struct tuple;
    template <> struct tuple<> {};
    
    template <typename Head, typename... Tail> 
    struct tuple<Head, Tail...> : tuple<Tail...> {
        constexpr tuple(Head h, Tail... t) 
            : tuple<Tail...>(forward<Tail>(t)...), value(forward<Head>(h)) {}
        Head value;
    };

    // --- ADD THESE DEDUCTION GUIDES ---
    tuple() -> tuple<>;

    template <typename... Args>
    tuple(Args...) -> tuple<Args...>;

    template <size_t I, typename Tuple> struct tuple_element;

    template <typename Head, typename... Tail>
    struct tuple_element<0, tuple<Head, Tail...>> {
        using type = Head;
    };

    template <size_t I, typename Head, typename... Tail>
    struct tuple_element<I, tuple<Head, Tail...>> {
        using type = typename tuple_element<I - 1, tuple<Tail...>>::type;
    };

    template <size_t I, typename Tuple>
    using tuple_element_t = typename tuple_element<I, Tuple>::type;

    template <typename T> struct tuple_size;
    template <typename... Types> struct tuple_size<tuple<Types...>> {
        static constexpr size_t value = sizeof...(Types);
    };
    template <typename T> inline constexpr size_t tuple_size_v = tuple_size<T>::value;

    template <size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(tuple<Head, Tail...>& t) noexcept {
        if constexpr (I == 0) {
            return (t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<tuple<Tail...>&>(t));
        }
    }

    template <size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(const tuple<Head, Tail...>& t) noexcept {
        if constexpr (I == 0) {
            return (t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<const tuple<Tail...>&>(t));
        }
    }

    template <size_t I, typename Head, typename... Tail>
    [[nodiscard]] constexpr decltype(auto) get(tuple<Head, Tail...>&& t) noexcept {
        if constexpr (I == 0) {
            return mini_std::move(t.value);
        } else {
            return mini_std::get<I - 1>(static_cast<tuple<Tail...>&&>(t));
        }
    }

    struct string_view {
        const char* data_ptr = nullptr;
        size_t len = 0;
        constexpr string_view() = default;
        constexpr string_view(const char* str) noexcept : data_ptr(str) { while (str[len] != '\0') { ++len; } }
        constexpr bool operator==(const string_view& other) const noexcept {
            if (len != other.len) return false;
            for (size_t i = 0; i < len; ++i) { if (data_ptr[i] != other.data_ptr[i]) return false; }
            return true;
        }
    };

    namespace strHash {
        constexpr unsigned long long fnv_basis = 14695981039346656037ULL;
        constexpr unsigned long long fnv_prime = 1099511628211ULL;

        constexpr unsigned long long hash_str(const char* str) noexcept {
            unsigned long long hash = fnv_basis;
            size_t i = 0;
            if (!str) return hash;
            while (str[i] != '\0') {
                hash ^= static_cast<unsigned long long>(str[i]);
                hash *= fnv_prime;
                ++i;
            }
            return hash;
        }
    }

    template <typename T>
    struct UniversalView {
        const T* ptr = nullptr;
        size_t length = 0;

        constexpr UniversalView() = default;

        template <size_t N>
        constexpr UniversalView(const T (&arr)[N]) noexcept : ptr(arr), length(N) {}

        template <typename ContainerType>
        constexpr UniversalView(const ContainerType& container) noexcept 
            : ptr(container.data()), length(container.size()) {}

        template <typename IteratorType>
        constexpr UniversalView(IteratorType first, IteratorType last) noexcept {
            if constexpr (is_pointer_v<IteratorType>) {
                ptr = first;
                length = static_cast<size_t>(last - first);
            } else {
                ptr = &(*first);
                length = static_cast<size_t>(last - first);
            }
        }

        constexpr const T* data() const noexcept { return ptr; }
        constexpr size_t size() const noexcept { return length; }
    };
}

namespace used_std {
    using namespace std; 
    
    // template<typename... T>
    // using tuple = mini_std::tuple<T...>;
    // template<mini_std::size_t I,typename Tuple>
    // using tuple_element = mini_std::tuple_element<I, Tuple>;
    template<typename T>
    using UniversalView = mini_std::UniversalView<T>;
    // template<typename T>
    using namespace mini_std::strHash;
}
// Global user-defined string literal hash shortcut
constexpr unsigned long long operator""_hash(const char* str, used_std::size_t) noexcept {
    return used_std::hash_str(str);
}

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
constexpr auto make_fields_match(Rules&&... rules) noexcept {
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

template <auto LabelID> struct goto_case_t {};
template <auto LabelID> constexpr auto goto_case() noexcept { return goto_case_t<LabelID>{}; }

template <auto LabelID, typename T> struct GotoValue { T value; };
template <auto LabelID, typename T> constexpr auto pass_and_goto(T&& val) noexcept { return GotoValue<LabelID, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

struct AnyType {};
// constexpr AnyType any_value{};

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
    template <typename KeyType, typename TargetType>
    concept ContainsRange = requires(KeyType k, TargetType t) { { k.contains(t) }; };

    template <typename KeyType, typename TargetType>
    concept MatchesPredicate = requires(KeyType k, TargetType t) { { k.matches(t) };  };

    template <typename ActionType, typename... Args>
    concept InvocableAction = requires(ActionType a, Args&&... args) { used_std::forward(a)(used_std::forward(args)...); };

    template <typename ActionType>
    concept InvocableActionNoargs = requires(ActionType a) { used_std::forward(a)(); };


    template <typename T> concept IsWildcard = used_std::is_same_v<used_std::decay_t<T>, AnyType>;
    template <typename T> concept IsPureFallthrough = used_std::is_same_v<used_std::decay_t<T>, fallthrough_t>;

    template <typename T> struct is_fallthrough_value { static constexpr bool value = false; };
    template <typename T> struct is_fallthrough_value<FallthroughValue<T>> { static constexpr bool value = true; };
    template <typename T> concept IsValueFallthrough = is_fallthrough_value<used_std::decay_t<T>>::value;

    template <typename T> struct is_goto_case { static constexpr bool value = false; static constexpr int label = 0; };
    template <auto LabelID> struct is_goto_case<goto_case_t<LabelID>> { static constexpr bool value = true; static constexpr auto label = LabelID; };
    template <typename T> concept IsPureGoto = is_goto_case<used_std::decay_t<T>>::value;

    template <typename T> struct is_goto_value { static constexpr bool value = false; static constexpr int label = 0; };
    template <auto LabelID, typename T> struct is_goto_value<GotoValue<LabelID, T>> { static constexpr bool value = true; static constexpr auto label = LabelID; };
    template <typename T> concept IsValueGoto = is_goto_value<used_std::decay_t<T>>::value;

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
        
        // Use tuple_size_v instead of sizeof...
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
        
        // Use tuple_size_v instead of sizeof...
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
template <auto LabelID, typename KeyType, typename ActionType, BranchHint HintValue>
struct ImplCase {
    KeyType key;
    ActionType action;
    static constexpr auto label = LabelID;
    static constexpr BranchHint hint = HintValue;
};

// ============================================================================
// 8. OVERLOADED PIPELINE SUGAR GENERATORS (operator>>)
// ============================================================================
template <auto LabelID, BranchHint Hint, typename KeyType>
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
template <typename T> constexpr auto Case(T&& val) noexcept { return SugarProxyKey<0, BranchHint::None, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto likely_Case(T&& val) noexcept { return SugarProxyKey<0, BranchHint::Likely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <typename T> constexpr auto unlikely_Case(T&& val) noexcept { return SugarProxyKey<0, BranchHint::Unlikely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <auto LabelID, typename T> constexpr auto label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::None, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <auto LabelID, typename T> constexpr auto likely_label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::Likely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }
template <auto LabelID, typename T> constexpr auto unlikely_label_Case(T&& val) noexcept { return SugarProxyKey<LabelID, BranchHint::Unlikely, used_std::decay_t<T>>{ used_std::forward<T>(val) }; }

// Hardware Prediction Branch Optimizer Hints Primitive Mapping
template <BranchHint Hint>
[[nodiscard]] constexpr bool apply_hardware_hint(bool condition) noexcept {
    if constexpr (Hint == BranchHint::Likely) { return __builtin_expect(!!(condition), 1); }
    else if constexpr (Hint == BranchHint::Unlikely) { return __builtin_expect(!!(condition), 0); }
    else { return condition; }
}

template <typename ActionType, typename ContextType>
constexpr decltype(auto) execute_action(ActionType&& action, ContextType& ctx) {
    if constexpr (mini_concepts::InvocableAction<ActionType, ContextType&>) {
        return used_std::forward<ActionType>(action)(ctx);
    } else {
        return used_std::forward<ActionType>(action)();
    }
}

template <typename T>
struct UnwrapReturnType { using type = used_std::remove_cvref_t<T>; };

template <typename T>
requires mini_concepts::IsValueFallthrough<T> || mini_concepts::IsValueGoto<T>
struct UnwrapReturnType<T> { 
    using type = used_std::remove_cvref_t<decltype(std::declval<T>().value)>; 
};
// Unrolled Matrix State Router Engine Loop
template <typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple>
constexpr auto universal_switch_matrix(const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
    constexpr used_std::size_t TotalCases = used_std::tuple_size_v<used_std::remove_cvref_t<CasesTuple>>;
    
    using CoreReturnType = decltype(execute_action(default_action, ctx));
    using CleanReturnType = typename UnwrapReturnType<CoreReturnType>::type;
    
    auto unrolled_matrix_router = [&]<used_std::size_t... Is>(used_std::index_sequence<Is...>) -> CleanReturnType {
        CleanReturnType result{};
        bool matched = false;
        bool force_execute_next = false;
        bool jump_requested = false;
        
        // FIX HERE: Strip 'const' from LabelType so it can be assigned inside the loop
        using RawFirstCase = used_std::remove_cvref_t<decltype(used_std::get<0>(cases))>;
        using LabelType    = used_std::remove_cvref_t<decltype(RawFirstCase::label)>;
        
        LabelType target_jump_label{};
        
        used_std::size_t loop_guard = 0;
        constexpr used_std::size_t MaxAllowedJumps = TotalCases * 2;
        
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

                    if constexpr (mini_concepts::IsPureFallthrough<decltype(action_res)>) {
                        force_execute_next = true;
                    }
                    else if constexpr (mini_concepts::IsValueFallthrough<decltype(action_res)>) {
                        if constexpr (!used_std::is_same_v<CleanReturnType, void>) { result = action_res.value; }
                        force_execute_next = true;
                    }
                    else if constexpr (mini_concepts::IsPureGoto<decltype(action_res)>) {
                        target_jump_label = mini_concepts::is_goto_case<decltype(action_res)>::label;
                        jump_requested = true;
                        current_iteration_jumped = true;
                    }
                    else if constexpr (mini_concepts::IsValueGoto<decltype(action_res)>) {
                        if constexpr (!used_std::is_same_v<CleanReturnType, void>) { result = action_res.value; }
                        target_jump_label = mini_concepts::is_goto_value<decltype(action_res)>::label;
                        jump_requested = true;
                        current_iteration_jumped = true;
                    }
                    else {
                        if constexpr (!used_std::is_same_v<CleanReturnType, void>) { result = action_res; }
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

    template <typename Tuple, used_std::size_t... Is>
    constexpr auto make_cases_tuple_impl(Tuple&& full_tuple, used_std::index_sequence<Is...>) {
        // Explicitly specify element types in tuple template arguments
        return used_std::tuple<
            used_std::remove_cvref_t<decltype(used_std::get<Is>(full_tuple))>...
        >(used_std::get<Is>(used_std::forward<Tuple>(full_tuple))...);
    }

    template <used_std::size_t... Is, typename TargetType, typename DefaultType, typename ContextTuple, typename CasesTuple>
    constexpr auto evaluate_sorted_matrix(used_std::index_sequence<Is...>, const TargetType& target, DefaultType&& default_action, ContextTuple& ctx, CasesTuple&& cases) {
        if constexpr (sizeof...(Is) > 0) {
            // Pass cases as a single tuple argument (4 parameters total)
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

template <typename TargetType, typename ContextTuple, typename... AllTrailingArgs>
constexpr auto universal_switch(const TargetType& target, ContextTuple& ctx, AllTrailingArgs&&... args) {
    constexpr used_std::size_t TotalArgs = sizeof...(AllTrailingArgs);
    static_assert(TotalArgs >= 1, "Library Error: You must supply a terminal fallback default action.");
    
    constexpr used_std::size_t CaseCount = TotalArgs - 1;
    
    // Extract default action (the last argument)
    decltype(auto) default_action = mini_pack::pack_element<CaseCount, AllTrailingArgs...>::get(used_std::forward<AllTrailingArgs>(args)...);
    
    if constexpr (CaseCount == 0) {
        return execute_action(used_std::forward<decltype(default_action)>(default_action), ctx);
    } else {
        // Pack all arguments into a temporary full tuple
        auto full_tuple = used_std::tuple<typename used_std::remove_reference_t<AllTrailingArgs>...>(used_std::forward<AllTrailingArgs>(args)...);
        
        // Extract ONLY the cases (indices 0 to CaseCount - 1)
        auto cases_tuple = mini_pack::make_cases_tuple_impl(
            used_std::move(full_tuple), 
            used_std::make_index_sequence<CaseCount>{}
        );

        return mini_pack::evaluate_sorted_matrix(
            used_std::make_index_sequence<CaseCount>{}, 
            target, 
            used_std::forward<decltype(default_action)>(default_action), 
            ctx, 
            cases_tuple
        );
    }
}

// ============================================================================
// 10. HIGH-UTILITY PROXY WRAPPERS (uswitch DSL FRONTEND ENTRY)
// ============================================================================
template <typename TargetType, typename ContextTuple>
struct SwitchPipelineProxy {
    const TargetType& target;
    ContextTuple ctx; // Correctly holds tuple of references

    template <typename... CaseTypes>
    constexpr decltype(auto) operator()(CaseTypes&&... cases) && {
        return universal_switch(target, ctx, used_std::forward<CaseTypes>(cases)...);
    }
};

template <typename TargetType>
struct SwitchTargetProxy {
    const TargetType& target;

    // Fix: Capture arguments as lvalue references std::tuple<ContextArgs&...>
    template <typename... ContextArgs>
    constexpr auto operator[](ContextArgs&... args) && noexcept {
        using TupleType = used_std::tuple<ContextArgs&...>;
        return SwitchPipelineProxy<TargetType, TupleType>{ target, TupleType(args...) };
    }

    // // Overload for cases where no context variables are passed: Match(val)[]
    // constexpr auto operator[]() && noexcept {
    //     using TupleType = used_std::tuple<>;
    //     return SwitchPipelineProxy<TargetType, TupleType>{ target, TupleType{} };
    // }
};

template <typename TargetType>
constexpr auto Match(const TargetType& target) noexcept {
    return SwitchTargetProxy<TargetType>{ target };
}

int main () {
    int num = 0;
    int ret = Match(num)[]  (
        Case(10) >> [] { return 5; },
        label_Case<1>(make_range(2,9)) >> [&] {num++; return 5; },
        Case(0)  >> [] { return goto_case<1>(); },
        []{return 555;}
    );

    std::cout << ret << " "<< num;
    return ret;
}