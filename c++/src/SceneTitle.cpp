#include "SceneTitle.h"
#include "SceneMain.h"
#include "SceneSettings.h"  // 添加这行
#include "Game.h"
#include <string>

// 在init()方法中添加初始化提示按钮的代码
void SceneTitle::init()
{
    // 使用全局音乐管理播放幻想.mp3
    Game::getInstance().playBgm("assets/music/幻想.mp3");
    
    showLeaderboard = false;
    showHelp = false;
    initButtons();
    
    // 初始化提示按钮
    auto& game = Game::getInstance();
    helpButton.rect = {20, 20, 40, 40};
    helpButton.text = "?"; // 使用问号作为提示按钮的图标
    helpButton.normalColor = {80, 80, 200, 255}; // 蓝色按钮
    helpButton.hoverColor = {100, 100, 220, 255};
    helpButton.pressedColor = {135, 206, 250, 255}; // 统一为浅蓝色
    
    // 设置自定义光标
    Game::getInstance().setCustomCursor();
}

// 在handleEvent方法中添加处理提示按钮点击的代码
void SceneTitle::handleEvent(SDL_Event *event)
{
    // 处理鼠标移动事件 - 更新按钮悬停状态
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        // 创建转换后的事件对象
        SDL_Event convertedEvent = *event;
        SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
        
        float mouseX = convertedEvent.motion.x;
        float mouseY = convertedEvent.motion.y;
        
        // 检查提示按钮
        helpButton.isHovered = isPointInButton(helpButton, mouseX, mouseY);
        
        for (auto& button : buttons) {
            button.isHovered = isPointInButton(button, mouseX, mouseY);
        }
    }
    
    // 处理鼠标按下 - 播放音效并设置按下状态
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            // 创建转换后的事件对象
            SDL_Event convertedEvent = *event;
            SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
            
            float mouseX = convertedEvent.button.x;
            float mouseY = convertedEvent.button.y;
            
            // 检查是否点击了帮助按钮
            if (isPointInButton(helpButton, mouseX, mouseY)) {
                helpButton.isPressed = true;
                // 播放按钮点击音效
                Game::getInstance().playSfx("button_click");
                return;
            }
            
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
            // 创建转换后的事件对象
            SDL_Event convertedEvent = *event;
            SDL_ConvertEventToRenderCoordinates(Game::getInstance().getRenderer(), &convertedEvent);
            
            float mouseX = convertedEvent.button.x;
            float mouseY = convertedEvent.button.y;
            
            // 检查帮助按钮弹起
            if (helpButton.isPressed && isPointInButton(helpButton, mouseX, mouseY)) {
                // 切换显示/隐藏帮助内容
                showHelp = !showHelp;
            }
            helpButton.isPressed = false;
            
            // 检查普通按钮弹起
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

// 在render方法中添加渲染提示按钮和帮助内容的代码
void SceneTitle::render()
{
    auto& game = Game::getInstance();
    
    if (showLeaderboard) {
        renderLeaderboard();
    } else {
        std::string titleText = "冻青蛙"; // 渲染标题文字
        
        // 保存原始颜色设置
        SDL_Color originalColor = {255, 255, 255, 255};
        
        // 设置浅蓝色文本颜色
        SDL_Color lightBlueColor = {135, 206, 250, 255}; // 浅蓝色
        game.setTextColor(lightBlueColor);
        
        game.renderTextCentered(titleText, 0.3f, true);
        
        // 恢复原始颜色
        game.setTextColor(originalColor);
    }
    
    renderButtons();
    
    // 渲染提示按钮
    auto renderer = game.getRenderer();
    SDL_Color color = helpButton.normalColor;
    if (helpButton.isPressed) {
        color = helpButton.pressedColor;
    } else if (helpButton.isHovered) {
        color = helpButton.hoverColor;
    }
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &helpButton.rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &helpButton.rect);
    
    float textX = helpButton.rect.x + helpButton.rect.w / 2 - 10;
    float textY = helpButton.rect.y + helpButton.rect.h / 2 - 16;
    game.renderTextPos(helpButton.text, textX, textY, true);
    
    // 如果需要显示帮助内容，则渲染
    if (showHelp) {
        renderHelpContent();
    }
}

// 添加渲染帮助内容的方法
// 在文件末尾添加这个函数的实现
void SceneTitle::renderHelpContent()
{
    auto& game = Game::getInstance();
    auto renderer = game.getRenderer();
    
    // 创建半透明背景
    SDL_FRect helpRect = {50, 50, game.getWindowWidth() - 100, game.getWindowHeight() - 100};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // 半透明黑色
    SDL_RenderFillRect(renderer, &helpRect);
    
    // 添加边框
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 白色边框
    SDL_RenderRect(renderer, &helpRect);
    
    // 渲染帮助内容标题
    game.renderTextCentered("游戏帮助", 0.1f, true);
    
    // 渲染帮助文本
    float textY = game.getWindowHeight() * 0.2f;
    game.renderTextPos("操作说明：", 100, textY);
    textY += 40;
    game.renderTextPos("- 使用方向键或WASD移动飞船", 100, textY);
    textY += 40;
    game.renderTextPos("- 使用J键加速", 100, textY);
    textY += 40;
    game.renderTextPos("- 回车键暂停游戏", 100, textY);
    textY += 60;
    
    game.renderTextPos("游戏目标：", 100, textY);
    textY += 40;
    game.renderTextPos("- 躲避敌人子弹", 100, textY);
    textY += 40;
    game.renderTextPos("- 消灭敌人获得分数", 100, textY);
    textY += 40;
    game.renderTextPos("- 打败最终boss", 100, textY);
}
void SceneTitle::initButtons()
{
    buttons.clear(); // 清空现有按钮
    auto& game = Game::getInstance();
    float centerX = game.getWindowWidth() / 2;
    float buttonWidth = 200;
    float buttonHeight = 50;
    float startY = game.getWindowHeight() * 0.55f;
    float spacing = 60;
    
    // 开始游戏按钮
    Button startButton;
    startButton.rect = {centerX - buttonWidth/2, startY, buttonWidth, buttonHeight};
    startButton.text = "开始游戏";
    buttons.push_back(startButton);
    
    // 设置按钮
    Button settingsButton;
    settingsButton.rect = {centerX - buttonWidth/2, startY + spacing, buttonWidth, buttonHeight};
    settingsButton.text = "游戏设置";
    buttons.push_back(settingsButton);
    
    // 查看排行榜按钮
    Button leaderboardButton;
    leaderboardButton.rect = {centerX - buttonWidth/2, startY + spacing * 2, buttonWidth, buttonHeight};
    leaderboardButton.text = "得分榜";
    buttons.push_back(leaderboardButton);
    
    // 退出游戏按钮
    Button exitButton;
    exitButton.rect = {centerX - buttonWidth/2, startY + spacing * 3, buttonWidth, buttonHeight};
    exitButton.text = "退出游戏";
    buttons.push_back(exitButton);
}

void SceneTitle::initLeaderboardButtons()
{
    buttons.clear(); // 清空现有按钮
    auto& game = Game::getInstance();
    float centerX = game.getWindowWidth() / 2;
    float buttonWidth = 150;
    float buttonHeight = 40;
    float startY = game.getWindowHeight() * 0.85f;
    
    // 返回主菜单按钮
    Button backButton;
    backButton.rect = {centerX - buttonWidth/2, startY, buttonWidth, buttonHeight};
    backButton.text = "返回";
    buttons.push_back(backButton);
}

void SceneTitle::handleButtonClick(int buttonIndex)
{
    auto& game = Game::getInstance();
    
    // 移除音效播放代码，因为现在在按下时播放
    
    if (showLeaderboard) {
        handleLeaderboardButtonClick(buttonIndex);
        return;
    }
    
    switch (buttonIndex) {
        case 0: // 开始游戏
        {
            auto sceneMain = new SceneMain();
            game.changeScene(sceneMain);
            break;
        }
        case 1: // 游戏设置
        {
            auto sceneSettings = new SceneSettings();
            game.changeScene(sceneSettings);
            break;
        }
        case 2: // 得分榜
        {
            showLeaderboard = true;
            initLeaderboardButtons();
            break;
        }
        case 3: // 退出游戏
        {
            SDL_Event quitEvent = {}; // 初始化整个结构体
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
            break;
        }
    }
}

void SceneTitle::handleLeaderboardButtonClick(int buttonIndex)
{
    switch (buttonIndex) {
        case 0: // 返回
        {
            showLeaderboard = false;
            initButtons();
            break;
        }
    }
}

void SceneTitle::update(float deltaTime)
{
    timer += deltaTime; // 更新时间
    if (timer > 1.0f){
        timer -= 1.0f;
    }

}


void SceneTitle::renderLeaderboard()
{
    auto& game = Game::getInstance();
    
    // 显示当前难度的排行榜
    std::string title = "得分榜 - ";
    switch(game.getDifficulty()) {
        case 0: title += "简单"; break;
        case 1: title += "普通"; break;
        case 2: title += "困难"; break;
    }
    
    game.renderTextCentered(title, 0.05f, true);
    
    int index = 1;
    for (const auto& item : game.getLeaderBoard()){
        std::string text = std::to_string(index) + ". " + item.second + ": " + std::to_string(item.first);
        game.renderTextCentered(text, 0.15f + index * 0.08f, false);
        index++;
    }
}

void SceneTitle::renderButtons()
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
        float textX = button.rect.x +  40;
        float textY = button.rect.y + button.rect.h / 2 - 16;
        game.renderTextPos(button.text, textX, textY, true);
    }
}

void SceneTitle::clean()
{
    // 恢复默认光标
    Game::getInstance().setDefaultCursor();
    
    // 移除音乐清理代码，由Game类统一管理
    buttons.clear();
}

bool SceneTitle::isPointInButton(const Button& button, float x, float y)
{
    return x >= button.rect.x && x <= button.rect.x + button.rect.w &&
           y >= button.rect.y && y <= button.rect.y + button.rect.h;
}
