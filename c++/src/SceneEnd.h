#ifndef SCENE_END_H
#define SCENE_END_H

#include "Scene.h"
#include "Object.h"
#include <string>
#include <vector>
#include <SDL3_mixer/SDL_mixer.h>

// 结算/结束场景
class SceneEnd : public Scene{
public:
    SceneEnd(bool isVictory = false) : Scene(), bgm(nullptr), failureTexture(nullptr), victoryTexture(nullptr), isVictory(isVictory) {} // 添加failureTexture初始化
    virtual void init(); // 初始化
    virtual void update(float deltaTime); // 更新逻辑
    virtual void render(); // 渲染
    virtual void clean(); // 清理资源
    virtual void handleEvent(SDL_Event* event); // 处理输入

private:
    bool isTyping = true; // 是否正在输入名字
    std::string name = ""; // 玩家输入的名字
    float blinkTimer = 1.0f; // 光标闪烁计时
    Mix_Music* bgm; // 背景音乐
    SDL_Texture* failureTexture; // 失败结算图像
    std::vector<Button> buttons; // 按钮列表

    void renderPhase1() const; // 渲染输入名字阶段 - 添加const
    void renderPhase2(); // 渲染排行榜阶段
    void initButtons(); // 初始化按钮
    void updateButtons(float deltaTime); // 更新按钮状态
    void renderButtons(); // 渲染按钮
    void handleButtonClick(int buttonIndex); // 处理按钮点击
    bool isPointInButton(const Button& button, float x, float y); // 检查点是否在按钮内
    void removeLastUTF8Char(std::string& str); // 删除最后一个UTF8字符
    bool isVictory = false;              // 是否胜利
    SDL_Texture* victoryTexture;         // 胜利结算图像
};

#endif // SCENE_END_H