
// #include <chrono>
// #include <tuple>
#include <synchapi.h>
#include <GL/gl.h>

// import lib;
import lib.std;
import lib.types;
import lib.win;

int main() {
    using et = EventType;
    Window app;
    InputState input;
    Renderer<"GL"> glCtx;
    DisplayManager disp;

    app.addDisplayManager(&disp);
    app.addInputState(&input);
    app.addRenderer(&glCtx);
    app.createWindow("MainWindow",800,600,WindowFlags::WinOGL,nullptr,{10,22});
    glCtx.Initialize(app.mHWND, 4, 3);
    glCtx.Resize(app.Desc.width, app.Desc.height);
    
    KeyMap <
    keyData<et::escape>,
    keyData<et::e>,
    keyData<et::w>,
    keyData<et::a>,
    keyData<et::s>,
    keyData<et::d>,
    keyData<et::q>,
    keyData<et::controlL>> keyMap (&input);

    float x = 0 ,y = 0;

    keyMap.get<et::escape>().setFn([&]() { std::cout << "Hello\n";
        app.CloseApp();}
    );
    keyMap.get<et::w>().setFn([&]() {++(y); std::cout << fmt(x," " ,y,"\n"); });
    keyMap.get<et::a>().setFn([&]() {--(x); std::cout << fmt(x," " ,y,"\n"); });
    keyMap.get<et::s>().setFn([&]() {--(y); std::cout << fmt(x," " ,y,"\n"); });
    // keyMap.get<et::z>().setFn([&]() {std::print("ctrlL pressed \n") ; });
    keyMap.get<et::d>().setFn([&]() {
        keyMap.getState<et::controlL>().IsToggled() ? x += 10 : ++(x); 
        std::cout << fmt(x," " ,y,"\n");
    });
    
    auto& times = Clock::get();
    while (app.IsRunning()) {
        times.start();
        app.ProcessEvents();
        if(app.GetInput().scrollDirection > 0) {
            auto monitor = disp.GetPrimaryMonitor();
            std::cout << fmt( "Monitor\n X: {} Y: {} {}x{} isPrimary: {}\n",monitor->x,monitor->y,monitor->width,monitor->height,monitor->isPrimary ? "true" : "false");   
            // std::cout << "\nMonitor \n"<< "X: "<< monitor->x << " Y: " << monitor->y << " Res: " << monitor->width  << "x" << monitor->height << " Is Primary: " << monitor->isPrimary << "\n";   
        }

        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glCtx.SwapBuffer();
        keyMap.update(8,times.delta_time());
        int timeSleep = times.end(16);
        Sleep(timeSleep);
    }
    return 0;
}

// template <typename T>
// concept TupleLike = requires {
//     typename std::tuple_size<std::remove_cvref_t<T>>::type;
// };

// template<Key V>
// class keyData {
// public:
//     static constexpr Key Value = V;
//     bool isDown = false;
//     bool wasDown = false;
//     bool changed = false;

//     using ActionFunc = std::function<void()>;
//     using invoke = void(*)(void*,bool);
//     invoke action = nullptr; 
//     void* ptr = nullptr;
    
//     template<typename F>
//     constexpr void setFn(F&& inFN) {
//         using decay = std::decay_t<F>;
//         static_assert(sizeof(decay) <= 24, "Callable too large for in-place buffer");
//         static_assert(alignof(decay) <= alignof(std::max_align_t), "Alignment mismatch");
//         ptr = new (buffer) decay(std::forward<F>(inFN));
        
//         action = [](void* ptr,bool destroy) -> void {
//             if (destroy) {
//                 return static_cast<decay*>(ptr)->~decay();
//             }
//             return (*static_cast<decay*>(ptr))();
//         };
//     }
//     ~keyData() {
//         if(action) {
//             action(ptr,true);
//         }
//     }
//     constexpr bool toggle() {
//         return changed;
//     }
//     void operator ()() {
//         return action(ptr,false);
//     }
//     constexpr void update() {
//         changed = (isDown != wasDown);
//         wasDown = isDown;
//     }
//     alignas(std::max_align_t) char buffer[24];
// };
// void test () {
//     std::cout << "test\n";
// }
// class App
// {
//     public:
//     mEvent   ev;
//     mWindow  win;
//     Renderer<"GL"> ren;
    
//     App& init()
//     {
//         createWindow("win test",100,100,800,600, Flags::WinCenter ,&win);
//         ren.init(4,6,&win);
//         return *this;
//     }
//     template<typename... Tuples>
//     App& update(Tuples&&... args)
//     {
//         for (; ShouldClose(&this->win) == 0;) 
//         {
//             ren.swapbuffer();
            
//             // A simple fold expression over the passed tuples
//             ([this](auto&& tuple) {
//                 // Unpack the tuple into its individual components
//                 std::apply([this](auto&& func, auto&&... params) {
//                     // func is the function pointer (e.g., &inputUpdate)
//                     // params... are the remaining arguments (e.g., &shader, &x, &y)
                    
//                     if constexpr (sizeof...(params) > 0) {
//                         func(this, std::forward<decltype(params)>(params)...);
//                     } else {
//                         func(this);
//                     }
//                 }, std::forward<decltype(tuple)>(tuple));
//             }(std::forward<Tuples>(args)), ...);
//         }   
//         return *this;
//     }
    
// };

// template<typename... KeyDataTypes>
// struct StaticMap {
//     std::tuple<KeyDataTypes...> keys;

//     constexpr StaticMap(KeyDataTypes&&... args) 
//         : keys(std::forward<KeyDataTypes>(args)...) {}

//     constexpr void update() {
//         ((isPressed<KeyDataTypes::Value>(),get<KeyDataTypes::Value>().update()),...);
//     }
//     template<Key K>
//     constexpr void isPressed() {
//         auto& data = std::get<keyData<K>>(keys);
//         data.isDown = isKeyDown(data.Value);
//         if(data.isDown && data.wasDown) {
//             data();
//         }
//     }
//     template<Key K>
//     constexpr auto& get() {
//         return std::get<keyData<K>>(keys);
//     }

//     // template<Key K>
//     // constexpr auto& set() {
//     //     return std::get<keyData<K>>(keys).action;
//     // }
// };


// int main ()
// {
//     App app;
//     auto* ren = &app.ren;
//     auto start = std::chrono::high_resolution_clock::now();
//     Shader shader("res");
//     StaticMap keyMap{
//         keyData<Key::key_escape>{},
//         keyData<Key::key_e>{},
//         keyData<Key::key_w>{},
//         keyData<Key::key_a>{},
//         keyData<Key::key_s>{},
//         keyData<Key::key_d>{},
//         keyData<Key::key_q>{},
//         keyData<Key::key_controlL>{},
//     };
    
    
//     float x = 0,y = 0;
//     app.init();

//     float vertices[] {
//         -0.95f,-0.6f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,
//         -0.5f,0.6f,0.0f,0.0f,1.0f,0.0f,1.0f,0.0f,
//         -0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,0.5f,1.0f
//     };
//     float vertices2[] {
//         0.95f,-0.6f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,
//         0.5f,0.6f,0.0f,1.0f,0.0f,0.0f,1.0f,0.0f,
//         0.1f,-0.6f, 0.0f,0.0f,0.0f,1.0f,0.5f,1.0f
//     };


//     float box[] {
//         0.5f, 0.5f,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
//         0.5f, -0.5f,0.0f,0.0f,0.0f,1.0f,-0.5f,1.0f,
//         -0.5f,-0.5f,0.0f,0.0f,1.0f,0.0f,0.5f,1.0f,
//         -0.5f,0.5f,0.0f,0.0f,0.0f,1.0f,1.0f,0.5f,
//     };

//     unsigned int boxIndices[] = {
//         0,1,3,
//         1,2,3
//     };
    
//     // ren->BufferObj(3,12, box,boxIndices,6),
//     ren->initBuffer<3>({3,3,2},128);
//     ren->pushVertices(vertices,24);
//     ren->pushVertices(vertices2,24);
//     // 
//     // ren->pushVertices(box,32);
//     shader.CompileShader();
//     shader.CompileProgram();

    
    
    
//     keyMap.get<Key::key_escape>().setFn([&]() { CloseWindow(&app.win);});
//     keyMap.get<Key::key_w>().setFn([&]() {++(y); std::cout << x << "," << y <<"\n"; });
//     keyMap.get<Key::key_a>().setFn([&]() {--(x); std::cout << x << "," << y <<"\n"; });
//     keyMap.get<Key::key_s>().setFn([&]() {--(y); std::cout << x << "," << y <<"\n"; });
//     keyMap.get<Key::key_d>().setFn([&]() {
//         keyMap.get<Key::key_controlL>().wasDown ? x += 10 : ++(x); 
//             std::cout << x << "," << y <<"\n";
//     });
//     keyMap.get<Key::key_e>().setFn(test);

//     auto inputUpdate = [&](App* app,
//         Shader* shader,
//         float* x,float* y) {
            

//             int32 b = 0;
//             // keyMap.newFrame();
//             PollEvent(16);
//             for (;CheckEvent(&app->win,&app->ev);) {
//                 keyMap.update();
//             }
//     };
    
//     auto renderUpd = [&](App* ptr,
//         std::chrono::time_point<std::chrono::high_resolution_clock>* start,
//         Shader* shader
//     ){
//         auto* ren = &ptr->ren;
        
//         auto end = std::chrono::high_resolution_clock::now();
//         std::chrono::duration<float> duration = end - *start;
//         float second = (std::sin(duration.count()) * 0.5f) + 0.5f;

//         int vertexColorLocation = glGetUniformLocation(shader->ID,"second");
//         glUniform1f(vertexColorLocation,second);

//         glClearColor(0.0f,0.2f,0.2f,1.f);
//         glClear(GL_COLOR_BUFFER_BIT);
        
//         glBindVertexArray(ren->VAO);
//         shader->use();
    
//         glDrawArrays(GL_TRIANGLES,0,128);
//         // glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

//     };
    
//     app.update (
//         std::tuple{inputUpdate,&shader,&x,&y},
//         std::tuple{renderUpd,&start,&shader}
//     );
//     return 0;
// }