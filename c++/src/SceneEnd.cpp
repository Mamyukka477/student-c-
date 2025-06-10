#include "SceneEnd.h"
#include "SceneTitle.h"
#include "SceneMain.h"
#include "Game.h"
#include <string>

void SceneEnd::init()
{
    // 使用全局音乐管理播放幻想.mp3（不会重新开始播放）
    Game::getInstance().playBgm("assets/music/幻想.mp3");

    if (isVictory) {
        // 加载胜利结算图像
        victoryTexture = IMG_LoadTexture(Game::getInstance().getRenderer(), "assets/image/胜利结算.png");
        if (victoryTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load victory texture: %s", SDL_GetError());
        }
    } else {
        // 加载失败结算图像
        failureTexture = IMG_LoadTexture(Game::getInstance().getRenderer(), "assets/image/失败结算.png");
        if (failureTexture == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load failure texture: %s", SDL_GetError());
        }
    }

    if (!SDL_TextInputActive(Game::getInstance().getWindow())) {
        SDL_StartTextInput(Game::getInstance().getWindow());
    }
    if (!SDL_TextInputActive(Game::getInstance().getWindow())) {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Failed to start text input: %s", SDL_GetError());
    }
    
    initButtons();
    // 设置自定义光标
    Game::getInstance().setCustomCursor();
}
    
void SceneEnd::initButtons()
{
    auto& game = Game::getInstance();
    float centerX = game.getWindowWidth() / 2;
    float buttonWidth = 150;
    float buttonHeight = 40;
    float startY = game.getWindowHeight() * 0.85f;
    float spacing = 60;
    
    // 重新开始按钮
    Button restartButton;
    restartButton.rect = {centerX - buttonWidth - 10, startY, buttonWidth, buttonHeight};
    restartButton.text = "重新开始";
    buttons.push_back(restartButton);
    
    // 返回主菜单按钮
    Button menuButton;
    menuButton.rect = {centerX + 10, startY, buttonWidth, buttonHeight};
    menuButton.text = "回主菜单";
    buttons.push_back(menuButton);
}

void SceneEnd::update(float deltaTime)
{
    blinkTimer -= deltaTime; // 更新光标闪烁计时
    if (blinkTimer <= 0){
        blinkTimer += 1.0f;
    }
    
    if (!isTyping) {
        updateButtons(deltaTime);
    }
}

void SceneEnd::updateButtons(float deltaTime)
{
    // 悬停状态现在在handleEvent中的SDL_EVENT_MOUSE_MOTION事件中更新
    // 这个函数可以保留为空，或者用于其他按钮动画逻辑
}

void SceneEnd::render()
{
    auto& game = Game::getInstance();
    if (isTyping){
        renderPhase1(); // 渲染输入名字阶段
    }else{
        renderPhase2(); // 渲染排行榜阶段
        renderButtons(); // 渲染按钮
    }
}

void SceneEnd::renderButtons()
{
    auto& game = Game::getInstance();
    auto renderer = game.getRenderer();
    
    for (const auto& button : buttons) {
        // 选择按钮颜色
        SDL_Color color = button.normalColor;
        if (button.isPressed) {
            color = button.pressedColor;
        } else if (button.isHovered) {
            color = button.hoverColor;
        }
        
        // 绘制按钮背景
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &button.rect);
        
        // 绘制按钮边框
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &button.rect);
        
        // 绘制按钮文字 - 计算按钮左上角位置，让renderTextPos从那里开始绘制
        float textX = button.rect.x + 10;
        float textY = button.rect.y + button.rect.h / 2 - 15;
        game.renderTextPos(button.text, textX, textY, true);
    }
}

void SceneEnd::clean()
{
    // 清理失败结算图像纹理
    if (failureTexture != nullptr) {
        SDL_DestroyTexture(failureTexture);
        failureTexture = nullptr;
    }
    
    // 恢复默认光标
    Game::getInstance().setDefaultCursor();
    
    if (bgm != nullptr){
        Mix_HaltMusic(); // 停止音乐
        Mix_FreeMusic(bgm); // 释放音乐资源
        bgm = nullptr;
    }
    buttons.clear(); // 清理按钮
    
    if (victoryTexture != nullptr) {
        SDL_DestroyTexture(victoryTexture);
        victoryTexture = nullptr;
    }
}

void SceneEnd::handleEvent(SDL_Event *event)
{
    if (isTyping){
        if (event->type == SDL_EVENT_TEXT_INPUT){
            name += event->text.text; // 输入字符追加到名字
        }
        if (event->type == SDL_EVENT_KEY_DOWN){
            if (event->key.scancode == SDL_SCANCODE_RETURN){
                isTyping = false; // 回车确认输入
                SDL_StopTextInput(Game::getInstance().getWindow());
                if (name == ""){
                    name = "无名氏";
                }
                Game::getInstance().insertLeaderBoard(Game::getInstance().getFinalScore(), name); // 插入排行榜
            }
            if (event->key.scancode == SDL_SCANCODE_BACKSPACE){
                removeLastUTF8Char(name); // 删除最后一个字符
            }
        }
    }
    else{
        // 处理鼠标移动事件 - 更新按钮悬停状态
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
                        handleButtonClick((int)i);
                        break;
                    }
                }
                
                // 重置所有按钮状态
                for (auto& button : buttons) {
                    button.isPressed = false;
                }
            }
        }
    }
}

void SceneEnd::handleButtonClick(int buttonIndex)
{
    auto& game = Game::getInstance();
    
    // 移除音效播放代码，因为现在在按下时播放
    
    switch (buttonIndex) {
        case 0: // 重新开始
        {
            auto sceneMain = new SceneMain();
            game.changeScene(sceneMain);
            break;
        }
        case 1: // 返回主菜单
        {
            auto sceneTitle = new SceneTitle();
            game.changeScene(sceneTitle);
            break;
        }
    }
}

bool SceneEnd::isPointInButton(const Button& button, float x, float y)
{
    return x >= button.rect.x && x <= button.rect.x + button.rect.w &&
           y >= button.rect.y && y <= button.rect.y + button.rect.h;
}

void SceneEnd::renderPhase1() const
{
    auto& game = Game::getInstance();
    
    auto score = game.getFinalScore();
    std::string scoreText = "你的得分是：" + std::to_string(score);
    std::string resultText = isVictory ? "胜利了" : "失败了";  // 根据胜负显示不同文字
    std::string instrutionText = "请输入你的名字，按回车键确认：";
    
    game.renderTextCentered(scoreText, 0.1f, false);
    
    // 渲染对应的结算图像（只渲染一次）
    if (isVictory && victoryTexture) {
        float textureWidth, textureHeight;
        SDL_GetTextureSize(victoryTexture, &textureWidth, &textureHeight);
        
        // 计算图像位置（居中显示，小尺寸）
        float windowWidth = static_cast<float>(game.getWindowWidth());
        float windowHeight = static_cast<float>(game.getWindowHeight());
        float imageX = (windowWidth - textureWidth) / 2.0f;
        float imageY = windowHeight * 0.25f; // 在屏幕25%位置显示
        
        SDL_FRect imageRect = {imageX, imageY, textureWidth, textureHeight};
        SDL_RenderTexture(game.getRenderer(), victoryTexture, NULL, &imageRect);
    } else if (!isVictory && failureTexture) {
        float textureWidth, textureHeight;
        SDL_GetTextureSize(failureTexture, &textureWidth, &textureHeight);
        
        // 计算图像位置（居中显示，小尺寸）
        float windowWidth = static_cast<float>(game.getWindowWidth());
        float windowHeight = static_cast<float>(game.getWindowHeight());
        float imageX = (windowWidth - textureWidth) / 2.0f;
        float imageY = windowHeight * 0.25f; // 在屏幕25%位置显示
        
        SDL_FRect imageRect = {imageX, imageY, textureWidth, textureHeight};
        SDL_RenderTexture(game.getRenderer(), failureTexture, NULL, &imageRect);
    }
    
    // 在图像下方显示对应的结果文字
    game.renderTextCentered(resultText, 0.20f, true);
    game.renderTextCentered(instrutionText, 0.6f, false);
    
    if (name != ""){
        SDL_FPoint p = game.renderTextCentered(name, 0.8f, false);
        if (blinkTimer < 0.5){
            game.renderTextPos("_", p.x, p.y); // 显示闪烁光标
        }
    }else{
        if (blinkTimer < 0.5){
            game.renderTextCentered("_", 0.8f, false);
        }
    }
}

void SceneEnd::renderPhase2()
{
    auto& game = Game::getInstance();
    game.renderTextCentered("得分榜", 0.05f, true); // 修改：添加f后缀
    auto posY = static_cast<float>(0.2 * game.getWindowHeight()); // 修改：显式转换为float
    auto i = 1;
    for (const auto& item : game.getLeaderBoard()){
        std::string name = std::to_string(i) + ". " + item.second;
        std::string score = std::to_string(item.first);
        game.renderTextPos(name, 100.0f, posY); // 修改：添加f后缀
        game.renderTextPos(score, 100.0f, posY, false); // 修改：添加f后缀
        posY += 45.0f; // 修改：添加f后缀
        i++;
    }
}

void SceneEnd::removeLastUTF8Char(std::string &str)
{
    if (str.empty()) return;
    
    auto lastchar = str.back();
    if ((lastchar & 0b10000000) == 0b10000000) { // 中文字符的后续字节
        str.pop_back();
        while((str.back() & 0b11000000) != 0b11000000) { // 判断是否是中文字符的第一个字节
            str.pop_back();
        }
    }
    str.pop_back();// 删除最后一个字符
}
