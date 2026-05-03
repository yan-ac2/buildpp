import lib;
import lib.renderer;

class App
{
    mWindow  win;
    mEvent   ev;
    sRenderer ren;
    public:
    int i;
    Rect box;
    int32 x,y = 0;
    bool xmax,ymax = true;
    RGBA red = {255,0,0,255};
    App& init()
    {
        std::printf("init \n");
        initWindow(&this->win,"win test", 800, 600, Flags::WinCenter);
        return *this;
    }
    
    App& reninit()
    {
        ren.init(&win);
        box.init(&ren.buffer,50,50);
        return *this;
    }
    inline void renupdate()
    {
        box.draw(x,y,color2);
        if (x == win.w - 50) {xmax = false;}else if (x == 0) {xmax = true;};
        if (y == win.h - 50) {ymax = false;}else if (y == 0) {ymax = true;};
        xmax ? x++ : x--;
        ymax ? y++ : y--;
        ren.update(&this->win);
    }
    void renThread()
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            auto start = std::chrono::high_resolution_clock::now(); 
            renupdate();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> d = end - start; 
            std::printf("%f ms\n",d.count() * 1000);
            std::this_thread::sleep_for(std::ms(16));
        }
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
                            case Key::key_a: { clear(ren.buffer.get(),ren.buffer.width,ren.surface->w,ren.surface->h,color); break;}
                            case Key::key_b: { drawRect(&ren.buffer, 10, 0, 50, 50, color2); break;}
                            case Key::key_c: { 
                                for (int i = 0; i  <win.w + 10; i++) {
                                    for(int y=0; y <100;y++)
                                    ren.buffer.set(0 + i,0 + y, color2);
                                }  
                                break;
                            }
                            case Key::key_d: {  break;}
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


int main ()
{    
    App app;
    app.init().reninit();
    std::jthread ren ([&app]{app.renThread();});
    app.update();
    return 0;
}