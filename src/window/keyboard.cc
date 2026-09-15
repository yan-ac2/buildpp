#include "keyboard.hpp"

#include <type_traits>

// KeyState
constexpr void KeyState::setRawDown(bool down) {
    if (down) kState |= rawKeyDown;
    else      kState &= ~rawKeyDown;
}

constexpr void KeyState::setToggle(bool totalState) {
    if (totalState) kState |= kIsToggled;
    else            kState &= ~kIsToggled;
}

constexpr void KeyState::update() {
    // 1. Keep toggle state safe
    uint8 currentToggle = kState & kIsToggled;

    // 2. Read raw hardware state
    bool isCurrentlyDown = (kState & rawKeyDown) != 0;

    // 3. Start clean, shift the previous frame's isDown into wasDown
    uint8 nextState = None;
    if (kState & kIsDown) {
        nextState |= kWasDown;
    }

    // 4. Set the new frame's isDown bit only if currently held
    if (isCurrentlyDown) {
        nextState |= kIsDown;
    }

    // 5. Calculate if it changed this frame (XOR)
    uint8 changed = ((nextState ^ (nextState >> 1)) & kIsDown) << 2;
    nextState |= changed;

    // 6. Handle toggle flips on transitions
    bool justPressed = (nextState & (kIsDown | kWasDown)) == kIsDown;
    if (justPressed) {
        currentToggle ^= kIsToggled;
    }

    // Merge everything together, preserving the raw state sent by Windows
    kState = nextState | currentToggle | (kState & rawKeyDown);
}

// Basic state getters
constexpr bool KeyState::IsDown()      const { return (kState & kIsDown) != 0; }
constexpr bool KeyState::WasDown()     const { return (kState & kWasDown) != 0; }
constexpr bool KeyState::ChangedDown() const { return (kState & kChangedDown) != 0; }
constexpr bool KeyState::IsToggled()   const { return (kState & kIsToggled) != 0; }
constexpr bool KeyState::RawKeyDown()  const { return (kState & rawKeyDown) != 0; }
// --- Action Helpers ---
constexpr KeyState::operator bool() const { return (kState & kIsDown) != 0; }
// True only on the exact frame the key transitions from UP to DOWN
constexpr bool KeyState::justPressed() const { 
    return (kState & (kIsDown | kWasDown)) == kIsDown; 
}

// True only on the exact frame the key transitions from DOWN to UP
constexpr bool KeyState::justReleased() const { 
    return (kState & (kIsDown | kWasDown)) == kWasDown; 
}

//
//  Keyboard
//
constexpr bool Keyboard::poll(int32 tickMS = 0, int32 dtimeMS = 0) noexcept {
    inputAccumulator += inputAccumulator >= tickMS ? -tickMS : dtimeMS;
    if (inputAccumulator >= tickMS) {
        gatherRawInputs(indices);
        return true;
    }
    return false;
}

template<size_t... Is>
inline constexpr void Keyboard::gatherRawInputs(std::index_sequence<Is...>) noexcept {
    BYTE windowsRawBuffer[256] = { 0 };
    if (GetKeyboardState(windowsRawBuffer)) {
        ((KeysState[Is].setRawDown(((windowsRawBuffer[Is] & 0x80) != 0)),KeysState[Is].update()), ...);
    }
}

constexpr KeyState& Keyboard::operator[](EventType::Key key) {
    using underlying = std::underlying_type_t<decltype(key)>;
    return KeysState[static_cast<underlying>(EventType::toPlatform(key))];
}

#ifdef _WIN32
    constexpr size_t castPlatform(size_t inchar, LPARAM lParam) {
        HKL currentLayout = GetKeyboardLayout(0);
        UINT mappedChar = MapVirtualKeyExA(inchar, 2, currentLayout);
        
        switch(mappedChar) {
            case ' ':  return space;
            case '-':  return minus;
            case '=':  return equal;
            case '`':  return backtick;
            case '[':  return bracket;
            case ']':  return closeBracket;
            case ';':  return semicolon;
            case '\'': return apostrophe;
            case ',':  return comma;
            case '.':  return period;
            case '/':  return slash;
            case '\\': return backSlash;
        }
        switch (inchar) {
            // --- Char ---
            case 'A': return a; case 'B': return b; case 'C': return c;
            case 'D': return d; case 'E': return e; case 'F': return f;
            case 'G': return g; case 'H': return h; case 'I': return i;
            case 'J': return j; case 'K': return k; case 'L': return l;
            case 'M': return m; case 'N': return n; case 'O': return o;
            case 'P': return p; case 'Q': return q; case 'R': return r;
            case 'S': return s; case 'T': return t; case 'U': return u;
            case 'V': return v; case 'W': return w; case 'X': return x;
            case 'Y': return y; case 'Z': return z;
            // --- Control Keys ---
            case VK_BACK:       return backSpace;
            case VK_TAB:        return tab;
            case VK_RETURN:     return enter;
            case VK_PAUSE:      return pause;
            case VK_ESCAPE:     return escape;
            case VK_PRIOR:      return pageUp;  
            case VK_NEXT:       return pageDown;   
            case VK_END:        return end;  
            case VK_HOME:       return home;  
            case VK_LEFT:       return left;  
            case VK_UP:         return up;  
            case VK_RIGHT:      return right;  
            case VK_DOWN:       return down;
            case VK_SNAPSHOT:   return printScreen;  
            case VK_INSERT:     return insert;  
            case VK_DELETE:     return kDelete;
            
            // --- Numpad ---
            case VK_NUMPAD0:   return kp0;  
            case VK_NUMPAD1:   return kp1;  
            case VK_NUMPAD2:   return kp2;  
            case VK_NUMPAD3:   return kp3;  
            case VK_NUMPAD4:   return kp4;  
            case VK_NUMPAD5:   return kp5;  
            case VK_NUMPAD6:   return kp6;  
            case VK_NUMPAD7:   return kp7;  
            case VK_NUMPAD8:   return kp8;  
            case VK_NUMPAD9:   return kp9;  
            case VK_MULTIPLY:  return kpMultiply;
            case VK_ADD:       return kpPlus;
            case VK_SUBTRACT:  return kpMinus;  
            case VK_DECIMAL:   return kpPeriod; 
            case VK_DIVIDE:    return kpSlash;  
            
            // --- Function Keys ---
            case VK_F1:        return F1;  case VK_F13:       return F13;
            case VK_F2:        return F2;  case VK_F14:       return F14;  
            case VK_F3:        return F3;  case VK_F15:       return F15;  
            case VK_F4:        return F4;  case VK_F16:       return F16; 
            case VK_F5:        return F5;  case VK_F17:       return F17; 
            case VK_F6:        return F6;  case VK_F18:       return F18;  
            case VK_F7:        return F7;  case VK_F19:       return F19;  
            case VK_F8:        return F8;  case VK_F20:       return F20; 
            case VK_F9:        return F9;  case VK_F21:       return F21; 
            case VK_F10:       return F10; case VK_F22:       return F22; 
            case VK_F11:       return F11; case VK_F23:       return F23;
            case VK_F12:       return F12; case VK_F24:       return F24;
            
            // --- Modifiers & Locks ---
            case VK_NUMLOCK:   return numLock;  
            case VK_SCROLL:    return scrollLock;
            case VK_CAPITAL:   return capsLock;      // Added VK_CAPITAL mapping
            // case VK_LSHIFT:    return shiftL;  
            // case VK_RSHIFT:    return shiftR; 
            // case VK_LCONTROL:  return controlL;  
            // case VK_RCONTROL:  return controlR;  
            // case VK_RMENU:     return altR;
            // case VK_LMENU:     return altL;   
            case VK_LWIN:      return superL;        // Added Windows key support
            case VK_RWIN:      return superR;
            case VK_APPS:      return menu;          // Added context menu key support

            case VK_CONTROL: {
                // Bit 24 of lParam tells us if it's an extended key (Right Control)
                bool isRight = (lParam & (1 << 24)) != 0;
                return isRight ? controlR : controlL;
            }
            case VK_MENU: { // Alt key
                bool isRight = (lParam & (1 << 24)) != 0;
                return isRight ? altR : altL;
            }
            case VK_SHIFT: {
                // Shift is unique: you map its hardware scan code to find out L vs R
                UINT scancode = (lParam & 0x00FF0000) >> 16;
                UINT exactKey = MapVirtualKeyA(scancode, MAPVK_VSC_TO_VK_EX);
                return (exactKey == VK_RSHIFT) ? shiftR : shiftL;
            }
            default: {
                return keyNULL;
            }
        }
    };

    constexpr size_t toPlatform(size_t engineKey) {
        // 1. Convert our internal engine punctuation enums back to ASCII characters
        UINT asciiChar = 0;
        switch (engineKey) {
            case space:        asciiChar = ' ';  break;
            case minus:        asciiChar = '-';  break;
            case equal:        asciiChar = '=';  break;
            case backtick:     asciiChar = '`';  break;
            case bracket:      asciiChar = '[';  break;
            case closeBracket: asciiChar = ']';  break;
            case semicolon:    asciiChar = ';';  break;
            case apostrophe:   asciiChar = '\''; break;
            case comma:        asciiChar = ',';  break;
            case period:       asciiChar = '.';  break;
            case slash:        asciiChar = '/';  break;
            case backSlash:    asciiChar = '\\'; break;
        }

        // If it matched a dynamic keyboard layout punctuation mark, find its VK code
        if (asciiChar != 0) {
            HKL currentLayout = GetKeyboardLayout(0);
            // Map type 1 translates an ASCII character into a Virtual Key code
            UINT vk = MapVirtualKeyExA(asciiChar, 1, currentLayout);
            if (vk != 0) return vk;
        }

        switch (engineKey) {
            // --- Char ---
            case a: return 'A'; case b: return 'B'; case c: return 'C';
            case d: return 'D'; case e: return 'E'; case f: return 'F'; 
            case g: return 'G'; case h: return 'H'; case i: return 'I';
            case j: return 'J'; case k: return 'K'; case l: return 'L'; 
            case m: return 'M'; case n: return 'N'; case o: return 'O';
            case p: return 'P'; case q: return 'Q'; case r: return 'R';
            case s: return 'S'; case t: return 'T'; case u: return 'U'; 
            case v: return 'V'; case w: return 'W'; case x: return 'X';
            case y: return 'Y'; case z: return 'Z';

            // --- Control Keys ---
            case backSpace:   return VK_BACK;
            case tab:         return VK_TAB;
            case enter:       return VK_RETURN;
            case pause:       return VK_PAUSE;
            case escape:      return VK_ESCAPE;
            case pageUp:      return VK_PRIOR;  
            case pageDown:    return VK_NEXT;   
            case end:         return VK_END;  
            case home:        return VK_HOME;  
            case left:        return VK_LEFT;  
            case up:          return VK_UP;  
            case right:       return VK_RIGHT;  
            case down:        return VK_DOWN;
            case printScreen: return VK_SNAPSHOT;  
            case insert:      return VK_INSERT;  
            case kDelete:     return VK_DELETE;

            // --- Numpad ---
            case kp0:         return VK_NUMPAD0;  
            case kp1:         return VK_NUMPAD1;  
            case kp2:         return VK_NUMPAD2;  
            case kp3:         return VK_NUMPAD3;  
            case kp4:         return VK_NUMPAD4;  
            case kp5:         return VK_NUMPAD5;  
            case kp6:         return VK_NUMPAD6;  
            case kp7:         return VK_NUMPAD7;  
            case kp8:         return VK_NUMPAD8;  
            case kp9:         return VK_NUMPAD9;  
            case kpMultiply:  return VK_MULTIPLY;
            case kpPlus:      return VK_ADD;
            case kpMinus:     return VK_SUBTRACT;  
            case kpPeriod:    return VK_DECIMAL; 
            case kpSlash:     return VK_DIVIDE;  

            // --- Function Keys ---
            case F1:  return VK_F1;  case F13: return VK_F13;
            case F2:  return VK_F2;  case F14: return VK_F14;
            case F3:  return VK_F3;  case F15: return VK_F15;
            case F4:  return VK_F4;  case F16: return VK_F16;
            case F5:  return VK_F5;  case F17: return VK_F17;
            case F6:  return VK_F6;  case F18: return VK_F18;
            case F7:  return VK_F7;  case F19: return VK_F19;
            case F8:  return VK_F8;  case F20: return VK_F20; 
            case F9:  return VK_F9;  case F21: return VK_F21;
            case F10: return VK_F10; case F22: return VK_F22;
            case F11: return VK_F11; case F23: return VK_F23;
            case F12: return VK_F12; case F24: return VK_F24;

            
            // --- Modifiers & Locks ---
            case numLock:    return VK_NUMLOCK;  
            case scrollLock: return VK_SCROLL;
            case capsLock:   return VK_CAPITAL;  
            case shiftL:     return VK_LSHIFT;  
            case shiftR:     return VK_RSHIFT; 
            case controlL:   return VK_LCONTROL;  
            case controlR:   return VK_RCONTROL;  
            case altR:       return VK_RMENU;
            case altL:       return VK_LMENU;  
            case superL:     return VK_LWIN;  
            case superR:     return VK_RWIN;
            case menu:       return VK_APPS;  

            default:         return 0;
        }
    };
#endif