
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
#include <meshoptimizer.h>
import lib;

struct buffer {char buff;};
inline void* operator new(size_t size,buffer* Buff) {return Buff;} 

class App
{
    public:
    mEvent   ev;
    mWindow  win;
    Renderer<"software"> ren;
    App& init()
    {
        std::cout << "current location" << std::filesystem::current_path() << "\n"; 
        createWindow("win test",100,100,800,600, Flags::WinCenter ,&win);
        ren.init(&win);
        return *this;
    }
    template<auto... callback>
    App& update()
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            (callback(this),...);
            ren.swapbuffer();
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
    
}
inline void srenderUpdate(void* ptr) {
    thread_local App* app = static_cast<App*>(ptr);
    RGBA red(255,0,0,255);
    
    Rect box[13] {
        {100,100,200,100,red},
        {100,100,300,100,{100,100,0,255}},
        {100,100,400,100,{50,10,80,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
        {100,100,500,100,{50,0,150,255}},
    };
    int x[12] {0} , y[12] {0};
    timeutl times;
    thread_local Rect* pbox = box;
    thread_local int *px = x;
    thread_local int *py = y;
    while(ShouldClose(&app->win) == 0) {
        times.tstart();
        for (int i = 0; i < 12 ;i++) {
            pbox[i].draw(&app->ren,x[i],y[i]);
            if (pbox[i].x == 500 || pbox[i].x == 0) {pbox[i].ex = pbox[i].x==500 ? false : pbox[i].x == 0 ? true : false;}
            if (pbox[i].y == 500 || pbox[i].y == 0) {pbox[i].ey = pbox[i].y==500 ? false : pbox[i].y == 0 ? true : false;}
            px[i] = pbox[i].ex ? 10 : -10, py[i] = pbox[i].ey ? 10 : -10;
        }
        // for (int i = 0; i < floor.faces.size(); i++) {

            // auto [ax,ay] = app->ren.projection(floor.vert(1,0))verts[faces[iface*3+nthvert]];
            // auto [bx,by] = app->ren.projection(floor.vert(i,1));
            // auto [cx,cy] = app->ren.projection(floor.vert(i,2));
            // app->ren.triangle(ax,ay,bx,by,cx,cy,red);
        // }
        app->ren.end();
        times.tend();
        times.sleep(16);
    }
    
}

int main ()
{
    App app;
    app.init();
    std::jthread ren(srenderUpdate,&app);
    app.update<
    inputUpdate
    >();
    ren.join();
    return 0;
}