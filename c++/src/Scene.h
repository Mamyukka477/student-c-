#ifndef SCENE_H     // 防止头文件重复包含的宏定义开始
#define SCENE_H     // 定义头文件标识符

#include <SDL3/SDL.h>  // SDL3核心库头文件

class Game;  // 前向声明Game类，避免循环包含

// 场景基类，所有游戏场景都需要继承此抽象类
class Scene{
public:
    Scene(); // 构造函数，初始化场景基本状态
    virtual ~Scene() = default; // 虚析构函数，确保派生类正确析构

    virtual void init() = 0; // 纯虚函数：初始化场景资源和状态
    virtual void update(float deltaTime) = 0; // 纯虚函数：更新场景逻辑，deltaTime为帧时间间隔
    virtual void render() = 0; // 纯虚函数：渲染场景内容到屏幕
    virtual void clean() = 0; // 纯虚函数：清理场景资源，释放内存
    virtual void handleEvent(SDL_Event* event) = 0; // 纯虚函数：处理用户输入事件
protected:
    Game& game; // 引用Game单例对象，用于访问全局游戏状态
};

#endif // SCENE_H  // 防止头文件重复包含的宏定义结束