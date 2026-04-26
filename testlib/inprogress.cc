#include <cstddef>
#include <cstdio>
#include <cstdlib>

template <class Ty, Ty Val>
struct integral_constant {
    static constexpr Ty value = Val;
    using value_type = Ty;
    using type       = integral_constant;
    constexpr operator value_type() const noexcept  {return value;}
    constexpr value_type operator()() const noexcept {return value;}
};
template <bool Val> struct bool_constant {enum{value = Val};};
using false_t = bool_constant<false>;
using true_t  = bool_constant<true>;
template<class T,class U> struct is_same {enum : bool{value = false} ;using type = T;};
template<class T> struct is_same<T,T>    {enum : bool{value = true} ;using type = T;};

template<typename T> struct is_ptr :false_t     {using type = T;};
template<typename T> struct is_ptr<T*> :true_t  {using type = T;};

// template<typename T> concept nonull  = requires (T p) { {p != nullptr};};

struct variantBuffer{char buffer;};
inline void* operator new (size_t size,variantBuffer* p) {return p;}
inline constexpr void memcopy(void* to,const void* from,const size_t len) noexcept {
    unsigned char* pto = static_cast<unsigned char*>(to);
    const unsigned char* fptr = static_cast<const unsigned char*>(from);
    const unsigned char* efptr = static_cast<const unsigned char*>(from) + len;
    for (;fptr < efptr;){
        *pto++ = *fptr++;
    }
}

class string {
    struct small {
        char str[22]; 
        small() noexcept {str[0] = '\0';}
        constexpr small& copy(const size_t to,const char* from,const size_t* len) noexcept {
            char* tptr = str + to;
            const char* efptr = from + *len;
            for (;from < efptr;){
                *tptr++ = *from++;
            }*tptr++ = '\0';
            return *this;
        }
        ~small(){}
    };
    struct large {
        char* str;
        size_t cap;
        large() noexcept : str(nullptr) , cap(0){}
        large(const size_t* in) noexcept : cap(*in) {}
        constexpr large& copy(size_t to,const char* from,const size_t len) {
            char* tptr = str + to;
            const char* efptr = from + len;
            for (;from < efptr;){
                *tptr++ = *from++;
            }   *tptr++ = '\0';
            return *this;
        }
        large& addCap (const size_t* other) {cap += *other;return *this;}
        large& move (char*& temp) {temp = str;str = nullptr;return *this;}
        large& newCap (const size_t* in) noexcept {cap = *in; return *this;}
        large& deleteStr() noexcept {
            if (str != nullptr) {
                delete[] str;
                str = nullptr;
            }
            return *this;
        };
        large& newStr() noexcept {str = new char[cap + 1]; return *this;}
        large& newStr(const char* other,const size_t* len) noexcept {
            str = new char[cap + 1];
            return other ? copy(0, other, *len) : *this;
        }
        large(large&& other) noexcept : cap(other.cap) {other.str = nullptr;}
        ~large() noexcept {deleteStr();}
        large(const large&) = delete;
    };

    struct variant{
        template <typename T> static constexpr bool is_large = is_same<T, large>::value ;
        template <typename T> static constexpr bool is_small = is_same<T, small>::value ;

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
            if (type_index == s) {reinterpret_cast<small*>(buffer)->~small();}
            type_index = no_type; 
        }
        template <typename T> requires (is_large<T> || is_small<T>)
        constexpr variant& set(T&& value) noexcept {
            destroy_current();
            new (buffer) T(static_cast<T&&>(value));
            type_index = is_large<T> ? l : s; 
            return *this;
        }

        template <typename T> requires (is_large<T> || is_small<T>)
        constexpr T& get() noexcept { return *reinterpret_cast<T*>(buffer);}
        
        template <typename T> requires (is_large<T> || is_small<T>)
        const T& get() const noexcept { return *reinterpret_cast<const T*>(buffer);}
        ~variant() { destroy_current();}
    } storage;

    inline constexpr size_t getLen(const char* str) noexcept
    {
        size_t inlen = 0;
        for (;str[inlen] != '\0';inlen++){}
        return inlen;
    }
    public:

    explicit constexpr string() noexcept : storage({.len = 0}) {}

    string(const char* instr) noexcept {
        const size_t len = getLen(instr);
        if (len > 21) {
            storage.set(large(&len)).get<large>().newStr(instr,&len);
        } else {
            storage.set(small()).get<small>()
            .copy(0,instr, &len);
        }
        storage.len = len;
    }

    constexpr string(const string& other) noexcept : storage({.len = other.storage.len}) {
        if (other.storage.type_index == 1) { // Large
            const large& o_large = other.storage.get<large>();
            storage.set(large(&o_large.cap)).get<large>().newStr(o_large.str,&other.storage.len);
        } else { // Small
            storage.set(small()).get<small>()
            .copy(0,other.storage.get<small>().str, &other.storage.len);
        }
    }

    constexpr string& operator=(const char* inStr) noexcept {
        storage.len = getLen(inStr);
    
            // Logic: Should we stay/become Large?
        if (storage.len > 21 || (storage.type_index == 1 && !storage.autoshrink)) {
            if (storage.type_index == 1) {
                auto & stored = storage.get<large>();
                if (stored.cap < storage.len) {
                    stored.newCap(&storage.len).deleteStr().newStr(inStr,&storage.len);
                } else stored.copy(0, inStr,storage.len);
            } else {
                storage.set(large(&storage.len)).get<large>().newStr(inStr,&storage.len);
            }
        } else {
            // Stay/become Small
            if (storage.type_index == 1) {
                storage.set(small());
            }
            storage.get<small>()
            .copy(0,inStr,&storage.len);
        }
        return *this;
    }

    string& reserve(size_t in) noexcept {
        const size_t newSize = storage.len + in;
        if ((newSize) > 21) 
        {
            if (storage.type_index == 1) 
            {
                auto& stored = storage.get<large>();
                char* temp; 
                stored.addCap(&newSize).move(temp).newStr(temp,&storage.len);
                delete [] temp;
            } else {
                small temp = storage.get<small>();
                storage.set(large(&newSize)).get<large>().newStr(temp.str,&storage.len);
            }
            return *this;
        } else {
            return *this;
        }
    }

    string& append(const char* in) noexcept {
        const size_t inlen = getLen(in);
        const size_t totalLen = storage.len + inlen;

        if (totalLen > 21 || storage.type_index == 1) {
            if (storage.type_index == 0) { // Upgrade Small to Large
                small old = storage.get<small>();
                // Current storage is now large, copy the 'append' part
                storage.set(large(&totalLen)).get<large>()
                .newStr(old.str, &storage.len)
                .copy(storage.len, in, inlen);
            } else { // Already Large
                auto& stored = storage.get<large>();
                if (totalLen > stored.cap) {
                    char* temp;
                    stored.move(temp).newCap(&totalLen).newStr(temp,&storage.len);
                    delete [] temp;
                } 
                stored.copy(storage.len, in, inlen);
            }
        } else {// Stay Small
            storage.get<small>().copy(storage.len, in, &inlen);
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