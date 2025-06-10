#include "Game.h"
#include "SceneMain.h"
#include "SceneTitle.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <fstream>
#include <sstream>
// 游戏主类构造函数
Game::Game()
    : deltaTime(0.0f), frameTime(0), textFont(nullptr), titleFont(nullptr) // 初始化成员变量
{
    // 注意：构造函数中不应该调用虚函数或复杂操作
    // 实际的初始化工作在init()方法中完成
}

// 析构函数，确保资源正确释放
Game::~Game()
{
    clean(); // 清理所有资源
}

/**
 * 游戏主循环，控制帧率和游戏的三个核心步骤
 */
void Game::run()
{
    while (isRunning)
    {
        auto frameStart = SDL_GetTicks(); // 记录帧开始时间
        SDL_Event event;
        
        handleEvent(&event);      // 处理输入事件
        update(deltaTime);        // 更新游戏逻辑
        render();                 // 渲染画面
        
        auto frameEnd = SDL_GetTicks(); // 记录帧结束时间
        auto diff = frameEnd - frameStart; // 计算实际帧时间
        
        // 帧率控制：如果帧时间小于目标时间，则延迟
        if (diff < frameTime){
            SDL_Delay(static_cast<Uint32>(frameTime - diff));
            deltaTime = frameTime / 1000.0f; // 使用目标帧时间
        }
        else{
            deltaTime = diff / 1000.0f; // 使用实际帧时间
        }
    }
}

// 初始化游戏资源和SDL相关库
void Game::init()
{
    frameTime = 1000 / FPS;
    // SDL 初始化
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        isRunning = false;
    }
   // 设置初始全屏模式为关闭
   isFullscreen = false;
    // 创建窗口（确保不是全屏模式）
    window = SDL_CreateWindow("冻青蛙", 
                             static_cast<int>(windowWidth),   
                             static_cast<int>(windowHeight),  
                             SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    
    // 添加窗口图标设置
    SDL_Surface* iconSurface = IMG_Load("assets/image/SpaceShip.png"); // 替换为您的图标文件路径
    if (iconSurface != nullptr) {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_DestroySurface(iconSurface); // 释放表面资源
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to load window icon: %s", SDL_GetError());
    }
    
    // 创建渲染器
    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    // 设置逻辑分辨率
    SDL_SetRenderLogicalPresentation(renderer, static_cast<int>(windowWidth), static_cast<int>(windowHeight), SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // 初始化SDL_mixer
    if (Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG) != (MIX_INIT_MP3 | MIX_INIT_OGG)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    
    // 打开音频设备
    if (!Mix_OpenAudio(0, NULL)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_mixer could not open audio! SDL_mixer Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    // 设置音效channel数量
    Mix_AllocateChannels(32);

    // 设置音乐音量
    Mix_VolumeMusic(bgmVolume * MIX_MAX_VOLUME / 100);
    Mix_Volume(-1, sfxVolume * MIX_MAX_VOLUME / 100);

    // 初始化SDL_ttf
    if (!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", SDL_GetError());
        isRunning = false;
    }

    // 初始化背景卷轴
    nearStars.texture = IMG_LoadTexture(renderer, "assets/image/背景.png");
    if (nearStars.texture == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_image could not initialize! SDL_image Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    SDL_GetTextureSize(nearStars.texture, &nearStars.width, &nearStars.height);   
   // 修改背景图大小为窗口大小
    nearStars.width = windowWidth;
    nearStars.height = windowHeight;

    farStars.texture = IMG_LoadTexture(renderer, "assets/image/背景下.png");
    if (farStars.texture == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_image could not initialize! SDL_image Error: %s\n", SDL_GetError());
        isRunning = false;
    }
    SDL_GetTextureSize(farStars.texture, &farStars.width, &farStars.height);
    farStars.width = windowWidth;
    farStars.height = windowHeight;
    farStars.speed = 20;

    // 载入字体
    titleFont = TTF_OpenFont("assets/font/VonwaonBitmap-16px (2).ttf", 64);
    textFont = TTF_OpenFont("assets/font/VonwaonBitmap-16px (2).ttf", 32);
    if (titleFont == nullptr || textFont == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_OpenFont: %s\n", SDL_GetError());
        isRunning = false;
    }
    
    // 加载自定义光标
    defaultCursor = SDL_GetCursor(); // 保存默认光标
    SDL_Surface* cursorSurface = IMG_Load("assets/image/pointer_c_shaded.png");
    if (cursorSurface != nullptr) {
        customCursor = SDL_CreateColorCursor(cursorSurface, 0, 0); // 热点在左上角
        SDL_DestroySurface(cursorSurface);
        if (customCursor == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create custom cursor: %s\n", SDL_GetError());
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load cursor image: %s\n", SDL_GetError());
    }

    // 载入得分
    loadData();

    // 载入设置
    loadSettings();

    // 初始化场景为开场动画
    currentScene = new SceneIntro();
    currentScene->init();
    
    // 加载全局音效
    globalSounds["button_click"] = Mix_LoadWAV("assets/sound/按钮声音.mp3");
    globalSounds["player_death"] = Mix_LoadWAV("assets/sound/死亡音效.mp3");
}
void Game::setBgmVolume(int volume)
{
    bgmVolume = std::max(0, std::min(100, volume));
    Mix_VolumeMusic(bgmVolume * MIX_MAX_VOLUME / 100);
}

// 设置音效音量
void Game::setSfxVolume(int volume)
{
    sfxVolume = std::max(0, std::min(100, volume));
    Mix_Volume(-1, sfxVolume * MIX_MAX_VOLUME / 100);
}

// 保存设置到文件
void Game::saveSettings() const
{
    std::ofstream file("assets/settings.dat");
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open settings file for writing");
        return;
    }
    file << "bgm_volume " << bgmVolume << std::endl;
    file << "sfx_volume " << sfxVolume << std::endl;
    file << "difficulty " << difficulty << std::endl;  // 添加难度保存
    file.close();
}

void Game::loadSettings()
{
    std::ifstream file("assets/settings.dat");
    if (!file.is_open()) {
        SDL_Log("Settings file not found, using defaults");
        // 应用默认设置
        setBgmVolume(bgmVolume);
        setSfxVolume(sfxVolume);
        return;
    }
    
    std::string key;
    int value;
    while (file >> key >> value) {
        if (key == "fullscreen") {
            isFullscreen = (value == 1);
        } else if (key == "bgm_volume") {
            setBgmVolume(value);
        } else if (key == "sfx_volume") {
            setSfxVolume(value);
        } else if (key == "difficulty") {  // 添加难度加载
            difficulty = value;
        }
    }
    file.close();
}
// 清理所有资源
void Game::clean()
{
    if (currentScene != nullptr)
    {
        currentScene->clean();
        delete currentScene;
    }
    if (nearStars.texture != nullptr){
        SDL_DestroyTexture(nearStars.texture);
    }
    if (farStars.texture != nullptr){
        SDL_DestroyTexture(farStars.texture);
    }
    if (titleFont != nullptr){
        TTF_CloseFont(titleFont);
    }
    if (textFont != nullptr){
        TTF_CloseFont(textFont);
    }
    
    // 清理自定义光标
    if (customCursor != nullptr) {
        SDL_DestroyCursor(customCursor);
    }
    for (auto& sound : globalSounds) {
        if (sound.second != nullptr) {
            Mix_FreeChunk(sound.second);
        }
    }
    globalSounds.clear();
    // 清理SDL_mixer
    Mix_CloseAudio();
    Mix_Quit();
    // 清理SDL_ttf
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// 切换场景
void Game::changeScene(Scene *scene)
{
     // 先保存旧场景指针
     Scene* oldScene = currentScene;
    
     // 立即设置新场景，防止在清理过程中currentScene为空
     currentScene = scene;
     
     // 初始化新场景
     if (currentScene != nullptr) {
         currentScene->init();
     }
     
     // 最后清理旧场景
     if (oldScene != nullptr)
     {
         oldScene->clean();
         delete oldScene;
     }
 }

// 处理输入事件，包括退出、全屏切换等
void Game::handleEvent(SDL_Event *event)
{
    while (SDL_PollEvent(event))
    {
        if (event->type == SDL_EVENT_QUIT)
        {
            isRunning = false;
        }
        if (event->type == SDL_EVENT_KEY_DOWN){
            if (event->key.scancode == SDL_SCANCODE_F4){
                isFullscreen = !isFullscreen;
                if (isFullscreen){
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                }else{
                    SDL_SetWindowFullscreen(window, 0);
                }
            }
        }
        // 添加空指针检查
        if (currentScene != nullptr) {
            currentScene->handleEvent(event);
        }
    }
}

// 更新背景和当前场景
void Game::update(float deltaTime)
{
    backgroundUpdate(deltaTime);
    if (currentScene != nullptr) {
        currentScene->update(deltaTime);
    }
}

// 渲染背景和当前场景
void Game::render()
{
    // 设置清屏颜色为黑色
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // 清空
    SDL_RenderClear(renderer);
    // 渲染星空背景
    renderBackground();

    if (currentScene != nullptr) {
        currentScene->render();
    }
    // 显示更新
    SDL_RenderPresent(renderer);
}

// 居中渲染文本，返回文本右下角坐标
SDL_FPoint Game::renderTextCentered(std::string text, float posY, bool isTitle)
{
    SDL_Color color = textColor; // 使用设置的颜色
    SDL_Surface *surface;
    if (isTitle){
        surface = TTF_RenderText_Solid(titleFont, text.c_str(), 0 , color);
    }else{
        surface = TTF_RenderText_Solid(textFont, text.c_str(), 0 , color);
    }
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    float y = (getWindowHeight() - surface->h) * posY;
    SDL_FRect rect = {getWindowWidth() / 2 - surface->w / 2,
                     y,
                     static_cast<float>(surface->w),
                     static_cast<float>(surface->h)};
    SDL_RenderTexture(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
    return {rect.x + rect.w, y};
}

// 按指定位置渲染文本
void Game::renderTextPos(std::string text, float posX, float posY, bool isLeft)
{
    SDL_Color color = textColor; // 使用设置的颜色
    SDL_Surface *surface = TTF_RenderText_Solid(textFont, text.c_str(), 0 , color);
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect rect;
    if (isLeft){
        rect = {posX, posY, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    }else{
        rect = {getWindowWidth() - posX - static_cast<float>(surface->w), posY, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    }
    SDL_RenderTexture(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

// 更新背景星空的偏移量，实现卷轴效果
void Game::backgroundUpdate(float deltaTime)
{
    //水平滚动从右向左
    nearStars.offset -= nearStars.speed * deltaTime;
    if (nearStars.offset <= -nearStars.width)
    {
        nearStars.offset += nearStars.width;
    }

    farStars.offset -= farStars.speed * deltaTime;
    if (farStars.offset <= -farStars.width){
        farStars.offset += farStars.width;
    }
}

// 渲染星空背景
void Game::renderBackground()
{
    // 渲染远处的星星 水平
    for (float posX = farStars.offset; posX < getWindowWidth(); posX += farStars.width){
        for (float posY = 0; posY < getWindowHeight(); posY += farStars.height){
            SDL_FRect ds = {posX, posY, farStars.width, farStars.height};
            SDL_RenderTexture(renderer, farStars.texture, NULL, &ds);
        }
    }
    // 渲染近处的星星 水平
    for (float posX = nearStars.offset; posX < getWindowWidth(); posX += nearStars.width)
    {
        for (float posY = 0; posY < getWindowHeight(); posY += nearStars.height)
        {
            SDL_FRect dstRect = {posX, 0, nearStars.width, nearStars.height};
            SDL_RenderTexture(renderer, nearStars.texture, NULL, &dstRect);
        }
    }
}

// 保存排行榜数据到文件
void Game::saveData()
{
    std::ofstream file("assets/save.dat");
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open save file for writing");
        return;
    }
    
    // 保存各难度得分榜的数据
    file << "easy_scores" << std::endl;
    for (const auto &entry : leaderBoardEasy){
        file << entry.first << " " << entry.second << std::endl;
    }
    
    file << "normal_scores" << std::endl;
    for (const auto &entry : leaderBoardNormal){
        file << entry.first << " " << entry.second << std::endl;
    }
    
    file << "hard_scores" << std::endl;
    for (const auto &entry : leaderBoardHard){
        file << entry.first << " " << entry.second << std::endl;
    }
    
    file.close();
}

void Game::loadData()
{
    std::ifstream file("assets/save.dat");
    if (!file.is_open()) {
        SDL_Log("Save file not found, starting fresh");
        return;
    }
    
    leaderBoardEasy.clear();
    leaderBoardNormal.clear();
    leaderBoardHard.clear();
    
    std::string line;
    std::multimap<int, std::string, std::greater<int>>* currentBoard = nullptr;
    
    while (std::getline(file, line)) {
        if (line == "easy_scores") {
            currentBoard = &leaderBoardEasy;
        } else if (line == "normal_scores") {
            currentBoard = &leaderBoardNormal;
        } else if (line == "hard_scores") {
            currentBoard = &leaderBoardHard;
        } else if (currentBoard != nullptr) {
            std::istringstream iss(line);
            int score;
            std::string name;
            if (iss >> score >> name) {
                currentBoard->insert({score, name});
            }
        }
    }
    
    file.close();
}

void Game::insertLeaderBoard(int score, std::string name)
{
    auto& currentLeaderBoard = getLeaderBoard();
    currentLeaderBoard.insert({score, name});
    if (currentLeaderBoard.size() > 8){
        currentLeaderBoard.erase(--currentLeaderBoard.end());
    }
    saveData();
}

// 设置自定义光标
void Game::setCustomCursor()
{
    if (customCursor != nullptr) {
        SDL_SetCursor(customCursor);
    }
}

// 恢复默认光标
void Game::setDefaultCursor()
{
    if (defaultCursor != nullptr) {
        SDL_SetCursor(defaultCursor);
    }
}
void Game::playBgm(const std::string& musicPath, bool forceRestart)
{
    // 如果正在播放相同的音乐且不强制重启，则不做任何操作
    if (!forceRestart && currentBgmPath == musicPath && Mix_PlayingMusic()) {
        return;
    }
    
    // 停止当前音乐
    if (currentBgm != nullptr) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentBgm);
        currentBgm = nullptr;
    }
    
    // 加载新音乐
    currentBgm = Mix_LoadMUS(musicPath.c_str());
    if (currentBgm == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %s", SDL_GetError());
        currentBgmPath = "";
        return;
    }
    
    // 播放音乐
    Mix_PlayMusic(currentBgm, -1);
    currentBgmPath = musicPath;
}

void Game::stopBgm()
{
    if (currentBgm != nullptr) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentBgm);
        currentBgm = nullptr;
        currentBgmPath = "";
    }
}
void Game::playSfx(const std::string& soundPath)
{
    auto it = globalSounds.find(soundPath);
    if (it != globalSounds.end() && it->second != nullptr) {
        // 使用音效音量设置播放
        Mix_PlayChannel(-1, it->second, 0);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Sound effect not found: %s", soundPath.c_str());
    }
}
bool Game::isPlayingBgm(const std::string& musicPath) const
{
    return currentBgmPath == musicPath && Mix_PlayingMusic();
}
