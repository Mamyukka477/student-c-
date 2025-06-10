#ifndef SCENE_TITLE_H
#define SCENE_TITLE_H

#include "Scene.h"
#include "Object.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

/**
 * 标题场景类
 * 负责显示游戏主菜单、排行榜、帮助信息等
 * 提供游戏入口和各种功能的导航
 */
class SceneTitle : public Scene
{
public:
    /**
     * 构造函数，初始化基本状态
     */
    SceneTitle() : showLeaderboard(false), showHelp(false) {}
    
    // 场景生命周期方法
    virtual void init() override; // 初始化场景资源
    virtual void update(float deltaTime) override; // 更新场景逻辑
    virtual void render() override; // 渲染场景内容
    virtual void clean() override; // 清理场景资源
    virtual void handleEvent(SDL_Event* event) override; // 处理用户输入

private:
    // 场景状态
    float timer = 0.0f; // 通用计时器
    bool showLeaderboard; // 是否显示排行榜界面
    bool showHelp; // 是否显示帮助信息
    
    // UI元素
    std::vector<Button> buttons; // 主要按钮列表
    Button helpButton; // 帮助按钮
    
    // 按钮管理方法
    void initButtons(); // 初始化主菜单按钮
    void initLeaderboardButtons(); // 初始化排行榜界面按钮
    void renderButtons(); // 渲染所有按钮
    
    // 事件处理方法
    void handleButtonClick(int buttonIndex); // 处理主菜单按钮点击
    void handleLeaderboardButtonClick(int buttonIndex); // 处理排行榜按钮点击
    bool isPointInButton(const Button& button, float x, float y); // 检查点是否在按钮内
    
    // 渲染方法
    void renderLeaderboard(); // 渲染排行榜内容
    void renderHelpContent(); // 渲染帮助内容
};

#endif // SCENE_TITLE_H