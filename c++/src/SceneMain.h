#ifndef SCENE_MAIN_H
#define SCENE_MAIN_H

#include "Scene.h"
#include "Object.h"
#include <list>
#include <random>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "ObjectPool.h" 

class Game;

// 游戏主场景
class SceneBoss; // 前向声明

// 场景过渡状态枚举
enum class TransitionState {
    NORMAL,           // 正常游戏状态
    PREPARING_BOSS,   // 准备Boss战：敌人退场，停止射击
    WAITING_CLEAR,    // 等待清理完成
    MOVING_TO_BOSS,   // 主角移动到目标位置
    READY_FOR_BOSS    // 准备完成，可以切换场景
};

class SceneMain : public Scene
{
public:
    SceneMain() : Scene(), uiShield(nullptr), uiHealth(nullptr), scoreFont(nullptr), bgm(nullptr)
    {
        // 初始化player的位置
        player.position.x = 400;
        player.position.y = 300;
    }
    void update(float deltaTime) override; // 更新逻辑
    void render() override; // 渲染
    void handleEvent(SDL_Event* event) override; // 处理输入
    void init() override; // 初始化
    void clean() override; // 清理资源
    bool shouldChangeToBoss = false; // 标记是否需要切换到Boss场景
    bool enemiesRetreating = false; // 敌人是否正在退场
   
private:
    Player player; // 玩家对象
    Mix_Music* bgm; // 背景音乐
    SDL_Texture* uiHealth; // 血量UI纹理
    SDL_Texture* uiShield = nullptr; // 护盾UI纹理
    SDL_Texture* bouncedBulletTexture = nullptr; // 反弹后的子弹纹理
    TTF_Font* scoreFont; // 分数字体
    int score = 0; // 当前分数
    float timerEnd = 0.0f; // 结束计时

    bool isDead = false; // 玩家是否死亡
    bool isPaused = false; // 游戏是否暂停
    std::mt19937 gen; // 随机数生成器
    std::uniform_real_distribution<float> dis; // 随机数分布器

    // 过渡状态管理
    TransitionState transitionState = TransitionState::NORMAL;
    float transitionTimer = 0.0f;
    SDL_FPoint targetPlayerPosition = {100.0f, 300.0f}; // Boss场景中主角的目标位置
    float playerMoveSpeed = 200.0f; // 主角移动到目标位置的速度
    
    // 模板对象
    Enemy enemyTemplate; // 敌人模板
    Enemy enemyTemplate1; // 敌人1模板
    Enemy enemyTemplate2; // 敌人2模板
    ProjectilePlayer projectilePlayerTemplate; // 玩家子弹模板
    ProjectileEnemy projectileEnemyTemplate; // 敌人子弹模板
    Explosion explosionTemplate; // 爆炸模板
    Item itemLifeTemplate; // 生命道具模板
    Item itemShieldTemplate; // 护盾道具模板
    Item itemTimeTemplate; // 时间道具模板
    Item itemGoldTemplate; // 金币道具模板（带动画）

    // 游戏对象容器
    std::list<Enemy*> enemies; // 敌人列表
    std::list<ProjectilePlayer*> projectilesPlayer; // 玩家子弹列表
    std::list<ProjectileEnemy*> projectilesEnemy; // 敌人子弹列表
    std::list<Explosion*> explosions; // 爆炸列表
    std::list<Item*> items; // 道具列表
    std::map<std::string, Mix_Chunk*> sounds; // 音效

    // 渲染相关
    void renderItems(); // 渲染道具
    void renderUI(); // 渲染UI
    void renderExplosions(); // 渲染爆炸
    void renderPlayerProjectiles(); // 渲染玩家子弹
    void renderEnemyProjectiles(); // 渲染敌人子弹
    void renderEnemies(); // 渲染敌人

    // 更新相关
    void updateEnemies(float deltaTime); // 更新敌人
    void updateEnemyProjectiles(float deltaTime); // 更新敌人子弹
    void updatePlayer(float deltaTime); // 更新玩家
    void updateItems(float deltaTime); // 更新道具
    void updateExplosions(float deltaTime); // 更新爆炸
    void updatePlayerProjectiles(float deltaTime); // 更新玩家子弹
    void keyboardControl(float deltaTime); // 键盘控制
    void spawEnemy(); // 生成敌人
    void changeSceneDelayed(float deltaTime, float delay); // 延迟切换场景

    // 其它
    void playerGetItem(Item* item); // 玩家获得道具
    void shootPlayer(); // 玩家射击
    void shootEnemy(Enemy* enemy); // 敌人射击
    SDL_FPoint getDirection(Enemy *enemy) const; // 获取敌人射击方向
    void enemyExplode(Enemy* enemy); // 敌人爆炸
    void dropItem(Enemy* enemy); // 敌人掉落道具
    void dropGold(float x, float y); // 新增金币掉落函数
    void renderPauseOverlay(); // 渲染暂停覆盖层
    // 敌人2的多方向射击函数
    void shootEnemyMultiDirection(Enemy* enemy, int bulletCount);
    
    // 添加对象池
    ObjectPool<ProjectilePlayer> playerBulletPool;
    ObjectPool<ProjectileEnemy> enemyBulletPool;
    ObjectPool<Explosion> explosionPool; // 爆炸对象池

    
    // 新增过渡相关函数
    void updateTransition(float deltaTime); // 更新过渡状态
    void startBossTransition(); // 开始Boss过渡
    bool areAllBulletsCleared(); // 检查所有子弹是否已清理
    void movePlayerToTarget(float deltaTime); // 移动主角到目标位置
    void renderTransitionEffect(); // 渲染过渡效果
    
    // 添加缺少的函数声明
    void clearAllEnemiesAndBullets();
    
    // 武器升级系统
    bool weaponUpgradeAvailable = false;    // 是否有武器升级可用
    bool weaponUpgradePaused = false;       // 武器升级暂停状态
    std::vector<WeaponUpgrade> upgradeOptions; // 当前升级选项
    size_t selectedUpgrade = 0;             // 当前选中的升级（改为size_t类型）
    int lastUpgradeScore = 0;               // 上次升级分数（新增成员变量）
    
    // 武器升级相关函数
    void checkWeaponUpgrade();              // 检查是否触发武器升级
    void generateUpgradeOptions();          // 生成升级选项
    void applyWeaponUpgrade(WeaponUpgrade upgrade); // 应用武器升级
    void renderWeaponUpgradeUI();           // 渲染武器升级界面
    void handleWeaponUpgradeInput(SDL_Event* event); // 处理武器升级输入
    
    // 修改射击函数以支持武器系统
    void shootPlayerWithWeapon();           // 使用武器系统的射击
    void createSplitBullets(SDL_FPoint startPos, SDL_FPoint direction, int count); // 创建分裂子弹
    std::string getUpgradeText(WeaponUpgrade upgrade); // 获取升级选项文本
};

#endif // SCENE_MAIN_H

