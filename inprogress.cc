#include <cstdio>
#include <cstddef>
#include <typeinfo>
#include <cstdlib>
#include <iostream>
#include <new>
template<size_t T,size_t... Rest>
struct getMax {
    static constexpr size_t value = T > getMax<Rest...>::value 
    ? T 
    : getMax<Rest...>::value;
    };
template<size_t T>
struct getMax<T> { static constexpr size_t value = T; };

template <typename T, typename... Types>
struct match;

// Match found!
template <typename T, typename... Rest>
struct match<T, T, Rest...> {
    static constexpr int value = 0;
};

// Not a match, keep looking...
template <typename T, typename U, typename... Rest>
struct match<T, U, Rest...> {
    static constexpr int value = 1 + match<T, Rest...>::value;
};

template <typename T> struct removeArray      { using type = T;};
template <typename T> struct removeArray<T[]> { using type = T;};

template<typename... Types>
struct variant{
    static constexpr size_t data_size = getMax<sizeof(Types)...>::value;
    static constexpr size_t data_align = getMax<alignof(Types)...>::value;
    alignas(data_align) char buffer[data_size];
    int type_index;
    template<typename T>
    void destroy_current() {
        if (type_index == -1) return;
        ((T*)buffer)->~T();
    // We use a "Visitor" pattern or a recursive switch to call:
    // ((T*)buffer)->~T();
    }
    template <typename T>
    void set(T&& value) {
        // 1. Destroy whatever was there before
        // destroy_current();

        // 2. "Placement New" - Construct T exactly at the address of our buffer
        new (buffer) T(static_cast<T&&>(value));
        
        // 3. Update the index so we know how to delete it later
        type_index = match<T,Types...>::value; 
    }
    template <typename T>
    T& get() {
        // 1. Get the compile-time index of type T
        constexpr int target_index = match<T, Types...>::value;

        // 2. Runtime Check: Does it match what's actually in the buffer?
        if (type_index != target_index) {
            // In a no-stdlib environment, you might log an error or assert
            // instead of throwing an exception.
            std::cerr << "Invalid variant access! \n";
            std::exit(1);
        }
        // 3. The Magic Trick: Reinterpret Cast
        // We treat the address of 'buffer' as the address of an object of type T.
        return *reinterpret_cast<T*>(buffer);
    }
    template <typename T>
    const T& get() const {
        constexpr int target_index = match<T, Types...>::value;
        if (type_index != target_index) printf("Invalid access");

        return *reinterpret_cast<const T*>(buffer);
    }
};
template<typename T>
class uptr
{
    T* ptr;
    public:
    explicit uptr(T* p = nullptr) :ptr(p){}
    ~uptr() {delete ptr; ptr = nullptr;}

    uptr(const uptr&) = delete;
    uptr& operator=(const uptr&) = delete;

    uptr(uptr&& other) noexcept {
    ptr = other.ptr;     // Steal the pointer
    other.ptr = nullptr; // Leave the old one empty
    }

    // 4. Move Assignment
    uptr& operator=(uptr&& other) noexcept {
        if (this != &other) {
            delete ptr;      // Clean up our current pointer first!
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }

    // Helper to get the raw pointer if needed
    T* get() const { return ptr; }

    // Release ownership without deleting
    T* release() {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }
};
template <typename T>
class uptr<T[]> { 
    T* ptr;
public:
    explicit uptr(T* p = nullptr) : ptr(p) {}
    ~uptr() { delete[] ptr; } 
    T& operator[](size_t index) {
        return ptr[index]; 
    }
    const T& operator[](size_t index) const {
        return ptr[index];
    }

    uptr(const uptr&) = delete;
    uptr& operator=(const uptr&) = delete;

    uptr(uptr&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }

    // 4. Move Assignment
    uptr& operator=(uptr&& other) noexcept {
        if (this != &other) {
            delete ptr;      // Clean up our current pointer first!
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    operator char*() {return ptr;}
    operator const char*() const {return ptr;}
    bool isNull() {return ptr == nullptr;}
    void remove() {delete[] ptr;}
};

template <typename T, typename... Args>
uptr<T> make_unique(Args&&... args) {
    // We use placement new logic or just a straight new call
    return uptr<T>(new T(static_cast<Args&&>(args)...));
}

template <typename T>
uptr<T> make_uniquearr(size_t size) {
    // For a char array, we usually want a raw allocation.
    // 'new char[size]()' would zero-initialize. 
    // 'new char[size]' leaves it uninitialized (faster).
    return uptr<T>(new typename removeArray<T>::type[size]);
}
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

class string {
    void copy(char* dest,const char* src)
    {
        size_t i = 0;
        for(;src[i] == '\0';i++) {dest[i] = src[i];}
        if (storage.type_index == 2) {storage.get<large>().str[i] = '\0';}
        else {storage.get<small>().str[23] = '\0';}
    }
    public:
    constexpr string() {
        len = 0;
    }
    string(const char* str) {
        len = char_trait::len(str);
        if (len > sizeof(storage))
        {
            printf("str construct \n");
            storage.set<large>({
                make_uniquearr<char[]>(storage.get<large>().cap),
                len
            });
            copy(storage.get<large>().str,  str);
            storage.get<large>().str[storage.get<large>().cap] = '\0';
        } else
        {
            copy(this->storage.get<small>().str,str);
        }
    }
    string(const char* str,size_t len) : len(len) {
        if (len > sizeof(storage)) {
            printf("str construct2 \n");
            storage.set<large>({
                make_uniquearr<char[]>(storage.get<large>().cap),
                len
            });
            copy(this->storage.get<large>().str,str);
            storage.get<large>().str[storage.get<large>().cap] = '\0';
        } else {
            copy(this->storage.get<small>().str,str);
        }
    }
    constexpr string(size_t value)  : len(Itoa(value, storage.get<small>().str, sizeof(storage.get<small>().str))) {
        storage.get<small>().str[23] = '\0';
        
    }
    constexpr string(int value)     : len(Itoa(value, storage.get<small>().str, sizeof(storage.get<small>().str))) {
        storage.get<small>().str[23] = '\0';
    }
    constexpr string(float value) {
        this->len = char_trait::len(storage.get<small>().str);
        Ftoa(value, storage.get<small>().str, 3);
        storage.get<small>().str[23] = '\0';
    }
    // constexpr string& append(const char* strin)
    // {
    //     this->len += char_trait::len(strin);
    //     isLarge();
    //     if (_isLarge) {
    //         storage.get<large>().str = make_uniquearr<char[]>(storage.get<large>().cap);

    //     } 
    //     return *this;
    // }
    string& operator=(const char* str)  {
        this->len = char_trait::len(str);
        if (len > sizeof(storage)) {
            if (len > storage.get<large>().cap)
            { 
                if (!storage.get<large>().str.isNull()) {
                    printf("add new notnull ");
                    storage.get<large>().str.remove();
                    storage.set<large>({
                        make_uniquearr<char[]>(storage.get<large>().cap),
                        len
                    });
                }
                if (storage.get<large>().str.isNull())
                {
                    printf("add new ");
                    storage.set<large>({
                        make_uniquearr<char[]>(storage.get<large>().cap),
                        len
                    });
                }
                copy(this->storage.get<large>().str,str);
                // storage.get<large>().str[len] = '\0'; 
            } else {
                copy(this->storage.get<large>().str,str);
                // storage.get<large>().str[len] = '\0'; 
            }
        } else { 
            copy(this->storage.get<small>().str,str); 
        }
        return *this;
    }
    string& operator=(size_t& value)    {
        this->len = Itoa(value,this->storage.get<small>().str,sizeof(storage)); 
        return *this;
    }
    string& operator=(size_t&& value)   {
        this->len = Itoa(value,this->storage.get<small>().str,sizeof(storage)); 
        return *this;
    }

    ~string(){
    }

    constexpr size_t size() const   { return len;}
    constexpr size_t cap()  const   { 
        if (storage.type_index == 2) {return storage.get<large>().cap;}
        else {return len;}
    }
    constexpr const char* data() const  {
        if (storage.type_index == 1)  {return storage.get<small>().str;}
        else {return storage.get<large>().str;} 
    }
    constexpr operator const char*() const { return storage.get<small>().str;}
    private:
        struct small {
            char str[23];
        };
        struct large {
            uptr<char[]> str;
            size_t cap;
            // Add this constructor
            large(uptr<char[]>&& s, size_t c) 
                : str(static_cast<uptr<char[]>&&>(s)), cap(c) {}

            // Important: If you add a constructor, you should also 
            // define the move constructor for the struct itself
            large(large&& other) noexcept 
                : str(static_cast<uptr<char[]>&&>(other.str)), cap(other.cap) {}
        };
    variant<small,large> storage;
    size_t len;
};

int main()
{
    string s;
    string fl (1232);
    printf("%s %zu\n", fl.data() ,fl.cap());
    s = "sssssssssssssssssssssssssssssssss";
    printf("%s %zu\n", s.data() ,s.cap());
    s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
    printf("%s %zu\n", s.data() ,s.cap());
    s = "wwwwwwwwwwwwwwwwwwwwww";
    printf("%s %zu\n", s.data() ,s.cap());
    s = "aaa";
    printf("%s %zu\n", s.data() ,s.cap());
    return 0; 
}