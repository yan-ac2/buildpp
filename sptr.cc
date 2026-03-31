#include <cstring>
#include <iostream>

template<typename T,T v>
struct integral_const{
    static constexpr T value = v;
    using value_type = T;
    using type = integral_const;
    constexpr operator value_type() const { return value; }
    constexpr value_type operator()() const { return value; }
};
template<bool T = true>  struct true_t : integral_const<bool, T> {};
template<bool T = false> struct false_t : integral_const<bool, T> {};
template<typename T>     struct isBool : false_t<> {};
template<>               struct isBool<bool> : true_t<> {};

template<typename T>     struct isPtr : false_t<> {};
template<typename T>     struct isPtr<T*> : true_t<> {};

template<typename T, typename U> struct is_same_t       {static constexpr bool value = false_t<>::value;};
template<typename T>             struct is_same_t<T, T> {static constexpr bool value = true_t<>::value;};

template<typename T> struct remove_ptr           { using type = T;};
template<typename T> struct remove_ptr<T*>       { using type = T;};
template<typename T> struct remove_ptr<T* const> { using type = T;};

template<typename F>                   struct isFn : false_t<> {};
template<typename F, typename... Args> struct isFn<F(Args...)> : true_t<> {};


template<typename F>                   struct isFn_Ptr : false_t<> {};
template<typename F,typename... Args>  struct isFn_Ptr<F(*)(Args...)> : true_t<> {};

static_assert(isFn<remove_ptr<int(*)()>::type>::value, "is fucntion");
//static_assert(is_bool<bool>::value, "is bool" );
static_assert(is_same_t<remove_ptr<int(*)()>::type,int()>::value, "is true");
//static_assert(is_fn<int>::value, "is fn");

template<typename T>
class sPtr
{
    T* ptr;
    int* ref;

    constexpr void cleanup()
    {
        if (ref != nullptr) {
            (ref)--;
            if (ref == 0) {
                delete ptr;
                delete ref;
                ptr = nullptr;
                ref = nullptr;
                
            }
        }
    }
    public:
    
    explicit constexpr sPtr(T* p = nullptr) : ptr(p)
    {
        if (ptr) { ref = new int(1);}
        else { ref = nullptr; }
    }
    constexpr sPtr(const sPtr& other) : ptr(other.ptr), ref(other.ref) {
        if (ref) {(*ref)++;}
    }
    
    constexpr sPtr& operator=(const sPtr& other) {
        if (this != &other) cleanup();
        ptr = other.ptr;
        ref = other.ref;
        if (ref != nullptr) {
            (*ref)++;
        }
        return *this;
    }

    constexpr sPtr(sPtr&& other) noexcept : ptr(other.ptr), ref(other.ref) {
        other.ptr = nullptr;
        other.ref = nullptr;
    }
    constexpr sPtr& operator=(sPtr&& other) {
        if (this != other) {   
            cleanup();
            ptr = other.ptr;
            ref = other.ref;
            other.ptr = nullptr;
            other.ref = nullptr;
        }
        return *this;
    }
    constexpr T& operator*() {return *ptr;}
    constexpr T* operator->() {return ptr;}
    constexpr int use_count() { return ref ? *ref : 0; }
    constexpr T* get() { return ptr; }
    ~sPtr() { cleanup(); }
};

class fn
{
    void(*_fn)();
    public:
    template<typename Fn> requires (isPtr<Fn>::value && isFn_Ptr<Fn>::value)
    constexpr fn(Fn&& f) 
    {
        std::memcpy(&_fn,&f,8);
    }
    template<typename... Args>
    constexpr fn& run(auto&& f, Args&&... args) {
        auto fn = reinterpret_cast<decltype(f)>(_fn);
        fn(std::forward<Args>(args)...);
        return *this;
    }
};
int test()
{
    return 1;
};

int main ()
{
    sPtr<int> sptr(new int(5));
    fn f(&test);

    std::cout << *sptr << " sizeof ptr " << sizeof(sptr) << std::endl;
    std::cout << sptr.use_count() << std::endl;
    int i = *sptr;
    std::cout << sptr.use_count() << std::endl;
    std::cout << i << std::endl;
    *sptr = 10; 
    sPtr<int> sptr2 = sptr;
    std::cout << sptr.use_count() << std::endl;
    std::cout << *sptr2 << " " << sptr2.use_count() << std::endl;
    int s = *sptr;
    std::cout << sptr.use_count() << std::endl;
    std::cout << s << std::endl;
    return 0;
}