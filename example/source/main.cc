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
    RGBA red = {255,0,0,255};
    App& init()
    {
        std::printf("init \n");
        initWindow(this->win,"win test", 800, 600, Flags::WinCenter);
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
                            case Key::key_a: {  break;}
                            case Key::key_b: { drawRect(ren->buffer.get(), ren->buffer.width, 100, 100, 50, 50, color2); break;}
                            case Key::key_c: {
                                for (int i = 0; i <100; i++) {
                                    for(int y=0;y<100;y++)
                                    ren->buffer.set(100 + i, 100 + y, color2);
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