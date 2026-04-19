
class string {
    struct largeStr {
        char* str;
        size_t cap;
        bool mem;
    };
    struct smallStr {char str[24];};

    size_t len;
    bool large;
    struct store {
        union {
            largeStr Large;
            smallStr Small;
        }type;
        
        store& newLarge(size_t newLen) {
            auto& l = type.Large;
            l.mem = true;
            l.cap = newLen;
            l.str = new char[l.cap];
            return *this;
        }
        store& capLarge(size_t newLen) {
            auto& l = type.Large;
            l.cap = newLen;
            return *this;
        }
        store& delLarge() {
            auto& l = type.Large;
            if (l.mem) {
                printf("delete \n");
                delete [] type.Large.str;
                type.Large.str = nullptr;
            }
            l.mem = false;
            return *this;
        }
    }storage;

    size_t getLen(const char* str) 
    {
        size_t len = 0;
        for (;str[len] != '\0';len++){}
        return len;
    }
    size_t copy(char* to,const char* from) 
    {
        size_t i = 0;
        for (;from[i] != '\0';i++){
            to[i] = from[i];
        }
        return i;
    }
    
    public:

    explicit constexpr string() : storage({.len = 0,.large = false}) {
        storage.type.Small.str[0] = '\0';
    }
    
    constexpr string(const char* inStr) {
        storage.len = getLen(inStr);
        if (storage.len > 23) {
            storage.large = true;
            auto& large = storage.type.Large; 
            storage.capLarge(storage.len).newLarge(storage.len);
            large.str[copy(large.str, inStr)] = '\0';
        } else {
            storage.large = false;
            auto& small = storage.type.Small; 
            small.str[copy(small.str, inStr)] = '\0';
        }
    }
    // constexpr string(string&& other) {
    //     this->storage = other.storage;
    //     //if (other.storage.allocated == storage.yes) { other.storage.delLarge();}
    // }
    constexpr string(const string& other) :storage({.len = other.storage.len,.large = other.storage.large}) {
        if (this->storage.large) {
            this->storage.capLarge(other.storage.type.Large.cap)
            .newLarge(other.storage.len);
            this->storage.type.Large.str[copy(storage.type.Large.str, other.storage.type.Large.str)] = '\0';
        } else {
            this->storage.type.Small.str[copy(storage.type.Small.str, other.storage.type.Small.str)] = '\0';
        }
    }
    
    constexpr string& operator =(const char* inStr) {
        size_t newLen = getLen(inStr);
        
        if (newLen > 23) { // 22 + 1 for null terminator = 23 (your Small size)
            if (storage.large == true) {
                // Already large, check if we can reuse the existing heap buffer
                    if (storage.type.Large.cap < newLen) {
                        storage.delLarge().newLarge(newLen); // Should allocate cap + 1
                    }
                    } else {
                        // Switching from small to large
                        storage.large = true;
                        storage.capLarge(newLen).newLarge(newLen);
                    }
                copy(storage.type.Large.str, inStr);
                storage.type.Large.str[newLen] = '\0';
            } else {
                // Destination is small
                if (storage.large == true) {
                    storage.delLarge();
                }
                storage.large = false;
                copy(storage.type.Small.str, inStr);
                storage.type.Small.str[newLen] = '\0';
            }
        storage.len = newLen;
        return *this;
    }

    string& append(const char* in) {
        size_t inlen = getLen(in);
        size_t totalLen = storage.len + inlen;

        if (totalLen > 23) {
            if (!storage.large) { // Upgrade Small to Large
                smallStr old = storage.type.Small;
                size_t i = 0;
                // Current storage is now large, copy the 'append' part
                storage.type.Large.cap = totalLen;
                storage.newLarge(totalLen);
                char* dest = storage.type.Large.str;
                for (; old.str[i] != '\0'; i++) { dest[i] = in[i]; }
                for (; i < inlen; i++) { dest[storage.len + i] = in[i]; }
                dest[totalLen] = '\0';
            } else { // Already Large
                largeStr& l = storage.type.Large;
                if (l.cap < totalLen) {
                    char* newStr = new char[storage.len + 1]();
                    copy(newStr, l.str);
                    storage.delLarge().newLarge(totalLen);
                }
                for (size_t i = 0; i < inlen; i++) { l.str[storage.len + i] = in[i]; }
                l.str[totalLen] = '\0';
            }
        } else {
            // Stay Small
            char* dest = storage.type.Small.str;
            for (size_t i = 0; i < inlen; i++) { dest[storage.len + i] = in[i]; }
            dest[totalLen] = '\0';
        }
        storage.len = totalLen;
        return *this;
    }
    
    const char* data() const {
        return storage.large ? 
        storage.type.Large.str:
        storage.type.Small.str;
    }
    size_t cap() const {
        return storage.large ? 
        storage.type.Large.cap:
        23;
    }
    ~string() {
        if(storage.large == true) {
            if (storage.type.Large.mem == true)
            {
                printf("delete string\n");
                storage.delLarge();
            }
        }
    }
};
