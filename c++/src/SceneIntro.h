#ifndef SCENE_INTRO_H
#define SCENE_INTRO_H

#include "Scene.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>

// 开场动画场景
class SceneIntro : public Scene
{
public:
    SceneIntro() : gifAnimation(nullptr), currentFrame(0), frameTimer(0.0f), totalTimer(0.0f), showSkipText(true), skipTextTimer(0.0f) {}
    virtual void init() override;
    virtual void update(float deltaTime) override;
    virtual void render() override;
    virtual void clean() override;
    virtual void handleEvent(SDL_Event* event) override;

private:
    IMG_Animation* gifAnimation;  // GIF动画数据
    int currentFrame;             // 当前帧索引
    float frameTimer;             // 帧计时器
    float totalTimer;             // 总计时器
    const float MAX_DURATION = 21.3f; // 最大播放时长21.3秒
    
    bool showSkipText;            // 是否显示跳过提示
    float skipTextTimer;          // 跳过文字闪烁计时器
    
    void goToMainMenu();          // 跳转到主菜单
};

#endif // SCENE_INTRO_H