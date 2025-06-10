#ifndef SCENE_SETTINGS_H
#define SCENE_SETTINGS_H

#include "Scene.h"
#include "Object.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <string>

// 设置场景
class SceneSettings : public Scene
{
public:
    SceneSettings() : bgm(nullptr) {}
    virtual void init() override;
    virtual void update(float deltaTime) override;
    virtual void render() override;
    virtual void clean() override;
    virtual void handleEvent(SDL_Event* event) override;

private:
    Mix_Music* bgm;
    std::vector<Button> buttons;
    std::vector<Slider> sliders;     // 添加滑动条列表
    
    // 设置项
    int difficulty = 1;              // 难度 0=简单, 1=普通, 2=困难
    
    void initButtons();
    void initSliders();              // 初始化滑动条
    void renderButtons();
    void renderSliders();            // 渲染滑动条
    void renderSettings();
    void handleButtonClick(int buttonIndex);
    void handleSliderEvent(SDL_Event* event);  // 处理滑动条事件
    bool isPointInButton(const Button& button, float x, float y);
    bool isPointInSlider(const Slider& slider, float x, float y);  // 检查点是否在滑动条内
    void updateSliderValue(Slider& slider, float mouseX);          // 更新滑动条值
    void applySettings();
    void loadSettings();
    void saveSettings();
};

#endif // SCENE_SETTINGS_H