
#include <cstdio>
#include <cstdlib>
#include <memory>

using size_t = decltype(sizeof(0));

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
        small() noexcept { str[0] = '\0'; }
        ~small() {}
        auto* begin() { return str; }
        auto* end() { return (str + 22); }
    };

    struct large {
        char* str;
        size_t cap;
        
        constexpr large() noexcept : str(nullptr), cap(0) {}
        constexpr large(size_t capacity) noexcept : str(nullptr), cap(capacity) {}
        ~large() noexcept { deleteStr(); }

        constexpr large& deleteStr() noexcept {
            delete[] str;
            str = nullptr;
            return *this;
        }

        constexpr large& allocate_and_copy(const char* source, size_t source_len, size_t new_cap) {
            char* new_buffer = new char[new_cap + 1];
            if (source && source_len > 0) {
                for (std::size_t i = 0; i < source_len; ++i) {
                    new_buffer[i] = source[i];
                }
            }
            new_buffer[source_len] = '\0'; // Properly null terminate

            delete[] str;
            str = new_buffer; // Points safely to the beginning of allocation
            cap = new_cap;
            return *this;
        }

        auto* begin() { return str; }
        auto* end() { return (str + cap); }

        constexpr large(large&& other) noexcept : str(other.str), cap(other.cap) {
            other.str = nullptr;
            other.cap = 0;
        }
        constexpr large(const large&) = delete;
        constexpr large& operator=(const large&) = delete;
    };

    struct variant {
        template <typename T> using is_large = is_same<T, large>;
        template <typename T> using is_small = is_same<T, small>;
        template <typename T> using is_literal = is_same<T, const char*>;
        // Using standard aligned_storage representation safely
        alignas(alignof(large) > alignof(small) ? alignof(large) : alignof(small)) 
        variantBuffer buffer[sizeof(large) > sizeof(small) ? sizeof(large) : sizeof(small)] {};
        
        bool autoshrink = false;
        enum : char { no_type = -1, smll = 0, lrg = 1 ,literal = 2} type_index = no_type;
        size_t len = 0;
        
        constexpr void destroy_current() noexcept {
            if (type_index == no_type) return;
            if (type_index == lrg) { static_cast<large*>(static_cast<void*>(buffer))->~large(); }
            type_index = no_type; 
        }

        template <typename T>
        constexpr variant& set(T value) noexcept {
            destroy_current();
            std::construct_at(reinterpret_cast<T*>(buffer), std::forward<T>(value));
            // new (buffer) T(static_cast<decltype(value)>(value));
            type_index = is_large<T>::value ? lrg : is_small<T>() ? smll : is_literal<T>() ? literal : no_type; 
            return *this;
        }

        template <typename T> constexpr T& get() noexcept { return *static_cast<T*>(static_cast<void*>(buffer)); }
        template <typename T> constexpr const T& get() const noexcept { return *static_cast<const T*>(static_cast<const void*>(buffer)); }
        ~variant() { destroy_current(); }
    } storage;

    inline constexpr size_t getLen(const char* str) const noexcept {
        if (!str) return 0;
        size_t inlen = 0;
        for (; str[inlen] != '\0'; inlen++) {}
        return inlen;
    }

    enum constValue { smallSize = 21 }; // 22 - 1 for null terminator

public:
    // rule 5
    constexpr string() noexcept {
        storage.set(small());
        storage.len = 0;
    }

    constexpr string(const char* instr) noexcept {
        const size_t len = getLen(instr);
        if (len > smallSize) {
            storage.set(large{}).get<large>().allocate_and_copy(instr, len, len);
        } else {
            char* ptr = storage.set(small()).get<small>().str;
            for (size_t i = 0; i < len; ++i) {
                ptr[i] = instr[i];
            }
            ptr[len] = '\0';
        }
        storage.len = len;
    }

    constexpr string(const string& other) noexcept {
        storage.len = other.storage.len;
        if (other.storage.type_index == 1) { 
            const large& o_large = other.storage.get<large>();
            storage.set(large{}).get<large>().allocate_and_copy(o_large.str, storage.len, o_large.cap);
        } else { 
            char* dest = storage.set(small()).get<small>().str;
            const char* src = other.get();
            for (size_t i = 0; i <= storage.len; ++i) { // Copies null terminator too
                dest[i] = src[i];
            }
        }
    }

    string(string&& other)  = delete;

    constexpr string& operator=(const char* inStr) noexcept {
        std::size_t inLen = getLen(inStr);

        if (inLen > smallSize || (storage.type_index == 1 && !storage.autoshrink)) {
            if (storage.type_index == 1) {
                auto& stored = storage.get<large>();
                if (stored.cap < inLen) {
                    stored.allocate_and_copy(inStr, inLen, inLen);
                } else {
                    for (size_t i = 0; i < inLen; ++i) stored.str[i] = inStr[i];
                    stored.str[inLen] = '\0';
                }
            } else {
                storage.set(large{}).get<large>().allocate_and_copy(inStr, inLen, inLen);
            }
        } else {
            char* dest = storage.set(small()).get<small>().str;
            for (size_t i = 0; i < inLen; ++i) {
                dest[i] = inStr[i];
            }
            dest[inLen] = '\0';
        }
        storage.len = inLen;
        return *this;
    }

    constexpr string& reserve(size_t in) {
        const size_t newSize = storage.len + in;
        if (newSize > smallSize) {
            if (storage.type_index == 1) { 
                auto& stored = storage.get<large>();
                if (newSize > stored.cap) {
                    stored.allocate_and_copy(stored.str, storage.len, newSize);
                }
            } else { 
                small temp = storage.get<small>();
                storage.set(large()).get<large>().allocate_and_copy(temp.str, storage.len, newSize);
            }
        }
        return *this;
    }

    constexpr string& append(const char* in) {
        const size_t inlen = getLen(in);
        if (inlen == 0) return *this;
        const size_t totalLen = storage.len + inlen;

        if (totalLen > smallSize || storage.type_index == 1) {
            if (storage.type_index == 0) { 
                small old = storage.get<small>();
                storage.set(large(totalLen)).get<large>().allocate_and_copy(old.str, storage.len, totalLen);
                for (size_t i = 0; i < inlen; ++i) {
                    storage.get<large>().str[storage.len + i] = in[i];
                }
                storage.get<large>().str[totalLen] = '\0';
            } else { 
                auto& stored = storage.get<large>();
                if (totalLen > stored.cap) {
                    stored.allocate_and_copy(stored.str, storage.len, totalLen);
                }
                for (size_t i = 0; i < inlen; ++i) {
                    stored.str[storage.len + i] = in[i];
                }
                stored.str[totalLen] = '\0';
            }
        } else { 
            char* base = storage.get<small>().str + storage.len;
            for (size_t i = 0; i < inlen; ++i) {
                base[i] = in[i];
            }
            base[inlen] = '\0';
        }
        storage.len = totalLen;
        return *this;
    }

    constexpr char* get() noexcept {
        return storage.type_index == 1 ? storage.get<large>().str : storage.get<small>().str;
    }
    
    constexpr const char* get() const noexcept {
        return storage.type_index == 1 ? storage.get<large>().str : storage.get<small>().str;
    }

    constexpr char& front() noexcept { return get()[0]; }
    constexpr const char& front() const noexcept { return get()[0]; }
    constexpr char& back() noexcept { return get()[storage.len - 1]; }
    constexpr const char& back() const noexcept { return get()[storage.len - 1]; }
    constexpr char* data() noexcept { return get(); }
    
    constexpr size_t size() const noexcept { return storage.len; }
    constexpr size_t cap() const noexcept { return storage.get<large>().cap; }
    constexpr int getindex() const noexcept { return storage.type_index; }
};



int main()
{
        string s ("hello world before");
        s.reserve(35);
        // printf("%s %zu \n",s.data() , s.size());
        s.append(" new char");
        // s.front() = 'f';
        // s.back() = 's';
        // // printf("%s %zu \n",s.data() , s.size());
        s.reserve(50);
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
        printf("%s %zu \n", s.data(), s.cap());
    
    return 0; 
}