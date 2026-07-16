
// #include <chrono>
// #include <tuple>
#include <synchapi.h>

// import lib;
import lib.std;
import lib.types;
import lib.win;
int main() {
    using et = EventType;
    DisplayManager disp;
    Window app;
    app.addDisplayManager(&disp);
    KeyMap keyMap(&app.GetInput(),
        keyData<et::escape>{},
        keyData<et::e>{},
        keyData<et::w>{},
        keyData<et::a>{},
        keyData<et::s>{},
        keyData<et::d>{},
        keyData<et::q>{},
        keyData<et::controlL>{}
    );

    if (!app.CreateAppWindow("Modern C++20 Module Window", 800, 600,WindowFlags::WinCenter)) {
        return -1;
    }
    const auto& input = app.GetInput();

    float x = 0 ,y = 0;

    keyMap.get<et::escape>().setFn([&]() { std::print("Hello\n");app.CloseApp();});
    keyMap.get<et::w>().setFn([&]() {++(y); std::print("{} {} \n", x , y); });
    keyMap.get<et::a>().setFn([&]() {--(x); std::print("{} {} \n", x , y) ; });
    keyMap.get<et::s>().setFn([&]() {--(y); std::print("{} {} \n", x , y) ; });
    keyMap.get<et::d>().setFn([&]() {
        keyMap.get<et::controlL>().isDown ? x += 10 : ++(x); 
        std::print("{} {} \n", x , y) ;
    });
    // auto print_status_stream = [](int32_t work_time, int32_t dt) {
    //     // 1. Save the cursor's current position down where the inputs are scrolling
    //     std::cout << "\033[s\033[H";


    //     // 3. Overwrite the top line with your live timing data
    //     // (We add spaces at the end to clean up any leftover text if the numbers shrink)
        
    //     // 4. Restore the cursor back to the scrolling area below
    //     std::cout << "\033[u";
    //     std::cout << "Time: " << work_time << "ms dt: " << dt << "    \033[K" << std::flush;
        
    //     // 5. Print the new input logs normally (they will scroll naturally down the console)
    //     // std::cout << input_log << "\n";
        
    //     // Flush the stream immediately so the frame ticks look perfectly live
    //     // std::cout << std::flush;
    // };
    auto& times = Clock::get();
    while (app.isRunning()) {
        // std::cout << "\033[2J\033[H\n";
        times.start();
        // 1. Process OS messages & update inputs
        app.ProcessEvents();
        if(app.GetInput().scrollDirection > 0) {
            auto monitor = disp.GetPrimaryMonitor();
            // std::cout << "\nMonitor \n"<< "X: "<< monitor->x << " Y: " << monitor->y << " Res: " << monitor->width  << "x" << monitor->height << " Is Primary: " << monitor->isPrimary << "\n";   
        }
        keyMap.update(16,times.delta_time());
        int timeSleep = times.end(16);
        std::print("\rTime Sleep: {} dt: {}  \033[6n\n",timeSleep,times.delta_time());
        // std::cout << "\rTime: " << timeSleep << "ms dt: " << times.delta_time();
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