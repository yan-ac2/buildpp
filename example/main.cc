

#include <sstream>
#include <fstream>
#include <omp.h>
import lib;
import lib.RGFW;
import lib.renderer;

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
    Rect box[1000];
    Model floor;
    RGBA red = {255,0,0,255};
    App& init()
    {
        initWindow("win test", 800, 600, Flags::WinCenter,&this->win);
        return *this;
    }
    
    App& reninit()
    {
        ren.init(&win);
        floor.init(".\\floor.obj");
        bool b1,b2,b3;
        for (int i = 0 ; i < 1000 ; i++) {
            box[i].init(50,50, 10 , 10,RGBA{
                static_cast<uint8>(i == 0 ? box[i].color.col[0] = 1 : box[i].color.col[0] = box[i-1].color.col[0] + (b1 ? 3 : -3)),
                static_cast<uint8>(i == 0 ? box[i].color.col[0] = 1 : box[i].color.col[0] = box[i-1].color.col[1] + (b2 ? 2 : -2)),
                static_cast<uint8>(i == 0 ? box[i].color.col[0] = 1 : box[i].color.col[0] = box[i-1].color.col[2] + (b3 ? 3 : -3)),
                255});
            b1 = i % 128 == 0 ? true : false; 
            b2 = i % 255 == 0 ? true : false; 
            b3 = i % 128 == 0 ? true : false; 
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
        // bot.init(&ren.buffer,100,100,50,50,red);
        // bot.draw(20,20);
        // ren.buffer.triangle(100,100,100,200,300,200,red);
        
        #pragma omp for
        for (int i = 0;i < 1000;i += 10) {
            thread_local Rect* b = box + i;
            for (int j = 0;j < 10;j++) {
                if (b[j].x >= win.w - 50) {if(gen.next()){b[j].ey = true;} b[j].ex = false;}
                else if (b[j].x <= 0) {if(gen.next()){b[j].ey = true;} b[j].ex = true;};
                if (b[j].y >= win.h - 50) {if(gen.next()){b[j].ex = true;} b[j].ey = false;}
                else if (b[j].y <= 0) {if(gen.next()){b[j].ex = true;} b[j].ey = true;};
                
                b[j].draw(&ren.buffer,b[j].ex ? b[j].x + 1 : b[j].x - 1, b[j].ey ? b[j].y + 1 : b[j].y - 1);
            }
        }
        // int32 aix = 0;
        // int32 aiy = 0;
        // int32 bix = 100;
        // int32 biy = 100; 
        // int32 cix = 100;
        // int32 ciy = 200;
        // int32 bbminx = min(min(aix,bix),cix); 
        // int32 bbminy = min(min(aiy,biy),ciy); 
        // int32 bbmaxx = max(max(aix,bix),cix); 
        // int32 bbmaxy = max(max(aiy,biy),ciy);
        // int32 bbmidx = (aix+bix+cix) - bbmaxx - bbminx;
        // int32 bbmidy = (aiy+biy+ciy) - bbmaxy - bbminy;
        // for (int32 x = bbminx; x <= bbmaxx;x++) {
        //     for (int32 y = bbminy; y <= bbmaxy;y++) {
        //         // std::printf("min: %d %d max: %d %d mid: %d %d \n",x,y,bbmaxx,bbmaxy,bbmidx,bbmidy);
        //         ren.buffer.line(x,y,bbmidx,bbmidy,red);
        //     }
        // }
        // for (int i = 10; i <= 100; i++) {
        //     line(&ren.buffer, 50, 10, i, 200+i, red);
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
                                    ren.buffer.set(0 + i,0 + y, red);
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