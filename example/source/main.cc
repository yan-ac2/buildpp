import lib;
import lib.renderer;

class App
{
    mWindow  _win;
    mEvent   _ev;
    sRenderer _ren;
    public:
    mWindow* win = &_win;
    mEvent* ev = &_ev;
    sRenderer* ren = &_ren;
    int i;
    RGBA red = {255,255,0,0};
    App& init()
    {
        std::printf("init \n");
        initWindow(this->win,"win", 800, 600, Flags::WinCenter | Flags::WinOGL);
        std::printf("col %hhu",red[0]);
        return *this;
    }
    
    App& reninit()
    {
        ren->init(win);
        return *this;
    }
    
    App& update()
    {
        std::printf("loop \n");
        for (;ShouldClose(this->win) == 0;) 
        {
            PollEvent(16);
            for (;CheckEvent(this->win,this->ev);) {
                switch (this->ev->type)
                {
                    case eventType::keyPressed:
                    {
                        std::printf("%i \n",getKey(this->ev));
                        switch(getKey(this->ev))
                        {
                            case Key::key_escape: {CloseWindow(this->win); break;}
                            case Key::key_a: { clear(ren->buffer, ren->surface->w, win->w, win->h, color); break;}
                            case Key::key_b: { drawRect(ren->buffer, ren->surface->w, 200, 200, 200, 200, red); break;}
                            case Key::key_c: { drawBitmap(ren->buffer, ren->surface->w, icon, 100, 100, 50, 20); break;}
                            case Key::key_d: { drawLine(ren->buffer,ren->surface->w,ren->surface->h, 10, 10, 11, 20,red); break;}
                            default: break;
                        }
                    }
                    default: break;
                }
            }
            ren->update(this->win);
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