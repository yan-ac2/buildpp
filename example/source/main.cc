
import lib;
import lib.renderer;

class App
{
    mWindow  win;
    mEvent   ev;
    sRenderer ren;
    public:
    int i;
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
        return *this;
    }
    void renupdate()
    {
        box.draw(x,y,color2);
        if (x == win.w - 50) {xmax = false;}else if (x == 0) {xmax = true;};
        if (y == win.h - 50) {ymax = false;}else if (y == 0) {ymax = true;};
        xmax ? x++ : x--;
        ymax ? y++ : y--;
        std::printf("%d \n",y);
        ren.update(&this->win);
    }
    App& update()
    {
        int32 x,y = 0;
        bool xmax,ymax = true;
        Rect box;
        box.init(&ren.buffer,50,50);
        std::printf("loop \n");
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
    app.init()
    .reninit()
    .update();
    return 0;
}