import lib;

class App
{
    Window  _win;
    Event   _ev;
    Gl      _ren;
    public:
    Window* win = &_win;
    Event* ev = &_ev;
    Gl* ren = &_ren;
    int i;
    void init();
    void update();
    void reninit();
    void deinit();
};

void App::init()
{
    std::printf("init \n");
    initWindow(this->win,"win", 800, 600, Flags::WinCenter | Flags::WinOGL);
}

void App::reninit()
{
    ren->init(4, 6,win);
}

void App::update()
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
                        case Key::key_a: {polygonMode(gl::FrontandBack,gl::Line); break;}
                        case Key::key_b: {polygonMode(gl::FrontandBack,gl::Fill); break;}
                        default: break;
                    }
                }
                default: break;
            }
        }
        ren->update(this->win);
        this->i = i <=60 ? i += 1 : 0;
    }   
}

int main ()
{    
    App app;
    app.init();
    app.reninit();
    app.update();
    return 0;
}