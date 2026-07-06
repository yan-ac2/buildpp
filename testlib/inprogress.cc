#include <cstddef>
#include <cstdio>
#include <cstdlib>

template <class Ty, Ty Val>
struct integral_constant {
    enum : Ty {value = Val};
    using value_type = Ty;
    using type       = integral_constant;
    
    consteval operator value_type() const noexcept   {return static_cast<value_type>(value);}
    consteval value_type operator()() const noexcept {return static_cast<value_type>(value);}
};
template <bool Val> struct bool_constant : integral_constant<bool,Val> {};
using false_t = bool_constant<false>;
using true_t  = bool_constant<true>;

template<class T,class U> struct is_same : false_t{};
template<class T> struct is_same<T,T>    : true_t {};

template<typename  T,typename U> struct is_enum : false_t {};


template<typename T> struct is_ptr     :false_t {};
template<typename T> struct is_ptr<T*> :true_t  {};



struct variantBuffer{char buffer;};
inline void* operator new (size_t size,variantBuffer* p) {return p;}

class string {
    struct small {
        char str[22]; 
        small() noexcept {str[0] = '\0';}
        ~small(){}
    };
    struct large {
        char* str;
        size_t cap;
        
        large() noexcept : str(nullptr), cap(0) {}
        large(size_t capacity) noexcept : str(nullptr), cap(capacity) {}
        
        // Clean up properly
        ~large() noexcept { deleteStr(); }

        large& deleteStr() noexcept {
            delete[] str;
            str = nullptr;
            return *this;
        }

        // Atomic allocation and copy. Replaces the broken state chaining.
        large& allocate_and_copy(const char* source, size_t source_len, size_t new_cap) {
            char* new_buffer = new char[new_cap + 1];
            if (source && source_len > 0) {
                // Manual inline copy to avoid dependency on unsafe loops
                for (size_t i = 0; i < source_len; ++i) {
                    new_buffer[i] = source[i];
                }
            }
            new_buffer[source_len] = '\0';

            // Safe swap-state transition
            delete[] str;
            str = new_buffer;
            cap = new_cap;
            return *this;
        }
        auto* begin() {
            return (str+0);
        }
        auto* end() {
            return (str+cap);
        }

        // Explicit move mechanics
        large(large&& other) noexcept : str(other.str), cap(other.cap) {
            other.str = nullptr;
            other.cap = 0;
        }
        large(const large&) = delete;
    };


    struct variant{
        template <typename T> using is_large = is_same<T, large>;
        template <typename T> using is_small = is_same<T, small>;

        alignas(alignof(large) > alignof(small) ? alignof(large) : alignof(small) ) 
        variantBuffer buffer[sizeof(large)>sizeof(small)? sizeof(large) : sizeof(small)] {};
        bool autoshrink = false;
        enum : char {
            no_type = -1,
            s = 0,
            l = 1
        } type_index = no_type;
        size_t len;
        
        void destroy_current() noexcept {
            if (type_index == no_type) return;
            if (type_index == l) {reinterpret_cast<large*>(buffer)->~large();}
            type_index = no_type; 
        }
        template <typename T> requires (is_large<T>()() || is_small<T>()())
        constexpr variant& set(T&& value) noexcept {
            destroy_current();
            new (buffer) T(static_cast<T&&>(value));
            type_index = is_large<T>()() ? l : s; 
            return *this;
        }

        template <typename T> requires (is_large<T>()() || is_small<T>()())
        constexpr T& get() noexcept { return *reinterpret_cast<T*>(buffer);}
        
        template <typename T> requires (is_large<T>()() || is_small<T>()())
        const T& get() const noexcept { return *reinterpret_cast<const T*>(buffer);}
        ~variant() { destroy_current();}
    } storage;

    inline constexpr size_t getLen(const char* str) noexcept
    {
        size_t inlen = 0;
        for (;str[inlen] != '\0';inlen++){}
        return inlen;
    }

    enum constValue {
        smallSize = (sizeof(small::str) - 1)
    };

    public:

    explicit constexpr string() noexcept : storage({.len = 0}) {}

    string(const char* instr) noexcept {
        const size_t len = getLen(instr);
        if (len > smallSize) {
            storage.set(large{}).get<large>().allocate_and_copy(instr, len, len+1);
        } else {
            auto& ptr = storage.set(small()).get<small>();
            for (const auto& I : instr) {

            }
        }
        storage.len = len;
    }

    constexpr string(const string& other) noexcept : storage({.len = other.storage.len}) {
        if (other.storage.type_index == 1) { // Large
            const large& o_large = other.storage.get<large>();
            storage.set(large{}).get<large>().allocate_and_copy(o_large.str,other.storage.len,o_large.cap);
        } else { // Small
            storage.set(small()).get<small>()
            .copy(0,other.storage.get<small>().str, &other.storage.len);
        }
    }

    constexpr string& operator=(const char* inStr) noexcept {
        std::size_t inLen = getLen(inStr);

            // Logic: Should we stay/become Large?
        if (storage.len > smallSize || (storage.type_index == 1 && !storage.autoshrink)) {
            if (storage.type_index == 1) {
                auto & stored = storage.get<large>();
                if (stored.cap < storage.len) {
                    stored.allocate_and_copy(inStr, inLen, inLen + 1);
                };
            } else {
                storage.set(large{}).get<large>().allocate_and_copy(inStr,storage.len,storage.len);
            }
        } else {
            // Stay/become Small
            if (storage.type_index == 1) {
                storage.set(small());
            }
            storage.get<small>()
            .copy(0,inStr,&storage.len);
        }
        storage.len = inLen;
        return *this;
    }

    string& reserve(size_t in) {
        const size_t newSize = storage.len + in;
        if (newSize > smallSize) {
            if (storage.type_index == 1) { // Large
                auto& stored = storage.get<large>();
                if (newSize > stored.cap) {
                    // Safely reallocates without changing state prematurely
                    stored.allocate_and_copy(stored.str, storage.len, newSize);
                }
            } else { // Small -> Large upgrade
                small temp = storage.get<small>();
                storage.set(large(newSize)).get<large>().allocate_and_copy(temp.str, storage.len, newSize);
            }
        }
        return *this;
    }

    string& append(const char* in) {
        const size_t inlen = getLen(in);
        if (inlen == 0) return *this;
        const size_t totalLen = storage.len + inlen;

        if (totalLen > smallSize || storage.type_index == 1) {
            if (storage.type_index == 0) { // Small -> Large
                small old = storage.get<small>();
                storage.set(large(totalLen)).get<large>().allocate_and_copy(old.str, storage.len, totalLen);
                // Append the second string
                for (size_t i = 0; i < inlen; ++i) {
                    storage.get<large>().str[storage.len + i] = in[i];
                }
                storage.get<large>().str[totalLen] = '\0';
            } else { // Already Large
                auto& stored = storage.get<large>();
                if (totalLen > stored.cap) {
                    stored.allocate_and_copy(stored.str, storage.len, totalLen);
                }
                // Append the string data safely
                for (size_t i = 0; i < inlen; ++i) {
                    stored.str[storage.len + i] = in[i];
                }
                stored.str[totalLen] = '\0';
            }
        } else { // Stay Small
            char* base = storage.get<small>().str + storage.len;
            for (size_t i = 0; i < inlen; ++i) {
                *base++ = in[i];
            }
            *base = '\0';
        }
        storage.len = totalLen;
        return *this;
    }
    constexpr char* get() noexcept {
        return storage.type_index == 1 ?
        storage.get<large>().str:
        storage.get<small>().str;
    }
    using cstr = const char*;
    constexpr cstr get() const noexcept {
        return storage.type_index == 1 ?
        storage.get<large>().str:
        storage.get<small>().str;
    }
    constexpr char& front() noexcept {return get()[0];}
    constexpr const char& front() const noexcept {return get()[0];}
    constexpr char& back() noexcept { return get()[storage.len-1];}
    constexpr const char& back() const noexcept { return get()[storage.len-1];}
    constexpr char* data() noexcept { return get();}
    constexpr size_t size() const noexcept {
        return storage.type_index == 1 ?
        storage.get<large>().cap: 
        21;
    }
    constexpr int getindex() const noexcept {return storage.type_index;}
};


int main()
{
        string s ("hello world before");
        s.reserve(35);
        // printf("%s %zu \n",s.data() , s.size());
        s.append(" new char");
        s.front() = 'f';
        s.back() = 's';
        // printf("%s %zu \n",s.data() , s.size());
        // s.reserve(50);
        s.append(" after append");
        // printf("%s %zu \n",s.data() , s.size());
        // string c (s);
        // c.append(" copy");
        // printf("%s %zu \n",c.data() , c.size());
        s = ("hello world from world number");
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello world numbers 3200";
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello again from world number 3200";
        // printf("%s %zu \n",s.data() , s.size());
        s = "small";
        // printf("%s %zu \n",s.data() , s.size());
        s.append(" append");
        // printf("%s %zu \n",s.data() , s.size());
        s = "again";
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello again from world number 4200";
        // printf("%s %zu \n",s.data() , s.size());
        s = "sssssssssssssssssssssssssssssssss";
        // printf("%s %zu \n",s.data() , s.size());
        s = "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww";
        // printf("%s %zu \n",s.data() , s.size());
        s = "wwwwwwwwwwwwwwwwwwwwww";
        // printf("%s %zu \n",s.data() , s.size());
        s = "aaa";
        printf("%s %zu \n", s.data(), s.size());
    
    return 0; 
}