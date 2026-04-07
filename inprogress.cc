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
    template<typename T> struct removeRef {using type = T;};
    template<typename T> struct removeRef<T&> {using type = T;};
    template<typename T> struct removeRef<T&&> {using type = T;};
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
    variant& set(T&& value) {
        // 1. Destroy whatever was there before
        if (type_index != -1) destroy_current();
        
        // T* ptr = static_cast<T*>(static_cast<void*>(buffer));
        // *ptr = static_cast<T&&>(value);
        // 2. "Placement New" - Construct T exactly at the address of our buffer
        new (buffer) T(static_cast<T&&>(value));
        
        // 3. Update the index so we know how to delete it later
        type_index = match<T,Types...>::value; 
        return *this;
    }
    template <typename T>
    inline T& get() {
        // 1. Get the compile-time index of type T
        constexpr int target_index = match<T, Types...>::value;
        
        // 2. Runtime Check: Does it match what's actually in the buffer?
        if (type_index != target_index) {
            // In a no-stdlib environment, you might log an error or assert
            // instead of throwing an exception.
            printf( "Invalid variant access! \n");
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
        
        return *static_cast<const T*>(static_cast<const void*>(buffer));
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
    using char_ptr = char*;
    struct small {char str[23]; 
        small(){str[0] = '\0';}
        small(const char* other) {
            str[copy(str, other)] = '\0';
        }
    };
    struct large {
        char* str;
        size_t cap;
        large() : str(nullptr) , cap(0){}
        large(size_t in, const char* other) : cap(in) {
            str = new char[cap + 1]();
            if (other) {str[copy(str, other)] = '\0';}
        }
        ~large() {
            if (str != nullptr) {
                delete[] str;
                str = nullptr;
            }
        }
        large(large&& other) noexcept : str(other.str), cap(other.cap) {
            other.str = nullptr;
        }

        large(const large&) = delete;
    };

    bool autoShrink : 1;
    size_t len : 63;
    variant<small,large> storage;
    static constexpr size_t getLen(const char* str) 
    {
        size_t inlen = 0;
        for (;str[inlen] != '\0';inlen++){}
        return inlen;
    }
    static size_t copy(char* to,const char* from) 
    {
        size_t i = 0;
        for (;from[i] != '\0';i++){
            to[i] = from[i];
        }
        return i;
    }
    public:

    explicit string() : autoShrink(true),len(0) {}

    string(const char* instr) : autoShrink(true) {
        len = getLen(instr);
        if (len > 22) {
            storage.set(large{len, instr});
        } else {
            storage.set(small{instr});
        }
    }

    string(const string& other) : autoShrink(other.autoShrink),len(other.len)  {
        if (other.storage.type_index == 1) { // Large
            const large& o_large = other.storage.get<large>();
            storage.set(large{o_large.cap, o_large.str});
        } else { // Small
            storage.set(small{other.storage.get<small>().str});
        }
    }

    string& operator=(const char* inStr) {
        size_t newLen = getLen(inStr);
        
        // Logic: Should we stay/become Large?
        if (newLen > 22 || (storage.type_index == 1 && !autoShrink)) {
            if (storage.type_index == 1) {
                large& l = storage.get<large>();
                if (l.cap < newLen) {
                    delete[] l.str;
                    l.str = new char[newLen + 1]();
                    l.cap = newLen;
                }
                l.str[copy(l.str, inStr)] = '\0';
            } else {
                storage.set(large{newLen, inStr});
            }
        } else {
            // Stay/become Small
            if (storage.type_index == 1) {storage.set(small{inStr});}
            else {storage.get<small>().str[copy(storage.get<small>().str, inStr)] = '\0';}
        }
        len = newLen;
        return *this;
    }

    constexpr string& reserve(size_t newSize) {
        if ((len + newSize) > 22) 
        {
            if (storage.type_index == 1) 
            {
                large& l = storage.get<large>();
                char* temp = new char[len]();
                temp[copy(temp, l.str)] = '\0';
                l.cap += newSize;
                delete [] l.str;
                l.str = new char[l.cap]();
                temp[copy(l.str, temp)] = '\0'; 
            } else {
                large& l = storage.set(large{}).get<large>();
                l.str = new char[newSize]();
            }
            return *this;
        } else {
            return *this;
        }
    }

    string& append(const char* in) {
        size_t inlen = getLen(in);
        size_t totalLen = len + inlen;

        if (totalLen > 22) {
            if (storage.type_index == 0) { // Upgrade Small to Large
                small old = storage.get<small>();
                // Current storage is now large, copy the 'append' part
                char* dest = storage.set(large{totalLen, old.str})
                .get<large>().str;
                
                for (size_t i = 0; i < inlen; i++) { dest[len + i] = in[i]; }
                dest[totalLen] = '\0';
            } else { // Already Large
                large& l = storage.get<large>();
                if (l.cap < totalLen) {
                    char* newStr = new char[totalLen + 1]();
                    copy(newStr, l.str);
                    delete[] l.str;
                    l.str = newStr;
                    l.cap = totalLen;
                }
                for (size_t i = 0; i < inlen; i++) { l.str[len + i] = in[i]; }
                l.str[totalLen] = '\0';
            }
        } else {
            // Stay Small
            char* dest = storage.get<small>().str;
            for (size_t i = 0; i < inlen; i++) { dest[len + i] = in[i]; }
            dest[totalLen] = '\0';
        }
        len = totalLen;
        return *this;
    }

    const char* data() {
        return storage.type_index == 1 ? 
        storage.get<large>().str:
        storage.get<small>().str;
    }
    size_t getCap() {
        return storage.type_index == 1 ? 
        storage.get<large>().cap : 
        sizeof(small) ;
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
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        //s.reserve(50);
        s.append(" after append");
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        string c (s);
        c.append(" copy");
        printf("%s %zu %d \n",c.data(),c.getCap(),c.getindex());
        s = ("hello world from world number");
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "hello world numbers 3200";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "hello again from world number 3200";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "small";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s.append(" append");
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "again";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "hello again from world number 4200";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "sssssssssssssssssssssssssssssssss";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        s = "wwwwwwwwwwwwwwwwwwwwww";
        printf("%s %zu %d \n",s.data(),s.getCap(),s.getindex());
        // s = "aaa";
        // printf("%s \n", s.data());
        
    // } catch (DetailedException e)
    // {
    //     printf("%d  \n",e.where().line());
    // }
    return 0; 
}