#ifndef LIB_TYPES
#define  LIB_TYPES

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <array>
#include <algorithm>
#include <span>
#include <utility>

using f32       = float;
using f64       = double;
using int8      = char;
using uint8     = std::uint8_t;
using int16     = std::int16_t;
using uint16    = std::uint16_t;
using int32     = std::int32_t;
using uint32    = std::uint32_t;
using int64     = std::int64_t;
using uint64    = std::uint64_t;
using size_t    = decltype(sizeof(0));
using ptrdiff   = decltype(static_cast<int32*>(0) - static_cast<int32*>(0));

    template <size_t Num>
    struct Swizzle_impl {
        size_t num[Num];
        
        constexpr Swizzle_impl(const char (&input)[Num + 1]) : num{} {
            for (size_t i = 0; i < Num; ++i) {
                char c = input[i];

                num[i] = (c == 'x' ? 0 :
                        c == 'y' ? 1 : 
                        c == 'z' ? 2 :
                        c == 'w' ? 3 : 4);                
            }
        }
        constexpr size_t operator[](size_t i) const {
            return num[i];
        }
        constexpr size_t size() const { return Num; }
        constexpr bool in_bound(int m) const { 
            for (const int& i : num) {
                if (i > m - 1) return false;
            }
            return true;
        }
    };
    template <size_t Num>
    Swizzle_impl(const char(&)[Num]) -> Swizzle_impl<Num - 1>;

    template<size_t Max>
    struct index_guard {
        size_t idx {};
        constexpr index_guard(size_t in) {
            assert(in > Max);
            idx = in;
        }
        constexpr operator size_t() {return idx;}
    };

    template <size_t N>
    struct bitBool {
        // 32 bytes * 8 bits = 256 bits total capacity
        uint8 bit[N] = {0};

        // Helper proxy class to allow writing: myBits[10] = true;
        struct BitReference {
            uint8& byteRef;
            uint8 mask;

            // Automatically implicitly converts to bool when reading: bool value = myBits[10];
            operator bool() const {
                return (byteRef & mask) != 0;
            }

            // Handles writing: myBits[10] = true;
            BitReference& operator=(bool value) {
                if (value) byteRef |= mask;
                else       byteRef &= ~mask;
                return *this;
            }
        };

        // Non-const operator[]: supports both reading and writing
        BitReference operator[](size_t IDX) {
            size_t bytePos = IDX / 8; // Find which of the 32 bytes holds the bit
            size_t bitPos  = IDX % 8; // Find the bit offset inside that byte
            
            uint8 mask = static_cast<uint8>(1 << bitPos);
            return BitReference{ bit[bytePos], mask };
        }

        // Const operator[]: for when the bitBool instance is read-only
        bool operator[](size_t IDX) const {
            size_t bytePos = IDX / 8;
            size_t bitPos  = IDX % 8;
            return (bit[bytePos] & (1 << bitPos)) != 0;
        }
    };

    template <typename T,size_t Len>
    struct Vec {
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using const_reference = const T&;
        using const_pointer = const T*;
        using tuple_size = std::integral_constant<size_t, Len>;
        
        using vec_index_seq = std::make_index_sequence<Len>;
        template<size_t N>
        using vec_util = Swizzle_impl<N>;

        T m_elem[Len];

        [[nodiscard]] constexpr reference operator [](size_t idx) noexcept {
            return m_elem[idx];
        }
        [[nodiscard]] constexpr const_reference operator[](size_t idx) const noexcept {
            return m_elem[idx];
        }
        

        template <vec_util s> requires (s.size() > 1 && s.in_bound(Len))
        [[nodiscard]] constexpr std::array<value_type, s.size()> swizzle() const noexcept {
            constexpr auto seq = std::make_index_sequence<s.size()>{};
            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::array<value_type, s.size()>{m_elem[s.num[I]]...};
            }(seq); 
        }
        template <vec_util s> requires (s.size() > 1 && s.in_bound(Len))
        [[nodiscard]] constexpr Vec<pointer,s.size()> swizzle_ptr() & noexcept {
            constexpr auto seq = std::make_index_sequence<s.size()>{};
            return [this]<size_t... I>(std::index_sequence<I...>) {
                return Vec<pointer,s.size()>{&m_elem[s.num[I]]...};
            }(seq); 
        }
        [[nodiscard]] constexpr Vec operator +(const Vec& other) noexcept {
            return [&]<size_t... I>(std::index_sequence<I...>) constexpr {
                return Vec{((m_elem[I] += other.m_elem[I]),...)};
            }(vec_index_seq{});
        }
        [[nodiscard]] constexpr Vec& operator +=(const Vec& other) noexcept {
            [&]<size_t... I>(std::index_sequence<I...>) constexpr {
                ((m_elem[I] += other.m_elem[I]),...);
            }(vec_index_seq{});
            return *this;
        }

        [[nodiscard]] constexpr pointer data() {return m_elem;}
        [[nodiscard]] constexpr const_pointer data() const {return m_elem;}
        [[nodiscard]] constexpr size_t size() const {return Len;}

        [[nodiscard]] constexpr pointer begin() {return m_elem;}
        [[nodiscard]] constexpr pointer end() {return m_elem + Len;}
        [[nodiscard]] constexpr const_pointer begin() const {return m_elem;}
        [[nodiscard]] constexpr const_pointer end() const {return m_elem + Len;}

        template<size_t I>
        [[nodiscard]] constexpr reference get() {static_assert(I < Len, "out of bound"); return m_elem[I];}
        template<size_t I>
        [[nodiscard]] constexpr const_reference get() const {static_assert(I < Len, "out of bound");return m_elem[I];}
        
        [[nodiscard]] constexpr bool operator==(const Vec&) const = default;
        [[nodiscard]] constexpr bool operator==(std::span<const value_type> other) const {
            return (other.size() > Len) ? false :
            std::equal(begin(),end(),other.begin());
        };
        
        template<size_t offset,size_t Count = Len>
        [[nodiscard]] constexpr bool eq_from(std::span<const value_type> other) const {
            if constexpr (Count != Len) {return false;}
            return (offset > other.size() || Len > (other.size() - offset)) ? false :
            (*this == other.subspan(offset,Len));
        }
        [[nodiscard]] constexpr bool eq_from(size_t offset,std::span<const value_type> other) const {
            return (offset > other.size() || Len > (other.size() - offset)) ? false : 
            (*this == other.subspan(offset,Len));
        }
    };

    template<typename T> using Vec2 = Vec<T,2>;
    template<typename T> using Vec3 = Vec<T,3>;
    template<typename T> using Vec4 = Vec<T,4>;

    
    struct RGBA {
        uint32 data {0};
        constexpr RGBA (uint8 r,uint8 g,uint8 b,uint8 a) 
        : data((r << RGBA::r) | (g << RGBA::g) | (b << RGBA::b) | (a << RGBA::a)) {}
        
        constexpr RGBA (std::span<const uint8> arr)
        : data((arr[0] << r) | (arr[1] << g) | (arr[2] << b) | (arr[3] << a)) {}
        constexpr uint8 red() const   { return (data >> r) & 0xFF;}
        constexpr uint8 green() const { return (data >> g) & 0xFF;}
        constexpr uint8 blue() const  { return (data >> b) & 0xFF;}
        constexpr uint8 alpha() const { return (data >> a) & 0xFF;}
        constexpr std::array<uint8, sizeof(data)> to_array() {
            return std::array{red(),green(),blue(),alpha()};
        }
        
        struct bitRef {
            RGBA& parent;
            uint32 Shift;

            [[nodiscard]] constexpr operator uint8_t() const noexcept {
                return static_cast<uint8_t>((parent.data >> Shift) & 0xFFU);
            }
            [[nodiscard]] constexpr bool operator ==(const auto& rhs) const noexcept {
                return static_cast<uint8_t>((parent.data >> Shift) & 0xFFU) == rhs;
            }

            constexpr bitRef& operator=(uint8_t value) noexcept {
                parent.data = (parent.data & ~(0xFFU << Shift)) | 
                (static_cast<uint32_t>(value) << Shift);
                return *this;
            }

            constexpr bitRef& operator+=(uint8_t value) noexcept {
                return *this = static_cast<uint8_t>(*this + value);
            }
            
            constexpr bitRef& operator-=(uint8_t value) noexcept {
                return *this = static_cast<uint8_t>(*this - value);
            }
        };
        constexpr bitRef operator[](size_t index) {
            return {*this,get_col[index]};
        };
        constexpr uint8 operator[](size_t index) const {
            return (data >> get_col[index]) & 0xFF;
        };
        
        template<size_t count>
        constexpr size_t copy(uint8* to) {
            constexpr auto seq = std::make_index_sequence<count>{};
            return [to]<size_t... I>(const RGBA& col,std::index_sequence<I...>) {
                ((*(to + I) = col[I]),...);
                return count;
            }(*this,seq);
        };

        enum channel {
            r = 24,g = 16,b = 8,a = 0,
        };
        static constexpr uint32 get_col[] {r,g,b,a};
    };

    static_assert([] {
        RGBA col {Vec4<uint8>{1,2,3,12}};
        // auto [r,g,b,a] = RGBA{2,4,5,6}.to_array();
        return 12 == col[3];
    }(), "");


    template<typename T>
    struct mat {
        using member = T;

    };



static_assert([] {
    Vec3<int32> v {1,2,3};
    // Vec3<i32> v2 {1,2,3};
    std::array<int32,7> a {1,1,1,1,1,2,3};
    // auto s = v2.swizzle_ptr<"xxx">();
    return v.eq_from<4>(a);
}() == true, "");

#endif