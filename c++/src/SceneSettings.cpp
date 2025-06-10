#include "SceneSettings.h"
#include "SceneTitle.h"
#include "Game.h"
#include <string>
#include <algorithm>

void SceneSettings::init()
{
    // 使用全局音乐管理播放幻想.mp3（不会重新开始播放）
    Game::getInstance().playBgm("assets/music/幻想.mp3");
    
    loadSettings();
    initButtons();
    initSliders();
    
    // 设置自定义光标
    Game::getInstance().setCustomCursor();
}

void SceneSettings::initButtons()
{
    auto& game = Game::getInstance();
    float centerX = game.getWindowWidth() / 2;
    float buttonWidth = 200;
    float buttonHeight = 40;
    float startY = game.getWindowHeight() * 0.75f;
    float spacing = 60;
    
    // 全屏切换按钮 - 与实际状态保持一致
    Button fullscreenButton;
    fullscreenButton.rect = {centerX - buttonWidth/2, startY, buttonWidth, buttonHeight};
    fullscreenButton.text = "全屏：关"; // 始终显示为关闭状态
    buttons.push_back(fullscreenButton);
    
    // 难度设置按钮
    Button difficultyButton;
    difficultyButton.rect = {centerX - buttonWidth/2, startY + spacing, buttonWidth, buttonHeight};
    std::string diffText = "难度：";
    switch(difficulty) {
        case 0: diffText += "简单"; break;
        case 1: diffText += "普通"; break;
        case 2: diffText += "困难"; break;
    }
    difficultyButton.text = diffText;
    buttons.push_back(difficultyButton);
    
    // 返回按钮
    Button backButton;
    backButton.rect = {centerX - buttonWidth/2, startY + spacing * 2, buttonWidth, buttonHeight};
    backButton.text = "返回主菜单";
    buttons.push_back(backButton);
}

void SceneSettings::initSliders()
{
    auto& game = Game::getInstance();
    float centerX = game.getWindowWidth() / 2;
    float sliderWidth = 300;
    float sliderHeight = 20;
    float handleWidth = 20;
    float startY = game.getWindowHeight() * 0.45f;
    float spacing = 80;
    
    // 背景音量滑动条
    Slider bgmSlider;
    bgmSlider.rect = {centerX - sliderWidth/2, startY, sliderWidth, sliderHeight};
    bgmSlider.label = "背景音量";
    bgmSlider.value = game.getBgmVolume();
    bgmSlider.handle = {
        bgmSlider.rect.x + (bgmSlider.value / 100.0f) * (sliderWidth - handleWidth),
        bgmSlider.rect.y - 5,
        handleWidth,
        sliderHeight + 10
    };
    sliders.push_back(bgmSlider);
    
    // 音效音量滑动条
    Slider sfxSlider;
    sfxSlider.rect = {centerX - sliderWidth/2, startY + spacing, sliderWidth, sliderHeight};
    sfxSlider.label = "音效音量";
    sfxSlider.value = game.getSfxVolume();
    sfxSlider.handle = {
        sfxSlider.rect.x + (sfxSlider.value / 100.0f) * (sliderWidth - handleWidth),
        sfxSlider.rect.y - 5,
        handleWidth,
        sliderHeight + 10
    };
    sliders.push_back(sfxSlider);
}

void SceneSettings::render()
{
    auto& game = Game::getInstance();
    
    // 渲染标题
    game.renderTextCentered("游戏设置", 0.15f, true);
    
    // 渲染设置项
    renderSettings();
    
    // 渲染滑动条
    renderSliders();
    
    // 渲染按钮
    renderButtons();
}

void SceneSettings::renderSettings()
{
    auto& game = Game::getInstance();
    float startY = game.getWindowHeight() * 0.3f;
    float spacing = 40;
    
    // 显示当前设置值
    game.renderTextCentered("当前设置：", startY, false);
    game.renderTextCentered("全屏模式：" + std::string(game.getIsFullscreen() ? "开启" : "关闭"), startY + spacing, false);
    
    std::string diffText = "游戏难度：";
    switch(difficulty) {
        case 0: diffText += "简单"; break;
        case 1: diffText += "普通"; break;
        case 2: diffText += "困难"; break;
    }
    game.renderTextCentered(diffText, startY + spacing * 2, false);
}

void SceneSettings::renderSliders()
{
    auto& game = Game::getInstance();
    SDL_Renderer* renderer = game.getRenderer();
    
    for (auto& slider : sliders) {
        // 渲染标签
        std::string labelText = slider.label + ": " + std::to_string(slider.value) + "%";
        game.renderTextPos(labelText, slider.rect.x, slider.rect.y - 40, true);
        
        // 渲染滑动条轨道
        SDL_SetRenderDrawColor(renderer, slider.trackColor.r, slider.trackColor.g, slider.trackColor.b, slider.trackColor.a);
        SDL_RenderFillRect(renderer, &slider.rect);
        
        // 渲染滑动条边框
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &slider.rect);
        
        // 渲染滑块
        SDL_Color handleColor = slider.isDragging ? slider.activeColor : slider.handleColor;
        SDL_SetRenderDrawColor(renderer, handleColor.r, handleColor.g, handleColor.b, handleColor.a);
        SDL_RenderFillRect(renderer, &slider.handle);
        
        // 渲染滑块边框
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &slider.handle);
    }
}

void SceneSettings::handleEvent(SDL_Event *event)
{
    // 处理滑动条事件
    handleSliderEvent(event);
    
    // 处理鼠标移动事件
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        SDL_Event convertedEvent = *event;
        SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
        
        float mouseX = convertedEvent.motion.x;
        float mouseY = convertedEvent.motion.y;
        
        for (auto& button : buttons) {
            button.isHovered = isPointInButton(button, mouseX, mouseY);
        }
    }
    
    // 处理鼠标按下 - 播放音效并设置按下状态
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            SDL_Event convertedEvent = *event;
            SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
            
            float mouseX = convertedEvent.button.x;
            float mouseY = convertedEvent.button.y;
            
            for (size_t i = 0; i < buttons.size(); ++i) {
                if (isPointInButton(buttons[i], mouseX, mouseY)) {
                    buttons[i].isPressed = true;
                    // 播放按钮点击音效
                    Game::getInstance().playSfx("button_click");
                    break;
                }
            }
        }
    }
    
    // 处理鼠标弹起 - 执行按键功能
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            SDL_Event convertedEvent = *event;
            SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
            
            float mouseX = convertedEvent.button.x;
            float mouseY = convertedEvent.button.y;
            
            for (size_t i = 0; i < buttons.size(); ++i) {
                if (buttons[i].isPressed && isPointInButton(buttons[i], mouseX, mouseY)) {
                    handleButtonClick(static_cast<int>(i));
                    break;
                }
            }
        }
        
        // 重置所有按钮状态和滑动条状态
        for (auto& button : buttons) {
            button.isPressed = false;
        }
        for (auto& slider : sliders) {
            slider.isDragging = false;
        }
    }
}

void SceneSettings::handleSliderEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        SDL_Event convertedEvent = *event;
        SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
        
        float mouseX = convertedEvent.button.x;
        float mouseY = convertedEvent.button.y;
        
        for (auto& slider : sliders) {
            if (isPointInSlider(slider, mouseX, mouseY)) {
                slider.isDragging = true;
                updateSliderValue(slider, mouseX);
                break;
            }
        }
    }
    
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        SDL_Event convertedEvent = *event;
        SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
        
        float mouseX = convertedEvent.motion.x;
        
        for (auto& slider : sliders) {
            if (slider.isDragging) {
                updateSliderValue(slider, mouseX);
            }
        }
    }
}

bool SceneSettings::isPointInSlider(const Slider& slider, float x, float y)
{
    return x >= slider.rect.x && x <= slider.rect.x + slider.rect.w &&
           y >= slider.rect.y && y <= slider.rect.y + slider.rect.h;
}

void SceneSettings::updateSliderValue(Slider& slider, float mouseX)
{
    float handleWidth = slider.handle.w;
    float relativeX = mouseX - slider.rect.x;
    float percentage = relativeX / (slider.rect.w - handleWidth);
    percentage = std::max(0.0f, std::min(1.0f, percentage));
    
    slider.value = static_cast<int>(percentage * (slider.maxValue - slider.minValue) + slider.minValue);
    
    // 更新滑块位置
    slider.handle.x = slider.rect.x + percentage * (slider.rect.w - handleWidth);
    
    // 应用音量设置
    auto& game = Game::getInstance();
    if (&slider == &sliders[0]) { // 背景音量
        game.setBgmVolume(slider.value);
    } else if (&slider == &sliders[1]) { // 音效音量
        game.setSfxVolume(slider.value);
    }
}

void SceneSettings::handleButtonClick(int buttonIndex)
{
    auto& game = Game::getInstance();
    switch(buttonIndex) {
        case 0: // 全屏切换
        {
            bool newFullscreen = !game.getIsFullscreen();
            if (newFullscreen) {
                SDL_SetWindowFullscreen(game.getWindow(), SDL_WINDOW_FULLSCREEN);
            } else {
                SDL_SetWindowFullscreen(game.getWindow(), 0);
            }
            game.setIsFullscreen(newFullscreen);  // 更新状态
            buttons[0].text = newFullscreen ? "全屏：开" : "全屏：关";
            break;
        }
        case 1: // 难度设置
            difficulty = (difficulty + 1) % 3;
            game.setDifficulty(difficulty);  // 同步到Game类
            {
                std::string diffText = "难度：";
                switch(difficulty) {
                    case 0: diffText += "简单"; break;
                    case 1: diffText += "普通"; break;
                    case 2: diffText += "困难"; break;
                }
                buttons[1].text = diffText;
            }
            break;
        case 2: // 返回主菜单
            game.saveSettings();  // 保存设置
            {
                auto sceneTitle = new SceneTitle();
                game.changeScene(sceneTitle);
            }
            break;
    }
}

void SceneSettings::loadSettings()
{
    auto& game = Game::getInstance();
    // 从Game类同步难度设置
    difficulty = game.getDifficulty();
}

void SceneSettings::saveSettings()
{
    // 设置保存现在由Game类处理
    auto& game = Game::getInstance();
    game.saveSettings();
}

void SceneSettings::clean()
{
    // 恢复默认光标
    Game::getInstance().setDefaultCursor();
    
    if (bgm != nullptr) {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm = nullptr;
    }
    buttons.clear();
    sliders.clear();
}

void SceneSettings::renderButtons()
{
    auto& game = Game::getInstance();
    SDL_Renderer* renderer = game.getRenderer();
    
    for (auto& button : buttons) {
        // 根据按钮状态设置颜色
        SDL_Color color = button.normalColor;
        if (button.isPressed) {
            color = button.pressedColor;
        } else if (button.isHovered) {
            color = button.hoverColor;
        }
        
        // 渲染按钮背景
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &button.rect);
        
        // 渲染按钮边框
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &button.rect);
        
        // 渲染按钮文本 - 修正参数数量
        float textX = button.rect.x + button.rect.w / 2 - 70;
        float textY = button.rect.y + button.rect.h / 2 - 15;
        game.renderTextPos(button.text, textX, textY, false);  // 只传4个参数，false表示居中显示
    }
}

void SceneSettings::update(float deltaTime)
{
    // 设置场景的更新逻辑
    // 如果没有特殊的更新逻辑，可以保持为空实现
}

bool SceneSettings::isPointInButton(const Button& button, float x, float y)
{
    return x >= button.rect.x && x <= button.rect.x + button.rect.w &&
           y >= button.rect.y && y <= button.rect.y + button.rect.h;
}