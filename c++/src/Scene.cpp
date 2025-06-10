#include "Scene.h"  // 包含场景基类头文件
#include "Game.h"   // 包含游戏主类头文件

// 场景基类构造函数实现
Scene::Scene() : game(Game::getInstance()) // 初始化列表：将game引用绑定到Game单例对象
{
}