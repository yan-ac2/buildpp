
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <chrono>
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

int main ()
{
    App app;
    auto* ren = &app.ren;
    auto start = std::chrono::high_resolution_clock::now();
    Shader shader("res");
    app.init();

    float vertices[] {
        -0.95f,-0.6f,0.0f,1.0f,0.0f,0.0f,
        -0.5f,0.6f,0.0f,0.0f,1.0f,0.0f,
        -0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,
    };
    float vertices2[] {
        0.95f,-0.6f,0.0f,0.0f,1.0f,0.0f,
        0.5f,0.6f,0.0f,1.0f,0.0f,0.0f,
        0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,
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
    
    // ren->BufferObj(3,12, box,boxIndices,6),
    ren->initBuffer<2>(3,64);
    ren->pushVertices(vertices,18);
    ren->pushVertices(vertices2,18);
    shader.CompileShader();
    shader.CompileProgram();
    
    auto inputUpdate = [](App* app,Shader* shader) {
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
                        case Key::key_a: { shader->CompileShader(),shader->CompileProgram(); break;}
                        case Key::key_b: { break;}
                        case Key::key_c: { break;}
                        case Key::key_d: { break;}
                        default: break;
                    }
                }
                default: break;
            }
        }
    };
    
    auto renderUpd = [](App* ptr,
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
    
        glDrawArrays(GL_TRIANGLES,0,64);
        // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    };

    app.update<inputUpdate,renderUpd> (
        &shader,
        std::make_tuple(&start,&shader)
    );
    
    return 0;
}