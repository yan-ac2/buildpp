
#include <cstdio>
#include <string_view>
#include <string>
#include "match.hpp"
// --- A. Numeric & Range matching ---
void showcase_numeric_and_ranges(int score) {
    std::printf("\n=== 1. Numeric & Range Pattern matching ===\n");
    // std::size_t score2 = 2;
    std::string_view result = match(score) (
        Case(100)                             >> "Perfect Score!",
        Case(90 < Interval <= 99) >> "Grade: A",
        Case(80 <= Interval <= 89)  >> "Grade: B",
        Case(70 < Interval < 79)  >> "Grade: C",
        (score < 70 ? "Grade: Fail" : "Grade: Invalid")
    );

    std::printf("Score [%d] -> %.*s\n", score, static_cast<int>(result.size()), result.data());
}

// --- B. StaticLabel / FNV-1a Hash matching ---
void showcase_hash_labels(std::string_view command) {
    std::printf("\n=== 2. StaticLabel Hash matching ===\n");
    int test = 1;
    std::size_t cmd_hash = used_std::strHash::fnv1a_hash(command.data(), command.size());

    std::string_view response = match(command) (
        Case<"start">("start") >> "System Starting...",
        Case<"stop"> ("stop")  >>  [&](const std::string_view& cmd) {std::printf("%.*s\n", static_cast<int>(cmd.size()), cmd.data()); return test == 1 ? Goto<"start"> : Goto<"err">;},
        Case("pause")          >> "System Paused.",
        Case("pause2")         >> "System Paused.",
        Case("pause3")         >> "System Paused.",
        Case("pause4")         >> "System Paused.",
        Case("pause5")         >> "System Paused.",
        Case("pause6")         >> "System Paused.",
        Case("pause7")         >> "System Paused 7.",
        Case<"err">(__)        >> "err"
        
    );

    std::printf("Command [\"%*s\"] (Hash: %zu) -> %.*s\n",static_cast<int>(command.size()), command.data(), cmd_hash, static_cast<int>(response.size()), response.data());
}
auto visit(auto&& arg) {
    std::printf("\n=== Visit ===\n");
    std::string_view ret = match(arg) (
        Case(Type_is<int>)    >> "int",
        Case(Type_is<float>)  >> "float",
        Case(Type_is<double>) >> "double",
        Case(__)              >> "err",
        "UNDEFINED!"
    );
    return ret;
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
    std::printf(
    "=================================================\n"
    "        PATTERN MATCHING LIBRARY SHOWCASE         \n"
    "=================================================\n"
    "=== Free Function & Member Function matching ===\n");

    // -------------------------------------------------------------
    // 1. Standalone / Free Function Evaluation
    // -------------------------------------------------------------
    int number = -43;
    std::string_view num_res = match(number) (
        Case(ProjectionCase(true,&is_even))     >> [] { return "Even Number"; },
        Case(&is_positive) >> [] { return "Positive Odd Number"; },
        [] { return "Other"; }
    );
    std::printf("Number %d -> %.*s\n", number, static_cast<int>(num_res.size()), num_res.data());

    // -------------------------------------------------------------
    // 2. Unbound Member Function Evaluation (Target is the Instance)
    // -------------------------------------------------------------
    User u1{"Alice", 22, true};
    
    std::string_view user_res = match(u1) (
        Case(ProjectionCase(true,&User::is_adult))  >> "Adult User",
        Case(Predicate(&User::is_active)) >> "Active Minor",
        "Inactive Minor"
    );
    std::printf("%.*s -> %.*s\n", static_cast<int>(u1.name.size()), u1.name.data(), static_cast<int>(user_res.size()), user_res.data());
    
    // std::variant<int,double,float> var;
    // var = 12.f;
    std::string_view visited = visit(12); 
    std::printf("value : %.*s\n", static_cast<int>(visited.size()), visited.data());
    // -------------------------------------------------------------
    // 3. Bound Member Function Evaluation (External Instance)
    // -------------------------------------------------------------
    Validator validator{30};
    int score = 75;

    std::string_view val_res = match(score) (
        Case(Predicate(&Validator::exceeds_threshold,&validator)) >> "Passed Validation",
        "Failed Validation"
    );
    std::printf("Score %d -> %.*s\n", score, static_cast<int>(val_res.size()), val_res.data());

    // 4. Numeric Range Showcase
    showcase_numeric_and_ranges(95);
    showcase_numeric_and_ranges(72);
    showcase_numeric_and_ranges(45);

    // 4. Compile-Time Hash Labels Showcase
    showcase_hash_labels("start");
    showcase_hash_labels("stop");
    showcase_hash_labels("pause7");
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
    static_assert(match(num) (
        Case<"id">(IntervalCompound(0 >= Interval <= 15,20 < Interval < 30)) >> true,
        false), "" );
    static_assert(match(num) (
        Case(0 > Interval <= 10) >> []{return true;},
        []{return false;}), "" );
    constexpr int a = 0b1010;
    static_assert(match(a) (
        Case(bits_all_clear(0b0100 | 0b0001)) >> []{return true;},
        []{return false;}), "" );
        
    match(test) (
        Case(field(&s::i,20)) >> []{
            std::printf("is 20");
        },
        Case(__) >> [&]() {
            std::printf("default%d\n", test.i);
        },
        []{
            std::printf("Error");
        }
    );
    std::printf("\n");
    int num2 = 0;
    for (int i = 0 ;i < 5; ++i) {
        match(num2) (
            Case<"inRange">(20 <= Interval <= 40) >> [&]{
                std::printf("is in range");
                num2 = 0;
            },
            Case<"outRange">(0 <= Interval <= 20) >> [&] {
                ++num2;
                std::printf("increase%d\n", num2);
                if (num2 >= 15) {
                    num2 = 50;
                    return Goto<"any">;
                }
                return Goto<"outRange">;
            },
            Case<"any">(__) >> [&] {
                --num2;
                std::printf("decrease%d\n", num2);
                return (20 <= Interval <= 40) == num2 ? Goto<"inRange"> : Goto<"any">;
            },
            []{
                std::printf("is not range");
            }
        );
    }

    int arrtest[] {1,2,3,4,5,6,7,8};
    for (int i : arrtest) {
        match(i) (
            Case<"eval">(IntervalCompound{1 <= Interval <= 2, Interval > 4}) >> Goto_v<"print">,
            Case<"print">(false) >> [i]{
                std::printf("%d, ", i);
            },
            false
        );
    }
    std::printf("\n");
    return 1;
}