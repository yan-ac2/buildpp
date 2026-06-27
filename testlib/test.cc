#include <cstdio>

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


struct fmt {
    enum colors {Not_color,
        Black,    Bold_Black,   High_Black,
        Red,      Bold_Red,     High_Red,
        Green,    Bold_Green,   High_Green,
        Yellow,   Bold_Yellow,  High_Yellow,
        Blue,     Bold_Blue,    High_Blue,
        Purple,   Bold_Purple,  High_Purple,
        Cyan,     Bold_Cyan,    High_Cyan,
        White,    Bold_White,   High_White,
    };
    
    string str;
    

    constexpr fmt(auto&&... args) {

        // size_t totalSize = [](auto&&... args) -> size_t {
        //     size_t size = 0;
        //     ((size += std::string_view(args).size()), ...);
        //     return size;
        // }(args...);
        // str.reserve(totalSize);
        
    };
    constexpr fmt& color(colors color) {return *this;}
    constexpr fmt& endl() {return *this;}
    
    constexpr strView sv() const { return this->str;}
    constexpr operator strView() const { return this->sv();}
    constexpr const char* cstr() const { return this->str.data();}
    constexpr operator const char*() const { return this->cstr();}
    constexpr operator string() const { return this->str;}
    //constexpr const char* cstr() const { return this->str.c_str();}
    //constexpr operator const char*() const { return this->cstr();}

    // private:
    static constexpr strView Color(colors color) {
        switch (color) {
            case Not_color:    return "\033[0m"; break;
            case Black:        return "\033[0;0m" ; break;
            case Red:          return "\033[0;31m"; break;
            case Green:        return "\033[0;32m"; break;
            case Yellow:       return "\033[0;33m"; break;
            case Blue:         return "\033[0;34m"; break;
            case Purple:       return "\033[0;35m"; break;
            case Cyan:         return "\033[0;36m"; break;
            case White:        return "\033[0;37m"; break;
            case Bold_Black:   return "\033[1;30m"; break;
            case Bold_Red:     return "\033[1;31m"; break;
            case Bold_Green:   return "\033[1;32m"; break;
            case Bold_Yellow:  return "\033[1;33m"; break;
            case Bold_Blue:    return "\033[1;34m"; break;
            case Bold_Purple:  return "\033[1;35m"; break;
            case Bold_Cyan:    return "\033[1;36m"; break;
            case Bold_White:   return "\033[1;37m"; break;
            case High_Black:   return "\033[0;90m"; break;
            case High_Red:     return "\033[0;91m"; break;
            case High_Green:   return "\033[0;92m"; break;
            case High_Yellow:  return "\033[0;93m"; break;
            case High_Blue:    return "\033[0;94m"; break;
            case High_Purple:  return "\033[0;95m"; break;
            case High_Cyan:    return "\033[0;96m"; break;
            case High_White:   return "\033[0;97m"; break;
        }
    };
};
constexpr fmt operator""_fmt(const char* str,size_t) { return fmt(str);}


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




int main ()
{
    sPtr<int> sptr(new int(5));
    const char* test1 = "53412.23123";
    const char* test2 = test1;
    float test = 53412.23123;
    for (int i = 0; i < 10; i++) {
        printf("printf %f %llu \n",test, char_trait::len(test2));
        print << fmt::Color(fmt::Red) << "convert " << fmt::Color(fmt::Not_color), test ," ","\n";
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