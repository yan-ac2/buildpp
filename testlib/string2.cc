#include <cstddef>
#include <cstdio>
#include <algorithm>
// #include <cstring>

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

    static constexpr auto assign(reference c1,const_reference c2) noexcept -> void { c1 = c2;}
    static constexpr auto assign(pointer dest, size_type count,char_type c) noexcept -> pointer {
        const size_t end{count};
        for (count = 0;count < end;) {
            assign(dest[count] , c);
            ++count;
        }
        return dest;
    }

    [[nodiscard]] static constexpr auto eq(char_type a,char_type b) noexcept -> bool {
        return a == b;
    }

    [[nodiscard]] static constexpr auto lt(char_type a,char_type b) noexcept -> bool {
        return a < b;
    }

    static constexpr auto copy(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        const size_t end{count};
        for (count = 0;count < end;) {
            assign(dest[count] , src[count]);
            ++count;
        }
        return dest;
    }
    static constexpr auto rcopy(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        for (;count > 0;) {
            assign(dest[count] , src[count]);
            --count;
        }
        return dest;
    }
    static constexpr auto move(pointer dest,const_pointer src,size_type count) noexcept -> pointer { 
        if (dest < src) {
            return copy(dest, src, count);
        } else {
            return rcopy(dest, src, count);
        }
    }


    [[nodiscard]] static constexpr auto compare(const_pointer s1,const_pointer s2,size_type count) noexcept -> int_type { 
        size_t idx {0};
        for (;idx < count;) {
            const bool lc {lt(s1[idx] , s2[idx])};
            const bool hc {lt(s2[idx] , s1[idx])};
            if (lc || hc) return lc ? -1 : hc ? 1 : 0;
            ++idx;
        };
        return 0;
    }

    [[nodiscard]] static constexpr auto length(const_pointer s) noexcept -> size_type {
        size_type idx {0};
        for (;!eq(s[idx], '\0');) {++idx;}
        return idx;
    }
    [[nodiscard]] static constexpr auto find(const_pointer s,size_type count,const_reference ch) noexcept -> const_pointer {
        size_t idx {0};
        for (;idx < count;) {
            if (eq(s[idx],ch)) {
                return s + idx;
            }
            ++idx;
        }
        return nullptr;
    }

    [[__nodiscard__]] 
    static constexpr auto to_char_type(int_type i) noexcept -> char_type {
        return static_cast<char_type>(i);
    }
    [[__nodiscard__]] 
    static constexpr auto to_int_type(char_type c) noexcept -> int_type {
        return static_cast<int_type>(static_cast<char_type>(c));
    }

    [[__nodiscard__]] 
    static constexpr auto eq_int_type(int_type c1,int_type c2) noexcept -> bool {
        return eq(to_char_type(c1) , to_char_type(c2));
    }
    
    [[__nodiscard__]] 
    static constexpr auto eof() noexcept -> int_type { return static_cast<int_type>(EOF);}
    [[__nodiscard__]] 
    static constexpr auto not_eof(int_type c) noexcept -> int_type {
    return eq_int_type(c, eof()) ? ~eof() : c;
  }
};

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
    constexpr bool operator<(const reverse_iterator& rhs) const noexcept {
        return current < rhs.current;
    };
};

template<typename cT = char>
struct string_view {
    using char_type = cT;

    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using r_iterator = reverse_iterator<const char_type>;

    using int_type = int;
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
    constexpr auto operator =(const string_view& other) noexcept -> string_view& {
        return *this = {other.data(),other.size()};
    }; 
    constexpr auto operator =(string_view&& other) noexcept -> string_view& {
        swap(other);
        return *this;
    };

    template<typename T> requires (requires(T t) {
        t.data();
        t.size();
    })
    constexpr string_view(const T& str) noexcept : data_(str.data()),len(str.size()) {}
    constexpr string_view(const_pointer s,size_type count) noexcept : data_(s),len(count) {}
    constexpr string_view(const_pointer s) noexcept : 
    data_(s),len(Traits::length(s)) {}
    constexpr string_view(const_pointer b,const_pointer e) noexcept : 
    data_(b),len(b < e ? e - b : e - b) {}

    //=========================================================================
    //                              Iterators
    //=========================================================================
    
    constexpr auto begin   () const noexcept -> iterator   { return (data_);    }
    constexpr auto cbegin  () const noexcept -> iterator   { return (data_);    }
    constexpr auto rbegin  () const noexcept -> r_iterator { return r_iterator{end()}; }
    constexpr auto crbegin () const noexcept -> r_iterator { return r_iterator{end()}; }
    
    constexpr auto end     () const noexcept -> iterator   { return (data_ + size());    }
    constexpr auto cend    () const noexcept -> iterator   { return (data_ + size());    }
    constexpr auto rend    () const noexcept -> r_iterator { return r_iterator{begin()}; }
    constexpr auto crend   () const noexcept -> r_iterator { return r_iterator{begin()}; }
    
    //=========================================================================
    //                           Element access
    //=========================================================================
    
    [[nodiscard]] constexpr auto operator[](size_type idx) const -> const_reference { return data_[idx];       }
    [[nodiscard]] constexpr auto at        (size_type idx) const -> const_reference { return data_[idx];       }
    [[nodiscard]] constexpr auto front     ()              const -> const_reference { return data_[0];         }
    [[nodiscard]] constexpr auto back      ()              const -> const_reference { return data_[size() - 1];}
    [[nodiscard]] constexpr auto data      ()              const -> const_pointer   { return data_;            }
    //=========================================================================
    //                              Capacity
    //=========================================================================
    
    [[nodiscard]] constexpr auto size   () const noexcept -> size_type { return len;}
    [[nodiscard]] constexpr auto length () const noexcept -> size_type { return len;}
    [[nodiscard]] constexpr auto empty  () const noexcept -> bool { return len == 0;}
    
    //=========================================================================
    //                             Modifiers
    //=========================================================================
    constexpr void remove_prefix(size_type n) noexcept { 
        const size_type nlen {size() - n > size() ? 0 : n}; 
        data_ = data_ + nlen; 
        len -= nlen;
    }
    constexpr void remove_suffix(size_type n) noexcept { 
        const size_type nlen {size() - n > size() ? 0 : n}; 
        len -= nlen;
    }
    
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
        const size_type maxCount{(count + pos > size()) ? (size() - pos) : count};
        const_pointer thisData = data() + pos;
        Traits::copy(dest,thisData, maxCount);
        return maxCount;
    };
    
    [[nodiscard]] constexpr auto substr(size_type pos,size_type count = npos) const noexcept -> string_view {
        const size_type maxCount{(count > size() ? size() - pos : count)};
        const_pointer thisData = data() + pos;
        return {thisData,maxCount};
    }

    [[nodiscard]] constexpr auto compare(const string_view& sv) const noexcept -> int_type {
        const size_type osize = sv.size();
        const size_type tsize = size();
        if(osize != tsize) return tsize < osize ? -1 : 1;
        return Traits::compare(data(),sv.data(), tsize);
    };

    //============================< Find >======================================

    private:
    [[nodiscard]] static constexpr auto find_impl(const_pointer src,size_type ssrc,const_pointer s,size_type ss,size_type pos) noexcept -> size_type {
        const_pointer temp   = (src + pos);
        const_pointer endPtr = (src + ssrc);
        for (;temp != endPtr;++temp) {
            const bool firstEq = Traits::eq(*temp,*s);
            const bool strEq = Traits::compare(temp, s, ss) == 0;
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
        if (ssrc < ss) return npos;
        const_pointer temp   = (src + pos);
        const_pointer endPtr = (src + ssrc);
        const_pointer send   = (s + ss);
        for (;temp > endPtr;++temp) {
            if constexpr (cnd) {
                for(const_pointer p{s};p < send;++p) {
                    const bool charEq = Traits::eq(*p,*temp);
                    if (charEq)  {return distance(src,temp);}
                }
            } else { 
                bool match_found = false;
                for (const_pointer p {s};p < send; ++p) {
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
    [[nodiscard]] constexpr size_type find_first_of(const_pointer s,size_type pos = 0) {
        const size_type len = Traits::length(s);
        if (s == 0) return npos;
        return find_first_of_impl<true>(data(), size(), s, len, pos);
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
        if (ssrc < ss) return npos;
        const_pointer temp    = (src + ssrc - pos - 1);
        const_pointer endPtr  = src;
        const_pointer send    = (s + ss);
        for (;temp > endPtr ;--temp) {
            if constexpr (cnd) {
                for (const_pointer p{s};p < send;++p) {
                    const bool charEq = Traits::eq(*p ,*temp);
                    if (charEq)  {
                        return distance(src,temp);
                    }
                }
            } else {
                bool match_found = false;
                for (const_pointer p {s};p < send; ++p) {
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
    private:
    constexpr auto starts_with_impl(const_pointer lhs,size_type lhssize,const_pointer rhs, size_type rhssize) const noexcept -> bool {
        const bool req = rhssize > lhssize; 
        if (req) {return false;}
        return Traits::compare(lhs,rhs,rhssize) == 0;
    }
    public:

    [[nodiscard]] constexpr auto starts_with(string_view sv) const noexcept -> bool {
        return starts_with_impl(data(),size(),sv.data(),sv.size());
    };
    [[nodiscard]] constexpr auto starts_with(const_pointer s) const noexcept -> bool {
        const size_type slen = Traits::length(s);
        return starts_with_impl(data(),size(),s,slen);
    };

    [[nodiscard]] constexpr auto starts_with(char_type c) const noexcept -> bool {
        if (size() < 1) {return false;}
        return front() == c;
    };
    
    //==============================< Ends_With >=======================================
    private:
    constexpr auto ends_with_impl(const_pointer lhs,size_type lhssize,const_pointer rhs, size_type rhssize) const noexcept -> bool {
        const bool req = rhssize > lhssize; 
        if (req) {return false;}
        const_pointer last = lhs - lhssize;
        return Traits::compare(last,rhs,rhssize) == 0;
    }
    public:
    [[nodiscard]] constexpr auto ends_with(string_view sv) const noexcept -> bool {
        return ends_with_impl(data_ + len,len,sv.data(),sv.size());
    };
    [[nodiscard]] constexpr auto ends_with(const_pointer s) const noexcept -> bool {
        return ends_with_impl(data_ + len,len,s,Traits::length(s));
    };
    [[nodiscard]] constexpr auto ends_with(char_type c) const noexcept -> bool {
        if (size() < 1) {return false;}
        return back() == c;
    };

    //==================================================================================
    //                             Operator Overload
    //==================================================================================
    [[nodiscard]] constexpr auto operator< (const string_view& rhs) const -> bool {
        return compare(rhs) < 0;
    }
    [[nodiscard]] constexpr auto operator<=(const string_view& rhs) const -> bool {
        return compare(rhs) <= 0;
    }
    [[nodiscard]] constexpr auto operator> (const string_view& rhs) const -> bool {
        return compare(rhs) > 0;
    }
    [[nodiscard]] constexpr auto operator>=(const string_view& rhs) const -> bool {
        return compare(rhs) >= 0;
    }
    [[nodiscard]] constexpr auto operator==(const string_view& rhs) const -> bool {
        return compare(rhs) == 0;
    }
    [[nodiscard]] constexpr auto operator< (const_pointer rhs) const -> bool {
        return compare(rhs) < 0;
    }
    [[nodiscard]] constexpr auto operator<=(const_pointer rhs) const -> bool {
        return compare(rhs) <= 0;
    }
    [[nodiscard]] constexpr auto operator> (const_pointer rhs) const -> bool {
        return compare(rhs) > 0;
    }
    [[nodiscard]] constexpr auto operator>=(const_pointer rhs) const -> bool {
        return compare(rhs) >= 0;
    }
    [[nodiscard]] constexpr auto operator==(const_pointer rhs) const -> bool {
        return compare(rhs) == 0;
    }
};


// static_assert([]{
//     using ctraits = char_traits<char>;
//     char ss[] = "Hello";
//     char ss1[] = "hEllo";
//     char ss3[] = "hello";
//     ctraits::move(ss3, ss3, 5);
//     // int e = 101;
//     // return ctraits::compare(ss, ss1, 5)
//     return ss3[4];
// }() == 'h');


static_assert([]{
    // string_view a  {"hello"};
    // string_view b  {"Hello"};
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
    // return a < b;
    return c[c.find_last_not_of("world")] == '0';
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
    static constexpr size_type SSOSize =  (24 / sizeof(char_type)); // 24 bytes
    static constexpr size_type SSOMaxStr =  SSOSize - 1; // 23 bytes
    enum Mode : size_type {
        SBO   = 0,
        HEAP   = 1,
    };
    struct large {
        pointer str = nullptr;
        size_t cap = 0;
    };
    struct small {
        char_type str[SSOSize]; // 23 usable chars + 1 null terminator
    };
    
    size_type len  : 63;
    size_type type : 1 ;
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
    
    constexpr auto nullterminate(size_type at) noexcept -> size_type{
        stored[static_cast<Mode>(type)][at] = '\0';
        return at;
    }
    constexpr auto initSBO(const_pointer s,size_type size) noexcept  {
        small ret {};
        Traits::copy(ret.str, s, size)[size + 1] = '\0';
        return ret;
    }
    constexpr auto initHeap(const_pointer s,size_type size) noexcept  {
        large ret {
            .str= new char_type[size + 1],
            .cap=size,
        };
        Traits::copy(ret.str, s, size)[size + 1] = '\0';
        return ret;
    }

public:
//=========================================================================
//                    Constructors and assignment
//=========================================================================
    explicit constexpr string() noexcept : type(SBO),len(0),stored{.Small=initSBO()} {}

    constexpr string(view_type sv) noexcept : 
    len(sv.size()),
    type(len < SSOMaxStr ? SBO : HEAP),
    stored(type == SBO ? storage{.Small=initSBO(sv.data(),len)} : storage{.Large=initHeap(sv.data(),len)})
    {}
    
    constexpr string(const_pointer s) noexcept : 
    len(Traits::length(s)),
    type(len < SSOMaxStr ? SBO : HEAP),
    stored(type == SBO ? storage{.Small=initSBO(s,len)} : storage{.Large=initHeap(s,len)})
    {}
    constexpr string(const_pointer s,size_type count) noexcept : 
    len(count),
    type(count < SSOMaxStr ? SBO : HEAP),
    stored(count < SSOMaxStr ? storage{.Small=initSBO(s,count)} : storage{.Large=initHeap(s,count)})
    {}

    constexpr string(const string& other) : 
    len(other.len),
    type(other.type),
    stored(other.size() < SSOMaxStr ? 
    storage{.Small=initSBO(other.data(),len)} : 
    storage{.Large=initHeap(other.data(),len)})
    {}

    constexpr string(string&& other) noexcept : 
    len(other.len),
    type(other.type),
    stored(other.stored)
    {
        other.stored = storage{};
        other.type   = SBO;
        other.len    = 0;
    }
    //=========================================================================
    //                           Element access
    //=========================================================================
    
    constexpr auto data() const noexcept -> const_pointer {
        return stored[static_cast<Mode>(type)];
    }
    constexpr auto data() noexcept -> pointer {
        return stored[static_cast<Mode>(type)];
    }
    constexpr auto c_str() const noexcept -> const_pointer{
        return data();
    }
    constexpr auto front() noexcept -> reference {
        return data()[0];
    }
    constexpr auto front() const noexcept -> const_reference {
        return data()[0];
    }
    constexpr auto back() noexcept -> reference {
        return data()[size() - 1];
    }
    constexpr auto back() const noexcept -> const_reference {
        return data()[size() - 1];
    }
    constexpr auto at(std::size_t idx) -> reference {
        return stored[static_cast<Mode>(type)][idx];
    }
    constexpr auto at(std::size_t idx) const -> const_reference {
        return stored[static_cast<Mode>(type)][idx];
    }
    constexpr auto operator[](std::size_t idx) -> reference {
        return stored[static_cast<Mode>(type)][idx];
    }
    constexpr auto operator[](std::size_t idx) const -> const_reference  {
        return stored[type][idx];
    }
    
    //=========================================================================
    //                           Iterators
    //=========================================================================
    constexpr auto begin   () noexcept       -> pointer       {return data();} 
    constexpr auto cbegin  () const noexcept -> const_pointer {return data();} 
    constexpr auto rbegin  () noexcept       -> pointer       {return view_type{data(),size()}.rbegin();} 
    constexpr auto crbegin () const noexcept -> const_pointer {return view_type{data(),size()}.crbegin();}

    constexpr auto end     ()   noexcept       -> pointer       {return data() + size();} 
    constexpr auto cend    ()   const noexcept -> const_pointer {return data() + size();} 
    constexpr auto rend    ()   noexcept       -> pointer       {return view_type{data(),size()}.rend();} 
    constexpr auto crend   ()   const noexcept -> const_pointer {return view_type{data(),size()}.crend();} 
    
    //=========================================================================
    //                           Capacity
    //=========================================================================
    constexpr auto empty   () const noexcept -> bool      { return len == 0;}
    constexpr auto size    () const noexcept -> size_type { return len;}
    constexpr auto length  () const noexcept -> size_type { return len;}
    constexpr auto capacity() const noexcept -> size_type {
        return type == HEAP ? stored.Large.cap : SSOMaxStr;
    }

    constexpr auto shrink_to_fit() noexcept {
        const bool isSBO = type == SBO;
        const bool ltCap = size() < capacity(); 
        if (ltCap && !isSBO) {
            const bool ltSSOCap = size() < SSOMaxStr; 
            const size_type SSODiff = (ltSSOCap ? 0 : size() - SSOMaxStr); 
            auto& ptr = stored.Large;
            pointer oldBuffer = ptr.str;
            const size_type currentLen = size();
            ptr.str = nullptr;
            type = SBO;
            len = nullterminate(view_type(oldBuffer,currentLen).copy(stored[static_cast<Mode>(type)],currentLen - SSODiff));
            if(oldBuffer != nullptr) delete [] oldBuffer;
        }
    }

    constexpr auto reserve(size_type newLen) -> string& {
        const bool isSBO  = type == SBO;
        const bool ltCap  = newLen < SSOMaxStr;
        if (ltCap) {
            if (!isSBO) {
                shrink_to_fit();
            };
            return *this;
        }
        const size_type currentLen = size();
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
        nullterminate(currentLen);
        return *this;
    }

    constexpr auto copy(pointer dest, size_type count, size_type pos = 0) const noexcept -> size_type {
        const size_type maxCount{(count + pos > size() ? size() - pos : count)};
        const_pointer thisData = data() + pos;
        Traits::copy(dest,thisData, maxCount);
        return maxCount;
    };
    
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
            len = nullterminate(end);
        } else {
            const size_t end = inStr.copy(ptr.str, inSize);
            len = nullterminate(end);
        }
        return *this;
    }
    
    
    
    constexpr auto assign(view_type inStr) -> string& {
        const size_type inLen = inStr.size();
        const bool fitSSO = inLen <= SSOMaxStr && type != HEAP;
        if (fitSSO) {
            auto& ptr = stored.Small;
            // CASE 1: Fits in Small SSO buffer
            // type = SBO;
            const size_type end = inStr.copy(ptr.str, inLen);
            len = nullterminate(end);
        } else {
            const bool needExpand = inLen > capacity();
            auto& ptr = stored.Large;
            if (needExpand) {
                // CASE 2: Needs larger allocation
                reserve(inLen);
            } 
            const size_type end = inStr.copy(ptr.str, inLen);
            len = nullterminate(end);
            
        }
        return *this;
    }
    
    constexpr auto append(view_type inStr) -> string& {
        const size_type inLen = inStr.size();
        if (inLen == 0) return *this;
        const size_type currentLen = size();
        const size_type totalLen = currentLen + inLen;
        
        const bool fitSSO = totalLen <= SSOMaxStr && type != HEAP;
        if (fitSSO) {
            auto& ptr = stored.Small;
            // CASE 1: Fits in Small SSO buffer
            Traits::copy(ptr.str + currentLen,inStr.data(),inLen);
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

    
    
    
    constexpr auto operator+=(const_pointer in) -> string& { return append(view_type(in)); }
    constexpr auto operator+=(view_type in)     -> string& { return append(in); }
    constexpr auto operator= (const_pointer in) -> string& { return assign(view_type(in)); }
    constexpr auto operator= (view_type in)     -> string& { return assign(in); }

    constexpr auto deallocate() -> void {if (type == HEAP) {delete[] stored.Large.str;}}
    
    constexpr auto mode()   const noexcept -> uint_type  { return type;}
};

template<size_t N>
struct idx_t {
    union s {
        unsigned char data[N];
    } storage;
};
static_assert(sizeof(string<char>) == 32);
static_assert(string<char>{"hell0 world number 123"}.back() == '3');
static_assert(string<char>{"h"}.reserve(22).size() == 1);
// static_assert(size_t i : 63 {-1} == 1);
int main()
{
    string s ("hello world before");
    printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
    s.reserve(35);
    printf("%s len: %zu cap: %zu \n",s.data() , s.size(),s.capacity());
    s.shrink_to_fit();
    printf("%s len: %zu cap: %zu type: %s \n",s.data() , s.size(),s.capacity(),s.mode()== 0 ? "SSO" : "HEAP");
    s.append(" new char");
    printf("%s len: %zu cap: %zu type: %s \n",s.data() , s.size(),s.capacity(),s.mode()== 0 ? "SSO" : "HEAP");
    s.reserve(21);
    // s.front() = 'f';
    // s.back() = 's';
    printf("%s len: %zu cap: %zu type: %s \n",s.data() , s.size(),s.capacity(),s.mode()== 0 ? "SSO" : "HEAP");
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
    // string_view r {"hello world"};
    // for (auto i = r.end();--i >= r.begin();) {
    //     putc(*i,stdout);
    // }
    // fflush(stdout);
    printf("\n");
    string_view a  {"hello"};
    string_view b  {a};
    string_view c  {"hello again from world number 3200"};
    string_view d  {"hello"};
    char sw[12];
    c.copy(sw,3,4);
    printf("copied %s\n",sw);
    if (char_traits<char>::compare(a.data(),sw,a.size())) printf("same\n"); 
    [[maybe_unused]] char cc = c.back();
    a.remove_prefix(2);
    c.remove_prefix(1);
    b.remove_suffix(2);
    std::size_t idx = c.find("world",4);
    // return (a == "ll") && b == "hel" && stringView(s).strcmp("hell") && 
    string t {c};
    t += " world is destroyed";
    printf("%s len: %zu cap: %zu \n",t.data() , t.size(),t.capacity());
    char test[] = "hello again from world number 3200";
    char test2[38];
    char_traits<char>::copy(test2,test,char_traits<char>::length(test));
    test2[char_traits<char>::length(test)] = '\0';
    printf("copy %s\n",test2);
    char_traits<char>::move(test2,test,char_traits<char>::length(test));
    printf("move %s\n",test2);
    char_traits<char>::move(test2 + 1,test2,char_traits<char>::length(test2));
    printf("move %s\n",test2);
    char_traits<char>::move(test,test2,char_traits<char>::length(test));
    printf("move %s\n",test);
    char test3[32];
    char_traits<char>::move(test3,test,char_traits<char>::length(test));
    printf("move %s\n",test3);
    auto p = char_traits<char>::find(test2,8, 'w');
    if (p) printf("find %c\n", *p);
    char test4[32];
    auto pt = char_traits<char>::assign(test4,31, 'w');
    *(pt + 31) = '\0';
    if (pt) printf("assign %s\n", test4);

    
    
    return idx == 16 && c[idx] == 'w' && sw[0] == 'o' && a.starts_with("ll") &&
    c.starts_with("wo") && c.ends_with("3200") && d.find_first_not_of("hl") == 1;
    return 0; 
}