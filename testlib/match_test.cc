
#include <iostream>
#include "match.hpp"
// --- A. Numeric & Range Matching ---
void showcase_numeric_and_ranges(int score) {
    std::cout << "\n=== 1. Numeric & Range Pattern Matching ===" << std::endl;
    char test = 't';
    // std::size_t score2 = 2;
    std::string_view result = Match(score)[score,test]  (
        Case(100)                      >> [] { return "Perfect Score!"; },
        Case(Range{90,99})        >> [] { return "Grade: A"; },
        Case(Range{80, 89})       >> [] { return "Grade: B"; },
        Case(Range{70, 79})       >> [] { return "Grade: C"; },
        [](int& s) { 
            return (s < 70 ? "Grade: Fail" : "Grade: Invalid"); 

        }
    );

    std::cout << "Score [" << score << "] -> " << result << std::endl;
}

// --- B. StaticLabel / FNV-1a Hash Matching ---
void showcase_hash_labels(std::string_view command) {
    std::cout << "\n=== 2. StaticLabel Hash Matching ===" << std::endl;
    int test = 1;
    std::size_t cmd_hash = used_std::strHash::fnv1a_hash(command.data(), command.size());

    std::string_view response = Match(command)[command,&cmd_hash,&test] (
        Case("start")   >> [] { return "System Starting..."; },
        label_Case<"stop">
        ("stop")        >> [](int* i) { 
            if (*i == 1) {
                return goto_case("start"); 
            } else {
                return goto_case("err"); 
            }
        },
        Case("pause")           >> [] { return "System Paused."; },
        label_Case<"err">(__)   >> [](std::string_view& s) { return "err"; },
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

    std::cout << "=== Free Function & Member Function Matching ===\n";

    // -------------------------------------------------------------
    // 1. Standalone / Free Function Evaluation
    // -------------------------------------------------------------
    int number = -43;
    std::string_view num_res = Match(number)[__](
        Case(&is_even)     >> [] { return "Even Number"; },
        Case(Predicate(&is_positive)) >> [] { return "Positive Odd Number"; },
        [] { return "Other"; }
    );
    std::cout << "Number " << number << " -> " << num_res << "\n";

    // -------------------------------------------------------------
    // 2. Unbound Member Function Evaluation (Target is the Instance)
    // -------------------------------------------------------------
    User u1{"Alice", 22, true};
    
    std::string_view user_res = Match(u1)[__] (
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

    // 4. Branch Prediction Showcase
    showcase_branch_hints(200);
    showcase_branch_hints(500);
    // std::printf("%d %d" , ret , num);

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
    static_assert(Match(num)[__] (
        label_Case<"id">(Range{0,15},Range{20,30}) >> []{return true;},
        []{return false;}), "" );
    static_assert(Match(num)[__] (
        Case(Range<RangeType::Or>{0,10},Range<RangeType::Or>{20,40}) >> []{return true;},
        []{return false;}), "" );
    constexpr int a = 0b1010;
    static_assert(Match(a)[__] (
        Case(bits_all_clear(0b0100 | 0b0001)) >> []{return true;},
        []{return false;}), "" );
        
    Match(test)[__] (
        Case(ProjectionCase(20,&s::get,&test)) >> []{
            std::cout << "is 20";
        },
        Case(__) >> [&]() {
            std::cout << "default" << test.i << '\n';
        },
        []{
            std::cout << "Error";
        }
    );
    int num2 = 0;
    Match(num2)[num2] (
        label_Case<"inRange">(Range{20,40}) >> []{
            std::cout << "is in range";
        },
        label_Case<"outRange">(Range{0,20}) >> [](int& i){
            ++i;
            std::cout << "increase" << i << '\n';
            if (i == 15) {
                i = 50;
                return goto_case("any");
            }
            return goto_case("outRange");
        },
        label_Case<"any">(__) >> [](int& i) {
            --i;
            std::cout << "decrease" << i << '\n';
            return goto_case("inRange");
        },
        []{
            std::cout << "is not range";
        }
    );
    return 1;
}