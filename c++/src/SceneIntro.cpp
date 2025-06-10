#include "SceneIntro.h"
#include "SceneTitle.h"
#include "Game.h"
#include <SDL3_image/SDL_image.h>

void SceneIntro::init()
{
    // 使用全局音乐管理播放幻想.mp3（不会重新开始播放）
    Game::getInstance().playBgm("assets/music/幻想.mp3");
    
    // 加载GIF动画
    gifAnimation = IMG_LoadAnimation("assets/video/冻青蛙.gif");
    if (gifAnimation == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load GIF animation: %s", SDL_GetError());
        // 如果加载失败，直接跳转到主菜单
        goToMainMenu();
        return;
    }
    
    currentFrame = 0;
    frameTimer = 0.0f;
    totalTimer = 0.0f;
    showSkipText = true;
    skipTextTimer = 0.0f;
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GIF loaded: %dx%d, %d frames", 
                gifAnimation->w, gifAnimation->h, gifAnimation->count);
}

void SceneIntro::update(float deltaTime)
{
    totalTimer += deltaTime;
    frameTimer += deltaTime;
    skipTextTimer += deltaTime;
    
    // 跳过文字闪烁效果（每0.5秒切换一次）
    if (skipTextTimer >= 0.5f) {
        showSkipText = !showSkipText;
        skipTextTimer = 0.0f;
    }
    
    // 检查是否超过最大播放时长
    if (totalTimer >= MAX_DURATION) {
        goToMainMenu();
        return;
    }
    
    // 更新GIF动画帧
    if (gifAnimation != nullptr && gifAnimation->count > 0) {
        // 计算当前帧的延迟时间（毫秒转秒）
        float currentFrameDelay = gifAnimation->delays[currentFrame] / 1000.0f;
        
        if (frameTimer >= currentFrameDelay) {
            currentFrame = (currentFrame + 1) % gifAnimation->count;
            frameTimer = 0.0f;
        }
    }
}

void SceneIntro::render()
{
    auto& game = Game::getInstance();
    SDL_Renderer* renderer = game.getRenderer();
    
    // 清屏为黑色
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // 渲染GIF当前帧
    if (gifAnimation != nullptr && gifAnimation->frames != nullptr) {
        SDL_Surface* currentSurface = gifAnimation->frames[currentFrame];
        if (currentSurface != nullptr) {
            // 创建纹理
            SDL_Texture* frameTexture = SDL_CreateTextureFromSurface(renderer, currentSurface);
            if (frameTexture != nullptr) {
                // 让GIF拉伸填满整个窗口
                float windowWidth = game.getWindowWidth();
                float windowHeight = game.getWindowHeight();
                
                SDL_FRect dstRect = {0, 0, windowWidth, windowHeight};
                SDL_RenderTexture(renderer, frameTexture, nullptr, &dstRect);
                
                SDL_DestroyTexture(frameTexture);
            }
        }
    }
    
    // 修复：渲染跳过提示文字，使用正确的比例值和粉色颜色
    if (showSkipText) {
        // renderTextCentered的第二个参数是比例值(0-1)，不是绝对像素值
        float textYRatio = 0.9f; // 90%的位置，接近底部
        
        // 保存原始颜色设置
        SDL_Color originalColor = {255, 255, 255, 255};
        
        // 设置粉色文本颜色
        SDL_Color pinkColor = {255, 105, 180, 255}; // 粉色
        game.setTextColor(pinkColor);
        
        game.renderTextCentered("按 Enter 键跳过", textYRatio, false);
        
        // 恢复原始颜色
        game.setTextColor(originalColor);
    }
    
    SDL_RenderPresent(renderer);
}

void SceneIntro::handleEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
            // 按Enter键跳过动画
            goToMainMenu();
        }
    }
}

void SceneIntro::goToMainMenu()
{
    auto& game = Game::getInstance();
    auto sceneTitle = new SceneTitle();
    game.changeScene(sceneTitle);
}

void SceneIntro::clean()
{
    // 释放GIF动画资源
    if (gifAnimation != nullptr) {
        IMG_FreeAnimation(gifAnimation);
        gifAnimation = nullptr;
    }
}