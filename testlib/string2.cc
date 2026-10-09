#include <cstddef>
#include <cstdio>

struct charTraits {
    using char_type = char;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;

    static constexpr auto assign(reference c1,reference c2) noexcept -> void { c1 = c2;}
    static constexpr auto assign(pointer ptr, size_type count,char_type c2) noexcept -> pointer { 
        const_pointer end = ptr + count;
        for (;ptr++ < end; *ptr = c2);
        return ptr;
    }
    static constexpr auto eq(char_type a,char_type b) noexcept -> bool {
        return a == b;
    }
    static constexpr auto lt(char_type a,char_type b) noexcept -> bool {
        return a < b;
    }
    static constexpr auto move(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        const_pointer end = src + count;
        for (;src++ < end; *dest++ = *src);
        return dest;
    }
    static constexpr auto copy(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        const_pointer end = src + count;
        for (;src++ < end; *dest++ = *src);
        return dest;
    }
    static constexpr auto compare(const_pointer s1,const_pointer s2,size_type count) noexcept -> bool { 
        const_pointer end = s1 + count;
        for (;s1++ < end; s2++) {if (*s1 != *s2) {
            return false;
        }};
        return true;
    }
    static constexpr auto length(const_pointer s) noexcept -> size_type {
        const_pointer begin = s;
        for (;*s != '\0';++s);
        return s - begin;
    }
    static constexpr auto find(const_pointer ptr,size_type count,const_reference ch) noexcept -> const_pointer {
        const_pointer begin = ptr + count;
        for (;*begin++ != ch;);
        return begin - 1;
    }
    [[__nodiscard__]] 
    static constexpr auto to_char_type(int c) noexcept -> char_type {
        return static_cast<char_type>(c);
    }
    [[__nodiscard__]] 
    static constexpr auto to_int_type(char_type c) noexcept -> int {
        return static_cast<int>(static_cast<unsigned char>(c));
    }
    [[__nodiscard__]] 
    static constexpr auto eq_int_type(int c1,int c2) noexcept -> bool {
        return c1 == c2;
    }
    [[__nodiscard__]] 
    static constexpr auto eof() noexcept -> int { return static_cast<int>(EOF);}
    [[__nodiscard__]] 
    static constexpr auto not_eof(int c) noexcept -> int {
    return eq_int_type(c, eof()) ? ~eof() : c;
  }

};

static_assert([]{
    char ss[] {"Hello"};
    // int e = 101;
    return charTraits::length(ss) == 5 && charTraits::find(ss, 0, 'e') == (ss + 1) && charTraits::to_int_type(ss[1]) == 101;

}());

struct stringView {
    using char_type = char;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;
    using Traits = charTraits;
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
    
    static constexpr size_type npos {size_type(-1)};
    const_pointer data_;
    size_type len;

    constexpr stringView() noexcept : data_(nullptr),len(0)  {}
    constexpr stringView(const stringView& other) noexcept = default;

    constexpr size_type strlen(const char* str) const noexcept {
        return Traits::length(str);
    }

    template<size_type N>
    constexpr stringView(const char_type (&str)[N]) noexcept : data_(str),len(N - 1) {}

    template<typename T> requires (requires(T t) { t.data(),t.size();})
    constexpr stringView(const T& str) noexcept : data_(str.data()),len(str.size()) {}
    constexpr stringView(const_pointer str,size_type count) noexcept : data_(str),len(count) {}
    constexpr stringView(const_pointer str) noexcept : data_(str),len(strlen(str)) {}

    constexpr stringView& operator =(const stringView& other) noexcept = default; 

    constexpr iterator begin  () const noexcept {return iterator{data_};} 
    constexpr iterator end    () const noexcept {return iterator{data_ + len - 1};} 
    constexpr reverse_iterator rbegin () const noexcept {return reverse_iterator{end()};}  
    constexpr reverse_iterator rend () const noexcept {return reverse_iterator{begin()};}  

    constexpr const_reference operator[](size_type idx) const { return data_[idx];}
    constexpr const_reference at        (size_type idx) const { return data_[idx];}

    constexpr const_reference front     () const { return data_[0];}
    constexpr const_reference back      () const { return data_[len - 1];}
    constexpr const_pointer   data      () const { return data_;}
    
    constexpr size_type size  ()const { return len;}
    constexpr size_type length()const { return len;}

    constexpr size_type empty()const { return len == 0;}

    constexpr stringView substr(size_type pos,size_type count = npos) {
        return {data_ + pos,count > len ? len : count};
    }

    constexpr void remove_prefix(size_type n) { data_ = data_ + n; len -= n;}
    constexpr void remove_suffix(size_type n) { len -= n;}
    constexpr void swap(stringView& other) { 
        stringView temp {*this}; 
        this->data_ = other.data_; 
        this->len   = other.len; 
        other.data_ = temp.data_;
        other.len   = temp.len;
    }

    constexpr size_type find(stringView v,size_type pos = 0) {
        const_pointer vBegin = v.begin();
        const size_type viewSize = v.size();
        const_pointer temp = data_ + pos;
        const_pointer endPtr = end();
        for (;temp < endPtr;++temp) {
            const bool firstEq = (*temp == *vBegin);
            const bool strEq = stringView(temp,viewSize) == v;
            if (firstEq && strEq)  {const size_type idx = temp - data_; return idx;}
        }
        return npos;
    }
    constexpr size_type find_first_of(stringView v,size_type pos = 0) {
        const_pointer temp = data_ + pos;
        const_pointer endPtr = end();
        auto eq = [&](char_type c) {
            if (v.size() > 1) {
                for (char sc : v) {if (sc == c) return true;}
            } else {
                return v[0] == c;
            }
            return false;
        };
        for (;temp < endPtr;++temp) {
            const bool charEq = eq(*temp);
            if (charEq)  {const size_type idx = temp - data_; return idx;}
        }
        return npos;
    }
    constexpr size_type find_first_of(char_type v,size_type pos = 0) {
        const_pointer temp = data_ + pos;
        const_pointer endPtr = end();
        for (;temp < endPtr;++temp) {
            const bool charEq = v == *temp;
            if (charEq)  {const size_type idx = temp - data_; return idx;}
        }
        return npos;
    }
    constexpr size_type find_first_not_of(stringView v,size_type pos = 0) {
        const_pointer temp = data_ + pos;
        const_pointer endPtr = end();
        auto eq = [&](char_type c) {
            if (v.size() > 1) {
                for (char sc : v) {if (sc != c) return true;}
            } else {
                return v[0] != c;
            }
            return false;
        };
        for (;temp < endPtr;++temp) {
            const bool charEq = eq(*temp);
            if (charEq)  {const size_type idx = temp - data_; return idx;}
        }
        return npos;
    }
    constexpr size_type find_first_not_of(char_type v,size_type pos = 0) {
        const_pointer temp = data_ + pos;
        const_pointer endPtr = end();
        for (;temp < endPtr;++temp) {
            const bool charEq = v != *temp;
            if (charEq)  {const size_type idx = temp - data_; return idx;}
        }
        return npos;
    }

    constexpr auto starts_with(stringView sv) const noexcept -> bool {
        const bool req = sv.size() > size(); 
        if (req) {return false;}
        return stringView{begin(),sv.size()} == sv;
    };
    constexpr auto starts_with(const_pointer s) const noexcept -> bool {
        const stringView temp{s};
        const bool req = temp.size() > size(); 
        if (req) {return false;}
        return stringView{begin(),temp.size()} == temp;
    };
    constexpr auto starts_with(char_type c) const noexcept -> bool {
        return *begin() == c;
    };
    constexpr auto ends_with(stringView sv) const noexcept -> bool {
        const bool req = sv.size() > size(); 
        if (req) {return false;}
        return stringView{end() - sv.size(),sv.size()} == sv;
    };
    constexpr auto ends_with(const_pointer s) const noexcept -> bool {
        const stringView temp{s};
        const size_type ts = temp.size(); 
        const size_type thisSize = size(); 
        const bool req = ts > thisSize; 
        if (req) {return false;}
        const_pointer last = data() + thisSize - ts;
        return stringView{last,ts} == temp;
    };
    constexpr auto ends_with(char_type c) const noexcept -> bool {
        return *(end()) == c;
    };

    constexpr size_t copy(char* dest, size_t count, size_t pos = 0) const noexcept {
        const size_t maxCount{(count + pos > size() ? size() - pos : count)};
        const char* thisData = data();
        size_t idx {0};
        for (;idx < maxCount;++idx) {
            char* destCurrent = dest + idx;
            const char* thisCurrent = thisData + idx + pos;
            *(destCurrent) = *(thisCurrent);
        }
        return idx;
    };

    constexpr bool strcmp(const stringView& str) const {
        const_pointer otherData  = str.data();
        for (size_type idx {0};idx < size();++idx) { 
            if (data_[idx] != otherData[idx]) return false;
        }
        return true;
    };
    constexpr bool operator==(const stringView& rhs) const {
        return len != rhs.size() ? false : strcmp(rhs);
    }
    template<size_type N>
    constexpr bool operator==(const char_type (&rhs)[N]) {
        stringView temp(rhs,N - 1);
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
    stringView d  {"hello"};
    char s[8];
    a.copy(s,5,4);
    [[maybe_unused]] char cc = c.back();
    a.remove_prefix(2);
    c.remove_prefix(1);
    b.remove_suffix(2);
    std::size_t idx = c.find("world",4);
    // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
    return idx == 16 && c[idx] == 'w' && s[0] == 'o' && a.starts_with("ll") &&
    b.starts_with("hel") && c.ends_with("3200") && d.find_first_not_of("hl") == 1;
}());


class string {
public:
    using char_type = char;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using size_type = std::size_t;
    using uint_type = unsigned int;
    using difference_t = std::ptrdiff_t;
    using Traits = charTraits;
    using view_type = stringView;
    
private:
    enum Mode : unsigned int {
        SBO   = 0,
        HEAP   = 1,
        NONE   = 2,
    };
    struct large {
        pointer str = nullptr;
        size_t cap = 0;
    };
    struct small {
        char_type str[24]; // 23 usable chars + 1 null terminator
    };

    static constexpr size_type sMaxStr = sizeof(small::str) - 1; // 23 bytes
    union storage {
        small Small;
        large Large;
        constexpr auto operator[](Mode t) noexcept -> pointer {
            return t == HEAP ? Large.str : Small.str;
        }
        constexpr auto operator[](Mode t) const noexcept -> const_pointer {
            return t == HEAP ? Large.str : Small.str;
        }
    } stored; // 24 bytes -> Total sizeof(store) == 32 bytes
    Mode type {NONE};
    uint_type len  {0};
    
    
public:
    constexpr auto copy(char* dest, size_t count, size_t pos = 0) const noexcept -> size_type 
    {
        const size_type maxCount{(count + pos > size() ? size() - pos : count)};
        const_pointer thisData = data();
        size_type idx {0};
        for (;idx < maxCount;++idx) {
            pointer destCurrent = dest + idx;
            const_pointer thisCurrent = thisData + idx + pos;
            *(destCurrent) = *(thisCurrent);
        }
        return idx;
    };
    // 1. Default constructor
    explicit constexpr string() noexcept : stored{.Small={}},type(SBO),len(0) {}

    constexpr string(view_type inStr) noexcept : 
    stored(inStr.size() < sMaxStr ? storage{.Small={}} : storage{.Large={
        .str= new char[inStr.size() + 1],
        .cap=inStr.size(),
    }}),
    type(inStr.size() < sMaxStr ? SBO : HEAP),
    len(inStr.copy(stored[type], inStr.size()))
    {}
    template<size_type N>
    constexpr string(const char_type (&inStr)[N]) noexcept : string(view_type{inStr,N - 1}) 
    {}
    constexpr string(const string& other) : 
    stored(other.size() < sMaxStr ? storage{.Small={}} : storage{.Large={
        .str= new char[other.stored.Large.cap + 1],
        .cap=other.stored.Large.cap,
    }}), 
    type(other.size() < sMaxStr ? SBO : HEAP), 
    len(other.copy(stored[type], other.len))
    {}
    constexpr string(string&& other) noexcept : 
    stored(other.stored), 
    type(other.type), 
    len(other.len) 
    {
        other.stored.Small.str[0] = '\0';
        other.type = SBO;
        other.len = 0;
    }
    
    constexpr ~string() {
        deallocate();
    }

    constexpr auto allocateBuffer(view_type inStr) -> string& {
        const size_t inSize = inStr.size();
        const bool needExpand = inSize > capacity();
        auto& ptr = stored.Large;
        if (needExpand) {
            reserve(inSize);
            const size_t end = inStr.copy(ptr.str, inSize);
            len = static_cast<size_t>(end);
            ptr.str[end] = '\0';
        } else {
            const size_t end = inStr.copy(ptr.str, inSize);
            len = static_cast<size_t>(end);
            ptr.str[end] = '\0';
        }
        return *this;
    }
    
    constexpr auto reserve(size_type newLen) -> string& {
        if (newLen <= capacity()) return *this;
        const size_type currentLen = static_cast<size_type>(len);
        auto& ptr = stored.Large;
        pointer newBuffer = new char_type[newLen + 1];
        
        // Copy existing data into the new buffer
        if (data() != nullptr && currentLen > 0) {
            copy(newBuffer, currentLen);
            deallocate();
        }
        type = HEAP;
        ptr.str = newBuffer;
        ptr.cap = newLen;
        return *this;
    }
    
    constexpr auto assign(view_type inStr) -> string& {
        const size_type inLen = inStr.size();
        const bool fitSSO = inLen <= sMaxStr && type != HEAP;
        if (fitSSO) {
            auto& ptr = stored.Small;
            // CASE 1: Fits in Small SSO buffer
            // type = SBO;
            const size_type end = inStr.copy(ptr.str, inLen);
            len = static_cast<uint_type>(end);
            ptr.str[end] = '\0';
        } else {
            const bool needExpand = inLen > capacity();
            auto& ptr = stored.Large;
            if (needExpand) {
                // CASE 2: Needs larger allocation
                reserve(inLen);
            } 
            const size_type end = inStr.copy(ptr.str, inLen);
            len = static_cast<uint_type>(end);
            ptr.str[end] = '\0';
        
        }
        return *this;
    }
    
    constexpr auto append(view_type inStr) -> string& {
        const size_type inLen = inStr.size();
        if (inLen == 0) return *this;
        const size_type currentLen = len;
        const size_type totalLen = currentLen + inLen;
        
        const bool fitSSO = totalLen <= sMaxStr && type != HEAP;
        if (fitSSO) {
            auto& ptr = stored.Small;
            // CASE 1: Fits in Small SSO buffer
            const size_type end = inStr.copy(ptr.str + currentLen, inLen);
            len = static_cast<uint_type>(totalLen);
            ptr.str[end] = '\0';
        } else {
            const bool needExpand = inLen > capacity();
            auto& ptr = stored.Large;
            if (needExpand) {
                // CASE 2: Needs larger allocation
                reserve(inLen);
            } 
            char* heapData = ptr.str + currentLen;
            const size_type end = inStr.copy(heapData, inLen);
            len = static_cast<uint_type>(totalLen);
            heapData[end] = '\0';
        }
        return *this;
    }

    constexpr auto data() const noexcept -> const_pointer {
        return stored[type];
    }
    constexpr auto data() noexcept -> pointer {
        return stored[type];
    }
    constexpr auto c_str() const noexcept -> const_pointer{
        return data();
    }
    constexpr auto operator[](std::size_t idx) -> reference {
        return  stored[type][idx];
    }
    constexpr auto operator[](std::size_t idx) const -> const_reference  {
        return  stored[type][idx];
    }
    
    constexpr auto begin()  noexcept -> pointer {return data();} 
    constexpr auto end()    noexcept -> pointer {return data() + size();} 
    constexpr auto cbegin() const noexcept -> const_pointer {return data();} 
    constexpr auto cend()   const noexcept -> const_pointer {return data() + size();} 

    constexpr auto capacity() const noexcept -> size_type {
        return type == HEAP ? stored.Large.cap : sMaxStr;
    }
    
    constexpr auto operator+=(const char* in) -> string& { return append(stringView(in)); }
    constexpr auto operator+=(stringView in)  -> string& { return append(in); }
    constexpr auto operator= (const char* in) -> string& { return assign(stringView(in)); }
    constexpr auto operator= (stringView in)  -> string& { return assign(in); }

    constexpr auto deallocate() -> void {if (type == HEAP) {delete[] stored.Large.str;}}
    constexpr auto size()   const noexcept -> size_type  { return len;}
    constexpr auto length() const noexcept -> size_type  { return len;}
    constexpr auto mode()   const noexcept -> uint_type  { return type;}
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
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // printf("%s %zu \n",s.data() , s.size());
        s = "hello again from world number 3200 sadasdasdsaa";
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // printf("%s %zu \n",s.data() , s.size());
        s = "small";
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        // printf("%s %zu \n",s.data() , s.size());
        s.append("append");
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
        string ss = static_cast<string&&>(s);
        printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
        printf("%s len: %zu cap: %zu \n",ss.data() , ss.size(),ss.capacity());
        // stringView r {"hello world"};
        // for (auto i = r.rbegin();i != r.rend();i++) {
        //     printf("%c", *i);
        // }
        // printf("\n");
        stringView a  {"hello"};
        stringView b  {a};
        stringView c  {"hello again from world number 3200"};
        stringView d  {"hello"};
        char sw[8];
        a.copy(sw,5,4);
        [[maybe_unused]] char cc = c.back();
        a.remove_prefix(2);
        c.remove_prefix(1);
        b.remove_suffix(2);
        std::size_t idx = c.find("world",4);
        // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
        return idx == 16 && c[idx] == 'w' && s[0] == 'o' && a.starts_with("ll") &&
        b.starts_with("hel") && c.ends_with("3200") && d.find_first_not_of("hl") == 1;
    
    return 0; 
}
