#include <cstdint>


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

template<typename T,size_t N>
struct taggedArray {
    enum SlotState : uintptr_t {
        VALID       = 0, // 00 -> Active inline value exists
        EMPTY       = 1, // 01 -> Never used
        DEALLOCATED = 2  // 10 -> Tombstone / Recently freed
    };
    T data[N];
    
    taggedArray() {
        for (auto& d : data) {
            std::uintptr_t =
        }
    }
};