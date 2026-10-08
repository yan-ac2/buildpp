#include <cstddef>
#include <cstdio>


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

    constexpr size_type strlen(const char* str) const noexcept {
        size_type i = 0; 
        while(str[i] != '\0') { ++i; } 
        return i;
    }

    template<size_type N>
    constexpr stringView(const value_type (&str)[N]) noexcept : data_(str),len(strlen(str)) {}

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
        const size_type maxCount{(count + pos > this->size() ? this->size() : count)};
        const_pointer thisData = data_;
        size_type idx {0};
        for (;idx < maxCount;++idx) {
            dest[idx] = thisData[idx];
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
        CStr   = 0,
        SBO   = 1,
        HEAP   = 2,
        NONE   = 3,
    };

private:
    struct typeIdx {
        unsigned int type {NONE};
        unsigned int len {0};
    };
    struct literal {
        unsigned int type {CStr};
        unsigned int len {0};
        const char* str = nullptr;
    };
    struct large {
        unsigned int type {HEAP};
        unsigned int len {0};
        char* str = nullptr;
        size_t cap = 0;
    };
    
    struct small {
        unsigned int type {SBO};
        unsigned int len {0};
        char str[24]; // 23 usable chars + 1 null terminator
    };

    static constexpr size_t sMaxStr = sizeof(small::str) - 1; // 23 bytes

    union {
        typeIdx stored;
        literal sliteral;
        large Large;
        small Small;
    }; // 24 bytes -> Total sizeof(store) == 32 bytes

public:
    constexpr size_t copy(char* dest, size_t count, size_t pos = 0) const noexcept {
        const size_t maxCount{(count + pos > this->size() ? this->size() : count)};
        const char* thisData = data();
        size_t idx {0};
        for (;idx < maxCount;++idx) {
            dest[idx] = thisData[idx];
        }
        return idx;
    };
    // 1. Default constructor
    explicit constexpr string() noexcept {
        stored.type = CStr;
        stored.len = 0;
        sliteral.str = nullptr;
    }

    template<size_t N>
    constexpr string(const char (&inStr)[N]) {
        stored.type = CStr;
        stored.len = static_cast<unsigned int>(N - 1);
        sliteral.str = inStr;
    }
    constexpr string(stringView inStr) {
        stored.type = CStr;
        stored.len = static_cast<unsigned int>(inStr.size());
        sliteral.str = inStr.data();
    }

    constexpr string& allocateBuffer(stringView inStr) {
        const size_t inSize = inStr.size();
        const bool needExpand = inSize > capacity();
        if (needExpand) {
            reserve(inSize);
            const size_t end = inStr.copy(Large.str, inSize);
            Large.len = static_cast<size_t>(end);
            Large.str[end] = '\0';
        } else {
            const size_t end = inStr.copy(Large.str, inSize);
            Large.len = static_cast<size_t>(end);
            Large.str[end] = '\0';
        }
        return *this;
    }
    
    constexpr string& reserve(size_t newLen) {
        if (newLen <= capacity()) return *this;

        const size_t currentLen = Large.len;
        char* newBuffer = new char[newLen + 1];

        // Copy existing data into the new buffer
        if (data() != nullptr && currentLen > 0) {
            copy(newBuffer, currentLen);
        }
        newBuffer[currentLen] = '\0';
        deallocate();
        // Explicitly retain 'len' and active 'type'
        stored.type = HEAP;
        Large.len = static_cast<unsigned int>(currentLen);
        Large.str = newBuffer;
        Large.cap = newLen;
        newBuffer = nullptr;
        return *this;
    }

    constexpr string& assign(stringView inStr) {
        const size_t inLen = inStr.size();
        const bool fitSSO = inLen <= sMaxStr;
        const bool needExpand = inLen > capacity();
        if (fitSSO) {
            // CASE 1: Fits in Small SSO buffer
            deallocate();
            const size_t end = inStr.copy(Small.str, inLen);
            stored.type = SBO;
            stored.len = static_cast<unsigned int>(end);
            Small.str[end] = '\0';
            return *this;
        } else if (!needExpand) {
            const size_t end = inStr.copy(Large.str, inLen);
            Large.len = static_cast<size_t>(end);
            Large.str[end] = '\0';
            return *this;
        } else {
            // CASE 2: Needs larger allocation
            return allocateBuffer(inStr);
        }
    }

    constexpr string& append(stringView in) {
        const size_t inlen = in.size();
        if (inlen == 0) return *this;

        unsigned int& ptrLen = stored.len;
        const size_t currentLen = ptrLen;
        const size_t totalLen = currentLen + inlen;

        if (totalLen > sMaxStr) {
            reserve(totalLen);
            in.copy(Large.str + currentLen, inlen);
            Large.str[totalLen] = '\0';
        } else {
            if (stored.type == CStr) {
                stringView oldLiteral{sliteral.str, sliteral.len};
                stored.type = SBO;
                Small.len = oldLiteral.copy(Small.str, currentLen);
            }
            in.copy(Small.str + currentLen, inlen);
            Small.str[totalLen] = '\0';
        }

        ptrLen = static_cast<unsigned int>(totalLen);
        return *this;
    }

    constexpr const char* data() const noexcept {
        const bool onHeap = stored.type == HEAP;
        const bool onSBO = stored.type == SBO;
        return onHeap ? Large.str : onSBO ? Small.str : sliteral.str;
    }
    constexpr char& operator[](std::size_t idx) {
        const bool onHeap = stored.type == HEAP;
        return  onHeap ? Large.str[idx] : Small.str[idx];
    }
    constexpr const char& operator[](std::size_t idx) const  {
        const bool onHeap = stored.type == HEAP;
        const bool onSBO = stored.type == SBO;
        return  onHeap ? Large.str[idx] : onSBO ? Small.str[idx] : sliteral.str[idx];
    }
    
    constexpr const char* begin() {return data();} 
    constexpr const char* cbegin() const {return stringView{data(),size()}.begin();} 
    constexpr const char* end() {return data() + size();} 
    constexpr const char* cend() const {return stringView{data(),size()}.end();} 

    constexpr size_t capacity() const noexcept {
        return stored.type == HEAP ? Large.cap : sMaxStr;
    }

    constexpr string& operator=(const char* in)  { return assign(stringView(in)); }

    constexpr auto deallocate() -> void {if (stored.type == HEAP) {delete[] Large.str;}}
    constexpr size_t size() const noexcept { return stored.len;}
    constexpr size_t length() const noexcept  { return size(); }
    constexpr unsigned int mode() const noexcept { return stored.type; }
    constexpr bool isOnHeap() const noexcept { return (stored.type == HEAP) != 0; }

    constexpr ~string() {
        // Views and non-heap instances bypass heap deletion completely
        if (isOnHeap()) {
            delete[] Large.str;
        }
    }
};


int main()
{
        string s ("hello world before");
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        s.reserve(35);
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        s.append(" new char");
        // s.front() = 'f';
        // s.back() = 's';
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // s.reserve(50);
        // s.append(" after append ");
        s = "shit";
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // string c (s);
        // c.append(" copy");
        // printf("%s %zu \n",c.data() , c.size());
        s.append ("hello world from world number");
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello world numbers 3200";
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello again from world number 3200";
        // printf("%s %zu \n",s.data() , s.size());
        s = "small";
        // printf("%s %zu \n",s.data() , s.size());
        s.append(" append");
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
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
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        
        // stringView r {"hello world"};
        // for (auto i = r.rbegin();i != r.rend();i++) {
        //     printf("%c", *i);
        // }
        // printf("\n");
    
    return 0; 
}
