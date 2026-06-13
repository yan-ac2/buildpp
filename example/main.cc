
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <functional>
#include <type_traits>
#include <glad/glad.h>
import lib;

struct buffer {char buff;};
inline void* operator new(size_t size,buffer* Buff) {return Buff;} 

template <typename T>
concept TupleLike = requires {
    typename std::tuple_size<std::remove_cvref_t<T>>::type;
};

class App
{
    public:
    mEvent   ev;
    mWindow  win;
    Renderer<"GL"> ren;
    App& init()
    {
        std::cout << "current location" << std::filesystem::current_path() << "\n"; 
        createWindow("win test",100,100,800,600, Flags::WinCenter ,&win);
        ren.init(4,6,&win);
        return *this;
    }
    
    template<auto... F> 
    App& update(auto&&... args)
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            ren.swapbuffer();
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            
                auto process = [this]<auto Func, typename T>(T&& arg) {

                    if constexpr (TupleLike<T>) {

                        std::apply([this](auto&&... unpacked) {
                            Func(this, std::forward<decltype(unpacked)>(unpacked)...);
                        }, std::forward<T>(arg));
                    } else {
                
                        Func(this, std::forward<T>(arg));
                    }
                };

                (process.template operator()<F>(std::forward<decltype(args)>(args)), ...);

            }(std::index_sequence_for<decltype(F)...>{});
            // (std::invoke(*F,this),...);
        }   
        return *this;
    }
};

inline void inputUpdate(App* app,int) {
    PollEvent(16);
    for (;CheckEvent(&app->win,&app->ev);) {
        switch (app->ev.type)
        {
            case eventType::keyPressed:
            {
                std::printf("%i \n",getKey(&app->ev));
                switch(getKey(&app->ev))
                {
                    case Key::key_escape: {CloseWindow(&app->win); break;}
                    case Key::key_a: { break;}
                    case Key::key_b: { break;}
                    case Key::key_c: { break;}
                    case Key::key_d: { break;}
                    default: break;
                }
            }
            default: break;
        }
    }
}



int main ()
{
    App app;
    auto* ren = &app.ren;
    app.init();

    float vertices[] {
        -0.95f,-0.6f,0.0f,
        -0.5f,0.6f,0.0f,
        -0.1f,-0.6f, 0.0f,
    };
    float vertices2[] {
        0.95f,0.6f,0.0f,
        0.5f,-0.6f,0.0f,
        0.1f,0.6f, 0.0f,
    };


    float box[] {
        0.5f, 0.5f,0.0f,
        0.5f, -0.5f,0.0f,
        -0.5f,-0.5f,0.0f,
        -0.5f,0.5f,0.0f,
    };

    unsigned int boxIndices[] = {
        0,1,3,
        1,2,3
    };
    
    const char *vertexShaderSource = 
        R"(
        #version 460 core

        layout (location = 0) in vec3 aPos;

        out vec4 vertexColor;
        void main()
        {
            gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
            vertexColor = vec4(0.5,0.0,0.0,1.0);
            })"
            ;

    const char *fragmentShaderSource = 
        R"(
        #version 460 core
        out vec4 FragColor;

        in vec4 vertexColor;
        void main()
        {
            FragColor = vertexColor;
        })"
    ;
    
    
    
    // ren->BufferObj(3,12, box,boxIndices,6),
    ren->BufferObj(3,9, vertices),
    ren->BufferObj(3,18, vertices2),
    ren->VertexShader(vertexShaderSource),
    ren->FragmentShader(fragmentShaderSource),
    ren->CompileShader();
    
    auto renderUpd = [](App* ptr,int){
        auto* ren = &ptr->ren;
        glClearColor(0.0f,0.2f,0.2f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(ren->VAO);
        glUseProgram(ren->shaderProgram);
    
        glDrawArrays(GL_TRIANGLES,0,18);
        // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    };
    app.update<inputUpdate,renderUpd> (
        1,
        1
    );
    return 0;
}