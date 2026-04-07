#include <cstddef>
#include <cstdio>
#include <cstdlib>
// class DetailedException : std::exception
// {
//     public:
//     std::source_location where() {
//         return std::source_location::current();
//     }
// };


struct variantBuffer{char buffer;};
inline void* operator new (size_t size,variantBuffer* p) {return p;}

template<typename... Types>
struct variant{
    template<size_t T,size_t... Rest>
    struct getMax {
        static constexpr size_t value = T > getMax<Rest...>::value 
        ? T : getMax<Rest...>::value;
    };

    template<size_t T>
    struct getMax<T> { static constexpr size_t value = T; };
    
    template <typename T, typename... Type> struct match;
    
    // Match found!
    template <typename T, typename... Rest>
    struct match<T, T, Rest...> {static constexpr int value = 0;};
    
    // Not a match, keep looking...
    template <typename T, typename U, typename... Rest>
    struct match<T, U, Rest...> { static constexpr int value = 1 + match<T, Rest...>::value;};

    static constexpr size_t data_size   = getMax<sizeof(Types)...>::value;
    static constexpr size_t data_align  = getMax<alignof(Types)...>::value;

    alignas(data_align) variantBuffer buffer[data_size];
    char type_index = -1;

    template <size_t Index, typename T, typename... Rest>
    void destroy_at_index(int target) {
        if (Index == target) {
            static_cast<T*>(static_cast<void*>(buffer))->~T();
        } else if constexpr (sizeof...(Rest) > 0) {
            destroy_at_index<Index + 1, Rest...>(target);
        }
    }
    void destroy_current() {
        if (type_index == -1) return;
        
        // Start the recursive search for the correct type to destroy
        printf("destroy current \n");
        destroy_at_index<0, Types...>(type_index);
        
        type_index = -1; 
    }
    template <typename T>
    void set(T&& value) {
        // 1. Destroy whatever was there before
        if (type_index != -1) destroy_current();
        
        // T* ptr = static_cast<T*>(static_cast<void*>(buffer));
        // *ptr = static_cast<T&&>(value);
        // 2. "Placement New" - Construct T exactly at the address of our buffer
        new (buffer) T(static_cast<T&&>(value));
        
        // 3. Update the index so we know how to delete it later
        type_index = match<T,Types...>::value; 
    }
    template <typename T>
    inline T& get(int e = __LINE__) {
        // 1. Get the compile-time index of type T
        constexpr int target_index = match<T, Types...>::value;
        
        // 2. Runtime Check: Does it match what's actually in the buffer?
        if (type_index != target_index) {
            // In a no-stdlib environment, you might log an error or assert
            // instead of throwing an exception.
            printf( "%d  Invalid variant access! \n", e);
            std::exit(1);
        }
        // 3. The Magic Trick: Reinterpret Cast
        // We treat the address of 'buffer' as the address of an object of type T.
        return *static_cast<T*>(static_cast<void*>(buffer));
    }
    template <typename T>
    const T& get() const {
        constexpr int target_index = match<T, Types...>::value;
        if (type_index != target_index) printf("Invalid access");

        return *static_cast<T*>(static_cast<void*>(buffer));
    }
    ~variant() { destroy_current();}
};

// class string {
//     struct store {
//         size_t len;
//         enum : bool {
//             large,
//             small
//         }tag;
//         enum : bool {
//             yes,
//             no
//         }allocated;
//         union {
//             struct {
//                 char* str;
//                 size_t cap;
//             }Large;
//             struct {char str[24];} Small;
//         }type;
        
//         void newLarge(size_t newLen) {
//             allocated = yes;
//             type.Large.cap = newLen;
//             type.Large.str = new char[type.Large.cap];
//         }
//         void delLarge() {
//             allocated = no;
//             delete [] type.Large.str;
//             type.Large.str = nullptr;
//         }
//     }storage;
//     size_t getLen(const char* str) 
//     {
//         size_t len = 0;
//         for (;str[len] != '\0';len++){}
//         return len;
//     }
//     void copy(char* to,const char* from) 
//     {
//         for (size_t i = 0;from[i] != '\0';i++){
//             to[i] = from[i];
//         }
//     }
//     public:
//     explicit constexpr string() : storage({.len = 0,.tag = storage.small,.allocated = store::no}) {
//         storage.type.Small.str[0] = '\0';
//     }
//     constexpr string(const char* inStr) {
//         storage.len = getLen(inStr);
//         if (storage.len > sizeof(storage.type)) {
//             storage.tag = store::large;
//             storage.type.Large.cap = storage.len;
//             storage.newLarge(storage.len);
//             copy(storage.type.Large.str, inStr);
//             storage.type.Large.str[storage.len] = '\0';
//         } else {
//             storage.tag = store::small;
//             copy(storage.type.Small.str, inStr);
//             storage.type.Small.str[storage.len] = '\0';
//         }
//     }
//     // constexpr string(string&& other) {
//     //     this->storage = other.storage;
//     //     //if (other.storage.allocated == storage.yes) { other.storage.delLarge();}
//     // }
//     // constexpr string(const string& other) {
//     //     if (*this == other) {return;}
//     //     this->storage = other.storage;
//     // }
//     constexpr string& operator =(const char* inStr) {
//         size_t newLen = getLen(inStr);
        
//         if (newLen > 22) { // 22 + 1 for null terminator = 23 (your Small size)
//             if (storage.tag == store::large) {
//                 // Already large, check if we can reuse the existing heap buffer
//                     if (storage.type.Large.cap < newLen) {
//                         storage.delLarge();
//                         storage.newLarge(newLen); // Should allocate cap + 1
//                     }
//                     } else {
//                         // Switching from small to large
//                         storage.tag = store::large;
//                         storage.type.Large.cap = newLen;
//                         storage.newLarge(newLen);
//                     }
//                 copy(storage.type.Large.str, inStr);
//                 storage.type.Large.str[newLen] = '\0';
//             } else {
//                 // Destination is small
//                 if (storage.allocated == store::yes) {
//                     storage.delLarge();
//                 }
//                 storage.tag = store::small;
//                 copy(storage.type.Small.str, inStr);
//                 storage.type.Small.str[newLen] = '\0';
//             }
//         storage.len = newLen;
//         return *this;
//     }
//     const char* data() const {
//         if (storage.tag == store::small) {
//             return storage.type.Small.str;
//         } else {
//             return storage.type.Large.str;
//         }
//     }
//     ~string() {
//         if(storage.tag == storage.large) {
//             if (storage.allocated == storage.yes)
//             {
//                 printf("delete \n");
//                 delete[] storage.type.Large.str;
//                 storage.type.Large.str = nullptr;
//             }
//         }
//     }
// };

class string {
    struct small {char str[23]; small(){str[0] = '\0';}};
    struct large {
        char* str;
        size_t cap;
        large(){str = nullptr;}
        // void newStr() {this->str = new char[this->cap];}
        ~large() {
            if(str != nullptr) {
                printf("delete ptr \n");
                delete [] str;
            }
        }
    };
    bool autoShrink : 1 = true;
    size_t len : 63;
    variant<small,large> storage;
    constexpr size_t getLen(const char* str) 
    {
        size_t inlen = 0;
        for (;str[inlen] != '\0';inlen++){}
        return inlen;
    }
    constexpr void copy(char* to,const char* from) 
    {
        for (size_t i = 0;from[i] != '\0';i++){
            to[i] = from[i];
        }
    }
    public:
    explicit string() : len(0) {}

    string(const char* instr) {
        this->len = getLen(instr);
        if (len > 22) {
            storage.set(large{});
            // printf("storage index %d", storage.type_index);
            storage.get<large>().cap = len;
            storage.get<large>().str = new char[storage.get<large>().cap]();
            //storage.get<large>().newStr();
            copy(storage.get<large>().str, instr);
            storage.get<large>().str[len] = '\0';
        } else {
            storage.set(small{});
            copy(storage.get<small>().str,instr);
            storage.get<small>().str[len] = '\0';
        }
    }

    constexpr string& operator =(const char* inStr) {
        size_t newLen = getLen(inStr);
            if (newLen > 22 || (storage.type_index == 1 && autoShrink)) 
            { // 22 + 1 for null terminator = 23 (your Small size)
                if (storage.type_index == 1) 
                    {
                        // Already large, check if we can reuse the existing heap buffer
                        if (storage.get<large>().cap < newLen) {
                            delete[] storage.get<large>().str;
                            storage.get<large>().cap = newLen;
                            storage.get<large>().str = new char[newLen]();
                        }
                    } else {
                        // Switching from small to large
                        storage.set(large{});
                        storage.get<large>().cap = newLen;
                        storage.get<large>().str = new char[newLen]();
                    }
                copy(storage.get<large>().str, inStr);
                storage.get<large>().str[newLen] = '\0';
            } else {
                // Destination is small
                // storage.type_index = 0;
                if (storage.type_index != 0) storage.set(small{});
                copy(storage.get<small>().str, inStr);
                storage.get<small>().str[newLen] = '\0';
            }
        len = newLen;
        return *this;
    }

    constexpr string& reserve(size_t newSize) {
        if (newSize < 22) {return *this;}
        
        if (storage.type_index == 1) 
        {
            const char* temp = storage.get<large>().str;
            storage.get<large>().cap += newSize;
            delete [] storage.get<large>().str;
            storage.get<large>().str = new char[storage.get<large>().cap]();
            for (int i = 0;temp[i] != '\0';i++) {storage.get<large>().str[i] = temp[i];} 
        } else {
            storage.set(large{});
            storage.get<large>().cap += newSize;
            storage.get<large>().str = new char[newSize]();
        }

        return *this;
    }

    constexpr string& append (const char* in)
    {
        size_t inlen = getLen(in);
        if ((len + inlen) > 22 || (storage.type_index == 1 && autoShrink))
        {
            if (storage.type_index == 1)
            {
                if (storage.get<large>().cap < (len + inlen)) {
                    reserve(inlen);
                }
                
                for (int i = 0;in[i] != '\0';i++) {storage.get<large>().str[(len + i)] = in[i];}
                storage.get<large>().str[len + inlen] = '\0';
            }
        } else {
            for (int i = 0;in[i] != '\0';i++) {storage.get<small>().str[len + i] = in[i];}
            storage.get<small>().str[len + inlen] = '\0';
        }
        len += inlen;
        return *this;
    }
    const char* data() {
        return storage.type_index == 1 ? 
        storage.get<large>().str:
        storage.get<small>().str;
    }
    size_t getCap() {
        return storage.type_index == 1 
        ? storage.get<large>().cap
        : sizeof(small) ;
    }
    string& Shrink(bool b = false) {autoShrink = b; return *this;}
    int getindex() {return storage.type_index;}
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
        // struct smallstr {char str[24];};
        // variant<smallstr,string> var;
        // var.set(smallstr{});
        // const char* test = "variant test";
        // int i = 0;
        // for (;test[i] != '\0';i++) {var.get<smallstr>().str[i] = test[i];}
        // printf("%s \n", var.get<smallstr>().str);
        // var.set<string>("hello variant number 4000");
        // printf("%s \n", var.get<string>().data());
        // variant<int,float> test;
        // test.set(12);
        // printf("%d %d \n", test.get<int>() , test.type_index);
        // test.set(12.04f);
        // printf("%f %d \n" ,test.get<float>(),test.type_index);
        string s ("hello world before reserve");
        printf("%s %zu \n",s.data(),s.getCap());
        //s.reserve(50);
        s.append(" after append");
        printf("%s %zu \n",s.data(),s.getCap());
        s = ("hello world from world number");
        printf("%s %zu \n",s.data(),s.getCap());
        s = "hello world numbers 3200";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "hello again from world number 3200";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "small";
        printf("%s %zu \n",s.data(),s.getCap());
        s.append(" append");
        printf("%s %zu \n",s.data(),s.getCap());
        s = "again";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "hello again from world number 4200";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "sssssssssssssssssssssssssssssssss";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
        printf("%s %zu \n",s.data(),s.getCap());
        s = "wwwwwwwwwwwwwwwwwwwwww";
        printf("%s %zu \n",s.data(),s.getCap());
        // s = "aaa";
        // printf("%s \n", s.data());
        
    // } catch (DetailedException e)
    // {
    //     printf("%d  \n",e.where().line());
    // }
    return 0; 
}