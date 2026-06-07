
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
import lib;

struct buffer {char buff;};
inline void* operator new(size_t size,buffer* Buff) {return Buff;} 

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
    
    template<auto... callback>
    App& update()
    {
        for (;ShouldClose(&this->win) == 0;) 
        {
            ren.swapbuffer();
            (callback(this),...);
        }   
        return *this;
    }
};

inline void inputUpdate(void* ptr) {
    App* app = static_cast<App*>(ptr);
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

inline void renderUpdate(void* ptr) {
    App* app = static_cast<App*>(ptr);
    
}

int main ()
{
    App app;
    auto* ren = &app.ren;
    app.init();

    float vertices[] {
        -0.5f,-0.5f,0.0f,
        0.5f,-0.5f,0.0f,
        0.0f,0.5f, 0.0f,
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
    
    
    
    ren->BufferObj(3,12, box,boxIndices,6),
    ren->VertexShader(vertexShaderSource),
    ren->FragmentShader(fragmentShaderSource),
    ren->CompileShader();

    auto renderUpd = [](App* ptr){
        auto* ren = &ptr->ren;
        glClearColor(0.0f,0.2f,0.2f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(ren->VAO);
        glUseProgram(ren->shaderProgram);

        // glDrawArrays(GL_TRIANGLES,0,12);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    };

    app.update<
    inputUpdate,
    renderUpd
    >();
    return 0;
}