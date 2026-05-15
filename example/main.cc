
#include <cstddef>
#include <glad/glad.h>
import lib;

struct buffer {char buff;};
inline void* operator new(size_t size,buffer* Buff) {return Buff;} 

class App
{
    public:
    mEvent   ev;
    mWindow  win;
    Renderer<"GL"> ren;
    App& init()
    {
        createWindow("win test",100,100,800,600, Flags::WinCenter ,&win);
        ren.init(4,6,&win);
        return *this;
    }
    template<auto... callback>
    App& update()
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            (callback(this),...);
        }   
        return *this;
    }
};

inline void inputUpdate(void* ptr) {
    thread_local App* app = static_cast<App*>(ptr);
    PollEvent(16);
    for (;CheckEvent(&app->win,&app->ev);) {
        switch (app->ev.type)
        {
            case eventType::keyPressed:
            {
                std::printf("%i \n",getKey(&app->ev));
                switch(getKey(&app->ev))
                {
                    case Key::key_escape: {CloseWindow(&app->win); break;}
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

inline void renderUpdate(void* ptr) {
    thread_local App* app = static_cast<App*>(ptr);
    glClearColor(1.f,0.f,0.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    app->ren.update();
}

int main ()
{
    App app;
    app.init();
    app.update<inputUpdate,renderUpdate>();
    return 0;
}