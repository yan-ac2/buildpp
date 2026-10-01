#include <cstdio>
#include <string>
#include <iostream>
#include <format>
#include <charconv>

using size_t = __SIZE_TYPE__;
template<typename T,T v>
struct integral_const{
    static constexpr T value = v;
    using value_type = T;
    using type = integral_const;
    constexpr operator value_type() const { return value; }
    constexpr value_type operator()() const { return value; }
};
template<bool T = true>  struct true_t : integral_const<bool, T> {};
template<bool T = false> struct false_t : integral_const<bool, T> {};
template<typename T>     struct isBool : false_t<> {};
template<>               struct isBool<bool> : true_t<> {};

template<typename T>     struct isPtr : false_t<> {};
template<typename T>     struct isPtr<T*> : true_t<> {};

template<typename T, typename U> struct is_same_t       {static constexpr bool value = false_t<>::value;};
template<typename T>             struct is_same_t<T, T> {static constexpr bool value = true_t<>::value;};

template<typename T> struct remove_ptr           { using type = T;};
template<typename T> struct remove_ptr<T*>       { using type = T;};
template<typename T> struct remove_ptr<T* const> { using type = T;};

template<typename F>                   struct isFn : false_t<> {};
template<typename F, typename... Args> struct isFn<F(Args...)> : true_t<> {};


template<typename F>                   struct isFn_Ptr : false_t<> {};
template<typename F,typename... Args>  struct isFn_Ptr<F(*)(Args...)> : true_t<> {};

static_assert(isFn<remove_ptr<int(*)()>::type>::value, "is fucntion");
//static_assert(is_bool<bool>::value, "is bool" );
static_assert(is_same_t<remove_ptr<int(*)()>::type,int()>::value, "is true");
//static_assert(is_fn<int>::value, "is fn");
int Itoa(size_t value, char* buf, size_t size);
void Ftoa(double value, char* buf, int precision);

struct char_trait
{
    using char_t = char;
    using int_t  = int;
    static constexpr const char_t num[] = "0123456789"; 
    static constexpr void assign(char_t& lhs, char_t rhs) {lhs = rhs;}
    static constexpr bool eq(char_t lhs, char_t rhs) {return lhs == rhs;}
    static constexpr bool lt(char_t lhs, char_t rhs) {return lhs < rhs;}

    static constexpr size_t len(const char_t* str) {
        size_t size = 0;
        for (;*str++ != '\0';size++){};
        return size;    
    };

    static char_t* copy(char_t* to, const char_t* src) {
        for (size_t i = 0; src[i] != '\0'; i++) { to[i] = src[i];}
        return to;
    }

    static constexpr int cmp(const char_t* lhs, const char_t* rhs, size_t n) {
        for (size_t i = 0; i < n; i++) {
            if (lt(lhs[i],rhs[i])) {return -1;}
            if (lt(lhs[i],rhs[i])) {return 1;}
        }
        return 0;
    }

    static constexpr const char_t* find(const char_t* src,size_t n ,const char_t& cmp) {
        for (size_t i = 0; i < n; i++) {
            if (eq(src[i],cmp)) {return &src[i];}
        };
        return nullptr;
    }

    static constexpr int_t eof() {return -1;}
};

template<typename T>
struct TempPtr {
    T* ptr = nullptr;
    TempPtr(T* other) : ptr(other) {}
    ~TempPtr() {ptr == nullptr;}
};

template<typename T>
class sPtr
{
    T* ptr;
    int* ref;

    constexpr void cleanup()
    {
        if (ref != nullptr) {
            (ref)--;
            if (ref == 0) {
                delete ptr;
                delete ref;
                ptr = nullptr;
                ref = nullptr;
                
            }
        }
    }
    public:
    
    explicit constexpr sPtr(T* p = nullptr) : ptr(p)
    {
        if (ptr) { ref = new int(1);}
        else { ref = nullptr; }
    }
    constexpr sPtr(const sPtr& other) : ptr(other.ptr), ref(other.ref) {
        if (ref) {(*ref)++;}
    }
    
    constexpr sPtr& operator=(const sPtr& other) {
        if (this != &other) cleanup();
        ptr = other.ptr;
        ref = other.ref;
        if (ref != nullptr) {
            (*ref)++;
        }
        return *this;
    }

    constexpr sPtr(sPtr&& other) noexcept : ptr(other.ptr), ref(other.ref) {
        other.ptr = nullptr;
        other.ref = nullptr;
    }
    constexpr sPtr& operator=(sPtr&& other) {
        if (this != other) {   
            cleanup();
            ptr = other.ptr;
            ref = other.ref;
            other.ptr = nullptr;
            other.ref = nullptr;
        }
        return *this;
    }
    constexpr T& operator*() {return *ptr;}
    constexpr T* operator->() {return ptr;}
    constexpr int use_count() { return ref ? *ref : 0; }
    constexpr T* get() { return ptr; }
    ~sPtr() { cleanup(); }
};

// class fn
// {
//     void(*_fn)();
//     public:
//     template<typename Fn> requires (isPtr<Fn>::value && isFn_Ptr<Fn>::value)
//     constexpr fn(Fn&& f) 
//     {
//         std::memcpy(&_fn,&f,8);
//     }
//     template<typename... Args>
//     constexpr fn& run(auto&& f, Args&&... args) {
//         auto fn = reinterpret_cast<decltype(f)>(_fn);
//         fn(std::forward<Args>(args)...);
//         return *this;
//     }
// };


int Itoa(size_t value, char* buf, size_t size) {
    if (buf == nullptr || size < 2) return -1;

    char* p = buf + size - 1; 
    *p = '\0'; // Set null terminator at the end

    size_t uvalue = (value < 0) ? -value : value;
    bool negative = (value < 0);

    // Process digits from right to left
    do {
        if (p <= buf) return -1; // Buffer too small
        *--p = char_trait::num[uvalue % 10];
        uvalue *= 0.1f;
    } while (uvalue > 0);

    if (negative) {
        if (p <= buf) return -1;
        *--p = '-';
    }

    // Shift the string to the front of the buffer
    int len = (buf + size - 1) - p;
    for (int i = 0; i <= len; i++) {
        buf[i] = p[i];
    }

    return len;
}



class string {
    public:
    constexpr string() {
        len = 0;
    }
    constexpr void isLarge()
    {
        if (len > sizeof(storage) - 1) {storage.large.isLarge = true;}
        else {};
    }
    constexpr string(const char* str) {
        len = char_trait::len(str);
        isLarge();
        if (storage.large.isLarge)
        {
            printf("str construct \n");
            storage.large.cap = len + 1;
            storage.large.str = new char[storage.large.cap]();
            char_trait::copy(storage.large.str,  str);
            storage.large.str[storage.large.cap] = '\0';
        } else
        {
            char_trait::copy(this->storage.str,str);
        }
    }
    constexpr string(const char* str,size_t len) : len(len) {
        isLarge();
        if (storage.large.isLarge) {
            printf("str construct2 \n");
            storage.large.cap = len + 1;
            storage.large.str = new char[storage.large.cap];
            char_trait::copy(this->storage.large.str,str);
            storage.large.str[storage.large.cap] = '\0';
        } else {
            char_trait::copy(this->storage.str,str);
        }
    }
    constexpr string(size_t value)  : len(Itoa(value, storage.str, sizeof(storage.str))) {}
    constexpr string(int value)     : len(Itoa(value, storage.str, sizeof(storage.str))) {}
    constexpr string(float value) {
        this->len = char_trait::len(storage.str);
        Ftoa(value, storage.str, 3);
    }
    constexpr string& append(const char* strin)
    {
        this->len += char_trait::len(strin);
        isLarge();
        if (storage.large.isLarge) {
            storage.large.str = new char[len];

        } 
        return *this;
    }
    string& operator=(const char* str)  {
        this->len = char_trait::len(str); 
        isLarge();
        if (storage.large.isLarge) {
            if (len > storage.large.cap)
            { 
                if (storage.large.str != nullptr) {
                    printf("add new notnull ");
                    delete[] storage.large.str;
                    storage.large.cap = len;
                    storage.large.str = new char[storage.large.cap]();
                }
                if (storage.large.str == nullptr)
                {
                    printf("add new ");
                    storage.large.cap = len;
                    storage.large.str = new char[storage.large.cap]();
                }
                char_trait::copy(this->storage.large.str,str);
                storage.large.str[len] = '\0'; 
            } else {
                char_trait::copy(this->storage.large.str,str);
                storage.large.str[len] = '\0'; 
            }
        } else { 
            char_trait::copy(this->storage.str,str); 
        }
        return *this;
    }
    string& operator=(size_t& value)    {
        this->len = Itoa(value,this->storage.str,sizeof(storage)); 
        return *this;
    }
    string& operator=(size_t&& value)   {
        this->len = Itoa(value,this->storage.str,sizeof(storage)); 
        return *this;
    }

    ~string(){
        if (storage.large.isLarge && storage.large.str != nullptr) {
            printf("delete str");
            delete [] storage.large.str;
        }
    }

    constexpr size_t size() const   { return len;}
    constexpr size_t cap()  const   { 
        if (storage.large.isLarge) return storage.large.cap;
        else return sizeof(storage.str);
    }
    constexpr const char* data() const  {
        if (storage.large.isLarge) {return storage.large.str;} 
        else return storage.str;
    }
    constexpr operator const char*() const { return storage.str;}
    private:
    union 
    {
        char str[24];
        struct {
            bool isLarge;
            char* str;
            size_t cap;
        }large;
    }storage;
    size_t len;
};

class strView{
    const char* _str;
    size_t _len;
    public:
    constexpr strView(const char* str,size_t size) : _str(str), _len(size) {}
    constexpr strView(const char* str) : _str(str),_len(0){ for (;str[_len] != '\n';_len++); }
    constexpr strView(string sstr) : _str(sstr.data()), _len(sstr.size()) {}

    constexpr operator const char*() const { return _str;}
    constexpr operator string() const { return {_str,_len};};
    constexpr size_t size() const { return _len;}
    constexpr const char* data() const { return _str;}
    constexpr bool empty()const {return _len == 0;}
};

inline struct implPrint
{
    constexpr implPrint& operator <<(const char* in) {
        std::printf("%s",in);
        return *this;
    }
    constexpr implPrint& operator <<(strView in) {
        std::printf("%s",in.data());
        return *this;
    }
    constexpr implPrint& operator <<(string in) {
        std::printf("%s",in.data());
        return *this;
    }
    constexpr implPrint& operator ,(const char* in) {
        std::printf("%s",in);
        return *this;
    }
    constexpr implPrint& operator ,(string in) {
        std::printf("%s",in.data());
        return *this;
    }
    constexpr implPrint& operator ,(std::string in) {
        std::printf("%s",in.c_str());
        return *this;
    }
    constexpr implPrint& operator ,(double in) {
        std::printf("%f",in);
        return *this;
    }
    constexpr void operator <<(implPrint& f) {f = *this;}
}print;

void Ftoa(double value, char* buf, int precision) {
    char temp[16];

    // 1. Handle negative numbers
    if (value < 0) {
        *buf++ = '-';
        value = -value;
    }
    // 2. Extract integer part
    unsigned int ipart = static_cast<unsigned int>(value);
    temp[Itoa(ipart, temp, sizeof(temp))] = '.';
    int it = 0;
    for (;temp[it] != '.'; it++) {
        *buf++ = temp[it];
        temp[it]= 0;
    }
    *buf++ = temp[it];
    // 3. Extract fractional part
    float fpart = (value - static_cast<float>(ipart));
    it = 0;
    for (; it < precision; it++)
    {
        fpart *= 10.00005f;
        unsigned int f = (unsigned int)fpart;
        temp[it] = char_trait::num[f];
        fpart -= f;
    };
    temp[it] = '\0';
    for (int i = 0; i < precision + 1; i++) {
        if (temp[i] ==  '\0') {*buf++ = '\0'; break;}
        *buf++ = temp[i];
    }
}


template <typename T>
concept onlyStrConv = requires(T t) { { std::string_view(t) } -> std::same_as<std::string_view>; };
template <typename T>
concept Formattable = 
    onlyStrConv<T> || 
    std::integral<std::decay_t<T>> ||
    std::floating_point<std::decay_t<T>>;

template <typename... Args>
concept onlyStr = (Formattable<Args> && ...);

struct [[nodiscard]] fmt {
    std::string str;

    // 2. Format String Constructor: fmt("Value: {}, Status: {}", 42, "OK")
    constexpr fmt(std::string_view fmtStr) : str(fmtStr) {}
    template<onlyStr... Args>
    constexpr fmt(std::string_view fmtStr, Args&&... args) noexcept : str(fmtStr) {
        if constexpr (sizeof...(Args) > 0) {
            size_t lastPos = 0;
            pos argsPos[sizeof...(Args)];
            size_t posIdx {0};
            size_t pbegin {0};
            size_t pend {0};
            bool foundop = false;
            for (std::size_t idx {0};idx < fmtStr.size();++idx) {
                if (fmtStr[idx] == '{') {
                    pbegin = idx;
                    foundop = true;
                }
                if (fmtStr[idx] == '}' && foundop) {
                    pend = idx;
                    argsPos[posIdx] = {pbegin,pend + 1 - pbegin };
                    foundop = false;
                    ++posIdx;
                }
            }
            // for (pos& V : argsPos) { // Fixed loop condition
            //     const size_t pbegin = fmtStr.find('{');
            //     const size_t pend = fmtStr.find('}');
            //     if (pbegin != std::string_view::npos && pend != std::string_view::npos) {
            //         const size_t start = pbegin + offset;
            //         const size_t advance = pend + 1;
            //         // Length = pend - pbegin + 1 (e.g. "{}" is 1 - 0 + 1 = 2 chars)
            //         const size_t len = advance - pbegin;
            //         V = {start, len};
            //         fmtStr.remove_prefix(advance); // Advance past '}'
            //         offset += static_cast<std::ptrdiff_t>(fmtStr.size()) - static_cast<std::ptrdiff_t>(len);
            //     }
            // }
            lastPos = 0;
            std::ptrdiff_t offset = 0;
            ((appendArg(std::forward<Args>(args),offset,argsPos[lastPos]),++lastPos), ...);
        }
    }

    // Collapse multiple contiguous spaces into a single space
    constexpr auto clean() -> fmt& { 
        if (str.empty()) return *this;

        size_t writeIdx = 0;
        bool inSpace = false;

        for (size_t readIdx = 0; readIdx < str.size(); ++readIdx) {
            char c = str[readIdx];
            if (c == ' ') {
                if (!inSpace) {
                    str[writeIdx++] = c;
                    inSpace = true;
                }
            } else {
                str[writeIdx++] = c;
                inSpace = false;
            }
        }
        str.resize(writeIdx);
        return *this;
    }

    constexpr auto endl() noexcept -> fmt& { str.append("\n"); return *this; }

    constexpr auto sv()   const noexcept -> std::string_view { return {str}; }
    constexpr auto cstr() const noexcept -> const char* { return str.c_str(); }

    constexpr operator std::string_view() const noexcept { return sv(); }
    constexpr explicit operator const char*() const & noexcept { return cstr(); }
    constexpr explicit operator std::string() && noexcept { return std::move(str); }
    
    friend auto operator<<(std::ostream& os, const fmt& f) -> std::ostream& {
        return os << f.str;
    }

private:
    struct pos {
        size_t start;
        size_t len;
    };

    template<Formattable T>
    constexpr auto appendArg(T&& arg, std::ptrdiff_t& offset, const pos& Pos) noexcept -> void {
        using Raw = std::remove_cvref_t<T>;
        std::string_view argStr;
        // 1. Calculate shifted start position
        const size_t actualStart = static_cast<size_t>(static_cast<std::ptrdiff_t>(Pos.start) + offset);

        if constexpr (std::is_convertible_v<Raw, std::string_view>) {
            argStr = std::string_view(arg);
            // 2. Replace only the placeholder length (Pos.len)
            str.replace(actualStart, Pos.len, argStr);
            // 3. Accumulate delta into offset for the next replacement
            offset += static_cast<std::ptrdiff_t>(argStr.size()) - static_cast<std::ptrdiff_t>(Pos.len);
        } else if constexpr (std::integral<Raw> || std::floating_point<Raw>) {
            char digit[64];
            auto [ptr, ec] = std::to_chars(digit, digit + sizeof(digit), arg);
            argStr = std::string_view(digit, static_cast<size_t>(ptr - digit));
            str.replace(actualStart, Pos.len, argStr);
            offset += static_cast<std::ptrdiff_t>(argStr.size()) - static_cast<std::ptrdiff_t>(Pos.len);
        }

    }

    

};

struct strColors {
    enum colors {
        Not_color = 0,
        Black,     Bold_Black,     High_Black,
        Red,       Bold_Red,       High_Red,
        Green,     Bold_Green,     High_Green,
        Yellow,    Bold_Yellow,    High_Yellow,
        Blue,      Bold_Blue,      High_Blue,
        Purple,    Bold_Purple,    High_Purple,
        Cyan,      Bold_Cyan,      High_Cyan,
        White,     Bold_White,     High_White,
    };
    
    static constexpr auto getColor(colors color) noexcept -> std::string_view {
        switch (color) {
            case Not_color:    return "\033[0m";
            case Black:        return "\033[0;30m";
            case Red:          return "\033[0;31m";
            case Green:        return "\033[0;32m";
            case Yellow:       return "\033[0;33m";
            case Blue:         return "\033[0;34m";
            case Purple:       return "\033[0;35m";
            case Cyan:         return "\033[0;36m";
            case White:        return "\033[0;37m";
            case Bold_Black:   return "\033[1;30m";
            case Bold_Red:     return "\033[1;31m";
            case Bold_Green:   return "\033[1;32m";
            case Bold_Yellow:  return "\033[1;33m";
            case Bold_Blue:    return "\033[1;34m";
            case Bold_Purple:  return "\033[1;35m";
            case Bold_Cyan:    return "\033[1;36m";
            case Bold_White:   return "\033[1;37m";
            case High_Black:   return "\033[0;90m";
            case High_Red:     return "\033[0;91m";
            case High_Green:   return "\033[0;92m";
            case High_Yellow:  return "\033[0;93m";
            case High_Blue:    return "\033[0;94m";
            case High_Purple:  return "\033[0;95m";
            case High_Cyan:    return "\033[0;96m";
            case High_White:   return "\033[0;97m";
            default:           __builtin_unreachable();
        }
    }
};

constexpr auto addColors(std::string_view str,strColors::colors c) noexcept -> std::string {
    static constexpr std::string_view notcolor = strColors::getColor(strColors::Not_color);
    const std::string_view color = strColors::getColor(c);
    const std::size_t len = str.size() + notcolor.size() + color.size();
    std::string temp;
    temp.reserve(len);
    for (auto& c : {color,str,notcolor}) {
        temp += c;
    }
    return temp;
}


void intconv(size_t i,char* buff) {
    const char* bit = "0123456789ABCDEF";
    buff[0] = '0';
    buff[1] = 'x';
    buff[10] = '\0';
    
    for (size_t j = 9; j >= 2; j--) {
        buff[j] = bit[i & 0xF];
        i >>= 4;
    }
};


constexpr std::string fm {fmt("Hello {}",2)};

int main ()
{
    std::cout << fm;
    sPtr<int> sptr(new int(5));
    const char* test1 = "53412.23123";
    const char* test2 = test1;
    float test = 53412.23123;
    for (int i = 0; i < 10; i++) {
        printf("printf %f %llu \n",test, char_trait::len(test2));
        std::cout << addColors( "convert ",strColors::Red) << test << "\n";
        test *= 2.0f;
    }
    // int te = 0x0003E174;
    // print << "convert ", te,"\n";
    // fn f(&test);
    print << *sptr," sizeof ptr ", sizeof(sptr), "\n";
    print << "use count ", sptr.use_count(),"\n";
    int i = *sptr;
    string test3;
    test3 = "use count 2 asdasdasdasdasdasdasd "; 
    print << test3 , sptr.use_count(),"\n";
    print << i,"\n";
    *sptr = 10; 
    sPtr<int> sptr2 = sptr;
    print <<"use count 3 ", sptr.use_count(),"\n";
    print << *sptr2," ",sptr2.use_count(),"\n";
    int s = *sptr;
    print <<"use count 4 ", sptr.use_count(),"\n";
    print << s,"\n";
    return 0;
}