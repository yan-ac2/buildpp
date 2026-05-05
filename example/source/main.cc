
import lib;
import lib.renderer;

class App
{
    mWindow  win;
    mEvent   ev;
    sRenderer ren;
    public:
    int i;
    Rect box[500];
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
        for (int i = 0 ; i < 500 ; i++) {
            box[i].init(&ren.buffer,50,50,2 * (i + 1) , 2 * (i + 1));
        }
        return *this;
    }
    inline void renupdate()
    {
        for (auto i = 0;i < 500;i++) {
            if (box[i].x == win.w - 50) {if(i % 4 == 1){box[i].ey = true;} box[i].ex = false;}else if (box[i].x == 0) {if(i % 3 == 1){box[i].ey = false;} box[i].ex = true;};
            if (box[i].y == win.h - 50) {if(i % 5 == 1){box[i].ey = false;} box[i].ey = false;}else if (box[i].y == 0) {if(i % 2 == 1){box[i].ex = true;} box[i].ey = true;};
            box[i].draw(box[i].ex ? box[i].x + 2 : box[i].x - 2,box[i].ey ? box[i].y + 2 : box[i].y - 2,color2);
        }
        ren.update(&this->win);
    }
    void renThread()
    {
        timeutl times;
        for (;ShouldClose(&this->win) == 0;) 
        {
            times.tstart(); 
            renupdate();
            times.tend(); 
            times.sleep(16);
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
                            case Key::key_b: { break;}
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
    ren.join();
    return 0;
}