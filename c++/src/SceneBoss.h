#ifndef SCENEBOSS_H
#define SCENEBOSS_H

#include "Scene.h"
#include "Object.h"
#include "ObjectPool.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <list>
#include <map>
#include <random>

class SceneBoss : public Scene
{
public:
    SceneBoss(int playerScore = 0);
    SceneBoss(int playerScore, const Player& mainPlayer); // 构造函数
    void update(float deltaTime) override;
    void render() override;
    void handleEvent(SDL_Event* event) override;
    void init() override;
    void clean() override;

private:
    Player player;                          // 玩家对象
    Boss boss;                              // Boss对象
    TTF_Font* scoreFont;                    // 分数字体
    SDL_Texture* uiHealth;                  // 血量UI纹理
    SDL_Texture* uiShield;                  // 护盾UI纹理
    int score;                              // 当前分数
    bool isDead = false;                    // 玩家是否死亡
    bool bossDefeated = false;              // Boss是否被击败
    bool isPaused = false;                  // 游戏是否暂停
    float timerEnd = 0.0f;                  // 结束计时
    
    // 添加Boss连续爆炸相关变量
    bool bossExploding = false;             // Boss是否正在连续爆炸
    float explosionTimer = 0.0f;            // 爆炸计时器
    int explosionCount = 0;                 // 已发生的爆炸次数
    const int maxExplosions = 8;            // 最大爆炸次数
    const float explosionInterval = 0.1f;   // 爆炸间隔时间
    
    // 添加Boss出场动画相关变量
    bool bossEntering = true;               // Boss是否正在出场
    float bossTargetX;                      // Boss目标X位置
    float bossEnterSpeed = 100.0f;          // Boss出场速度
    
    std::mt19937 gen;                       // 随机数生成器
    std::uniform_real_distribution<float> dis; // 随机数分布器
    
    // 模板对象
    ProjectilePlayer projectilePlayerTemplate;
    ProjectileBoss projectileBossTemplate;
    Explosion explosionTemplate;
    
    // 游戏对象容器
    std::list<ProjectilePlayer*> projectilesPlayer;
    std::list<ProjectileBoss*> projectilesBoss;
    std::list<Explosion*> explosions;
    std::map<std::string, Mix_Chunk*> sounds;
    
    // 对象池
    ObjectPool<ProjectilePlayer> playerBulletPool;
    ObjectPool<ProjectileBoss> bossBulletPool;
    ObjectPool<Explosion> explosionPool;
    
    // 渲染相关
    void renderUI();
    void renderExplosions();
    void renderPlayerProjectiles();
    void renderBossProjectiles();
    void renderBoss();
    void renderPauseOverlay();
    
    // 更新相关
    void updatePlayer(float deltaTime);
    void updateBoss(float deltaTime);
    void updatePlayerProjectiles(float deltaTime);
    void updateBossProjectiles(float deltaTime);
    void updateExplosions(float deltaTime);
    void keyboardControl(float deltaTime);
    void changeSceneDelayed(float deltaTime, float delay);
    
    // 射击相关
    void shootPlayer();
    void shootBossPattern1();  // 圆形弹幕
    void shootBossPattern2();  // 螺旋弹幕
    void shootBossPattern3();  // 扇形弹幕
    
    // 其他
    void bossExplode();
    void createSingleExplosion();           // 创建单个爆炸效果
};

#endif // SCENEBOSS_H