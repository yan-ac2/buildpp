#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <source_location>
#include <iostream>

class MyException : public std::runtime_error {
    std::string _file;
    int _line;
    std::string _function;

public:
    MyException(const std::string& msg, 
                const std::source_location& loc = std::source_location::current())
        : std::runtime_error(msg),
          _file(loc.file_name()),
          _line(loc.line()),
          _function(loc.function_name()) {}

    void print() const {
        std::cout << "Error at " << _file << ":" << _line 
                  << " in " << _function << ": " << std::runtime_error::what() << "\n";
    }
};

template <class Ty, Ty Val>
struct integral_constant {
    static constexpr Ty value = Val;
    using value_type = Ty;
    using type       = integral_constant;
    constexpr operator value_type() const noexcept  {return value;}
    constexpr value_type operator()() const noexcept {return value;}
};
template <bool Val> using bool_constant = integral_constant<bool, Val>;
using false_t = bool_constant<false>;
using true_t = bool_constant<true>;
template<class T,class U> struct is_same {static  constexpr char value = 0;};
template<class T> struct is_same<T,T>    {static  constexpr char value = 1;};


struct variantBuffer{char buffer;};
inline void* operator new (size_t size,variantBuffer* p) {return p;}

// template<typename... Types>
// struct variant{
//     template <typename T, typename... Type> struct match;
//     // Match found!
//     template <typename T, typename... Rest>
//     struct match<T, T, Rest...> {static constexpr int value = 0;};
    
//     // Not a match, keep looking...
//     template <typename T, typename U, typename... Rest>
//     struct match<T, U, Rest...> { static constexpr int value = 1 + match<T, Rest...>::value;};


//     alignas(8) variantBuffer buffer[32];
//     char type_index = -1;

//     template <size_t Index, typename T, typename... Rest>
//     void destroy_at_index(int target) {
//         if (Index == target) {
//             static_cast<T*>(static_cast<void*>(buffer))->~T();
//         } else if constexpr (sizeof...(Rest) > 0) {
//             destroy_at_index<Index + 1, Rest...>(target);
//         }
//     }
//     void destroy_current() {
//         if (type_index == -1) return;
        
//         // Start the recursive search for the correct type to destroy
//         printf("destroy current \n");
//         destroy_at_index<0, Types...>(type_index);
        
//         type_index = -1; 
//     }
//     template <typename T>
//     variant& set(T&& value) {
//         // 1. Destroy whatever was there before
//         if (type_index != -1) destroy_current();
        
//         // T* ptr = static_cast<T*>(static_cast<void*>(buffer));
//         // *ptr = static_cast<T&&>(value);
//         // 2. "Placement New" - Construct T exactly at the address of our buffer
//         new (buffer) T(static_cast<T&&>(value));
        
//         // 3. Update the index so we know how to delete it later
//         type_index = match<T,Types...>::value; 
//         return *this;
//     }
//     template <typename T>
//     inline T& get() {
//         // 1. Get the compile-time index of type T
//         constexpr int target_index = match<T, Types...>::value;
        
//         // 2. Runtime Check: Does it match what's actually in the buffer?
//         if (type_index != target_index) {
//             // In a no-stdlib environment, you might log an error or assert
//             // instead of throwing an exception.
//             printf( "Invalid variant access! \n");
//             std::exit(1);
//         }
//         // 3. The Magic Trick: Reinterpret Cast
//         // We treat the address of 'buffer' as the address of an object of type T.
//         return *static_cast<T*>(static_cast<void*>(buffer));
//     }
//     template <typename T>
//     const T& get() const {
//         constexpr int target_index = match<T, Types...>::value;
//         if (type_index != target_index) printf("Invalid access");
        
//         return *static_cast<const T*>(static_cast<const void*>(buffer));
//     }
//     ~variant() { destroy_current();}
// };


class string {
    struct small {char str[22]; 
        small(){str[0] = '\0';}
        small(const char* other,const size_t& len) {
            copy(str,other,len);
        }
    };
    struct large {
        char* str;
        size_t cap;
        large() : str(nullptr) , cap(0){}
        large(const size_t& in, const char* other,const size_t& len) : cap(in) {
            str = new char[cap + 1]();
            copy(str,other,len);
        }
        large& newCap (const size_t& in) {cap = in; return *this;}
        large& deleteStr() {
            if (str != nullptr) {
                delete[] str;
                str = nullptr;
            }
            return *this;
        };
        large& newStr(const char* other = "") {
            str = new char[cap + 1]();
            if (other) {
                size_t i = 0;
                for (;other[i] != '\0';i++){
                    str[i] = other[i];
                }
                str[i] = '\0';
            }
            return *this;
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

    struct variant{
        alignas(8) variantBuffer buffer[22];
        bool autoshrink = true;
        char type_index = -1;
        
        void destroy_current() {
            if (type_index == -1) return;
            
            // Start the recursive search for the correct type to destroy
            // printf("destroy current \n");
            if (type_index == 1) {
                static_cast<large*>(static_cast<void*>(buffer))->~large();
            }
            
            type_index = -1; 
        }
        template <typename T>
        constexpr variant& set(T&& value) {
            // 1. Destroy whatever was there before
            if (type_index != -1) destroy_current();
            
            new (buffer) T(static_cast<T&&>(value));
            type_index = is_same<T, large>::value; 
            return *this;
        }
        template <typename T>
        constexpr T& get() {
            return *static_cast<T*>(static_cast<void*>(buffer));
        }
        template <typename T>
        const T& get() const {
            
            return *static_cast<const T*>(static_cast<const void*>(buffer));
        }
        ~variant() { destroy_current();}
    } storage;

    size_t len ;
    constexpr static size_t getLen(const char* str) 
    {
        size_t inlen = 0;
        for (;str[inlen] != '\0';inlen++){}
        return inlen;
    }
    constexpr static void copy(char* to,const char* from,const size_t& len) 
    {
        size_t i = 0;
        for (;len - i != 0;i++){
            to[i] = from[i];
            to[len - i] = from[len - i];
        }
        to[len] = '\0';
    }
    public:

    explicit string() : len(0) {}

    string(const char* instr)  {
        len = getLen(instr);
        if (len > 21) {
            storage.set(large{len, instr,len});
        } else {
            storage.set(small{instr,len});
        }
    }

    string(const string& other) : len(other.len)  {
        if (other.storage.type_index == 1) { // Large
            const large& o_large = other.storage.get<large>();
            storage.set(large{o_large.cap, o_large.str,other.len});
        } else { // Small
            storage.set(small{other.storage.get<small>().str,other.len});
        }
    }

    string& operator=(const char* inStr) {
        len = getLen(inStr);
    
            // Logic: Should we stay/become Large?
        if (len > 21 || (storage.type_index == 1 && storage.autoshrink)) {
            if (storage.type_index == 1) {
                auto & stored = storage.get<large>();
                if (stored.cap < len) {
                    stored.newCap(len).deleteStr().newStr();
                }
                copy(stored.str, inStr,len);
            } else {
                storage.set(large{len, inStr,len});
            }
        } else {
            // Stay/become Small
            if (storage.type_index == 1) {
                storage.set(small{inStr,len});
            }
            auto& stored = storage.get<small>();
            copy(stored.str, inStr,len);
        }
        return *this;
    }

    constexpr string& reserve(size_t newSize) {
        if ((len + newSize) > 21) 
        {
            auto& stored = storage.get<large>();
            if (storage.type_index == 1) 
            {
                char* temp = new char[len];
                copy(temp, stored.str,len);
                stored.cap += newSize; 
                char * dest = stored.deleteStr().newStr().str; 
                copy(dest, temp,len);
                // delete [] storage.get<large>().str;
                // storage.get<large>().str = new char[storage.get<large>().cap]();
            } else {
                storage.set(large{newSize,"",0});
            }
            return *this;
        } else {
            return *this;
        }
    }

    string& append(const char* in) {
        size_t inlen = getLen(in);
        size_t totalLen = len + inlen;

        if (totalLen >= 21) {
            if (storage.type_index == 0) { // Upgrade Small to Large
                small old = storage.get<small>();
                // Current storage is now large, copy the 'append' part
                char* dest = storage.set(large{totalLen, old.str,len}).get<large>().str;
                
                for (size_t i = 0; (inlen - i) != 0; i++) { 
                    dest[len + i] = in[i]; 
                    dest[(len + inlen) - i] = in[inlen - i]; 
                }
                dest[totalLen] = '\0';

            } else { // Already Large
                auto& stored = storage.get<large>();
                if (stored.cap < totalLen) {
                    char* newStr = new char[len];
                    copy(newStr, stored.str,len);
                    char* dest = stored.deleteStr().newCap(totalLen).newStr().str;
                    copy(dest, newStr,len);
                } 
                for (size_t i = 0; (inlen - i) != 0; i++) { 
                    stored.str[len + i] = in[i]; 
                    stored.str[(len + inlen) - i] = in[inlen - i]; 
                }
                stored.str[totalLen] = '\0';
            }
        } else {
            // Stay Small
            char* dest = storage.get<small>().str;
            for (size_t i = 0; inlen - i != 0; i++) { 
                dest[len + i] = in[i];
                dest[inlen - i] = in[inlen - i];

            }
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
    size_t cap() {
        return storage.type_index == 1 ? 
        storage.get<large>().cap : 
        sizeof(small) ;
    }
    int getindex() {return storage.type_index;}
};
// class string {
//     struct largeStr {
//         char* str;
//         size_t cap;
//         bool mem;
//     };
//     struct smallStr {char str[24];};

//     size_t len;
//     bool large;
//     struct store {
//         union {
//             largeStr Large;
//             smallStr Small;
//         }type;
        
//         store& newLarge(size_t newLen) {
//             auto& l = type.Large;
//             l.mem = true;
//             l.cap = newLen;
//             l.str = new char[l.cap];
//             return *this;
//         }
//         store& capLarge(size_t newLen) {
//             auto& l = type.Large;
//             l.cap = newLen;
//             return *this;
//         }
//         store& delLarge() {
//             auto& l = type.Large;
//             if (l.mem) {
//                 printf("delete \n");
//                 delete [] type.Large.str;
//                 type.Large.str = nullptr;
//             }
//             l.mem = false;
//             return *this;
//         }
//     }storage;

//     size_t getLen(const char* str) 
//     {
//         size_t len = 0;
//         for (;str[len] != '\0';len++){}
//         return len;
//     }
//     size_t copy(char* to,const char* from) 
//     {
//         size_t i = 0;
//         for (;from[i] != '\0';i++){
//             to[i] = from[i];
//         }
//         return i;
//     }
    
//     public:

//     explicit constexpr string() : storage({.len = 0,.large = false}) {
//         storage.type.Small.str[0] = '\0';
//     }
    
//     constexpr string(const char* inStr) {
//         storage.len = getLen(inStr);
//         if (storage.len > 23) {
//             storage.large = true;
//             auto& large = storage.type.Large; 
//             storage.capLarge(storage.len).newLarge(storage.len);
//             large.str[copy(large.str, inStr)] = '\0';
//         } else {
//             storage.large = false;
//             auto& small = storage.type.Small; 
//             small.str[copy(small.str, inStr)] = '\0';
//         }
//     }
//     // constexpr string(string&& other) {
//     //     this->storage = other.storage;
//     //     //if (other.storage.allocated == storage.yes) { other.storage.delLarge();}
//     // }
//     constexpr string(const string& other) :storage({.len = other.storage.len,.large = other.storage.large}) {
//         if (this->storage.large) {
//             this->storage.capLarge(other.storage.type.Large.cap)
//             .newLarge(other.storage.len);
//             this->storage.type.Large.str[copy(storage.type.Large.str, other.storage.type.Large.str)] = '\0';
//         } else {
//             this->storage.type.Small.str[copy(storage.type.Small.str, other.storage.type.Small.str)] = '\0';
//         }
//     }
    
//     constexpr string& operator =(const char* inStr) {
//         size_t newLen = getLen(inStr);
        
//         if (newLen > 23) { // 22 + 1 for null terminator = 23 (your Small size)
//             if (storage.large == true) {
//                 // Already large, check if we can reuse the existing heap buffer
//                     if (storage.type.Large.cap < newLen) {
//                         storage.delLarge().newLarge(newLen); // Should allocate cap + 1
//                     }
//                     } else {
//                         // Switching from small to large
//                         storage.large = true;
//                         storage.capLarge(newLen).newLarge(newLen);
//                     }
//                 copy(storage.type.Large.str, inStr);
//                 storage.type.Large.str[newLen] = '\0';
//             } else {
//                 // Destination is small
//                 if (storage.large == true) {
//                     storage.delLarge();
//                 }
//                 storage.large = false;
//                 copy(storage.type.Small.str, inStr);
//                 storage.type.Small.str[newLen] = '\0';
//             }
//         storage.len = newLen;
//         return *this;
//     }

//     string& append(const char* in) {
//         size_t inlen = getLen(in);
//         size_t totalLen = storage.len + inlen;

//         if (totalLen > 23) {
//             if (!storage.large) { // Upgrade Small to Large
//                 smallStr old = storage.type.Small;
//                 size_t i = 0;
//                 // Current storage is now large, copy the 'append' part
//                 storage.type.Large.cap = totalLen;
//                 storage.newLarge(totalLen);
//                 char* dest = storage.type.Large.str;
//                 for (; old.str[i] != '\0'; i++) { dest[i] = in[i]; }
//                 for (; i < inlen; i++) { dest[storage.len + i] = in[i]; }
//                 dest[totalLen] = '\0';
//             } else { // Already Large
//                 largeStr& l = storage.type.Large;
//                 if (l.cap < totalLen) {
//                     char* newStr = new char[storage.len + 1]();
//                     copy(newStr, l.str);
//                     storage.delLarge().newLarge(totalLen);
//                 }
//                 for (size_t i = 0; i < inlen; i++) { l.str[storage.len + i] = in[i]; }
//                 l.str[totalLen] = '\0';
//             }
//         } else {
//             // Stay Small
//             char* dest = storage.type.Small.str;
//             for (size_t i = 0; i < inlen; i++) { dest[storage.len + i] = in[i]; }
//             dest[totalLen] = '\0';
//         }
//         storage.len = totalLen;
//         return *this;
//     }
    
//     const char* data() const {
//         return storage.large ? 
//         storage.type.Large.str:
//         storage.type.Small.str;
//     }
//     size_t cap() const {
//         return storage.large ? 
//         storage.type.Large.cap:
//         23;
//     }
//     ~string() {
//         if(storage.large == true) {
//             if (storage.type.Large.mem == true)
//             {
//                 printf("delete string\n");
//                 storage.delLarge();
//             }
//         }
//     }
// };

int main()
{
    try 
    {
        
        
        string s ("hello world before reserve");
        // s.reserve(20);
        printf("%s %llu \n",s.data() , s.cap());
        s.append(" new char");
        printf("%s %llu \n",s.data() , s.cap());
        //s.reserve(50);
        // s.append(" after append");
        // c.append(" copy");
        string c (s);
        c.append(" is c");
        printf("%s %llu \n",c.data() , c.cap());
        s = ("hello world from world number");
        printf("%s %llu \n",s.data() , s.cap());
        s = "hello world numbers 3200";
        printf("%s %llu \n",s.data() , s.cap());
        s = "hello again from world number 3200";
        printf("%s %llu \n",s.data() , s.cap());
        s = "small";
        printf("%s %llu \n",s.data() , s.cap());
        s = "again";
        printf("%s %llu \n",s.data() , s.cap());
        s = "hello again from world number 4200";
        printf("%s %llu \n",s.data() , s.cap());
        s = "sssssssssssssssssssssssssssssssss";
        printf("%s %llu \n",s.data() , s.cap());
        s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
        printf("%s %llu \n",s.data() , s.cap());
        s = "wwwwwwwwwwwwwwwwwwwwww";
        printf("%s %llu \n",s.data() , s.cap());
        // s = "aaa";
        // printf("%s \n", s.data());
        
    } catch (MyException e)
    {
        e.print();
    }
    return 0; 
}