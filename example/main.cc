
#include <glad/glad.h>
import lib;
struct variantBuffer{char buffer;};
inline void* operator new (size_t size,variantBuffer* p) {return p;}
struct funcRef {
    void (*ptr);
    template<typename T>
    funcRef(T&& fn) {
        ptr = reinterpret_cast<void(*)>(fn);
    }
    template<typename T,typename... Args>
    void run(T&& fn,Args&&... args) {
        auto fns = reinterpret_cast<decltype(fn)>(ptr);
        fns(args...);
    }
};
class App
{
    // FastBoolGenerator gen;
    public:
    mEvent   ev;
    mWindow  win;
    Renderer<"GL"> ren;
    App& init()
    {
        // initWindow("win test", 800, 600, Flags::WinCenter || Flags::WinOGL,win);
        createWindow("win test",100,100,800,600, Flags::WinCenter | Flags::WinOGL,&win);
        ren.init(4,6,&win);
        return *this;
    }
    App& renUpdate() {
        return *this;
    }
    App& update()
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            PollEvent(16);
            for (;CheckEvent(&this->win,&this->ev);) {
                switch (this->ev.type)
                {
                    case eventType::keyPressed:
                    {
                        std::printf("%i \n",getKey(&this->ev));
                        switch(getKey(&this->ev))
                        {
                            case Key::key_escape: {CloseWindow(&this->win); break;}
                            case Key::key_a: { break;}
                            case Key::key_b: { break;}
                            case Key::key_c: { break;}
                            case Key::key_d: { break;}
                            default: break;
                        }
                    }
                    default: break;
                }
            }
        }   
        return *this;
    }
};
void test2(const char* s) {
    std::printf("hello %s",s);
}

int main ()
{    
    funcRef test = {test2};
    test.run(test2,"s");
    App app;
    app.init();
    app.update();
    // ren.join();
    return 0;
}