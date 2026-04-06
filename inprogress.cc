#include <cstdio>

// class DetailedException : public std::exception {
// public:
//     DetailedException(std::string msg, 
//                       std::source_location loc = std::source_location::current())
//         : message(std::move(msg)), location(loc) {}

//     const char* what() const noexcept override {
//         return message.c_str();
//     }

//     const std::source_location& where() const {
//         return location;
//     }

// private:
//     std::string message;
//     std::source_location location;
// };

// template<typename T>
// class uptr
// {
//     T* ptr;
//     public:
//     explicit uptr(T* p = nullptr) :ptr(p){}
//     ~uptr() {delete ptr; ptr = nullptr;}

//     uptr(const uptr&) = delete;
//     uptr& operator=(const uptr&) = delete;

//     uptr(uptr&& other) noexcept {
//     ptr = other.ptr;     // Steal the pointer
//     other.ptr = nullptr; // Leave the old one empty
//     }

//     // 4. Move Assignment
//     uptr& operator=(uptr&& other) noexcept {
//         if (this != &other) {
//             delete ptr;      // Clean up our current pointer first!
//             ptr = other.ptr;
//             other.ptr = nullptr;
//         }
//         return *this;
//     }
//     T& operator*() const { return *ptr; }
//     T* operator->() const { return ptr; }

//     // Helper to get the raw pointer if needed
//     T* get() const { return ptr; }

//     // Release ownership without deleting
//     T* release() {
//         T* temp = ptr;
//         ptr = nullptr;
//         return temp;
//     }
// };
// template <typename T>
// class uptr<T[]> { 
//     T* ptr;
// public:
//     explicit uptr(T* p = nullptr) : ptr(p) {}

//     ~uptr() { delete[] ptr; } 

//     T& operator[](size_t index) {
//         return ptr[index]; 
//     }
//     const T& operator[](size_t index) const {
//         return ptr[index];
//     }

//     uptr& operator=(const uptr&) = delete;


//     // 4. Move Assignment
//     uptr& operator=(uptr&& other) noexcept {
//         if (this != &other) {
//             delete[] ptr;      // Clean up our current pointer first!
//             ptr = other.ptr;
//             other.ptr = nullptr;
//         }
//         return *this;
//     }
//     operator char*() {return ptr;}
//     operator const char*() const {return ptr;}
//     bool isNull() {return ptr == nullptr;}
//     void remove() {delete[] ptr;}
// };

// template <typename T, typename... Args>
// uptr<T> make_unique(Args&&... args) {
//     // We use placement new logic or just a straight new call
//     return uptr<T>(new T(static_cast<Args&&>(args)...));
// }

// template <typename T>
// uptr<T> make_uniquearr(size_t size) {
//     // For a char array, we usually want a raw allocation.
//     // 'new char[size]()' would zero-initialize. 
//     // 'new char[size]' leaves it uninitialized (faster).
//     return uptr<T>(new typename removeArray<T>::type[size]);
// }
// struct char_trait
// {
//     using char_t = char;
//     using int_t  = int;
//     static constexpr const char_t num[] = "0123456789"; 
//     static constexpr void assign(char_t& lhs, char_t rhs) {lhs = rhs;}
//     static constexpr bool eq(char_t lhs, char_t rhs) {return lhs == rhs;}
//     static constexpr bool lt(char_t lhs, char_t rhs) {return lhs < rhs;}

//     static constexpr size_t len(const char_t* str) {
//         size_t size = 0;
//         for (;*str++ != '\0';size++){};
//         return size;    
//     };

//     static char_t* copy(char_t* to, const char_t* src) {
//         for (size_t i = 0; src[i] != '\0'; i++) { to[i] = src[i];}
//         return to;
//     }

//     static constexpr int cmp(const char_t* lhs, const char_t* rhs, size_t n) {
//         for (size_t i = 0; i < n; i++) {
//             if (lt(lhs[i],rhs[i])) {return -1;}
//             if (lt(lhs[i],rhs[i])) {return 1;}
//         }
//         return 0;
//     }

//     static constexpr const char_t* find(const char_t* src,size_t n ,const char_t& cmp) {
//         for (size_t i = 0; i < n; i++) {
//             if (eq(src[i],cmp)) {return &src[i];}
//         };
//         return nullptr;
//     }

//     static constexpr int_t eof() {return -1;}
// };

// constexpr size_t Itoa(size_t value, char* buf, size_t size) {
//     if (buf == nullptr || size < 2) return -1;

//     char* p = buf + size - 1; 
//     *p = '\0'; // Set null terminator at the end

//     size_t uvalue = (value < 0) ? -value : value;
//     bool negative = (value < 0);

//     // Process digits from right to left
//     do {
//         if (p <= buf) return -1; // Buffer too small
//         *--p = char_trait::num[uvalue % 10];
//         uvalue /= 10;
//     } while (uvalue > 0);

//     if (negative) {
//         if (p <= buf) return -1;
//         *--p = '-';
//     }

//     // Shift the string to the front of the buffer
//     int len = (buf + size - 1) - p;
//     for (int i = 0; i <= len; i++) {
//         buf[i] = p[i];
//     }

//     return len;
// }

// void Ftoa(double value, char* buf, int precision) {
//     char temp[16];

//     // 1. Handle negative numbers
//     if (value < 0) {
//         *buf++ = '-';
//         value = -value;
//     }
//     // 2. Extract integer part
//     unsigned int ipart = static_cast<unsigned int>(value);
//     temp[Itoa(ipart, temp, sizeof(temp))] = '.';
//     int it = 0;
//     for (;temp[it] != '.'; it++) {
//         *buf++ = temp[it];
//         temp[it]= 0;
//     }
//     *buf++ = temp[it];
//     // 3. Extract fractional part
//     float fpart = (value - static_cast<float>(ipart));
//     it = 0;
//     for (; it < precision; it++)
//     {
//         fpart *= 10.00005f;
//         unsigned int f = (unsigned int)fpart;
//         temp[it] = char_trait::num[f];
//         fpart -= f;
//     };
//     temp[it] = '\0';
//     for (int i = 0; i < precision + 1; i++) {
//         if (temp[i] ==  '\0') {*buf++ = '\0'; break;}
//         *buf++ = temp[i];
//     }
// }

// class string {
//     void copy(char* dest,const char* src)
//     {
//         size_t i = 0;
//         for(;src[i] == '\0';i++) {dest[i] = src[i];}
//         dest[i] = '\0';
//     }
//     public:
//     string() : len(0) {}
//     constexpr string(const char* str) {
//         len = char_trait::len(str);
//         if (len > sizeof(storage))
//         {
//             printf("str construct \n");
//             storage.large.cap = len;
//             storage.large.str = make_uniquearr<char[]>(storage.large.cap);
//             copy(storage.large.str,  str);
//             storage.large.str[storage.large.cap] = '\0';
//         } else
//         {
//             copy(this->storage.small.str,str);
//         }
//     }
//     // string(const char* str,size_t len) : len(len) {
//     //     if (len > sizeof(storage)) {
//     //         printf("str construct2 \n");
//     //         storage.set<large>({
//     //             make_uniquearr<char[]>(storage.get<large>().cap),
//     //             len
//     //         });
//     //         copy(this->storage.get<large>().str,str);
//     //         storage.get<large>().str[storage.get<large>().cap] = '\0';
//     //     } else {
//     //         storage.set<small>({});
//     //         copy(this->storage.get<small>().str,str);
//     //     }
//     // }
//     constexpr string(size_t value) {
//         len = Itoa(value, storage.small.str, sizeof(storage.small.str));
//         storage.small.str[23] = '\0';
        
//     }
//     constexpr string(int value) {
//         len = Itoa(value, storage.small.str, sizeof(storage.small.str));
//         storage.small.str[23] = '\0';
//     }
//     constexpr string(float value) {
//         Ftoa(value, storage.small.str, 3);
//         storage.small.str[23] = '\0';
//     }
//     // constexpr string& append(const char* strin)
//     // {
//     //     this->len += char_trait::len(strin);
//     //     isLarge();
//     //     if (_isLarge) {
//     //         storage.get<large>().str = make_uniquearr<char[]>(storage.get<large>().cap);

//     //     } 
//     //     return *this;
//     // }
//     string& operator=(const char* str)  {
//         this->len = char_trait::len(str);
//         if (len > sizeof(storage)) {
//             if (len > storage.large.cap)
//             { 
//                 if (!storage.large.str.isNull()) {
//                     printf("add new notnull ");
//                     storage.large.str.remove();
//                     storage.large.cap = len;
//                     storage.large.str = make_uniquearr<char[]>(storage.large.cap);
//                     // storage.set<large> ({
//                     //     len,
//                     //     make_uniquearr<char[]>(storage.get<large>().cap)
//                     // });
//                 }
//                 if (storage.large.str.isNull())
//                 {
//                     printf("add new ");
//                     storage.large.cap = len;
//                     storage.large.str = make_uniquearr<char[]>(storage.large.cap);
//                 }
//                 copy(this->storage.large.str,str);
//                 storage.large.str[len] = '\0'; 
//             } else {
//                 copy(this->storage.large.str,str);
//                 storage.large.str[len] = '\0'; 
//             }
//         } else {
//             copy(this->storage.small.str,str); 
//         }
//         return *this;
//     }
//     string& operator=(size_t& value)    {
//         this->len = Itoa(value,this->storage.small.str,sizeof(storage)); 
//         return *this;
//     }
//     string& operator=(size_t&& value)   {
//         this->len = Itoa(value,this->storage.small.str,sizeof(storage)); 
//         return *this;
//     }

//     constexpr size_t size() const   { return len;}
//     constexpr size_t cap()  const   { 
//         if (isLarge) {return storage.large.cap;}
//         else {return len;}
//     }
//     constexpr const char* data() const  {
//         if (isLarge)  {return storage.small.str;}
//         else {return storage.large.str;} 
//     }
//     constexpr operator const char*() const { return storage.small.str;}
//     private:
//         struct sm {
//             char str[23];
//         };
//         struct l {
//             size_t cap;
//             uptr<char[]> str{};
//             ~l(){~str();}
            
//         };
//     union _ {
//         sm small{};
//         l large;
//     } storage;
//     bool isLarge;
//     size_t len;
// };
class string {
    struct store {
        enum {
            large = 0,
            small = 1
        }tag;
        unsigned int len;
        union {
            struct {
                char* str;
                size_t cap;
            }Large;
            struct {char str[23];} Small;
        };
        ~store(){
            if(tag == large) {
                if (Large.str != nullptr)
                {
                    printf("delete \n");
                    delete[] Large.str;
                    Large.str = nullptr;
                }
            }
        };
    }storage;
    unsigned int getLen(const char* str) 
    {
        unsigned int len = 0;
        for (;str[len] != '\0';len++){}
        return len;
    }
    void copy(char* to,const char* from) 
    {
        for (unsigned int i = 0;from[i] != '\0';i++){
            to[i] = from[i];
        }
    }
    public:
    string() : storage({.len = 0}) {}
    string(const char* inStr) {
        storage.len = getLen(inStr);
        if (storage.len > sizeof(storage.Small.str)) {
            storage.tag = store::large;
            storage.Large.cap = storage.len;
            storage.Large.str = new char[storage.Large.cap]();
            copy(storage.Large.str, inStr);
            storage.Large.str[storage.len] = '\0';
        } else {
            storage.tag = store::small;
            copy(storage.Small.str, inStr);
            storage.Small.str[storage.len] = '\0';
        }
    }
    string& operator =(const char* inStr) {
        storage.len = getLen(inStr);
        if (storage.len > sizeof(storage.Small.str)) {
            if (storage.Large.str != nullptr && storage.Large.cap < storage.len) {
                printf("new again \n");
                delete [] storage.Large.str;
                storage.Large.cap = storage.len;
                storage.Large.str = new char[storage.Large.cap];
            } else {
                printf("new \n");
                storage.Large.cap = storage.len;
                storage.Large.str = new char[storage.Large.cap];
            }
            copy(storage.Large.str, inStr);
            storage.Large.str[storage.len] = '\0';
        } else {
            if (storage.Large.str != nullptr && storage.tag == store::large) {
                printf("delete ptr \n");
                delete [] storage.Large.str;
                storage.Large.str = nullptr;
            }
            storage.tag = store::small;
            copy(storage.Small.str, inStr);
            storage.Small.str[storage.len] = '\0';
        }
        return *this;
    }
    const char* data() {
        if (storage.tag == store::small) {
            return storage.Small.str;
        } else {
            return storage.Large.str;
        }
    }
};

int main()
{
    // try 
    // {
        
        // va v;
        // v.tag = va::s;
        // const char* ss = "hello world trash";
        // int is = 0;
        // for (; ss[is] != '\0'; is++) {
        //     v.small.str[is] = ss[is];
        // }
        // v.small.str[is] = '\0';
        // printf("%s %d\n", v.small.str ,v.tag);
        // v.tag = va::l;
        // v.large.str = new char[32]();
        // v.large.cap = 32;
        // is = 0;
        // for (; ss[is] != '\0'; is++) {
        //     v.large.str[is] = ss[is];
        // }
        // v.large.str[is] = ss[is];
        // printf("%s %d\n", v.large.str ,v.tag);
        // const char* sw = "world hello";
        // is = 0;
        // for (; sw[is] != '\0'; is++) {
        //     v.large.str[is] = sw[is];
        // }
        // v.large.str[is] = ss[is];
        // printf("%s %d\n", v.large.str ,v.tag);

        string s;
        s = "hello world numbers 3200";
        printf("%s \n", s.data());
        s = "hello again";
        printf("%s \n", s.data());
        s = "hello again from world number 3200";
        printf("%s \n", s.data());
        s= "small";
        printf("%s \n", s.data());
        s= "again";
        printf("%s \n", s.data());
        // s = "sssssssssssssssssssssssssssssssss";
        // printf("%s %zu\n", s.data() ,s.cap());
        // s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
        // printf("%s %zu\n", s.data() ,s.cap());
        // s = "wwwwwwwwwwwwwwwwwwwwww";
        // printf("%s %zu\n", s.data() ,s.cap());
        // s = "aaa";
        // printf("%s %zu\n", s.data() ,s.cap());
        
    // } catch (DetailedException e)
    // {
    //     printf("%s %d %d  \n",e.what(),e.where().line(),e.where().column());
    // }
    return 0; 
}