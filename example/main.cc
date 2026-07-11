
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

template<Key V>
class keyData {
public:
    static constexpr Key Value = V;
    bool isDown = false;
    bool wasDown = false;
    bool changed = false;

    using ActionFunc = std::function<void()>;
    ActionFunc action = []{}; 
    constexpr bool toggle() {
        return changed;
    }
    constexpr void update() {
        changed = (isDown != wasDown);
        wasDown = isDown;
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

    // constexpr void newFrame() {
    //     ((),...);
    // }
    constexpr void update() {
        ((isPressed<KeyDataTypes::Value>(),get<KeyDataTypes::Value>().update()),...);
    }
    template<Key K>
    constexpr void isPressed() {
        auto& data = std::get<keyData<K>>(keys);
        data.isDown = isKeyDown(data.Value);
        if(data.isDown && data.wasDown) {
            data.action();
        }
    }
    template<Key K>
    constexpr auto& get() {
        return std::get<keyData<K>>(keys);
    }

    template<Key K>
    constexpr auto& set() {
        return std::get<keyData<K>>(keys).action;
    }
};


int main ()
{
    App app;
    auto* ren = &app.ren;
    auto start = std::chrono::high_resolution_clock::now();
    Shader shader("res");
    StaticMap keyMap{
        keyData<Key::key_escape>{},
        keyData<Key::key_w>{},
        keyData<Key::key_a>{},
        keyData<Key::key_s>{},
        keyData<Key::key_d>{},
        keyData<Key::key_q>{},
        keyData<Key::key_controlL>{},
    };
    
    
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

    
    
    
    keyMap.set<Key::key_escape>() = [&]() { CloseWindow(&app.win);};
    keyMap.set<Key::key_w>() = [&]() {++(y); std::cout << x << "," << y <<"\n"; };
    keyMap.set<Key::key_a>() = [&]() {--(x); std::cout << x << "," << y <<"\n"; };
    keyMap.set<Key::key_s>() = [&]() {--(y); std::cout << x << "," << y <<"\n"; };
    keyMap.set<Key::key_d>() = [&]() {
        keyMap.get<Key::key_controlL>().wasDown ? x += 10 : ++(x); 
            std::cout << x << "," << y <<"\n"; 
    };

    auto inputUpdate = [&](App* app,
        Shader* shader,
        float* x,float* y) {
            

            int32 b = 0;
            // keyMap.newFrame();
            PollEvent(16);
            for (;CheckEvent(&app->win,&app->ev);) {
                keyMap.update();
            }
    };
    
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
    
    app.update (
        std::tuple{inputUpdate,&shader,&x,&y},
        std::tuple{renderUpd,&start,&shader}
    );
    return 0;
}