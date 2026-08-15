#include <cstdint>
#include <cstddef>
#include <iostream>
#include <ranges>


// enum SlotState : uintptr_t {
//     STATE_VALID       = 0, // 00 -> Active inline value exists
//     STATE_EMPTY       = 1, // 01 -> Never used
//     STATE_DEALLOCATED = 2  // 10 -> Tombstone / Recently freed
// };

// template <typename T, size_t N>
// class TaggedArray {
//     // We use a internal storage node cell that pairs the item with state information
//     struct alignas(4) Cell {
//         T value;
//     };

//     // The tag mask system
//     static constexpr uintptr_t STATE_MASK = 0x3;
//     static constexpr uintptr_t CELL_MASK  = ~STATE_MASK;

//     // Pack cell wrapper addresses or custom data handles natively
//     struct Slot {
//         alignas(4) Cell cell;
//         uintptr_t state_flags;
//     };

//     std::array<Slot, N> storage_;

// public:
//     // --- Container Type Aliases ---
//     using value_type = T;
//     using size_type  = size_t;

//     // --- Proxy Element for Read/Write Syntaxes ---
//     class ElementProxy {
//         Slot& slot_ref;
//     public:
//         explicit ElementProxy(Slot& ref) : slot_ref(ref) {}

//         // Assignment: arr[i] = value;
//         ElementProxy& operator=(const T& val) {
//             slot_ref.cell.value = val;
//             slot_ref.state_flags = STATE_VALID;
//             return *this;
//         }

//         // Implicit reading conversion to value type
//         operator T() const {
//             if (slot_ref.state_flags != STATE_VALID) {
//                 throw std::runtime_error("Attempted to read from an unallocated or empty slot!");
//             }
//             return slot_ref.cell.value;
//         }

//         // Metadata inspection
//         SlotState state() const { return static_cast<SlotState>(slot_ref.state_flags & STATE_MASK); }
//         bool is_empty() const { return state() == STATE_EMPTY; }
//         bool is_deallocated() const { return state() == STATE_DEALLOCATED; }
//         bool is_valid() const { return state() == STATE_VALID; }

//         void mark_deallocated() { slot_ref.state_flags = STATE_DEALLOCATED; }
//         void mark_empty() { slot_ref.state_flags = STATE_EMPTY; }
//     };

//     // --- Initialization ---
//     TaggedArray() {
//         for (auto& slot : storage_) {
//             slot.state_flags = STATE_EMPTY;
//         }
//     }

//     // --- Element Access API ---
//     ElementProxy operator[](size_type index) { return ElementProxy(storage_[index]); }
    
//     T operator[](size_type index) const {
//         if (storage_[index].state_flags != STATE_VALID) {
//             throw std::runtime_error("Attempted to read from an unallocated or empty slot!");
//         }
//         return storage_[index].cell.value;
//     }

//     constexpr size_type size() const noexcept { return N; }

//     // --- Tombstone Management API ---
//     size_type tombstone_count() const noexcept {
//         size_type count = 0;
//         for (const auto& slot : storage_) {
//             if (slot.state_flags == STATE_DEALLOCATED) {
//                 ++count;
//             }
//         }
//         return count;
//     }

//     void clear_tombstones() noexcept {
//         for (auto& slot : storage_) {
//             if (slot.state_flags == STATE_DEALLOCATED) {
//                 slot.state_flags = STATE_EMPTY;
//             }
//         }
//     }
// };
template <typename T, std::size_t N>
class taggedArray {
    static constexpr std::size_t BITS_PER_WORD = 64;

    std::uint64_t skipfield[N < 64 ? 1 : (N + BITS_PER_WORD - 1) / BITS_PER_WORD]{0};
    struct element {
        T e;
        constexpr element& set(T value) {e = value; return *this;} 
        constexpr T& get() { return e;} 
        constexpr element& operator =(T rhs) { set(rhs); return *this; }

        constexpr operator T() {return e;}
    };
public:
    element data[N]{}; // Value-initialization compatible with any default-constructible type T

    constexpr taggedArray() {};

    // Mutator to set bit state
    constexpr void changeState(std::size_t idx, bool state) {

        const std::size_t word_idx = idx / BITS_PER_WORD;
        const std::size_t bit_idx  = idx % BITS_PER_WORD;
        const std::uint64_t mask   = 1ULL << bit_idx;

        if (state) {
            skipfield[word_idx] |= mask;
        } else {
            skipfield[word_idx] &= ~mask;
        }
    }

    // Accessor to query bit state
    [[nodiscard]] constexpr bool getState(std::size_t idx) const {
        const std::size_t word_idx = idx / BITS_PER_WORD;
        const std::size_t bit_idx  = idx % BITS_PER_WORD;
        
        return (skipfield[word_idx] & (1ULL << bit_idx)) != 0;
    }

    constexpr element& operator [](size_t idx) const {
        return data[idx];
    }
    
    constexpr element* begin() {return data;}
    constexpr element* end()   {return data + N;}
    constexpr const element* begin() const {return data;}
    constexpr const element* end() const {return data + N;}
};

int main() {
    taggedArray<int, 32> test;
    size_t i = 0;
    for (auto& a : test) {
        a = ++i;
    }
    size_t sum = 0;
    for (auto& a : test) {
        std::cout << a << ", ";
        sum += a;
    }
    std::cout << "\n";
    std::cout << sum << "\n";
}