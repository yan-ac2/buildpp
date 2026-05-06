

#include <sstream>
#include <fstream>
#include <random>
import lib;
import lib.renderer;

class FastBoolGenerator {
public:
    FastBoolGenerator() : m_rand(0), m_counter(0) {}

    bool next() {
        if (m_counter == 0) {
            // Fetch a new 64-bit random number
            m_rand = std::uniform_int_distribution<uint64_t>{}(m_gen);
            m_counter = 64;
        }
        // Extract the least significant bit
        bool result = (m_rand & 1);
        m_rand >>= 1; // Shift right for next call
        --m_counter;
        return result;
    }

private:
    std::mt19937 m_gen{std::random_device{}()};
    uint64_t m_rand;
    int m_counter;
};


class Model {
    std::vector<vec3> verts = {};    // array of vertices
    std::vector<int> facet_vrt = {}; // per-triangle index in the above array
public:
    // Model(const std::string filename) {
    //     init(filename);
    // }
    void init(const std::string& filename) {
        std::ifstream in(filename);
        if (in.fail()) return;
        std::string line;
        while (!in.eof()) {
            std::getline(in, line);
            std::istringstream iss(line.c_str());
            std::string type;
            iss >> type;
            char trash;
            if (type == "v ") {
                iss >> trash;
                vec3 v;
                iss >> v.x >> v.y >> v.z;
                verts.push_back(v);
            } else if (type == "f ") {
                int f,t,n, cnt = 0;
                iss >> trash;
                while (iss >> f >> trash >> t >> trash >> n) {
                    facet_vrt.push_back(--f);
                    cnt++;
                }
                if (3!=cnt) {
                    std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                    return;
                }
            }
        }
        std::cerr << "# v# " << nverts() << " f# "  << nfaces() << std::endl;
    }
    int nverts() const { return verts.size(); }
    int nfaces() const { return facet_vrt.size()/3; }

    vec3 vert(const int i) const {
        return verts[i];
    }

    vec3 vert(const int iface, const int nthvert) const {
        return verts[facet_vrt[iface*3+nthvert]];
    }
};

void line(fBuffer* buff,int32 aix,int32 aiy,int32 bix,int32 biy,RGBA col) {
    bool steep = mabs(aix - bix) < mabs(aiy - biy);
    if (steep) {
        swap(&aix, &aiy);
        swap(&bix, &biy);
    }
    if (aix > bix) {
        swap(&aix, &bix);
        swap(&biy, &biy);
    }
    int y = aiy;
    int ierror = 0;
    for (int x = aix; x <= bix;x++) {
        if (steep) buff->set(y,x,col);
        else       buff->set(x,y,col);
        ierror += 2 * mabs(biy - aiy);
        if (ierror > bix - aix) {
            y += biy > aiy ? 1 : -1;
            ierror -= 2 * (bix - aix);
        }
    }
}
std::tuple<int,int> project(vec3 v,mWindow* win) {
    return {(v.x + 1.)* (win->h/2) ,
            (v.y + 1.)* (win->w/2)};
}
class App
{
    mWindow  win;
    mEvent   ev;
    sRenderer ren;
    FastBoolGenerator gen;
    public:
    int i;
    Rect box[500];
    Model floor;
    RGBA red = {255,0,0,255};
    App& init()
    {
        initWindow(&this->win,"win test", 800, 600, Flags::WinCenter);
        return *this;
    }
    
    App& reninit()
    {
        ren.init(&win);
        floor.init(".\\floor.obj");
        for (int i = 0 ; i < 500 ; i++) {
            box[i].init(&ren.buffer,50,50,100 , 100,red);
        }
        return *this;
    }
    inline void renupdate()
    {
        // for (int i = 0; i < floor.nfaces();i++) {
        //     auto [ax,ay] = project(floor.vert(i,0), &win);
        //     auto [bx,by] = project(floor.vert(i,1), &win);
        //     auto [cx,cy] = project(floor.vert(i,2), &win);
        //     std::printf ("ax : %d,ay : %d \n",ax,ay);
        //     line(&ren.buffer,ax,ay,bx,by,red);
        //     line(&ren.buffer,ax,ay,cx,cy,red);
        //     line(&ren.buffer,bx,by,cx,cy,red);
        // }

        // for (int i = 0; i < floor.nverts();i++) {
        //     vec3 v = floor.vert(i);
        //     auto [x,y] = project(v, &win);
        //     ren.buffer.set(x,y,RGBA{0,0,255,0});
        // // }
        // Rect bot;
        // bot.init(&ren.buffer,100,100,50,50);
        // bot.clear().draw(20,20,red);
        // ren.buffer.triangle(100,100,100,200,300,200,red);
        for (int i = 0;i < 500;i += 10) {
            Rect* b = box + i;
            for (int j = 0;j < 10;j++) {
                if (b[j].x == win.w - 50) { if(gen.next()){b[j].ey = true;} b[j].ex = false;}
                else if (b[j].x <= 0) { if(gen.next()){b[j].ey = false;} b[j].ex = true;};
                if (b[j].y == win.h - 50) { if(gen.next()){b[j].ey = true;} b[j].ey = false;}
                else if (b[j].y <= 0) { if(gen.next()){b[j].ex = false;} b[j].ey = true;};
                
                b[j].draw(b[j].ex ? b[j].x + 1 : b[j].x - 1, b[j].ey ? b[j].y + 1 : b[j].y - 1);
            }
        }
        // for (int i = 0; i <= 100; i++) {
        //     line(&ren.buffer, 10, 0, i, 100, red);
        // }
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
                            case Key::key_a: { ren.buffer.clear(); break;}
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