
#include <cstddef>
#include <cstdio>
#include <string_view>
#include <__iterator/reverse_iterator.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/reverse_view.h>



struct stringView {
    using value_type = char;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;
    // using ite
    struct reverse_iterator {
        const_pointer current;
        
        constexpr explicit reverse_iterator(const_pointer ptr) noexcept : current(ptr) {}
        
        constexpr const_reference operator*() const noexcept { return *(current - 1); }
        constexpr const_pointer operator->() const noexcept { return current - 1; }
        
        // Increment moves BACKWARD
        constexpr reverse_iterator& operator++() noexcept {
            --current;
            return *this;
        }
        constexpr reverse_iterator operator++(int) noexcept {
            reverse_iterator temp = *this;
            --current;
            return temp;
        }
        
        constexpr reverse_iterator& operator--() noexcept {
            ++current;
            return *this;
        }
        constexpr reverse_iterator operator--(int) noexcept {
            reverse_iterator temp = *this;
            ++current;
            return temp;
        }
        
        constexpr bool operator==(const reverse_iterator& rhs) const noexcept = default;
    };
    
    const_pointer data_;
    size_type len{};

    static constexpr size_type npos {size_type(-1)};

    constexpr stringView() noexcept : data_(nullptr),len(0)  {}
    constexpr stringView(const stringView& other) noexcept = default;

    constexpr size_type strlen(const char* str) const noexcept {size_type i = 0; while(str[i++] != '\0'){}; return i;}
    template<size_type N>
    constexpr size_type strlen(const value_type (&str)[N]) const noexcept {size_type i = 0; while((str[i++]) != '\0' || i != N){}; return i;}

    template<size_type N>
    constexpr stringView(const value_type (&str)[N]) noexcept : data_(str),len(strlen<N>(str)) {}

    template<typename T> requires (requires(T t) { t.data(),t.size();})
    constexpr stringView(const T& str) noexcept : data_(str.data()),len(str.size()) {}
    constexpr stringView(const_pointer str,size_type count) noexcept : data_(str),len(count) {}
    constexpr stringView(const_pointer str) noexcept : data_(str),len(strlen(str)) {}

    constexpr stringView& operator =(const stringView& other) noexcept = default; 

    constexpr iterator begin  () const noexcept {return iterator{&data_[0]};} 
    constexpr iterator end    () const noexcept {return iterator{&data_[len] - 1};} 
    constexpr reverse_iterator rbegin () const noexcept {return reverse_iterator{end()};}  
    constexpr reverse_iterator rend () const noexcept {return reverse_iterator{begin()};}  

    constexpr const_reference operator[](size_type idx) const { return data_[idx];}
    constexpr const_reference at        (size_type idx) const { return data_[idx];}

    constexpr const_reference front     () const { return data_[0];}
    constexpr const_reference back      () const { return data_[len - 1];}
    constexpr const_pointer   data      () const { return data_;}
    
    constexpr size_type size  ()const { return len;}
    constexpr size_type length()const { return len;}

    constexpr size_type empty()const { return size() == 0;}

    constexpr stringView substr(size_type pos,size_type count = npos) {
        return {data_ + pos,count > len ? len : count};
    }

    constexpr void remove_prefix(size_type n) { data_ = data_ + n; len -= n;}
    constexpr void remove_suffix(size_type n) { len -= n;}
    constexpr void swap(stringView& other) { stringView temp {*this}; *this = other; other = temp;}

    constexpr size_type find(stringView v,size_type pos = 0) {
        auto temp = this->substr(pos);
        for (const char& c : temp) {
            size_type idx = (&c - data_);
            if (data_[idx] == v.front() && temp.substr(idx,v.size()) == v)  { return idx;}
        }
        return npos;
    }

    constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const noexcept {
        const std::size_t maxCount{(count > this->size() ? this->size() : count)};
        std::size_t idx{0};
        for (;idx < maxCount; idx++) {
            dest[idx] = this->data()[idx + pos];
        }
        return idx;
    };
    constexpr bool strcmp(const stringView& str) const {
        for (const char& c : str) {
            difference_t idx {&c - str.begin()}; 
            if (data_[idx] != c) return false;
        }
        return true;
    };
    constexpr bool operator==(const stringView& rhs) const {
        return len != rhs.size() ? false : strcmp(rhs);
    }
    template<size_type N>
    constexpr bool operator==(const value_type (&rhs)[N]) {
        stringView temp(rhs);
        return len != temp.size() ? false : strcmp(rhs);
    }
    constexpr bool operator==(const_pointer rhs) {
        stringView temp(rhs);
        return len != temp.size() ? false : strcmp(rhs);
    }
};

static_assert([]{
    stringView a  {"hello"};
    stringView b  {a};
    stringView c  {"hello again from world number 3200"};
    char s[8];
    c.copy(s,4,0);
    [[maybe_unused]] char cc = c.back();
    a.remove_prefix(2);
    a.remove_suffix(1);
    b.remove_suffix(2);
    std::size_t idx = c.find("wo",0);
    // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
    return idx;
}() == 17);


class string {
public:
    enum Mode : unsigned int {
        onHeap     = 1 << 0, // Bit 0 (1)  -> Dynamic heap allocation active (Owned)
        Large      = 1 << 1, // Bit 1 (2)  -> Structural layout is Large string
        Literal    = 1 << 2, // Bit 2 (4)  -> Structural layout points to external string view/literal
        View       = Literal,// Alias for clarity when using external buffers/views
        autoResize = 1 << 3, // Bit 3 (8)  -> 0 = Preserve heap cap (Default), 1 = Shrink back to SSO
        noHeap     = 1 << 4, // Bit 4 (16) -> Prevent heap allocations (Truncates on overflow)
        lenExt1    = 1 << 5, 
        Small      = 0       // Value 0    -> Default inline SSO buffer
    };

private:
    struct largeStr {
        char* str = nullptr;
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

public:
    constexpr std::size_t copy(char* dest, std::size_t count, std::size_t pos = 0) const noexcept {
        const std::size_t maxCount{(count > this->size() ? this->size() : count)};
        std::size_t idx{0};
        for (;idx < maxCount; idx++) {
            dest[idx] = this->data()[idx + pos];
        }
        return idx;
    };
    // 1. Default constructor
    explicit constexpr string() noexcept {
        storage.mode = Mode::Small;
        storage.len = 0;
        storage.type.Small = smallStr{};
        storage.type.Small.str[0] = '\0';
    }

    template<size_t N>
    constexpr string(const char (&inStr)[N]) {
        storage.mode = Mode::View | Mode::noHeap;
        storage.type.cExpr = inStr;
        storage.len = static_cast<unsigned int>(N);
    }
    constexpr string(stringView inStr) {
        storage.mode = Mode::View | Mode::noHeap;
        storage.type.cExpr = inStr.data();
        storage.len = static_cast<unsigned int>(inStr.size());
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

    constexpr string& reuseBuffer(stringView inStr) {
        auto& ptr = storage.type.Large;

        std::size_t end = inStr.copy(ptr.str, inStr.size());
        ptr.str[end] = '\0';
        storage.len = static_cast<unsigned int>(end);
        
        return *this;
    }

    constexpr string& allocateBuffer(stringView inStr) {
        auto& ptr = storage.type.Large;
        reserve(inStr.size());
        copy(*(&ptr.str + size()), inStr.size());
        return *this;
    }

    constexpr string& useSBO(stringView inStr) {
        if (storage.mode & Mode::onHeap) {
            delete[] storage.type.Large.str;
        }
        auto& ptr = storage.type.Small;
        unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
        storage.mode = Mode::Small | keepFlags;

        ptr = smallStr{};
        ptr.str[inStr.copy(storage.type.Small.str, inStr.size())] = '\0';
        storage.len = static_cast<unsigned int>(inStr.size());
        return *this;
    }

    
    constexpr string& reserve(size_t newLen) {
        if (newLen < capacity()) return *this;
        // bool reuseHeap = (storage.mode & Mode::onHeap) && (newLen <= storage.type.Large.cap);

        char* newBuffer = nullptr;
        size_t newCap = newLen;
        
        newBuffer = new char[newCap + 1];
        newBuffer[copy(newBuffer, size())] = '\0';
        if (storage.mode & Mode::onHeap) {
            delete[] storage.type.Large.str;
        }
    
        
        storage.type.Large = largeStr{
            .str = newBuffer,
            .cap = newCap
        };
        return *this;
    }

    constexpr string& assign(stringView inStr) {
        if ((storage.mode & Mode::onHeap)) {
            return reuseBuffer(inStr);
        }
        // CASE 2: Fits in Small SSO buffer
        if (inStr.size() <= sMaxStr) {
            return useSBO(inStr);
        } 
        // CASE 3: Needs larger allocation
        return allocateBuffer(inStr);
    }

    constexpr string& append(stringView in) {
        size_t inlen = in.size();
        if (inlen == 0) return *this;

        size_t currentLen = storage.len;
        size_t totalLen = currentLen + inlen;

        if (totalLen > sMaxStr) {
            auto& ptr = storage.type.Large;
            reserve(totalLen);
            in.copy(ptr.str + len(), inlen);

            unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
            storage.mode = Mode::onHeap | Mode::Large | keepFlags;
        } else {
            if (storage.mode & Mode::View) {
                stringView oldLiteral = storage.type.cExpr;
                unsigned int keepFlags = storage.mode & (Mode::autoResize | Mode::noHeap);
                storage.mode = Mode::Small | keepFlags;
                storage.type.Small = smallStr{};
                oldLiteral.copy(storage.type.Small.str, oldLiteral.len);
            }
            char* dest = (storage.mode & Mode::onHeap) ? storage.type.Large.str : storage.type.Small.str;
            in.copy(dest + currentLen, inlen);
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
    constexpr char& operator[](std::size_t idx) {
        if (storage.mode & Mode::Large) return storage.type.Large.str[idx];
        return storage.type.Small.str[idx];
    }
    
    constexpr const char* begin() {return data();} 
    constexpr const char* cbegin() const {return stringView{data(),size()}.begin();} 
    constexpr const char* end() {return data() + storage.len;} 
    constexpr const char* cend() const {return stringView{data(),size()}.end();} 

    constexpr size_t capacity() const noexcept {
        if (storage.mode & Mode::Large) return storage.type.Large.cap;
        return sMaxStr;
    }

    constexpr string& operator=(const char* in)  {
        return assign(stringView(in));
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

        stringView r {"hello world"};
        for (auto i = r.rbegin();i != r.rend();i++) {
            printf("%c", *i);
        }
        printf("\n");
    
    return 0; 
}

// constexpr string sss("hello wssssssss large nee");
constexpr string ss ("hello wssssssss large nee");

static_assert((ss.mode() & string::View) != 0, "yes");

static_assert((1 << 0) == 1);
static_assert((1 << 1) == 2);
static_assert((1 << 2) == 4);
static_assert((1 << 4) == 16);
static_assert((1 << 5) == 32);
static_assert((1 << 6) == 64);
static_assert((1 << 7) == 128);