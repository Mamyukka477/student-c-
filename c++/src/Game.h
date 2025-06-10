#ifndef GAME_H
#define GAME_H

#include "Scene.h"
#include "Object.h"
#include "SceneIntro.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <map>

/**
 * 游戏主控类，采用单例模式管理整个游戏的生命周期
 * 负责SDL初始化、场景管理、资源管理、事件处理等核心功能
 */
class Game
{
private:
    Game(); // 构造函数私有化，保证单例模式
    Game(const Game&) = delete; // 禁止拷贝构造
    Game& operator=(const Game&) = delete; // 禁止赋值操作

    // 字体资源
    TTF_Font* titleFont; // 标题字体
    TTF_Font* textFont;  // 正文字体
    
    // 光标资源
    SDL_Cursor* customCursor = nullptr;  // 自定义光标
    SDL_Cursor* defaultCursor = nullptr; // 系统默认光标
  
    // 渲染设置
    SDL_Color textColor = {255, 255, 255, 255}; // 文本颜色，默认白色
    
    // 游戏状态
    bool isRunning = true;      // 游戏运行标志
    bool isFullscreen = false;  // 全屏模式标志
    Scene* currentScene = nullptr; // 当前活动场景指针
    
    // SDL核心对象
    SDL_Window* window = nullptr;  // SDL窗口
    SDL_Renderer* renderer = nullptr; // SDL渲染器
    
    // 窗口和帧率设置
    float windowWidth = 1200;     // 窗口宽度
    float windowHeight = 750;    // 窗口高度
    int FPS = 60;                // 目标帧率
    Uint32 frameTime;            // 每帧目标时间（毫秒）
    float deltaTime;             // 实际每帧耗时（秒）
    
    // 游戏数据
    int finalScore = 0;          // 最终得分
    
    // 音频设置
    int bgmVolume = 50;          // 背景音乐音量 (0-100)
    int sfxVolume = 50;          // 音效音量 (0-100)
    int difficulty = 1;          // 游戏难度 (0=简单, 1=普通, 2=困难)

    // 背景系统
    Background nearStars; // 近景星空背景
    Background farStars;  // 远景星空背景

    // 分难度排行榜系统
    std::multimap<int, std::string, std::greater<int>> leaderBoardEasy;    // 简单难度排行榜
    std::multimap<int, std::string, std::greater<int>> leaderBoardNormal;  // 普通难度排行榜
    std::multimap<int, std::string, std::greater<int>> leaderBoardHard;    // 困难难度排行榜
    
    // 全局音频管理
    Mix_Music* currentBgm = nullptr;     // 当前播放的背景音乐
    std::string currentBgmPath = "";     // 当前音乐文件路径
    std::map<std::string, Mix_Chunk*> globalSounds; // 全局音效资源池

public:
    /**
     * 获取Game单例对象
     * @return Game实例的引用
     */
    static Game& getInstance(){
        static Game instance;
        return instance;
    }

    ~Game(); // 析构函数，负责资源释放
    
    // 核心游戏循环方法
    void run(); // 游戏主循环
    void init(); // 初始化游戏资源和SDL
    void clean(); // 清理所有资源
    void changeScene(Scene* scene); // 切换场景

    // 游戏循环的三个核心步骤
    void handleEvent(SDL_Event *event); // 处理输入事件
    void update(float deltaTime); // 更新游戏逻辑
    void render(); // 渲染画面

    // 文本渲染方法
    SDL_FPoint renderTextCentered(std::string text, float posY, bool isTitle); // 居中渲染文本
    void renderTextPos(std::string text, float posX, float posY, bool isLeft = true); // 指定位置渲染文本
    
    // 背景系统方法
    void backgroundUpdate(float deltaTime); // 更新背景滚动
    void renderBackground(); // 渲染星空背景

    // Setter方法
    void setFinalScore(int score) { finalScore = score; } // 设置最终得分
    void insertLeaderBoard(int score, std::string name); // 插入排行榜记录
    void setIsFullscreen(bool fullscreen) { isFullscreen = fullscreen; } // 设置全屏状态
    void setTextColor(SDL_Color color) { textColor = color; } // 设置文本颜色
    void setDifficulty(int diff) { difficulty = diff; } // 设置游戏难度

    // Getter方法
    SDL_Window* getWindow() { return window; } // 获取SDL窗口
    SDL_Renderer* getRenderer() { return renderer; } // 获取SDL渲染器
    float getWindowWidth() { return windowWidth; } // 获取窗口宽度
    float getWindowHeight() { return windowHeight; } // 获取窗口高度
    int getFinalScore() { return finalScore; } // 获取最终得分
    bool getIsFullscreen() const { return isFullscreen; } // 获取全屏状态
    SDL_Color getTextColor() const { return textColor; } // 获取文本颜色
    int getDifficulty() const { return difficulty; } // 获取当前难度
    
    /**
     * 根据当前难度返回对应的排行榜
     * @return 当前难度的排行榜引用
     */
    std::multimap<int, std::string, std::greater<int>>& getLeaderBoard() {
        switch(difficulty) {
            case 0: return leaderBoardEasy;
            case 1: return leaderBoardNormal;
            case 2: return leaderBoardHard;
            default: return leaderBoardNormal;
        }
    }
    
    /**
     * 获取指定难度的排行榜
     * @param diff 难度等级
     * @return 指定难度的排行榜引用
     */
    std::multimap<int, std::string, std::greater<int>>& getLeaderBoard(int diff) {
        switch(diff) {
            case 0: return leaderBoardEasy;
            case 1: return leaderBoardNormal;
            case 2: return leaderBoardHard;
            default: return leaderBoardNormal;
        }
    }
    
    // 音频控制方法
    void setBgmVolume(int volume); // 设置背景音乐音量
    void setSfxVolume(int volume); // 设置音效音量
    int getBgmVolume() const { return bgmVolume; } // 获取背景音乐音量
    int getSfxVolume() const { return sfxVolume; } // 获取音效音量
    
    // 设置文件操作
    void saveSettings() const; // 保存设置到文件
    void loadSettings(); // 从文件加载设置
    
    // 光标控制方法
    void setCustomCursor();   // 设置自定义光标
    void setDefaultCursor();  // 恢复默认光标

    // 音乐管理方法
    void playBgm(const std::string& musicPath, bool forceRestart = false); // 播放背景音乐
    void stopBgm(); // 停止背景音乐
    bool isPlayingBgm(const std::string& musicPath) const; // 检查是否正在播放指定音乐
    void playSfx(const std::string& soundPath); // 播放音效
    
    // 数据持久化方法
    void saveData(); // 保存排行榜数据
    void loadData(); // 加载排行榜数据
    
    /**
     * 设置背景滚动速度
     * @param nearSpeed 近景滚动速度
     * @param farSpeed 远景滚动速度
     */
    void setBackgroundSpeed(int nearSpeed, int farSpeed) {
        nearStars.speed = nearSpeed;
        farStars.speed = farSpeed;
    }
};

#endif // GAME_H