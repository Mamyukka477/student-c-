#include "SceneTitle.h"
#include "SceneMain.h"
#include "SceneEnd.h"
#include "SceneBoss.h"
#include "Game.h"
#include "ObjectPool.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <random>
#include <cmath>
#include <algorithm>  // 添加这个头文件用于std::shuffle
#include <vector>     // 添加这个头文件用于std::vector

// 将宏定义转换为 constexpr 常量
constexpr double M_PI = 3.14159265358979323846;

void SceneMain::update(float deltaTime)
{
    auto& game = Game::getInstance();
    
    // 如果武器升级暂停，只处理升级逻辑
    if (weaponUpgradePaused) {
        return;
    }
    
    // 如果游戏暂停，跳过所有更新逻辑
    if (isPaused) {
        return;
    }
    
    // 检查武器升级
    checkWeaponUpgrade();
    
    // 检查是否需要开始Boss过渡
    if (shouldChangeToBoss) {
        auto& game = Game::getInstance();
        // 传递玩家状态到Boss场景 - 使用正确的构造函数
        game.changeScene(new SceneBoss(score, player));
        return;
    }
    
    // 处理过渡状态
    if (transitionState != TransitionState::NORMAL) {
        updateTransition(deltaTime);
        // 在过渡期间继续更新子弹、爆炸效果等，但不生成新敌人
        keyboardControl(deltaTime); // 允许玩家在过渡期间移动
        updatePlayerProjectiles(deltaTime); // 继续更新玩家子弹
        updateEnemyProjectiles(deltaTime); // 继续更新敌人子弹
        updateEnemies(deltaTime); // 【修复】添加敌人更新，让敌人能够退场
        updateExplosions(deltaTime); // 继续更新爆炸效果
        updateItems(deltaTime); // 继续更新道具
        updatePlayer(deltaTime); // 更新玩家状态
        // 注意：不调用spawEnemy()，不生成新敌人
        return;
    }
    
    keyboardControl(deltaTime); // 处理玩家输入
    updatePlayerProjectiles(deltaTime); // 更新玩家子弹
    updateEnemyProjectiles(deltaTime); // 更新敌人子弹
    spawEnemy(); // 生成敌人
    updateEnemies(deltaTime); // 更新敌人
    updatePlayer(deltaTime); // 更新玩家状态
    updateExplosions(deltaTime); // 更新爆炸效果
    updateItems(deltaTime); // 更新道具
    if (isDead){
        changeSceneDelayed(deltaTime, 3); // 3秒后切换到标题场景
    }
}

void SceneMain::render()
{
    auto& game = Game::getInstance();
    
    // 渲染背景
    game.renderBackground();
    
    // 渲染玩家
    if (!isDead){
        SDL_FRect playerRect = {
            player.position.x, 
            player.position.y, 
            player.width, 
            player.height
        };
        // 使用SDL_RenderTextureRotated来支持翻转效果
        SDL_RenderTextureRotated(game.getRenderer(), player.texture, NULL, &playerRect, 0, NULL, player.flip);
    }
    
    // 渲染其他游戏对象
    renderEnemies();
    renderPlayerProjectiles();
    renderEnemyProjectiles();
    renderExplosions();
    renderItems();
    renderUI();
    
    // 渲染过渡效果
    if (transitionState != TransitionState::NORMAL) {
        renderTransitionEffect();
    }
    
    // 渲染武器升级界面
    if (weaponUpgradeAvailable) {
        renderWeaponUpgradeUI();
    }
    
    if (isPaused) {
        renderPauseOverlay();
    }
}

void SceneMain::handleEvent(SDL_Event* event)
{
    // 处理武器升级输入
    if (weaponUpgradeAvailable) {
        handleWeaponUpgradeInput(event);
        return;
    }
    
    if (event->type == SDL_EVENT_KEY_DOWN){
        if (event->key.scancode == SDL_SCANCODE_ESCAPE){
            auto sceneTitle = new SceneTitle();
            game.changeScene(sceneTitle); // 按ESC返回标题场景
        }
        // 新增：回车键暂停/恢复游戏
        else if (event->key.scancode == SDL_SCANCODE_RETURN){
            isPaused = !isPaused;
        }
    }
}

// 在init函数中（约第59行后）
void SceneMain::init()
{
    // 游戏场景播放音乐教室.mp3
    Game::getInstance().playBgm("assets/music/音乐教室.mp3");
    uiHealth = IMG_LoadTexture(game.getRenderer(), "assets/image/Health UI Black.png"); // 读取血量UI
    uiShield = IMG_LoadTexture(game.getRenderer(), "assets/image/护盾.png"); // 读取护盾UI（新增）
    scoreFont = TTF_OpenFont("assets/font/VonwaonBitmap-12px.ttf", 24); // 载入字体
    
    // 重置武器升级系统
    weaponUpgradeAvailable = false;
    weaponUpgradePaused = false;
    lastUpgradeScore = 0;  // 重置升级分数
    upgradeOptions.clear();
    selectedUpgrade = 0;
    
    // 加载衰减子弹纹理
    bouncedBulletTexture = IMG_LoadTexture(game.getRenderer(), "assets/image/衰减子弹.png");
    if (bouncedBulletTexture == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load bounced bullet texture: %s", SDL_GetError());
    }

    // 读取音效资源
    sounds["player_shoot"] = Mix_LoadWAV("assets/sound/laser_shoot4.mp3");
    sounds["enemy_shoot"] = Mix_LoadWAV("assets/sound/xs_laser.mp3");
    sounds["player_explode"] = Mix_LoadWAV("assets/sound/角色死亡音效.mp3");
    sounds["enemy_explode"] = Mix_LoadWAV("assets/sound/explosion3.mp3");
    sounds["hit"] = Mix_LoadWAV("assets/sound/eff11.mp3");
    sounds["get_item"] = Mix_LoadWAV("assets/sound/eff5.mp3");

    std::random_device rd;
    gen = std::mt19937(rd());
    dis = std::uniform_real_distribution<float>(0.0f, 1.0f);
    
    player.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/SpaceShip.png"); // 加载玩家纹理
    if (player.texture == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load player texture: %s", SDL_GetError());
    }
    SDL_GetTextureSize(player.texture, &player.width, &player.height);
    player.width /= 5;
    player.height /= 5;
    // 将玩家位置置于屏幕左侧中央
    player.position.x = 0;
    player.position.y = game.getWindowHeight() / 2 - player.height / 2;
    
    // 根据难度调整玩家射击速度和初始护盾
    int difficulty = game.getDifficulty();
    switch(difficulty) {
        case 0: // 简单模式 - 保持原有数据
            player.coolDown = 270;
            player.currentShield = 3;  // 简单模式初始3护盾
            break;
        case 1: // 普通难度 - 降低玩家子弹发射速度
            player.coolDown = 330; // 微微降低发射时间
            player.currentShield = 0; // 普通模式无初始护盾
            break;
        case 2: // 困难难度 - 降低玩家发射速度
            player.coolDown = 600; // 增加发射冷却时间
            player.currentShield = 0; // 困难模式无初始护盾
            break;
        default:
            player.coolDown = 300;
            player.currentShield = 0;
            break;
    }
    
    // 初始化各类模板对象
    projectilePlayerTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/子弹.png");
    SDL_GetTextureSize(projectilePlayerTemplate.texture, &projectilePlayerTemplate.width, &projectilePlayerTemplate.height);
    projectilePlayerTemplate.width /= 4;
    projectilePlayerTemplate.height /= 4;

    // 敌人0模板 - 加载随机纹理
    for (int i = 0; i < 10; i++) {
        std::string texturePath = "assets/image/随机敌人" + std::to_string(i) + ".png";
        enemyTemplate.randomTextures[i] = IMG_LoadTexture(game.getRenderer(), texturePath.c_str());
        if (enemyTemplate.randomTextures[i] == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", texturePath.c_str());
        }
    }
    // 设置默认纹理为第一个随机纹理
    enemyTemplate.texture = enemyTemplate.randomTextures[0];
    SDL_GetTextureSize(enemyTemplate.texture, &enemyTemplate.width, &enemyTemplate.height);
    enemyTemplate.width /= 4;
    enemyTemplate.height /= 4;
    enemyTemplate.speed = 140;
    enemyTemplate.currentHealth = 7;
    // 根据难度调整敌人射击速度
    switch(difficulty) {
        case 0: // 简单模式
            enemyTemplate.coolDown = 2000;
            break;
        case 1: // 普通难度 - 保持原有敌人射击速度
            enemyTemplate.coolDown = 2000;
            break;
        case 2: // 困难难度 - 提高敌人射击速度
            enemyTemplate.coolDown = 1200; // 减少冷却时间，提高射击速度
            break;
        default:
            enemyTemplate.coolDown = 2000;
            break;
    }
    enemyTemplate.type = 0;  // 设置为敌人0类型

    // 敌人1模板
    enemyTemplate1.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/敌人1.png");
    SDL_GetTextureSize(enemyTemplate1.texture, &enemyTemplate1.width, &enemyTemplate1.height);
    enemyTemplate1.width /= 3;
    enemyTemplate1.height /= 3;
    enemyTemplate1.speed = 200; // 更快
    enemyTemplate1.currentHealth = 1; // 血量少
    // 根据难度调整敌人1射击速度
    switch(difficulty) {
        case 0: // 简单模式
            enemyTemplate1.coolDown = 1500;
            break;
        case 1: // 普通难度
            enemyTemplate1.coolDown = 1500;
            break;
        case 2: // 困难难度 - 提高射击速度
            enemyTemplate1.coolDown = 900;
            break;
        default:
            enemyTemplate1.coolDown = 1500;
            break;
    }
    enemyTemplate1.type = 1;  // 设置为敌人1类型

    // 敌人2模板
    enemyTemplate2.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/敌人2.png");
    SDL_GetTextureSize(enemyTemplate2.texture, &enemyTemplate2.width, &enemyTemplate2.height);
    enemyTemplate2.width /= 2;
    enemyTemplate2.height /= 2;
    enemyTemplate2.speed = 100; // 更慢
    enemyTemplate2.currentHealth = 4; // 血量多
    // 根据难度调整敌人2射击速度
    switch(difficulty) {
        case 0: // 简单模式
            enemyTemplate2.coolDown = 3000;
            break;
        case 1: // 普通难度
            enemyTemplate2.coolDown = 3000;
            break;
        case 2: // 困难难度 - 提高射击速度
            enemyTemplate2.coolDown = 1800;
            break;
        default:
            enemyTemplate2.coolDown = 3000;
            break;
    }
    enemyTemplate2.type = 2;  // 设置为敌人2类型

    projectileEnemyTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/敌人子弹.png");
    SDL_GetTextureSize(projectileEnemyTemplate.texture, &projectileEnemyTemplate.width, &projectileEnemyTemplate.height);
    projectileEnemyTemplate.width /= 2;
    projectileEnemyTemplate.height /= 2;

    explosionTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/effect/explosion.png");
    SDL_GetTextureSize(explosionTemplate.texture, &explosionTemplate.width, &explosionTemplate.height);
    explosionTemplate.totlaFrame = static_cast<int>(explosionTemplate.width / explosionTemplate.height);
    float newHeight = explosionTemplate.height * 2.0f;  // 直接使用float类型
    explosionTemplate.height = newHeight;
    explosionTemplate.width = explosionTemplate.height;  // 都是float，无需转换

    itemLifeTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/bonus_life.png");
    SDL_GetTextureSize(itemLifeTemplate.texture, &itemLifeTemplate.width, &itemLifeTemplate.height);
    itemLifeTemplate.width /= 4;
    itemLifeTemplate.height /= 4;
    itemLifeTemplate.type = ItemType::Life;
    
    // 新增护盾道具模板
    itemShieldTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/bonus_shield.png");
    SDL_GetTextureSize(itemShieldTemplate.texture, &itemShieldTemplate.width, &itemShieldTemplate.height);
    itemShieldTemplate.width /= 4;
    itemShieldTemplate.height /= 4;
    itemShieldTemplate.type = ItemType::Shield;
    
    // 新增时间道具模板
    itemTimeTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/超级奖励.png");
    SDL_GetTextureSize(itemTimeTemplate.texture, &itemTimeTemplate.width, &itemTimeTemplate.height);
    itemTimeTemplate.width /= 4;
    itemTimeTemplate.height /= 4;
    itemTimeTemplate.type = ItemType::Time;
    
    // 新增金币道具模板（动画效果）
    itemGoldTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/effect/金币(gold_coin)_爱给网_aigei_com.png");
    SDL_GetTextureSize(itemGoldTemplate.texture, &itemGoldTemplate.width, &itemGoldTemplate.height);
    itemGoldTemplate.totlaFrame = static_cast<int>(itemGoldTemplate.width / itemGoldTemplate.height); // 8帧动画
    itemGoldTemplate.width = static_cast<float>(itemGoldTemplate.height); // 单帧宽度
    //itemGoldTemplate.width /= 2;金币大小调整现在挺好
    //itemGoldTemplate.height /= 2;
    itemGoldTemplate.type = ItemType::Gold;
    
    // 初始化对象池（在模板对象设置完成后）
    playerBulletPool.initialize(projectilePlayerTemplate, 50);
    enemyBulletPool.initialize(projectileEnemyTemplate, 200);
    explosionPool.initialize(explosionTemplate, 20); // 初始化爆炸对象池
}
void SceneMain::clean()
{
    // 清理容器
    for (const auto& sound : sounds){
        if (sound.second != nullptr){
            Mix_FreeChunk(sound.second);
        }
    }
    sounds.clear();
    
    // 清理子弹列表（不需要 delete，对象池会自动管理）
    for (auto &projectile : projectilesPlayer) {
        playerBulletPool.release(projectile);
    }
    projectilesPlayer.clear();

    for (auto &enemy : enemies){
        if (enemy != nullptr){
            delete enemy;
        }
    }
    enemies.clear();

    for (auto &projectile : projectilesEnemy) {
        enemyBulletPool.release(projectile);
    }
    projectilesEnemy.clear();

    // 清理爆炸列表（使用对象池释放）
    for (auto &explosion : explosions) {
        explosionPool.release(explosion);
    }
    explosions.clear();

    for (auto &item : items){
        if (item != nullptr){
            delete item;
        }
    }
    items.clear();

    // 清理ui
    if (uiHealth != nullptr){
        SDL_DestroyTexture(uiHealth);
    }
    if (uiShield != nullptr){ // 新增
        SDL_DestroyTexture(uiShield);
    }
    if (bouncedBulletTexture != nullptr) {
        SDL_DestroyTexture(bouncedBulletTexture);
        bouncedBulletTexture = nullptr;
    }
    // 清理字体
    if (scoreFont != nullptr){
        TTF_CloseFont(scoreFont);
    }

    // 清理模版
    if (player.texture != nullptr)
    {
        SDL_DestroyTexture(player.texture);
    }
    if (projectilePlayerTemplate.texture != nullptr)
    {
        SDL_DestroyTexture(projectilePlayerTemplate.texture);
    }
    if (enemyTemplate.texture != nullptr){
        SDL_DestroyTexture(enemyTemplate.texture);
    }
    // 清理随机敌人纹理
    for (int i = 0; i < 10; i++) {
        if (enemyTemplate.randomTextures[i] != nullptr) {
            SDL_DestroyTexture(enemyTemplate.randomTextures[i]);
            enemyTemplate.randomTextures[i] = nullptr;
        }
    }
    // 清理新敌人纹理
    if (enemyTemplate1.texture != nullptr){
        SDL_DestroyTexture(enemyTemplate1.texture);
    }
    if (enemyTemplate2.texture != nullptr){
        SDL_DestroyTexture(enemyTemplate2.texture);
    }
    if (projectileEnemyTemplate.texture != nullptr){
        SDL_DestroyTexture(projectileEnemyTemplate.texture);
    }
    if (explosionTemplate.texture != nullptr){
        SDL_DestroyTexture(explosionTemplate.texture);
    }
    if (itemLifeTemplate.texture != nullptr){
        SDL_DestroyTexture(itemLifeTemplate.texture);
    }
}

void SceneMain::keyboardControl(float deltaTime)
{
    if (isDead){
        return;
    }
    const bool* keyboardState = SDL_GetKeyboardState(NULL);
    
    // 基础移动速度
    float currentSpeed = static_cast<float>(player.speed);
    
    // J键加速功能
    if (keyboardState[SDL_SCANCODE_J]){
        currentSpeed *= 1.3f; // 加30%
    }
    
    // K键作弊功能 - 标记需要切换到Boss战
    if (keyboardState[SDL_SCANCODE_K]) {
        startBossTransition();
        return;
    }
    
    // 允许在所有状态下移动（除了死亡状态）
    if (keyboardState[SDL_SCANCODE_W]){
        player.position.y -= deltaTime * currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_S]){
        player.position.y += deltaTime * currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_A]){
        player.position.x -= deltaTime * currentSpeed;
        player.flip = SDL_FLIP_HORIZONTAL; // 向左移动时水平翻转
    }
    if (keyboardState[SDL_SCANCODE_D]){
        player.position.x += deltaTime * currentSpeed;
        player.flip = SDL_FLIP_NONE; // 向右移动时恢复正常
    }
    
    // 限制飞机的移动范围
    if (player.position.x < 0){
        player.position.x = 0;
    }
    if (player.position.x > Game::getInstance().getWindowWidth() - player.width){
        player.position.x = Game::getInstance().getWindowWidth() - player.width;
    }
    if (player.position.y < 0){
        player.position.y = 0;
    }
    if (player.position.y > Game::getInstance().getWindowHeight() - player.height){
        player.position.y = Game::getInstance().getWindowHeight() - player.height;
    }

    // 只在正常状态下自动发射子弹
    if (transitionState == TransitionState::NORMAL) {
        shootPlayerWithWeapon();
    }
}

void SceneMain::shootPlayer()
{
    // 在这里实现发射子弹的逻辑
    auto* projectile = playerBulletPool.create();
    if (projectile == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Player bullet pool exhausted!");
        return;
    }
    
    // 设置子弹属性
    *projectile = projectilePlayerTemplate;  // 重置为模板状态
    
    // 根据主角朝向设置子弹位置和方向
    if (player.flip == SDL_FLIP_HORIZONTAL) {
        // 向左发射
        projectile->position.x = player.position.x - projectile->width;
        projectile->direction = {-1, 0}; // 向左移动
    } else {
        // 向右发射
        projectile->position.x = player.position.x + player.width;
        projectile->direction = {1, 0}; // 向右移动
    }
    
    projectile->position.y = player.position.y + player.height / 2 - projectile->height / 2;
    projectile->bounceCount = 0; // 初始化弹射次数
    projectilesPlayer.push_back(projectile);
    Mix_PlayChannel(0, sounds["player_shoot"], 0);
}

void SceneMain::updatePlayerProjectiles(float deltaTime)
{
    int margin = 32; // 子弹超出屏幕外边界的距离
    for (auto it = projectilesPlayer.begin(); it != projectilesPlayer.end();){
        auto* projectile = *it; // 明确为指针，避免复制
        projectile->position.x += projectile->speed * deltaTime * projectile->direction.x;
        projectile->position.y += projectile->speed * deltaTime * projectile->direction.y;
        
        // 检查边界弹射（只在屏幕边缘）
        bool shouldDelete = false;
        
        // 上下边界弹射
        if (projectile->position.y <= 0 || projectile->position.y >= game.getWindowHeight() - projectile->height) {
            if (projectile->bounceCount < projectile->maxBounces) {
                projectile->direction.y = -projectile->direction.y; // 垂直方向反弹
                projectile->bounceCount++;
                // 确保子弹不会卡在边界
                if (projectile->position.y <= 0) projectile->position.y = 0;
                if (projectile->position.y >= game.getWindowHeight() - projectile->height) 
                    projectile->position.y = game.getWindowHeight() - projectile->height;
                
                // 更改子弹材质为衰减子弹
                if (bouncedBulletTexture != nullptr) {
                    projectile->texture = bouncedBulletTexture;
                }
            } else {
                shouldDelete = true;
            }
        }
        
        // 左右边界弹射
        if (projectile->position.x <= 0 || projectile->position.x >= game.getWindowWidth() - projectile->width) {
            if (projectile->bounceCount < projectile->maxBounces) {
                projectile->direction.x = -projectile->direction.x; // 水平方向反弹
                projectile->bounceCount++;
                // 确保子弹不会卡在边界
                if (projectile->position.x <= 0) projectile->position.x = 0;
                if (projectile->position.x >= game.getWindowWidth() - projectile->width) 
                    projectile->position.x = game.getWindowWidth() - projectile->width;
                
                // 更改子弹材质为衰减子弹
                if (bouncedBulletTexture != nullptr) {
                    projectile->texture = bouncedBulletTexture;
                }
            } else {
                shouldDelete = true;
            }
        }
        
        // 检查子弹是否超出屏幕边界（用于删除）
        if (projectile->position.x < -margin || projectile->position.x > game.getWindowWidth() + margin ||
            projectile->position.y < -margin || projectile->position.y > game.getWindowHeight() + margin){
            shouldDelete = true;
        }
        
        if (shouldDelete) {
            playerBulletPool.release(projectile);
            it = projectilesPlayer.erase(it);
        } else {
            bool hit = false;
            for (auto enemy : enemies){
                SDL_FRect enemyRect = {
                    enemy->position.x,
                    enemy->position.y,
                    enemy->width,
                    enemy->height
                };
                SDL_FRect projectileRect = {
                    projectile->position.x,
                    projectile->position.y,
                    projectile->width,
                    projectile->height
                };
                if (SDL_HasRectIntersectionFloat(&enemyRect, &projectileRect)){
                    // 检查这颗子弹是否已经击中过这个敌人（防止穿透子弹帧伤）
                    bool alreadyHit = false;
                    for (auto hitEnemy : projectile->hitEnemies) {
                        if (hitEnemy == enemy) {
                            alreadyHit = true;
                            break;
                        }
                    }
                    
                    if (!alreadyHit) {
                        enemy->currentHealth -= projectile->damage;
                        hit = true;
                        Mix_PlayChannel(-1, sounds["hit"], 0);
                        
                        // 将敌人添加到已击中列表
                        projectile->hitEnemies.push_back(enemy);
                        
                        // 如果子弹没有穿透能力，则删除子弹
                        if (!player.weapon.piercing) {
                            playerBulletPool.release(projectile);
                            it = projectilesPlayer.erase(it);
                            break;
                        }
                    }
                }
            }
            if (!hit || player.weapon.piercing){
                ++it;
            }
        }
    }
}

void SceneMain::renderPlayerProjectiles()
{
    for (auto* projectile : projectilesPlayer){
        SDL_FRect projectileRect = {
            projectile->position.x,
            projectile->position.y,
            projectile->width,
            projectile->height
        };
        SDL_RenderTexture(game.getRenderer(), projectile->texture, NULL, &projectileRect);
    }
}

void SceneMain::renderEnemyProjectiles()
{

    for (auto projectile : projectilesEnemy){
        SDL_FRect projectileRect = {
            projectile->position.x,
            projectile->position.y,
            projectile->width,
            projectile->height
        };
        float angle = static_cast<float>(atan2(projectile->direction.y, projectile->direction.x) * 180 / M_PI - 90);
        SDL_RenderTextureRotated(game.getRenderer(), projectile->texture, NULL, &projectileRect, angle, NULL, SDL_FLIP_NONE);
    }
}

void SceneMain::spawEnemy()
{
    // 过渡期间停止生成敌人
    if (transitionState != TransitionState::NORMAL) {
        return;
    }
    
    if (dis(gen) > 1 / 60.0f){
        return;
    }
    
    Enemy* enemy = nullptr;
    // 随机选择敌人类型（概率：原敌人10%，敌人1 70%，敌人2 20%）
    float randomValue = dis(gen);
    if (randomValue < 0.1f) {
        enemy = new Enemy(enemyTemplate);  // 10% 概率
        // 为敌人0随机选择纹理
        if (enemy->type == 0) {
            enemy->currentTextureIndex = static_cast<int>(dis(gen) * 10); // 0-9随机
            enemy->texture = enemy->randomTextures[enemy->currentTextureIndex];
        }
    } else if (randomValue < 0.8f) {
        enemy = new Enemy(enemyTemplate1); // 70% 概率
    } else {
        enemy = new Enemy(enemyTemplate2); // 20% 概率
    }
    
    // 敌人从屏幕右侧随机Y位置生成
    enemy->position.x = game.getWindowWidth();
    enemy->position.y = dis(gen) * (game.getWindowHeight() - enemy->height);
    enemies.push_back(enemy);
}

void SceneMain::changeSceneDelayed(float deltaTime, float delay)
{
    timerEnd += deltaTime;
    if (timerEnd > delay){
        auto sceneEnd = new SceneEnd();
        game.changeScene(sceneEnd);
    }
}

void SceneMain::updateEnemies(float deltaTime)
{
    auto currentTime = SDL_GetTicks();
    for (auto it = enemies.begin(); it != enemies.end();){
        auto enemy = *it;
        
        // 过渡期间让敌人向右移动退场
        if (transitionState == TransitionState::PREPARING_BOSS || enemiesRetreating) {
            enemy->position.x += enemy->speed * deltaTime * 2.0f; // 加速退场
            
            // 敌人移出屏幕右侧时删除
            if (enemy->position.x > game.getWindowWidth() + enemy->width) {
                delete enemy;
                it = enemies.erase(it);
                continue;
            }
            ++it;
            continue;
        }
        
        // 敌人向左移动
        enemy->position.x -= enemy->speed * deltaTime;
        
        // 敌人1的上下移动行为
        if (enemy->type == 1) {
            enemy->moveTimer += deltaTime;
            // 使用正弦函数产生平滑的上下移动
            float offsetY = sin(enemy->moveTimer * 3.0f) * 2.0f; // 小幅度移动
            enemy->position.y += offsetY;
            
            // 确保不会移出屏幕
            if (enemy->position.y < 0) {
                enemy->position.y = 0;
            } else if (enemy->position.y > game.getWindowHeight() - enemy->height) {
                enemy->position.y = game.getWindowHeight() - enemy->height;
            }
        }
        
        // 敌人2的旋转行为
        if (enemy->type == 2) {
            enemy->rotationAngle += 90.0f * deltaTime; // 每秒旋转90度
            if (enemy->rotationAngle >= 360.0f) {
                enemy->rotationAngle -= 360.0f;
            }
        }
        
        // 当敌人移出屏幕左侧时删除
        if (enemy->position.x < -enemy->width){
            delete enemy;
            it = enemies.erase(it);
        }else {
            // 敌人射击行为 - 敌人0不射击，敌人1正常射击，敌人2多方向射击
            if (currentTime - enemy->lastShootTime > enemy->coolDown && isDead == false){
                // 敌人0不射击
                if (enemy->type == 0) {
                    // 不执行射击
                } 
                // 敌人1正常射击
                else if (enemy->type == 1) {
                    shootEnemy(enemy);
                }
                // 敌人2多方向射击
                else if (enemy->type == 2) {
                    shootEnemyMultiDirection(enemy, 5); // 发射5个子弹
                }
                
                enemy->lastShootTime = static_cast<Uint32>(currentTime);
            }
            
            if (enemy->currentHealth <= 0){
                enemyExplode(enemy);
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void SceneMain::updateEnemyProjectiles(float deltaTime)
{
    auto margin = 32;
    for (auto it = projectilesEnemy.begin(); it != projectilesEnemy.end();){
        auto projectile = *it;
        projectile->position.x += projectile->speed * projectile ->direction.x * deltaTime;
        projectile->position.y += projectile->speed * projectile->direction.y * deltaTime;
        if (projectile->position.y > game.getWindowHeight() + margin ||
            projectile->position.y < - margin ||
            projectile->position.x < - margin ||
            projectile->position.x > game.getWindowWidth() + margin){
            enemyBulletPool.release(*it);
            it = projectilesEnemy.erase(it);
        }else {
            SDL_FRect projectileRect = {
                projectile->position.x,
                projectile->position.y,
                projectile->width,
                projectile->height
            };
            
            SDL_FRect playerRect = {
                player.position.x + player.width * 0.2f,  // 缩小碰撞范围
                player.position.y + player.height * 0.2f,
                player.width * 0.75f,   // 碰撞宽度为实际的75%
                player.height * 0.60f   // 碰撞高度为实际的60%
            };

            if (SDL_HasRectIntersectionFloat(&projectileRect, &playerRect) && !isDead){
                // 优先扣除护盾
                if (player.currentShield > 0) {
                    player.currentShield--;
                } else {
                    player.currentHealth -= projectile->damage;
                }
                enemyBulletPool.release(*it);
                it = projectilesEnemy.erase(it);
                Mix_PlayChannel(-1, sounds["hit"], 0);
            } else {
                ++it;
            }
        }
    }
}
            
void SceneMain::updatePlayer(float)
{
    if (isDead) {
        return;
    }
    if (player.currentHealth <= 0){
        // 玩家死亡，触发爆炸和切换场景
        auto currentTime = SDL_GetTicks();
        isDead = true;
        auto explosion = explosionPool.create(); // 使用对象池创建
        if (explosion != nullptr) {
            explosion->position.x = player.position.x + player.width / 2 - explosion->width / 2;
            explosion->position.y = player.position.y + player.height / 2 - explosion->height / 2;
            explosion->startTime = static_cast<Uint32>(currentTime);
            explosions.push_back(explosion);
        }
        Mix_PlayChannel(-1, sounds["player_explode"], 0);
        game.setFinalScore(score);
        return;
    }
            for (auto enemy : enemies){
            SDL_FRect enemyRect = {
            enemy->position.x,
            enemy->position.y,
            enemy->width,
            enemy->height
            };
            SDL_FRect playerRect = {
            player.position.x,
            player.position.y,
            player.width,
            player.height
            };
            if (SDL_HasRectIntersectionFloat(&playerRect, &enemyRect)){
            // 优先扣除护盾
            if (player.currentShield > 0) {
            player.currentShield--;
            } else {
            player.currentHealth -= 1;
            }
            enemy->currentHealth = 0;
            }
            }
            }

void SceneMain::renderEnemies()
{
    for (auto enemy : enemies){
        SDL_FRect enemyRect = {
            enemy->position.x,
            enemy->position.y,
            enemy->width,
            enemy->height
        };
        
        // 敌人2需要旋转渲染
        if (enemy->type == 2) {
            SDL_FPoint center = {enemy->width / 2, enemy->height / 2};
            SDL_RenderTextureRotated(game.getRenderer(), enemy->texture, NULL, &enemyRect, enemy->rotationAngle, &center, SDL_FLIP_NONE);
        } else {
            SDL_RenderTexture(game.getRenderer(), enemy->texture, NULL, &enemyRect);
        }
    }
}

void SceneMain::shootEnemy(Enemy *enemy)
{
    auto* projectile = enemyBulletPool.create();
    if (projectile == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy bullet pool exhausted!");
        return;
    }
    
    // 设置子弹属性
    *projectile = projectileEnemyTemplate;  // 重置为模板状态
    projectile->position.x = enemy->position.x + enemy->width / 2 - projectile->width / 2;
    projectile->position.y = enemy->position.y + enemy->height / 2 - projectile->height / 2;
    projectile->direction = getDirection(enemy);
    projectilesEnemy.push_back(projectile);
    Mix_PlayChannel(-1, sounds["enemy_shoot"], 0);
}

SDL_FPoint SceneMain::getDirection(Enemy *enemy) const
{
    auto x = (player.position.x + player.width / 2) - (enemy->position.x + enemy->width / 2);
    auto y = (player.position.y + player.height / 2) - (enemy->position.y + enemy->height / 2);
    auto length = sqrt(x * x + y * y);
    
    // 防止除零错误
    if (length == 0) {
        return SDL_FPoint{0, 0};
    }
    
    x /= length;
    y /= length;
    return SDL_FPoint{x, y};
}

void SceneMain::enemyExplode(Enemy* enemy)
{
    auto currentTime = SDL_GetTicks();
    auto explosion = explosionPool.create(); // 使用对象池创建
    if (explosion != nullptr) {
        explosion->position.x = enemy->position.x + enemy->width / 2 - explosion->width / 2;
        explosion->position.y = enemy->position.y + enemy->height / 2 - explosion->height / 2;
        explosion->startTime = static_cast<Uint32>(currentTime);
        explosions.push_back(explosion);
    }
    Mix_PlayChannel(-1, sounds["enemy_explode"], 0);
    score += 10;
    
    // 检查是否达到Boss战条件
    if (score >= 1300 && !isDead) {
        // 使用安全的过渡方式进入Boss战，与K键逻辑保持一致
        startBossTransition();
        return;
    }
    
    // 随机掉落道具
    if (dis(gen) < 0.3f) { // 30%概率掉落道具
        dropItem(enemy);
    }
    
    // 随机掉落金币
    if (dis(gen) < 0.8f) { // 80%概率掉落金币
        dropGold(enemy->position.x + enemy->width / 2, enemy->position.y + enemy->height / 2);
    }
    
    delete enemy;
}

void SceneMain::updateExplosions(float deltaTime)
{
    auto currentTime = SDL_GetTicks();
    for (auto it = explosions.begin(); it != explosions.end();)
    {
        auto explosion = *it;
        explosion->currentFrame = static_cast<int>((currentTime - explosion->startTime) * explosion->FPS / 1000);
        if (explosion->currentFrame >= explosion->totlaFrame){
            explosionPool.release(explosion); // 使用对象池释放
            it = explosions.erase(it);
        }else{
            ++it;
        }
    }
}

void SceneMain::renderExplosions()
{
    for (auto explosion : explosions)
    {
        SDL_FRect src = {explosion->currentFrame * explosion->width, 0, explosion->width / 2, explosion->height / 2};
        SDL_FRect dst = {
            explosion->position.x, 
            explosion->position.y, 
            explosion->width, 
            explosion->height
        };
        SDL_RenderTexture(game.getRenderer(), explosion->texture, &src, &dst);
    }
}

void SceneMain::dropItem(Enemy *enemy)
{
    // 随机选择掉落的物品类型
    float itemRoll = dis(gen);
    Item* item = nullptr;
    
    if (itemRoll < 0.4f) {
        // 40% 概率掉落生命
        item = new Item(itemLifeTemplate);
    } else if (itemRoll < 0.8f) {
        // 25% 概率掉落护盾
        item = new Item(itemShieldTemplate);
    } else if (itemRoll < 0.9f) {
        // 20% 概率掉落时间
        item = new Item(itemTimeTemplate);
    } else {
        // 15% 概率掉落金币
        item = new Item(itemGoldTemplate);
    }
    
    // 设置物品位置和运动方向（与原有逻辑相同）
    item->position.x = enemy->position.x + enemy->width / 2 - item->width / 2;
    item->position.y = enemy->position.y + enemy->height / 2 - item->height / 2;
    float angle = static_cast<float>(dis(gen) * 2 * M_PI);
    item->direction.x = cos(angle);
    item->direction.y = sin(angle);
    items.push_back(item);
}

void SceneMain::updateItems(float deltaTime)
{
    Uint32 currentTime = static_cast<Uint32>(SDL_GetTicks());
    
    for (auto it = items.begin(); it != items.end();)
    {
        auto item = *it;
        
        // 更新金币动画
        if (item->type == ItemType::Gold) {
            if (item->startTime == 0) {
                item->startTime = currentTime;
            }
            // 循环播放动画
            item->currentFrame = ((currentTime - item->startTime) * item->FPS / 1000) % item->totlaFrame;
        }
        
        // 更新位置
        item->position.x += item->direction.x * item->speed * deltaTime;
        item->position.y += item->direction.y * item->speed * deltaTime;
        // 处理屏幕边缘反弹
        if (item->position.x < 0 && item->bounceCount > 0) {
            item->direction.x = -item->direction.x;
            item->bounceCount--;
        }
        if (item->position.x + item->width > game.getWindowWidth() && item->bounceCount > 0) {
            item->direction.x = -item->direction.x;
            item->bounceCount--;
        }
        if (item->position.y < 0 && item->bounceCount > 0) {
            item->direction.y = -item->direction.y;
            item->bounceCount--;
        }
        if (item->position.y + item->height > game.getWindowHeight() && item->bounceCount > 0) {
            item->direction.y = -item->direction.y;
            item->bounceCount--;
        }
        
        // 如果超出屏幕范围则删除
        if (item->position.x + item->width < 0 || 
        item->position.x > game.getWindowWidth() ||
        item->position.y + item->height < 0 || 
        item->position.y > game.getWindowHeight()){
            delete item;
            it = items.erase(it);
        }
        else{
            SDL_FRect itemRect = {
                item->position.x, 
                item->position.y, 
                item->width, 
                item->height
            };
            SDL_FRect playerRect = {
                player.position.x, 
                player.position.y, 
                player.width, 
                player.height
            };
            if (SDL_HasRectIntersectionFloat(&itemRect, &playerRect) && isDead == false)
            {
                playerGetItem(item);
                delete item;
                it = items.erase(it);
            }else
            {
                ++it;
            }
        }
    }
}

void SceneMain::playerGetItem(Item* item)
{
    // 在playerGetItem函数中（约第670-680行）
    switch (item->type) {
    case ItemType::Life:
        if (player.currentHealth < player.maxHealth) {
            player.currentHealth++;
        }
        break;
    case ItemType::Shield:
        // 护盾效果实现
        player.currentShield++;
        break;
    case ItemType::Time:
        // 时间效果实现：所有敌人爆炸
        {
            // 创建敌人列表的副本，避免在遍历时修改列表
        std::list<Enemy*> enemiesCopy = enemies;  // 明确指定类型
            for (const auto& enemy : enemiesCopy) {
                // 创建爆炸效果
                auto currentTime = SDL_GetTicks();
                auto explosion = new Explosion(explosionTemplate);
                explosion->position.x = enemy->position.x + enemy->width / 2 - explosion->width / 2;
                explosion->position.y = enemy->position.y + enemy->height / 2 - explosion->height / 2;
                explosion->startTime = static_cast<Uint32>(currentTime);
                explosions.push_back(explosion);
                
                // 掉落道具和金币（不加分数）
                if (rand() % 100 < 50) {
                    dropItem(enemy);
                }
                if (rand() % 100 < 70) {
                    dropGold(enemy->position.x, enemy->position.y);
                }
                
                delete enemy;
            }
            // 清空敌人列表
            enemies.clear();
            // 播放爆炸音效
            Mix_PlayChannel(-1, sounds["enemy_explode"], 0);
        }
        break;
    case ItemType::Gold:
        // 金币效果：+10分
        score += 10;
        Mix_PlayChannel(-1, sounds["get_item"], 0);
        return; // 直接返回，不执行下面的统一+5分
    }
    
    // 其他道具统一效果（除了金币）
    score += 5;
    Mix_PlayChannel(-1, sounds["get_item"], 0);
}


void SceneMain::renderItems()
{
    for (auto &item : items)
    {
        SDL_FRect itemRect = {
            item->position.x, 
            item->position.y, 
            item->width, 
            item->height
        };
        
        // 如果是金币，使用动画帧渲染
        if (item->type == ItemType::Gold) {
            SDL_FRect src = {
                (float)(item->currentFrame * (item->texture->w / item->totlaFrame)), 
                0.0f, 
                (float)(item->texture->w / item->totlaFrame), 
                (float)(item->texture->h)
            };
            SDL_RenderTexture(game.getRenderer(), item->texture, &src, &itemRect);
        } else {
            SDL_RenderTexture(game.getRenderer(), item->texture, NULL, &itemRect);
        }
    }   
}

// 修改renderUI函数（约第710行）
void SceneMain::renderUI()
{
    // 渲染血条
    float x = 10;
    float y = 10;
    float size = 32;
    float offset = 40;
    SDL_SetTextureColorMod(uiHealth, 100, 100, 100); // 颜色减淡
    for (float i = 0; i < player.maxHealth; i++)
    {
        SDL_FRect rect = {x + i * offset, y, size, size};
        SDL_RenderTexture(game.getRenderer(), uiHealth, NULL, &rect);
    }
    SDL_SetTextureColorMod(uiHealth, 255, 255, 255); // 恢复颜色
    for (float i = 0; i < player.currentHealth; i++)
    {
        SDL_FRect rect = {x + i * offset, y, size, size};
        SDL_RenderTexture(game.getRenderer(), uiHealth, NULL, &rect);
    }
    
    // 渲染护盾（新增）
    if (player.currentShield > 0) {
        float shieldY = y + size + 10; // 在血量下面
        SDL_FRect shieldRect = {x, shieldY, size*2, size};
        SDL_RenderTexture(game.getRenderer(), uiShield, NULL, &shieldRect);
        
        // 渲染护盾数量文字
        auto shieldText = "x" + std::to_string(player.currentShield);
        SDL_Color color = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(scoreFont, shieldText.c_str(), 0, color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(game.getRenderer(), surface);
        SDL_FRect textRect = {x + size - 15, shieldY + 30, static_cast<float>(surface->w), static_cast<float>(surface->h)};
        SDL_RenderTexture(game.getRenderer(), texture, NULL, &textRect);
        SDL_DestroySurface(surface);  // SDL_FreeSurface -> SDL_DestroySurface
        SDL_DestroyTexture(texture);
    }
    
    // 渲染得分
    auto text = "SCORE:" + std::to_string(score);
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(scoreFont, text.c_str(),0, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(game.getRenderer(), surface);
    SDL_FRect rect = {game.getWindowWidth() - 10 - static_cast<float>(surface->w), 10, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_RenderTexture(game.getRenderer(), texture, NULL, &rect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

// 新增专门的金币掉落函数
// dropGold函数应该保持在文件末尾
void SceneMain::dropGold(float x, float y)
{
    Item* item = new Item();
    *item = itemGoldTemplate;
    item->position.x = x;
    item->position.y = y;
    // 修复：使用随机方向而不是调用getDirection
    float angle = static_cast<float>((rand() % 360) * M_PI / 180.0f);
    item->direction.x = cos(angle);
    item->direction.y = sin(angle);
    item->speed = 100;
    item->bounceCount = 3;
    item->startTime = 0; // 重置动画时间
    items.push_back(item);
}

// 敌人2的多方向射击函数
void SceneMain::shootEnemyMultiDirection(Enemy *enemy, int bulletCount)
{
    for (int i = 0; i < bulletCount; i++) {
        auto* projectile = enemyBulletPool.create();
        if (projectile == nullptr) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Enemy bullet pool exhausted!");
            return;
        }
        
        // 设置子弹属性
        *projectile = projectileEnemyTemplate;  // 重置为模板状态
        projectile->position.x = enemy->position.x + enemy->width / 2 - projectile->width / 2;
        projectile->position.y = enemy->position.y + enemy->height / 2 - projectile->height / 2;
        
        // 计算发射角度，均匀分布在360度范围内
        float angle = (360.0f / bulletCount) * i;
        float radians = static_cast<float>(angle * M_PI / 180.0f);
        
        // 设置子弹方向
        projectile->direction.x = cos(radians);
        projectile->direction.y = sin(radians);
        
        projectilesEnemy.push_back(projectile);
    }
    
    // 播放射击音效
    Mix_PlayChannel(-1, sounds["enemy_shoot"], 0);
}
void SceneMain::renderPauseOverlay()
{
    // 设置半透明黑色覆盖层
    SDL_SetRenderDrawBlendMode(game.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(game.getRenderer(), 0, 0, 0, 128); // 半透明黑色
    
    // 渲染全屏覆盖层
    SDL_FRect overlayRect = {0, 0, static_cast<float>(game.getWindowWidth()), static_cast<float>(game.getWindowHeight())};
    SDL_RenderFillRect(game.getRenderer(), &overlayRect);
    
    // 渲染暂停文字
    SDL_Color textColor = {255, 255, 255, 255}; // 白色文字
    SDL_Surface* surface = TTF_RenderText_Solid(scoreFont, "游戏暂停 - 按回车继续", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(game.getRenderer(), surface);
        if (texture) {
            // 居中显示文字
            float textX = (game.getWindowWidth() - surface->w) / 2.0f;
            float textY = (game.getWindowHeight() - surface->h) / 2.0f;
            SDL_FRect textRect = {textX, textY, static_cast<float>(surface->w), static_cast<float>(surface->h)};
            SDL_RenderTexture(game.getRenderer(), texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
    
    // 恢复默认混合模式
    SDL_SetRenderDrawBlendMode(game.getRenderer(), SDL_BLENDMODE_NONE);
}
// 更新过渡状态
void SceneMain::updateTransition(float deltaTime)
{
    switch (transitionState) {
        case TransitionState::PREPARING_BOSS: {
            transitionTimer += deltaTime;
            if (transitionTimer >= 2.0f) {
                // 清理所有敌人和子弹
                clearAllEnemiesAndBullets();
                
                transitionState = TransitionState::WAITING_CLEAR;
                transitionTimer = 0.0f;
            }
            break;
        }
        
        case TransitionState::WAITING_CLEAR: {
            transitionTimer += deltaTime;
            // 检查所有敌人是否已经退场（列表为空）和子弹是否清理完毕
            if (enemies.empty() && areAllBulletsCleared()) {
                // 直接跳转到准备Boss战状态，不需要移动玩家
                transitionState = TransitionState::READY_FOR_BOSS;
                transitionTimer = 0.0f;
                enemiesRetreating = false; // 重置退场状态
                
                // 开始背景加速滚动
                Game::getInstance().setBackgroundSpeed(60, 40);
            }
            break;
        }
        
        case TransitionState::MOVING_TO_BOSS: {
            // 这个状态现在不再使用，直接跳转
            transitionState = TransitionState::READY_FOR_BOSS;
            transitionTimer = 0.0f;
            break;
        }
        
        case TransitionState::READY_FOR_BOSS: {
            transitionTimer += deltaTime;
            if (transitionTimer >= 1.0f) {
                shouldChangeToBoss = true;
                
                // 恢复背景正常滚动速度
                Game::getInstance().setBackgroundSpeed(30, 20);
            }
            break;
        }
        
        default:
            break;
    }
}

// 开始Boss过渡
void SceneMain::startBossTransition()
{
    if (transitionState == TransitionState::NORMAL) {
        transitionState = TransitionState::PREPARING_BOSS;
        transitionTimer = 0.0f;
        // 移除缩放相关的初始化
    }
}

// 检查所有子弹是否已清理
bool SceneMain::areAllBulletsCleared()
{
    return projectilesPlayer.empty() && projectilesEnemy.empty();
}

// 移动主角到目标位置（现在不强制移动）
void SceneMain::movePlayerToTarget(float deltaTime)
{
    // 移除强制移动逻辑，让玩家自由控制
    // 可以在这里添加提示信息渲染
}

// 清理所有敌人和子弹
void SceneMain::clearAllEnemiesAndBullets()
{
    // 设置敌人退场状态，让它们向右移动退场
    enemiesRetreating = true;
    
    // 清理所有敌人子弹
    for (auto projectile : projectilesEnemy) {
        enemyBulletPool.release(projectile);
    }
    projectilesEnemy.clear();
    
    // 清理所有玩家子弹
    for (auto projectile : projectilesPlayer) {
        playerBulletPool.release(projectile);
    }
    projectilesPlayer.clear();
}

// 渲染过渡效果
void SceneMain::renderTransitionEffect()
{
    if (transitionState == TransitionState::MOVING_TO_BOSS) {
        auto& game = Game::getInstance();
        // 渲染提示信息
        game.renderTextCentered("Move to the left side to enter Boss battle!", 
                               game.getWindowHeight() - 100, true);
    }
}

// 检查武器升级触发
void SceneMain::checkWeaponUpgrade()
{
    // 第一次升级在100分，之后每500分升级一次
    int nextUpgradeScore;
    if (lastUpgradeScore == 0) {
        // 第一次升级
        nextUpgradeScore = 100;
    } else {
        // 后续升级：第一次升级后，每500分升级一次
        nextUpgradeScore = lastUpgradeScore + 500;
    }
    
    if (score >= nextUpgradeScore && !weaponUpgradeAvailable) {
        lastUpgradeScore = score;
        weaponUpgradeAvailable = true;
        weaponUpgradePaused = true;
        generateUpgradeOptions();
    }
}

// 生成升级选项
void SceneMain::generateUpgradeOptions()
{
    upgradeOptions.clear();
    
    // 随机生成3个不同的升级选项
    std::vector<WeaponUpgrade> allUpgrades = {
        WeaponUpgrade::DAMAGE_UP,
        WeaponUpgrade::SPEED_UP,
        WeaponUpgrade::BOUNCE_UP,
        WeaponUpgrade::SPLIT_UP,
        WeaponUpgrade::PIERCE_UP
    };
    
    std::shuffle(allUpgrades.begin(), allUpgrades.end(), gen);
    
    for (int i = 0; i < 3 && i < allUpgrades.size(); i++) {
        upgradeOptions.push_back(allUpgrades[i]);
    }
    
    selectedUpgrade = 0;
}

// 应用武器升级
void SceneMain::applyWeaponUpgrade(WeaponUpgrade upgrade)
{
    switch (upgrade) {
        case WeaponUpgrade::DAMAGE_UP:
            player.weapon.damage += 1;
            break;
        case WeaponUpgrade::SPEED_UP:
            player.weapon.fireRate += 0.3f;
            break;
        case WeaponUpgrade::BOUNCE_UP:
            player.weapon.bounceCount += 1;
            break;
        case WeaponUpgrade::SPLIT_UP:
            player.weapon.splitCount += 1;
            break;
        case WeaponUpgrade::PIERCE_UP:
            player.weapon.piercing = true;
            break;
    }
    
    player.weapon.level++;
    weaponUpgradeAvailable = false;
    weaponUpgradePaused = false;
}

// 渲染武器升级界面
void SceneMain::renderWeaponUpgradeUI()
{
    auto& game = Game::getInstance();
    SDL_Renderer* renderer = game.getRenderer();
    
    // 绘制半透明背景
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlay = {0, 0, static_cast<float>(game.getWindowWidth()), static_cast<float>(game.getWindowHeight())};
    SDL_RenderFillRect(renderer, &overlay);
    
    // 绘制升级选项
    float centerX = game.getWindowWidth() / 2.0f;
    float centerY = game.getWindowHeight() / 2.0f;
    
    // 标题
    SDL_Color titleColor = {255, 255, 255, 255};
    SDL_Surface* titleSurface = TTF_RenderText_Solid(scoreFont, "武器升级", 0, titleColor);
    if (titleSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        if (titleTexture) {
            SDL_FRect titleRect = {
                centerX - titleSurface->w / 2.0f,
                centerY - 120,
                static_cast<float>(titleSurface->w),
                static_cast<float>(titleSurface->h)
            };
            SDL_RenderTexture(renderer, titleTexture, NULL, &titleRect);
            SDL_DestroyTexture(titleTexture);
        }
        SDL_DestroySurface(titleSurface);
    }
    
    // 绘制选项
    for (int i = 0; i < upgradeOptions.size(); i++) {
        float optionY = centerY - 60 + i * 40;
        
        // 先获取文本尺寸
        std::string optionText = getUpgradeText(upgradeOptions[i]);
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* textSurface = TTF_RenderText_Solid(scoreFont, optionText.c_str(), 0, textColor);
        
        if (textSurface) {
            // 选中高亮 - 根据实际文本大小调整
            if (i == selectedUpgrade) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
                SDL_FRect highlight = {
                    centerX - textSurface->w / 2.0f - 10,  // 左边留10像素边距
                    optionY - 5,                           // 上边留5像素边距
                    static_cast<float>(textSurface->w + 20), // 宽度加20像素边距
                    static_cast<float>(textSurface->h + 10)  // 高度加10像素边距
                };
                SDL_RenderFillRect(renderer, &highlight);
            }
            
            // 绘制升级选项文本
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture) {
                SDL_FRect textRect = {
                    centerX - textSurface->w / 2.0f,
                    optionY,
                    static_cast<float>(textSurface->w),
                    static_cast<float>(textSurface->h)
                };
                SDL_RenderTexture(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_DestroySurface(textSurface);
        }
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// 处理武器升级输入
void SceneMain::handleWeaponUpgradeInput(SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.scancode) {
            case SDL_SCANCODE_UP:
                if (selectedUpgrade == 0) {
                    selectedUpgrade = upgradeOptions.size() - 1;
                } else {
                    selectedUpgrade--;
                }
                break;
            case SDL_SCANCODE_DOWN:
                selectedUpgrade = (selectedUpgrade + 1) % upgradeOptions.size();
                break;
            case SDL_SCANCODE_RETURN:
                applyWeaponUpgrade(upgradeOptions[selectedUpgrade]);
                break;
        }
    }
}

// 使用武器系统的射击
void SceneMain::shootPlayerWithWeapon()
{
    Uint64 currentTime = SDL_GetTicks();
    Uint64 weaponCooldown = player.weapon.getCooldown(player.coolDown);
    
    if (currentTime - player.lastShootTime >= weaponCooldown) {
        player.lastShootTime = currentTime;
        
        // 播放射击音效
        if (sounds["player_shoot"]) {
            Mix_PlayChannel(-1, sounds["player_shoot"], 0);
        }
        
        // 根据分裂数量创建子弹
        if (player.weapon.splitCount == 1) {
            // 单发子弹
            auto* bullet = playerBulletPool.create();
            if (bullet) {
                *bullet = projectilePlayerTemplate;
                
                // 根据主角朝向设置子弹位置和方向
                if (player.flip == SDL_FLIP_HORIZONTAL) {
                    // 向左发射
                    bullet->position.x = player.position.x - bullet->width;
                    bullet->direction = {-1, 0};
                } else {
                    // 向右发射
                    bullet->position.x = player.position.x + player.width;
                    bullet->direction = {1, 0};
                }
                
                bullet->position.y = player.position.y + player.height / 2 - bullet->height / 2;
                bullet->damage = player.weapon.damage;
                bullet->maxBounces = player.weapon.bounceCount;
                bullet->bounceCount = 0;
                projectilesPlayer.push_back(bullet);
            }
        } else {
            // 分裂子弹
            SDL_FPoint startPos = {
                player.position.x + player.width,
                player.position.y + player.height / 2
            };
            SDL_FPoint direction = {1, 0};
            if (player.flip == SDL_FLIP_HORIZONTAL) {
                startPos.x = player.position.x;
                direction = {-1, 0};
            }
            createSplitBullets(startPos, direction, player.weapon.splitCount);
        }
    }
}

// 创建分裂子弹
void SceneMain::createSplitBullets(SDL_FPoint startPos, SDL_FPoint direction, int count)
{
    float angleStep = 0.5f; // 分裂角度间隔
    float startAngle = -(count - 1) * angleStep / 2.0f;
    
    for (int i = 0; i < count; i++) {
        auto* bullet = playerBulletPool.create();
        if (bullet) {
            *bullet = projectilePlayerTemplate;
            bullet->position.x = startPos.x;
            bullet->position.y = startPos.y;
            bullet->position.w = bullet->width;
            bullet->position.h = bullet->height;
            
            float angle = startAngle + i * angleStep;
            bullet->direction.x = direction.x * cos(angle) - direction.y * sin(angle);
            bullet->direction.y = direction.x * sin(angle) + direction.y * cos(angle);
            
            bullet->damage = player.weapon.damage;
            bullet->maxBounces = player.weapon.bounceCount;
            bullet->bounceCount = 0;
            
            projectilesPlayer.push_back(bullet);
        }
    }
}

// 获取升级选项文本
std::string SceneMain::getUpgradeText(WeaponUpgrade upgrade)
{
    switch (upgrade) {
        case WeaponUpgrade::DAMAGE_UP:
            return "伤害 +1";
        case WeaponUpgrade::SPEED_UP:
            return "射速 +30%";
        case WeaponUpgrade::BOUNCE_UP:
            return "反弹次数 +1";
        case WeaponUpgrade::SPLIT_UP:
            return "分裂数量 +1";
        case WeaponUpgrade::PIERCE_UP:
            return "穿透能力";
        default:
            return "未知升级";
    }
}