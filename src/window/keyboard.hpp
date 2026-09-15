
#ifndef LIB_KEYBOARD
#define LIB_KEYBOARD
#include "pch.hpp"
#include "lib.types.hpp"
#include <utility>

namespace keyboard {
    enum Key : uint8 {
        keyNULL = 0,
        escape = '\033',
        backtick = '`',
        k0 = '0', k1 = '1', k2 = '2', k3 = '3', k4 = '4',
        k5 = '5', k6 = '6', k7 = '7', k8 = '8', k9 = '9',
        minus = '-',
        equal = '=',
        equals =  equal,
        backSpace = '\b',
        tab = '\t',
        space = ' ',
        a = 'a', b = 'b', c = 'c', d = 'd', e = 'e', f = 'f', g = 'g', h = 'h', i = 'i', j = 'j', k = 'k', l = 'l',
        m = 'm', n = 'n', o = 'o', p = 'p', q = 'q', r = 'r', s = 's', t = 't', u = 'u', v = 'v', w = 'w', x = 'x',
        y = 'y', z = 'z',        
        period = '.',
        comma = ',',
        slash = '/',
        bracket = '[',
        closeBracket = ']',
        semicolon = ';',
        apostrophe = '\'',
        backSlash = '\\',
        kReturn = '\n',
        enter =  kReturn,
        kDelete = '\177', /* 127 */
        F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17,F18,F19,F20,F21,F22,F23,F24,F25,
        capsLock,shiftL,controlL,altL,superL,shiftR,controlR,altR,superR,
        up,down,left,right,
        insert,menu,end,home,pageUp,pageDown,numLock,
        kpSlash,kpMultiply,kpPlus,kpMinus,kpEqual,
        kpEquals =  kpEqual,
        kp1,kp2,kp3,kp4,kp5,kp6,kp7,kp8,kp9,kp0,
        kpPeriod,kpReturn,
        scrollLock,printScreen,pause,
        world1,world2,
        keyLast = 255 
    };

    
    #ifdef _WIN32
    constexpr size_t castPlatform(size_t inchar, LPARAM lParam = 0);
    constexpr size_t toPlatform(size_t engineKey);
    #endif
    struct KeyState{
        enum Flags : uint8 {
            None         = 0,
            kIsDown      = 1 << 0, // 0000 0001
            kWasDown     = 1 << 1, // 0000 0010
            kChangedDown = 1 << 2, // 0000 0100
            kIsToggled   = 1 << 3, // 0000 1000
            rawKeyDown   = 1 << 4  // 0001 0000
        };
        uint8 kState = KeyState::None;
    
        constexpr void setRawDown(bool down) noexcept;
        constexpr void setToggle(bool totalState) noexcept;
        constexpr void update() noexcept;
    
        // Basic state getters
        constexpr bool IsDown()      const noexcept;
        constexpr bool WasDown()     const noexcept;
        constexpr bool ChangedDown() const noexcept;
        constexpr bool IsToggled()   const noexcept;
        constexpr bool RawKeyDown()  const noexcept;
        // --- Action Helpers ---
        constexpr operator bool() const noexcept;
        // True only on the exact frame the key transitions from UP to DOWN
        constexpr bool justPressed() const noexcept;
        // True only on the exact frame the key transitions from DOWN to UP
        constexpr bool justReleased() const noexcept;
    };
    
    struct Keyboard {
        KeyState KeysState[256] {};
        // const Event* event = nullptr;
        int32 inputAccumulator = 0;
        static constexpr auto indices = std::make_index_sequence<256>{};
    
        // constexpr Keyboard(Event* e) : event(e) {}
        constexpr bool poll(int32 tickMS = 0, int32 dtimeMS = 0) noexcept ;
        template<size_t... Is>
        inline constexpr void gatherRawInputs(std::index_sequence<Is...>) noexcept;
        constexpr KeyState& operator[](Key key);
        
    };

}

#endif
