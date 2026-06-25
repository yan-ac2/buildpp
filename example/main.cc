
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <type_traits>
#include <glad/glad.h>
#include <variant>
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

template <size_t N>
struct arrutl {
    std::size_t num[N];
    
    constexpr arrutl(const char (&input)[N + 1]) : num{} {
        for (size_t i = 0; i < N; ++i) { // Stop before the null-terminator
            num[i] = (input[i] == 'x' ? 0 :
                      input[i] == 'y' ? 1 : 
                      input[i] == 'z' ? 2 :
                      input[i] == 'w' ? 3 : 0);
        }
    }
    constexpr size_t getSize() const { return N; }
};

template <size_t N>
arrutl(const char(&)[N]) -> arrutl<N - 1>;
template <typename T>
class svec2 {
public:
    T vec[2];
    svec2() : vec(0) {}
    svec2(T x, T y) : vec(x,y) {}

    T& operator []() {

    }
    template <arrutl s>
    inline std::array<T&, s.getSize()> get() {
        std::array<T&, s.getSize()> buffer;
        
        for (size_t i = 0; i < s.getSize(); ++i) {
            // Correctly map the pointer using the compiled string indices
            buffer[i] = vec[s.num[i]];
        }
        
        return buffer; 
    
    }

    template <arrutl s> requires (s.getSize() == 1)
    inline T& get() {
        return vec[s.num[0]];
    }
};
int main ()
{
    svec2<int> s {2,5};

    s.get<"xy">() = 50;
    std::cout << s.get<"y">() << "\n";
    
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
                        case Key::key_k: {*x+=0.01f, shader->setFloat("x", x); break;}
                        case Key::key_h: {*x-=0.01f, shader->setFloat("x", x); break;}
                        case Key::key_u: {*y+=0.01f, shader->setFloat("y", y); break;}
                        case Key::key_j: {*y-=0.01f, shader->setFloat("y", y); break;}
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
    
        glDrawArrays(GL_TRIANGLES,0,128);
        // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    };

    app.update<inputUpdate,renderUpd> (
        std::tuple(&shader,&x,&y),
        std::tuple(&start,&shader)
    );
    
    return 0;
}