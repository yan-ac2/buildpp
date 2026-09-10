
#include <iostream>
// #include <ranges>
#include "match.hpp"

// --- A. Numeric & Range matching ---
void showcase_numeric_and_ranges(int score) {
    std::cout << "\n=== 1. Numeric & Range Pattern matching ===" << std::endl;
    char test = 't';
    // std::size_t score2 = 2;
    std::string_view result = match(score)(score,test)  (
        Case(100)                      >> "Perfect Score!",
        Case(Range{90,99})        >> "Grade: A",
        Case(Range{80, 89})       >> "Grade: B",
        Case(Range{70, 79})       >> "Grade: C",
        [](int& s) { 
            return (s < 70 ? "Grade: Fail" : "Grade: Invalid"); 

        }
    );

    std::cout << "Score [" << score << "] -> " << result << std::endl;
}

// --- B. StaticLabel / FNV-1a Hash matching ---
void showcase_hash_labels(std::string_view command) {
    std::cout << "\n=== 2. StaticLabel Hash matching ===" << std::endl;
    int test = 1;
    std::size_t cmd_hash = used_std::strHash::fnv1a_hash(command.data(), command.size());

    std::string_view response = match(command)(command,&cmd_hash,&test) (
        Case<"start">("start")   >> [] { return "System Starting..."; },
        Case<"stop">
        ("stop")        >>  [&](){ return test == 1 ? Goto<"start"> : Goto<"err">;},
        // [](int* i) { 
        //     if (*i == 1) {
        //         return Goto<"start">; 
        //     } else {
        //         return Goto<"err">; 
        //     }
        // },
        Case("pause")           >> [] { return "System Paused."; },
        Case<"err">(__)   >> []() { return "err"; },
        []{return "UNDEFINED!";}
    );

    std::cout << "Command [\"" << command << "\"] (Hash: " << cmd_hash << ") -> " << response << std::endl;
}


constexpr bool is_even(int val) { return val % 2 == 0; }
constexpr int add(int val,int val2) { return val + val2; }
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

    std::cout << "=== Free Function & Member Function matching ===\n";

    // -------------------------------------------------------------
    // 1. Standalone / Free Function Evaluation
    // -------------------------------------------------------------
    int number = -43;
    std::string_view num_res = match(number)()(
        Case(ProjectionCase(true,&is_even))     >> [] { return "Even Number"; },
        Case(&is_positive) >> [] { return "Positive Odd Number"; },
        [] { return "Other"; }
    );
    std::cout << "Number " << number << " -> " << num_res << "\n";

    // -------------------------------------------------------------
    // 2. Unbound Member Function Evaluation (Target is the Instance)
    // -------------------------------------------------------------
    User u1{"Alice", 22, true};
    
    std::string_view user_res = match(u1)() (
        Case(ProjectionCase(true,&User::is_adult))  >> [] { return "Adult User"; },
        Case(Predicate(&User::is_active)) >> [] { return "Active Minor"; },
        [] { return "Inactive Minor"; }
    );
    std::cout << u1.name << " -> " << user_res << "\n";

    // -------------------------------------------------------------
    // 3. Bound Member Function Evaluation (External Instance)
    // -------------------------------------------------------------
    Validator validator{30};
    int score = 75;

    std::string_view val_res = match(score)()(
        Case(Predicate(&Validator::exceeds_threshold,&validator)) >> [] {
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
    showcase_hash_labels("stop");
    showcase_hash_labels("pause");
    showcase_hash_labels("reboot");
    showcase_hash_labels("rebootss");


    struct s {
        int i;
        constexpr s(int i) : i(i){}
        constexpr int get() const {
            return i;
        }
        constexpr int add(int s) const {
            return i + s;
        }
    };
    constexpr s test = 20;
    constexpr int num = 15;
    static_assert(match(num)() (
        Case<"id">(make_compound_range(Range{0,15},Range{20,30})) >> true,
        false), "" );
    static_assert(match(num)() (
        Case(make_compound_range(Range<RangeType::Or>{0,10},Range<RangeType::Or>{20,40})) >> []{return true;},
        []{return false;}), "" );
    constexpr int a = 0b1010;
    static_assert(match(a)() (
        Case(bits_all_clear(0b0100 | 0b0001)) >> []{return true;},
        []{return false;}), "" );
        
    match(test)(__) (
        Case(field(&s::i,20)) >> []{
            std::cout << "is 20";
        },
        Case(__) >> [&]() {
            std::cout << "default" << test.i << '\n';
        },
        []{
            std::cout << "Error";
        }
    )();
    std::cout << "\n";
    int num2 = 0;
    for (int i = 0 ;i < 5; ++i) {
        match(num2)(&num2) (
            Case<"inRange">(Range{20,40}) >> [](int* i){
                std::cout << "is in range";
                *i = 0;
            },
            Case<"outRange">(Range{0,20}) >> [](int* i){
                ++*i;
                std::cout << "increase" << *i << '\n';
                if (*i >= 15) {
                    *i = 50;
                    return Goto<"any">;
                }
                return Goto<"outRange">;
            },
            Case<"any">(__) >> [](int* i) {
                --*i;
                std::cout << "decrease" << *i << '\n';
                return Range{20,40}.contains(*i) ? Goto<"inRange"> : Goto<"any">;
            },
            []{
                std::cout << "is not range";
            }
        )();
    }

    int arrtest[] {1,2,3,4,5,6,7,8};
    // auto rangetest = arrtest | std::ranges::views::filter(match(int{})()(Case(Range{1,5}) >> true,false).to_predicate());

    // for (int i : rangetest) {
    //     std::cout << i << ", ";
    // }
    // std::cout << "\n";
    for (int i : arrtest) {
        match(i)(__) (
            Case<"eval">(Range{1,5}) >> Goto<"print">,
            Case<"print">(false) >> [i]{
                std::cout << i << ", ";
            },
            false
        )();
    }
    std::cout << "\n";
    return 1;
}