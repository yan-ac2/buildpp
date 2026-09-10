
#include <cstddef>
#include <string_view>
class string {
public:
    enum Mode : unsigned int {
        onHeap     = 1 << 0, // Bit 0 (1)  -> Dynamic heap allocation active (Owned)
        Large      = 1 << 1, // Bit 1 (2)  -> Structural layout is Large string
        Literal    = 1 << 2, // Bit 2 (4)  -> Structural layout points to external string view/literal
        View       = Literal,// Alias for clarity when using external buffers/views
        autoResize = 1 << 3, // Bit 3 (8)  -> 0 = Preserve heap cap (Default), 1 = Shrink back to SSO
        noHeap     = 1 << 4, // Bit 4 (16) -> Prevent heap allocations (Truncates on overflow)
        lenExt1     = 1 << 5, 
        Small      = 0       // Value 0    -> Default inline SSO buffer
    };

private:
    struct largeStr {
        char* str = nullptr;
        char* end = nullptr;
        size_t cap = 0;
    };
    
    struct smallStr {
        char str[24] = {}; // 23 usable chars + 1 null terminator
    };

    static constexpr size_t sMaxStr = sizeof(smallStr::str) - 1; // 23 bytes

    struct store {
        unsigned int mode = Mode::Small; // 4 bytes (Flags)
        unsigned int len  = 0;           // 4 bytes (32-bit string length)

        union Type {
            const char* cExpr;
            largeStr Large;
            smallStr Small;

            constexpr Type() : Small{} {}
            constexpr ~Type() {}
        } type; // 24 bytes -> Total sizeof(store) == 32 bytes
    } storage;

    static constexpr size_t getLen(const char* str) noexcept {
        if (!str) return 0;
        size_t len = 0;
        while (str[len] != '\0') { len++; }
        return len;
    }

    static constexpr size_t copy(char* to, const char* from, size_t count) noexcept {
        size_t i = 0;
        while (i < count && from[i] != '\0') {
            to[i] = from[i];
            i++;
        }
        return i;
    }

    static constexpr size_t copy(char* to, const char* from) noexcept {
        size_t i = 0;
        while (from[i] != '\0') {
            to[i] = from[i];
            i++;
        }
        return i;
    }

public:
    // 1. Default constructor
    explicit constexpr string() noexcept {
        storage.mode = Mode::Small;
        storage.len = 0;
        storage.type.Small = smallStr{};
        storage.type.Small.str[0] = '\0';
    }

    // 2. C-String constructor: Literals/Large strings default to View mode for constexpr safety
    template<size_t N>
    constexpr string(const char (&inStr)[N]) {
        storage.mode = Mode::View | Mode::noHeap;
        storage.type.cExpr = inStr;
        storage.len = static_cast<unsigned int>(N);
        assign(inStr);
    }
    constexpr string(const char* inStr) {
        storage.mode = Mode::View | Mode::noHeap;
        storage.type.cExpr = inStr;
        storage.len = static_cast<unsigned int>(getLen(inStr));
    }
    constexpr string(const char* inStr,size_t Len) {
        storage.mode = Mode::View | Mode::noHeap;
        storage.type.cExpr = inStr;
        storage.len = static_cast<unsigned int>(Len);
    }

    // Flag Management Setters / Getters
    constexpr void setAutoResize(bool enable) noexcept {
        if (enable) storage.mode |= Mode::autoResize;
        else        storage.mode &= ~Mode::autoResize;
    }
    constexpr void setNoHeap(bool enable) noexcept {
        if (enable) storage.mode |= Mode::noHeap;
        else        storage.mode &= ~Mode::noHeap;
    }
    constexpr bool isAutoResizeEnabled() const noexcept { return (storage.mode & Mode::autoResize) != 0; }
    constexpr bool isNoHeapEnabled()     const noexcept { return (storage.mode & Mode::noHeap) != 0; }
    constexpr bool isView()              const noexcept { return (storage.mode & Mode::View) != 0; }

    inline constexpr string& reuseBuffer(const char* inStr,size_t writeLen) {
        auto& ptr = storage.type.Large;

        copy(ptr.str, inStr, writeLen);
        ptr.str[writeLen] = '\0';
        ptr.end = storage.type.Large.str + writeLen;
        storage.len = static_cast<unsigned int>(writeLen);
        
        return *this;
    }
    inline constexpr string& allocateBuffer(const char* inStr,size_t writeLen) {
        auto& ptr = storage.type.Large;

        bool allocateBuffer = (storage.mode & Mode::onHeap) && (writeLen <= storage.type.Large.cap);

        char* targetBuf = nullptr;
        size_t newCap = storage.type.Large.cap;

        if (allocateBuffer) {
            return reuseBuffer(inStr,writeLen);
        } else {
            if (storage.mode & Mode::onHeap) {
                delete[] storage.type.Large.str;
            }
            newCap = writeLen;
            targetBuf = new char[newCap + 1]();
        }
        copy(targetBuf, inStr,writeLen);
        targetBuf[writeLen] = '\0';

        unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
        storage.mode = Mode::onHeap | Mode::Large | keepFlags;
        storage.type.Large = largeStr{
            .str = targetBuf,
            .end = targetBuf + writeLen,
            .cap = newCap
        };
        storage.len = static_cast<unsigned int>(writeLen);
        return *this;
    }
    inline constexpr string& useSBO(const char* inStr,size_t writeLen) {
        auto& ptr = storage.type.Small;
        if (storage.mode & Mode::onHeap) {
            delete[] storage.type.Large.str;
        }

        unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
        storage.mode = Mode::Small | keepFlags;
        
        ptr = smallStr{};
        ptr.str[copy(storage.type.Small.str, inStr,writeLen)] = '\0';
        storage.len = static_cast<unsigned int>(writeLen);
        return *this;
    }
    inline constexpr string& reserve(size_t newLen) {
        const char* oldData = data();
        bool reuseHeap = (storage.mode & Mode::onHeap) && (newLen <= storage.type.Large.cap);

        char* newBuffer = nullptr;
        size_t newCap = storage.type.Large.cap;

        if (reuseHeap) {
            newBuffer = storage.type.Large.str;
        } else {
            newCap = newLen;
            newBuffer = new char[newCap + 1]();
            copy(newBuffer, oldData);
            if (storage.mode & Mode::onHeap) {
                delete[] storage.type.Large.str;
            }
        }
        
        storage.type.Large = largeStr{
            .str = newBuffer,
            .end = newBuffer + storage.len,
            .cap = newCap
        };
        newBuffer[storage.len] = '\0';
        return *this;
    }
    inline constexpr string& assign(const char* inStr) {
        size_t newLen = getLen(inStr);
        // CASE 1: Currently on Heap & autoResize is DISABLED -> Reuse existing heap buffer
        if ((storage.mode & Mode::onHeap) && !isAutoResizeEnabled()) {
            return reuseBuffer(inStr,newLen);
        }
        // CASE 2: Fits in Small SSO buffer
        if (newLen <= sMaxStr) {
            return useSBO(inStr,newLen);
        } 
        // CASE 3: Needs larger allocation
        return allocateBuffer(inStr,newLen);
    }

    inline constexpr string& append(const char* in) {
        size_t inlen = getLen(in);
        if (inlen == 0) return *this;

        size_t currentLen = storage.len;
        size_t totalLen = currentLen + inlen;

        if (totalLen > sMaxStr) {
            auto& ptr = storage.type.Large;
            reserve(totalLen);
            copy(ptr.str + len(), in,inlen);

            unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
            storage.mode = Mode::onHeap | Mode::Large | keepFlags;
        } else {
            if (storage.mode & Mode::View) {
                const char* oldLiteral = storage.type.cExpr;
                unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
                storage.mode = Mode::Small | keepFlags;
                storage.type.Small = smallStr{};
                copy(storage.type.Small.str, oldLiteral);
            }
            char* dest = (storage.mode & Mode::onHeap) ? storage.type.Large.str : storage.type.Small.str;
            copy(dest + currentLen, in);
            dest[totalLen] = '\0';
        }

        storage.len = static_cast<unsigned int>(totalLen);
        return *this;
    }

    constexpr const char* data() const noexcept {
        if (storage.mode & Mode::View)  return storage.type.cExpr;
        if (storage.mode & Mode::Large) return storage.type.Large.str;
        return storage.type.Small.str;
    }

    constexpr size_t capacity() const noexcept {
        if (storage.mode & Mode::Large) return storage.type.Large.cap;
        return sMaxStr;
    }

    constexpr string& operator=(const char* in) {
        assign(in);
        return *this;
    }

    constexpr size_t size() const noexcept { return storage.len; }
    constexpr size_t len() const noexcept  { return storage.len; }
    constexpr unsigned int mode() const noexcept { return storage.mode; }
    constexpr bool isOnHeap() const noexcept { return (storage.mode & Mode::onHeap) != 0; }

    constexpr ~string() {
        // Views and non-heap instances bypass heap deletion completely
        if (storage.mode & Mode::onHeap) {
            delete[] storage.type.Large.str;
        }
    }
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
        s.append(" after append ");
        // printf("%s %zu \n",s.data() , s.size());
        // string c (s);
        // c.append(" copy");
        // printf("%s %zu \n",c.data() , c.size());
        s.append ("hello world from world number");
        printf("%s %zu \n", s.data(), s.size());
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

// constexpr string sss("hello wssssssss large nee");
constexpr string ss ("hello wssssssss large nee");

static_assert((ss.mode() & string::View) != 0, "yes");