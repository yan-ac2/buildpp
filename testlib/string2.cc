#include <cstddef>
#include <cstdio>
#include <string_view>

template<typename T>
struct char_traits {
    using char_type = T;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;

    using int_type = int;
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;

    static constexpr auto assign(reference c1,reference c2) noexcept -> void { c1 = c2;}
    [[nodiscard]] static constexpr auto assign(pointer ptr, size_type count,char_type c2) noexcept -> pointer { 
        const_pointer end = ptr + count;
        for (;ptr++ < end; *ptr = c2);
        return ptr;
    }
    [[nodiscard]] static constexpr auto eq(char_type a,char_type b) noexcept -> bool {
        return a == b;
    }
    [[nodiscard]] static constexpr auto lt(char_type a,char_type b) noexcept -> bool {
        return a < b;
    }
    [[nodiscard]] static constexpr auto move(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        const_pointer end = src + count;
        for (;src != end;++src) {*dest++ = *src;}
        return dest;
    }
    [[nodiscard]] static constexpr auto copy(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        const_pointer end = src + count;
        for (;src != end;++src) {*dest++ = *src;}
        return dest;
    }
    [[nodiscard]] static constexpr auto compare(const_pointer s1,const_pointer s2,size_type count) noexcept -> bool { 
        const_pointer end = s1 + count;
        for (;s1 != end; ++s1,++s2) {if (*s1 != *s2) {
            return false;
        }};
        return true;
    }
    [[nodiscard]] static constexpr auto length(const_pointer s) noexcept -> size_type {
        size_type idx {0};
        for (;s[idx] != '\0';) {++idx;}
        return idx;
    }
    [[nodiscard]] static constexpr auto find(const_pointer ptr,size_type count,const_reference ch) noexcept -> const_pointer {
        const_pointer begin = ptr + count;
        for (;*begin++ != ch;);
        return begin - 1;
    }
    [[__nodiscard__]] 
    static constexpr auto to_char_type(int_type c) noexcept -> char_type {
        return static_cast<char_type>(c);
    }
    [[__nodiscard__]] 
    static constexpr auto to_int_type(char_type c) noexcept -> int_type {
        return static_cast<int_type>(static_cast<char_type>(c));
    }
    [[__nodiscard__]] 
    static constexpr auto eq_int_type(int_type c1,int_type c2) noexcept -> bool {
        return c1 == c2;
    }
    [[__nodiscard__]] 
    static constexpr auto eof() noexcept -> int_type { return static_cast<int_type>(EOF);}
    [[__nodiscard__]] 
    static constexpr auto not_eof(int_type c) noexcept -> int_type {
    return eq_int_type(c, eof()) ? ~eof() : c;
  }
};

// static_assert([]{
//     auto ss[] = u"Hello";
//     // int e = 101;
//     return char_traits::length(ss) == 5 && char_traits::find(ss, 0, 'e') == (ss + 1) && char_traits::to_int_type(ss[1]) == 101;

// }());

template<typename T>
struct reverse_iterator {
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;

    pointer current;
    constexpr explicit reverse_iterator(pointer ptr) noexcept : current(ptr) {}
    constexpr auto operator*()  noexcept -> reference { return *(current - 1); }
    constexpr auto operator->() noexcept -> pointer { return current - 1; } 
    
    constexpr auto operator++() noexcept -> reverse_iterator& {
        --current;
        return *this;
    }
    constexpr auto operator--() noexcept -> reverse_iterator& {
        ++current;
        return *this;
    }
    
    constexpr auto operator++(int) noexcept -> reverse_iterator {
        reverse_iterator temp = *this; --current;
        return temp;
    }
    constexpr auto operator--(int) noexcept -> reverse_iterator {
        reverse_iterator temp = *this;
        ++current;
        return temp;
    }
    constexpr bool operator==(const reverse_iterator& rhs) const noexcept {
        return current == rhs.current;
    };
};

template<typename cT = char>
struct [[nodiscard]] string_view {
    using char_type = cT;

    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using r_iterator = reverse_iterator<const char_type>;

    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;
    using Traits = char_traits<char_type>;
    private:
    template<typename T>
    [[nodiscard]] static constexpr auto exchange(T& from , T&& val) noexcept {
        T temp = static_cast<T&&>(from);
        from = static_cast<T&&>(val);
        return temp;
    }
    [[nodiscard]] static constexpr auto distance(const_pointer start , const_pointer current) noexcept -> size_type {
        return current - start;
    }
    public:
    static constexpr size_type npos {size_type(-1)};
    const_pointer data_;
    size_type len;

    //=========================================================================
    //                    Constructors and assignment
    //=========================================================================
    constexpr string_view() noexcept : data_(nullptr),len(0)  {}
    constexpr string_view(const string_view& other) noexcept : data_(other.data_),len(other.len) {};
    constexpr string_view(string_view&& other) noexcept : 
    data_(exchange<const_pointer>(other.data_,nullptr)),
    len(exchange<size_type>(other.len,0)) 
    {};

    template<size_type N>
    constexpr string_view(const char_type (&str)[N]) noexcept : data_(str),len(N - 1) {}

    template<typename T> requires (requires(T t) { t.data(),t.size();})
    constexpr string_view(const T& str) noexcept : data_(str.data()),len(str.size()) {}
    constexpr string_view(const_pointer str,size_type count) noexcept : data_(str),len(count) {}
    constexpr string_view(const_pointer str) noexcept : data_(str),len(strlen(str)) {}

    constexpr auto operator =(const string_view& other) noexcept -> string_view& {
        return *this = {other.data(),other.size()};
    }; 
    constexpr auto operator =(string_view&& other) noexcept -> string_view& {
        swap(other);
        return *this;
    };
    //=========================================================================
    //                              Iterators
    //=========================================================================
    
    constexpr auto begin   () const noexcept -> iterator         {return (data_);} 
    constexpr auto end     () const noexcept -> iterator         {return (data_ + len);} 
    constexpr auto cbegin  () const noexcept -> iterator         {return (data_);} 
    constexpr auto cend    () const noexcept -> iterator         {return (data_ + len);} 
    constexpr auto rbegin  () const noexcept -> r_iterator       {return r_iterator{end()};}  
    constexpr auto rend    () const noexcept -> r_iterator       {return r_iterator{begin()};}  
    constexpr auto crbegin () const noexcept -> r_iterator       {return r_iterator{end()};}  
    constexpr auto crend   () const noexcept -> r_iterator       {return r_iterator{begin()};}  
    
    //=========================================================================
    //                           Element access
    //=========================================================================
    
    
    [[nodiscard]] constexpr auto operator[](size_type idx) const -> const_reference { return data_[idx];}
    [[nodiscard]] constexpr auto at        (size_type idx) const -> const_reference { return data_[idx];}
    [[nodiscard]] constexpr auto front () const -> const_reference { return *(data_);}
    [[nodiscard]] constexpr auto back  () const -> const_reference { return *(data_ + len - 1);}
    [[nodiscard]] constexpr auto data  () const -> const_pointer   { return data_;}
    //=========================================================================
    //                              Capacity
    //=========================================================================
    
    [[nodiscard]] constexpr auto size  () const -> size_type { return len;}
    [[nodiscard]] constexpr auto length() const -> size_type { return len;}
    [[nodiscard]] constexpr auto empty () const -> bool { return len == 0;}
    
    //=========================================================================
    //                             Modifiers
    //=========================================================================
    constexpr void remove_prefix(size_type n) { data_ = data_ + n; len -= n;}
    constexpr void remove_suffix(size_type n) { len -= n;}
    
    constexpr void swap(string_view& other) noexcept { 
        string_view temp {*this}; 
        this->data_ = other.data_; 
        this->len   = other.len; 
        other.data_ = temp.data_;
        other.len   = temp.len;
    }
    //=========================================================================
    //                             Operations
    //=========================================================================
    constexpr auto copy(pointer dest, size_type count, size_type pos = 0) const noexcept -> size_type {
        const size_type maxCount{(count + pos > size() ? size() - pos : count)};
        const_pointer thisData = data() + pos;
        return distance(dest,Traits::copy(dest,thisData, maxCount));
    };
    
    [[nodiscard]] constexpr auto substr(size_type pos,size_type count = npos) const noexcept -> string_view {
        return {data_ + pos,count > len ? len : count};
    }

    [[nodiscard]] constexpr auto strlen(const_pointer str) const noexcept -> size_type {
        return Traits::length(str);
    }

    [[nodiscard]] constexpr auto compare(const string_view& str) const noexcept -> bool {
        return Traits::compare(str.data(), data(), size());
    };

    //============================< Find >======================================

    private:
    [[nodiscard]] static constexpr auto find_impl(const_pointer src,size_type ssrc,const_pointer s,size_type ss,size_type pos) noexcept -> size_type {
        const_pointer temp   = (src + pos);
        const_pointer endPtr = (src + ssrc);
        for (;temp != endPtr;++temp) {
            const bool firstEq = Traits::eq(*temp,*s);
            const bool strEq = string_view(temp,ss) == string_view(s,ss);
            if (firstEq && strEq)  {return distance(src,temp);}
        }
        return npos;
    }
    public:
    [[nodiscard]] constexpr auto find(string_view sv,size_type pos = 0) const noexcept -> size_type {
        if(sv.size() > size()) return npos;
        return find_impl(data(),size(),sv.data(),sv.size(),pos);
    }
    [[nodiscard]] constexpr auto find(const_pointer s,size_type pos = 0) const noexcept -> size_type {
        const size_type slen = Traits::length(s);
        if(slen > size()) return npos;
        return find_impl(data(),size(),s,slen,pos);
    }
    [[nodiscard]] constexpr auto find(char_type c,size_type pos = 0) const noexcept -> size_type {
        return find_impl(data(),size(),&c,1,pos);
    }
    //==========================< Find_First_Of >================================
    private:
    template<bool cnd> [[nodiscard]]
    static constexpr auto find_first_of_impl(const_pointer src,size_type ssrc,const_pointer s,size_type ss,size_type pos) noexcept -> size_type {
        const_pointer temp   = (src + pos);
        const_pointer endPtr = (src + ssrc);
        const_pointer send = (s + ss);
        for (;temp != endPtr;++temp) {
            if constexpr (cnd) {
                for(const_pointer p{s};p != send;++p) {
                    const bool charEq = Traits::eq(*p,*temp);
                    if (charEq)  {return distance(src,temp);}
                }
            } else { 
                bool match_found = false;
                for (const_pointer p {s};p != send; ++p) {
                    if (Traits::eq(*p,*temp)) {
                        match_found = true; break;
                    }
                }
                if (!match_found) {
                    return distance(src,temp);
                }
            }
        }
        return npos;
    };
    public:
    [[nodiscard]] constexpr size_type find_first_of(string_view sv,size_type pos = 0) {
        return find_first_of_impl<true>(data(), size(), sv.data(), sv.size(), pos);
    }
    [[nodiscard]] constexpr size_type find_first_of(char_type c,size_type pos = 0) {
        return find_first_of_impl<true>(data(), size(), &c, 1, pos);
    }
    [[nodiscard]] constexpr size_type find_first_not_of(string_view sv,size_type pos = 0) {
        if (sv.size() == 0) return npos;
        return find_first_of_impl<false>(data(), size(), sv.data(), sv.size(), pos);
    }
    [[nodiscard]] constexpr size_type find_first_not_of(const_pointer s,size_type pos = 0) {
        const size_type len = Traits::length(s);
        if (s == 0) return npos;
        return find_first_of_impl<false>(data(), size(),s, len, pos);
    }
    [[nodiscard]] constexpr size_type find_first_not_of(char_type c,size_type pos = 0) {
        return find_first_of_impl<false>(data(), size(),&c, 1, pos);
    }

    //===========================< Find_Last_Of >====================================

    private:
    template<bool cnd> [[nodiscard]]
    static constexpr auto find_last_of_impl(const_pointer src,size_type ssrc,const_pointer s,size_type ss,size_type pos) noexcept -> size_type {
        const_pointer temp    = (src + ssrc - pos - 1);
        const_pointer endPtr  = src;
        const_pointer send = (s + ss);
        for (;temp > endPtr ;--temp) {
            if constexpr (cnd) {
                for (const_pointer p{s};p != send;++p) {
                    const bool charEq = Traits::eq(*p ,*temp);
                    if (charEq)  {
                        return distance(src,temp);
                    }
                }
            } else {
                bool match_found = false;
                for (const_pointer p {s};p != send; ++p) {
                    if (Traits::eq(*p , *temp)) {
                        match_found = true; break;
                    }
                }
                if (!match_found) {
                    return distance(src,temp);
                }
            }
        }
        return npos;
    }
    public:
    [[nodiscard]] constexpr auto find_last_of(string_view sv,size_type pos = 0) const noexcept -> size_type {
        if(sv.empty()) return npos;
        return find_last_of_impl<true>(data(),size(),sv.data(),sv.size(),pos);
    }
    [[nodiscard]] constexpr auto find_last_of(const_pointer s,size_type pos = 0) const noexcept -> size_type {
        const size_type slen = Traits::length(s);
        if(slen == 0) return npos;
        return find_last_of_impl<true>(data(),size(),s,slen,pos);
    }
    [[nodiscard]] constexpr auto find_last_of(char_type c,size_type pos = 0) const noexcept -> size_type {
        return find_last_of_impl<true>(data(),size(),&c,1,pos);
    }

    [[nodiscard]] constexpr auto find_last_not_of(string_view sv,size_type pos = 0) const noexcept -> size_type {
        if(sv.empty()) return npos;
        return find_last_of_impl<false>(data(),size(),sv.data(),sv.size(),pos);
    }
    [[nodiscard]] constexpr auto find_last_not_of(const_pointer s,size_type pos = 0) const noexcept -> size_type {
        const size_type slen = Traits::length(s);
        if (slen == 0) return npos;
        return find_last_of_impl<false>(data(),size(),s,slen,pos);
    }
    [[nodiscard]] constexpr auto find_last_not_of(char_type c,size_type pos = 0) const noexcept -> size_type {
        return find_last_of_impl<false>(data(),size(),&c,1,pos);
    }

    //===============================< Starts_With >====================================

    [[nodiscard]] constexpr auto starts_with(string_view sv) const noexcept -> bool {
        const bool req = sv.size() > size(); 
        if (req) {return false;}
        return string_view{data(),sv.size()} == sv;
    };
    [[nodiscard]] constexpr auto starts_with(const_pointer s) const noexcept -> bool {
        const string_view temp{s};
        const bool req = temp.size() > size(); 
        if (req) {return false;}
        const string_view ttemp{data(),temp.size()};
        return ttemp == temp;
    };

    [[nodiscard]] constexpr auto starts_with(char_type c) const noexcept -> bool {
        if (size() < 1) {return false;}
        return front() == c;
    };
    
    //==============================< Ends_With >=======================================

    [[nodiscard]] constexpr auto ends_with(string_view sv) const noexcept -> bool {
        const size_type ts = sv.size(); 
        const size_type thisSize = size(); 
        const bool req = ts > thisSize; 
        if (req) {return false;}
        const_pointer last = end() - ts;
        return string_view{last,ts} == sv;
    };
    [[nodiscard]] constexpr auto ends_with(const_pointer s) const noexcept -> bool {
        const string_view temp{s};
        const size_type ts = temp.size(); 
        const size_type thisSize = size(); 
        const bool req = ts > thisSize; 
        if (req) {return false;}
        const_pointer last = end() - ts;
        return string_view{last,ts} == temp;
    };
    [[nodiscard]] constexpr auto ends_with(char_type c) const noexcept -> bool {
        if (size() < 1) {return false;}
        return back() == c;
    };

    //==================================================================================
    //                             Operator Overload
    //==================================================================================
    [[nodiscard]] constexpr bool operator==(const string_view& rhs) const {
        return len != rhs.size() ? false : compare(rhs);
    }
    template<size_type N> [[nodiscard]]
    constexpr bool operator==(const char_type (&rhs)[N]) {
        string_view temp(rhs,N - 1);
        return len != temp.size() ? false : compare({rhs,N - 1});
    }
    [[nodiscard]] constexpr bool operator==(const_pointer rhs) {
        string_view temp(rhs);
        return len != temp.size() ? false : compare(rhs);
    }
};

static_assert([]{
    string_view a  {"hello"};
    string_view b  {a};
    string_view c  {"hello again from world number 3200"};
    // string_view d  {"hello.cc"};
    // char s[8];
    // a.copy(s,5,4);
    // [[maybe_unused]] char cc = c.back();
    // a.remove_prefix(2);
    // c.remove_prefix(1);
    // b.remove_suffix(2);
    // std::size_t idx = c.find("world",4);
    // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
    // return idx == 16 && c[idx] == 'w' && s[0] == 'o' && a.starts_with("ll") &&
    // b.starts_with("hel") && c.ends_with("3200") && d.find_first_not_of("hl") == 1
    // && 
    return a == b && c.substr(c.find("again"),5) == "again" && a.starts_with("he");
    // return c.find("world");
}());

template<typename sT = char>
class string {
public:
    using char_type = sT;

    using reference = char_type&;
    using const_reference = const char_type&;
    
    using pointer = char_type*;
    using const_pointer = const char_type*;
    
    using iterator = pointer;
    using const_iterator = const_pointer;
    
    using Traits = char_traits<char_type>;
    using view_type = string_view<char_type>;
    
    using size_type = std::size_t;
    using difference_t = std::ptrdiff_t;
    using uint_type = unsigned int;
private:
    static constexpr size_type SSOSize =  (24 / sizeof(char_type)) - 1; // 23 bytes
    static constexpr size_type sMaxStr =  (24 / sizeof(char_type)) - 1; // 23 bytes
    enum Mode : uint_type {
        SBO   = 0,
        HEAP   = 1,
        NONE   = 2,
    };
    struct large {
        pointer str = nullptr;
        size_t cap = 0;
    };
    struct small {
        char_type str[SSOSize]; // 23 usable chars + 1 null terminator
    };

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
    
    constexpr auto nullterminate(size_type at) noexcept -> void{
        stored[type][at] = '\0'; 
    }
public:
    constexpr auto copy(pointer dest, size_type count, size_type pos = 0) const noexcept -> size_type {
        const size_type maxCount{(count + pos > size() ? size() - pos : count)};
        const_pointer thisData = data() + pos;
        return Traits::copy(dest,thisData, maxCount) - dest;
    };
    // 1. Default constructor
    explicit constexpr string() noexcept : stored{.Small={}},type(SBO),len(0) {}

    constexpr string(view_type inStr) noexcept : 
    stored(inStr.size() < sMaxStr ? storage{.Small={}} : storage{.Large={
        .str= new char_type[inStr.size() + 1],
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
        .str= new char_type[other.stored.Large.cap + 1],
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
        other.type = SBO;
        other.len = 0;
        other.nullterminate(0);
    }
    
    constexpr ~string() {
        deallocate();
    }

    constexpr auto allocateBuffer(view_type inStr) -> string& {
        const size_type inSize = inStr.size();
        const bool needExpand = inSize > capacity();
        auto& ptr = stored.Large;
        if (needExpand) {
            reserve(inSize);
            const size_t end = inStr.copy(ptr.str, inSize);
            len = static_cast<size_type>(end);
            nullterminate(end);
        } else {
            const size_t end = inStr.copy(ptr.str, inSize);
            len = static_cast<size_type>(end);
            nullterminate(end);
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
            nullterminate(end);
        } else {
            const bool needExpand = inLen > capacity();
            auto& ptr = stored.Large;
            if (needExpand) {
                // CASE 2: Needs larger allocation
                reserve(inLen);
            } 
            const size_type end = inStr.copy(ptr.str, inLen);
            len = static_cast<uint_type>(end);
            nullterminate(end);
            
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
            inStr.copy(ptr.str + currentLen, inLen);
            len = static_cast<uint_type>(totalLen);
            nullterminate(totalLen);
        } else {
            const bool needExpand = totalLen > capacity();
            auto& ptr = stored.Large;
            if (needExpand) {
                // CASE 2: Needs larger allocation
                reserve(totalLen);
            } 
            pointer heapData = ptr.str + currentLen;
            inStr.copy(heapData, inLen);
            len = static_cast<uint_type>(totalLen);
            nullterminate(totalLen);
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
    
    constexpr auto operator+=(const_pointer in) -> string& { return append(view_type(in)); }
    constexpr auto operator+=(view_type in)  -> string& { return append(in); }
    constexpr auto operator= (const_pointer in) -> string& { return assign(view_type(in)); }
    constexpr auto operator= (view_type in)  -> string& { return assign(in); }

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
    string ss = static_cast<string<char>&&>(s);
    printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
    printf("%s len: %zu cap: %zu \n",ss.data() , ss.size(),ss.capacity());
    string_view r {"hello world"};
    for (auto i = r.rbegin();i != r.rend();i++) {
        printf("%c", *i);
    }
    printf("\n");
    string_view a  {"hello"};
    string_view b  {a};
    string_view c  {"hello again from world number 3200"};
    string_view d  {"hello"};
    char sw[8];
    a.copy(sw,5,4);
    [[maybe_unused]] char cc = c.back();
    a.remove_prefix(2);
    c.remove_prefix(1);
    b.remove_suffix(2);
    std::size_t idx = c.find("world",4);
    // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
    string t {c};
    t += " world is destroyed";
    printf("%s len: %zu cap: %zu \n",t.data() , t.size(),t.capacity());
    return idx == 16 && c[idx] == 'w' && sw[0] == 'o' && a.starts_with("ll") &&
    c.starts_with("wo") && c.ends_with("3200") && d.find_first_not_of("hl") == 1;
    return 0; 
}
