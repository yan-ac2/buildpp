
// #include <chrono>
// #include <tuple>
// #include <iostream>

#include <cstddef>
#include <bit>
#include <functional>
#include <glad/glad.h>
#include <utility>

import lib;
import lib.std;

template <typename T>
concept TupleLike = requires {
    typename std::tuple_size<std::remove_cvref_t<T>>::type;
};

template<Key V, typename Fn>
class keyData {
public:
    // Fixed: remove_ptr needs std::remove_pointer_t, and this is a pointer type inside the class
    constexpr auto getType() -> typename std::remove_pointer<decltype(this)>::type;
    
    static constexpr uint8 Value = V; // Changed type to Key to match template parameter
    bool Pressed = false;
    void (*fn)()  = nullptr;                // Storing actual type Fn instead of void* makes constexpr trivial

    // Constexpr constructor
    constexpr keyData(Fn&& f) : fn(*std::bit_cast<void(*)()>(f)) {}

    // Made operator() constexpr
    constexpr void operator()() const {
        // auto ptr = *std::bit_cast<Fn>(fn);
        if (Pressed && fn) {
            fn();
        }
        // if(fn) {
        //     fn();
        // }
    }
};

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
    template<typename... Tuples>
    App& update(Tuples&&... args)
    {
        for (; ShouldClose(&this->win) == 0;) 
        {
            ren.swapbuffer();
            
            // A simple fold expression over the passed tuples
            ([this](auto&& tuple) {
                // Unpack the tuple into its individual components
                std::apply([this](auto&& func, auto&&... params) {
                    // func is the function pointer (e.g., &inputUpdate)
                    // params... are the remaining arguments (e.g., &shader, &x, &y)
                    
                    if constexpr (sizeof...(params) > 0) {
                        func(this, std::forward<decltype(params)>(params)...);
                    } else {
                        func(this);
                    }
                }, std::forward<decltype(tuple)>(tuple));
            }(std::forward<Tuples>(args)), ...);
        }   
        return *this;
    }
    
};

template<typename... KeyDataTypes>
struct StaticMap {
    std::tuple<KeyDataTypes...> keys;

    constexpr StaticMap(KeyDataTypes&&... args) 
        : keys(std::forward<KeyDataTypes>(args)...) {}

    constexpr void handleEvent(uint8 event, uint8 key) {
        std::apply([&](auto&... item) {
            ((item.Value == key ? item.Pressed = (event == eventType::keyPressed) : false),...);
        },keys);
    }

    constexpr void isPressed() {
        bool result = false;
        std::apply([&](auto&&... item) {
            ((item.Pressed ? item() : void()),...);
            // ((item.Pressed ? std::cout << "input\n" : std::cout << "noinput\n"),...);
        }, keys);
    }

};

int main ()
{

    vec2<int> ss {2,5};
    vec3<int> ss2 {2,5,10};
    vec<int,4> ss3 {2,5,10,15};
    for (const auto& i : ss.get<"yyyxx">()) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    for (const auto& i : ss2.get<"yyyxxz">()) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    for (const auto& i : ss3.get<"wyyxxzw">()) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    for (const auto& i : ss3.get<"wyywxzw">()) {
        std::cout << i << " ";
    }
    std::cout << "\n";

    App app;
    auto* ren = &app.ren;
    auto start = std::chrono::high_resolution_clock::now();
    Shader shader("res");
    float x = 0,y = 0;
    

    app.init();

    float vertices[] {
        -0.95f,-0.6f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,
        -0.5f,0.6f,0.0f,0.0f,1.0f,0.0f,1.0f,0.0f,
        -0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,0.5f,1.0f
    };
    float vertices2[] {
        0.95f,-0.6f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,
        0.5f,0.6f,0.0f,1.0f,0.0f,0.0f,1.0f,0.0f,
        0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,0.5f,1.0f
    };


    float box[] {
        0.5f, 0.5f,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
        0.5f, -0.5f,0.0f,0.0f,0.0f,1.0f,-0.5f,1.0f,
        -0.5f,-0.5f,0.0f,0.0f,1.0f,0.0f,0.5f,1.0f,
        -0.5f,0.5f,0.0f,0.0f,0.0f,1.0f,1.0f,0.5f,
    };

    unsigned int boxIndices[] = {
        0,1,3,
        1,2,3
    };
    
    // ren->BufferObj(3,12, box,boxIndices,6),
    ren->initBuffer<3>({3,3,2},128);
    ren->pushVertices(vertices,24);
    ren->pushVertices(vertices2,24);
    // 
    // ren->pushVertices(box,32);
    shader.CompileShader();
    shader.CompileProgram();
    
    auto inputUpdate = [](App* app,
        Shader* shader,
        float* x,float* y) {
            
        auto key_Escape = [&]() { CloseWindow(&app->win);};
        auto key_W = [&]() {++y; std::cout << x << "," << y <<"\n"; };
        auto key_A = [&]() {--x; std::cout << x << "," << y <<"\n"; };
        auto key_S = [&]() {--y; std::cout << x << "," << y <<"\n"; };
        auto key_D = [&]() {++x; std::cout << x << "," << y <<"\n"; };

        StaticMap myMap{
            keyData<Key::key_escape,decltype(key_Escape)*>(&key_Escape),
            keyData<Key::key_w,decltype(key_W)*>(&key_W),
            keyData<Key::key_a,decltype(key_A)*>(&key_A),
            keyData<Key::key_s,decltype(key_S)*>(&key_S),
            keyData<Key::key_d,decltype(key_D)*>(&key_D)
        };
        int32 b = 0;
        PollEvent(16);
        for (;CheckEvent(&app->win,&app->ev);) {
            myMap.handleEvent(app->ev.type, getKey(&app->ev));
        }
        myMap.isPressed();
    };
    
    // switch (app->ev.type)
    // {
    //     case eventType::keyPressed:
    //     {
    //         b = getKey(&app->ev);
    //         switch(b)
    //         {
    //             case Key::key_escape: {CloseWindow(&app->win); continue;}
    //             case Key::key_a: { shader->CompileShader(),shader->CompileProgram(); continue;}
    //             case Key::key_b: { continue;}
    //             case Key::key_c: { continue;}
    //             case Key::key_k: {*x+=0.01f, shader->setFloat("x", x); continue;}
    //             case Key::key_h: {*x-=0.01f, shader->setFloat("x", x); continue;}
    //             case Key::key_u: {*y+=0.01f, shader->setFloat("y", y); continue;}
    //             case Key::key_j: {*y-=0.01f, shader->setFloat("y", y); continue;}
    //             default: break;
    //         }
    //     }
    //     default: break;
    // }
    auto renderUpd = [&](App* ptr,
        std::chrono::time_point<std::chrono::high_resolution_clock>* start,
        Shader* shader
    ){
        auto* ren = &ptr->ren;
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - *start;
        float second = (std::sin(duration.count()) * 0.5f) + 0.5f;

        int vertexColorLocation = glGetUniformLocation(shader->ID,"second");
        glUniform1f(vertexColorLocation,second);

        glClearColor(0.0f,0.2f,0.2f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(ren->VAO);
        shader->use();
    
        glDrawArrays(GL_TRIANGLES,0,128);
        // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    };
    // app.update<inputUpdate,renderUpd> (
    //     std::tuple{&shader,&x,&y},
    //     std::tuple{&start,&shader}
    // );
    app.update (
        std::tuple{inputUpdate,&shader,&x,&y},
        std::tuple{renderUpd,&start,&shader}
    );
    return 0;
}