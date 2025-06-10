#include "SceneTitle.h"
#include "SceneBoss.h"
#include "SceneEnd.h"
#include "SceneTitle.h"
#include "Game.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>

constexpr double M_PI = 3.14159265358979323846;

SceneBoss::SceneBoss(int playerScore) : Scene(), score(playerScore)
{
    
}

// 添加新的构造函数重载，接受玩家状态
SceneBoss::SceneBoss(int playerScore, const Player& mainPlayer) : Scene(), score(playerScore)
{
    // 完全保留玩家的位置、血量、护盾状态
    player = mainPlayer;
    // 只在玩家位置过于靠右时进行微调
    if (player.position.x > Game::getInstance().getWindowWidth() - 200) {
        player.position.x = Game::getInstance().getWindowWidth() - 200;
    }
    
    // 根据玩家血量正确设置死亡状态
    isDead = (player.currentHealth <= 0);
}

void SceneBoss::init()
{
    // 播放Boss战音乐
    Game::getInstance().playBgm("assets/music/老鼠.mp3");
    
    // 设置Boss场景背景滚动速度与主场景过渡时相同
    Game::getInstance().setBackgroundSpeed(60, 40);
    
    auto& game = Game::getInstance();
    uiHealth = IMG_LoadTexture(game.getRenderer(), "assets/image/Health UI Black.png");
    uiShield = IMG_LoadTexture(game.getRenderer(), "assets/image/护盾.png");
    scoreFont = TTF_OpenFont("assets/font/VonwaonBitmap-12px.ttf", 24);
    
    // 加载音效
    sounds["player_shoot"] = Mix_LoadWAV("assets/sound/laser_shoot4.mp3");
    sounds["boss_shoot"] = Mix_LoadWAV("assets/sound/xs_laser.mp3");
    sounds["player_explode"] = Mix_LoadWAV("assets/sound/角色死亡音效.mp3");
    sounds["boss_explode"] = Mix_LoadWAV("assets/sound/explosion3.mp3");
    sounds["hit"] = Mix_LoadWAV("assets/sound/eff11.mp3");
    
    std::random_device rd;
    gen = std::mt19937(rd());
    dis = std::uniform_real_distribution<float>(0.0f, 1.0f);
    
    // 移除条件判断，确保每次都重新加载玩家纹理
    player.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/SpaceShip.png");
    if (player.texture == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load player texture: %s", SDL_GetError());
    }
    SDL_GetTextureSize(player.texture, &player.width, &player.height);
    player.width /= 5;
    player.height /= 5;
    player.coolDown = 300;
    
    // 初始化Boss - 修改位置和动画设置
    boss.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/大青蛙.png");
    SDL_GetTextureSize(boss.texture, &boss.width, &boss.height);
    boss.width /= 2;
    boss.height /= 2;
    
    // Boss目标位置向右移动更多
    bossTargetX = game.getWindowWidth() - boss.width +1; // Boss位置更靠右
    boss.position.x = game.getWindowWidth() + 50; // 从屏幕右侧外开始
    boss.position.y = game.getWindowHeight() / 2 - boss.height / 2;
    
    boss.currentHealth = 1000;
    boss.maxHealth = 1000;
    boss.coolDown = 100;
    boss.shootAngle = 0.0f;
    boss.shootPattern = 0;
    boss.patternChangeTime = static_cast<Uint32>(SDL_GetTicks());
    
    // 初始化Boss动画状态
    bossEntering = true;
    
    // 初始化子弹模板
    projectilePlayerTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/子弹.png");
    SDL_GetTextureSize(projectilePlayerTemplate.texture, &projectilePlayerTemplate.width, &projectilePlayerTemplate.height);
    projectilePlayerTemplate.width /= 4;
    projectilePlayerTemplate.height /= 4;
    
    // 加载Boss子弹纹理
    projectileBossTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/image/boss子弹.png");
    SDL_GetTextureSize(projectileBossTemplate.texture, &projectileBossTemplate.width, &projectileBossTemplate.height);
    projectileBossTemplate.width /= 3;
    projectileBossTemplate.height /= 3;
    
    // 初始化爆炸模板
    explosionTemplate.texture = IMG_LoadTexture(game.getRenderer(), "assets/effect/explosion.png");
    SDL_GetTextureSize(explosionTemplate.texture, &explosionTemplate.width, &explosionTemplate.height);
    explosionTemplate.totlaFrame = static_cast<int>(explosionTemplate.width / explosionTemplate.height);
    explosionTemplate.height *= 2.0f;
    explosionTemplate.width = explosionTemplate.height;
    
    // 初始化对象池
    playerBulletPool.initialize(projectilePlayerTemplate, 50);
    bossBulletPool.initialize(projectileBossTemplate, 300);
    explosionPool.initialize(explosionTemplate, 30);
}

void SceneBoss::update(float deltaTime)
{
    auto& game = Game::getInstance();
    
    // 如果游戏暂停，跳过所有更新逻辑
    if (isPaused) {
        return;
    }
    
    // 处理Boss连续爆炸
    if (bossExploding) {
        explosionTimer += deltaTime;
        
        // 检查是否需要创建下一个爆炸
        if (explosionTimer >= explosionInterval && explosionCount < maxExplosions) {
            createSingleExplosion();
            explosionTimer = 0.0f;
        }
        
        // 所有爆炸完成后，Boss消失
        if (explosionCount >= maxExplosions) {
            bossExploding = false;
            // Boss完全消失，可以在这里添加额外的效果
        }
    }
    
    keyboardControl(deltaTime);
    updatePlayerProjectiles(deltaTime);
    updateBossProjectiles(deltaTime);
    updatePlayer(deltaTime);
    updateBoss(deltaTime);
    updateExplosions(deltaTime);
    
    // 在update函数中，延长Boss死亡时间
    if (isDead || bossDefeated) {
        changeSceneDelayed(deltaTime, 5); // 延长到5秒
    }
}    

void SceneBoss::render()
{   
    auto& game = Game::getInstance();
    
    // 添加背景渲染
    game.renderBackground();
    
    renderPlayerProjectiles();
    renderBossProjectiles();
    
    if (!isDead) {
        SDL_FRect playerRect = {player.position.x, player.position.y, player.width, player.height};
        // 改为使用普通的SDL_RenderTexture函数，与主场景保持一致
        SDL_RenderTexture(game.getRenderer(), player.texture, NULL, &playerRect);
    }
    
    renderBoss();
    renderExplosions();
    renderUI();
    
    if (isPaused) {
        renderPauseOverlay();
    }
}

void SceneBoss::handleEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            // 返回主菜单而不是暂停
            auto& game = Game::getInstance();
            game.changeScene(new SceneTitle());
        }
        // 可以用其他键（如回车）来暂停
        else if (event->key.scancode == SDL_SCANCODE_RETURN) {
            isPaused = !isPaused;
        }
    }
}

void SceneBoss::keyboardControl(float deltaTime)
{
    auto& game = Game::getInstance();
    if (isDead) {
        return;
    }
    
    const bool* keyboardState = SDL_GetKeyboardState(NULL);
    float currentSpeed = static_cast<float>(player.speed);
    
    if (keyboardState[SDL_SCANCODE_J]) {
        currentSpeed *= 1.3f;
    }
    
    if (keyboardState[SDL_SCANCODE_W]) {
        player.position.y -= deltaTime * currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_S]) {
        player.position.y += deltaTime * currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_A]) {
        player.position.x -= deltaTime * currentSpeed;
        // 移除翻转效果 - 注释掉这行
        // player.flip = SDL_FLIP_HORIZONTAL;
    }
    if (keyboardState[SDL_SCANCODE_D]) {
        player.position.x += deltaTime * currentSpeed;
        // 移除翻转效果 - 注释掉这行
        // player.flip = SDL_FLIP_NONE;
    }
    
    // 限制移动范围
    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > game.getWindowWidth() - player.width) player.position.x = game.getWindowWidth() - player.width;
    if (player.position.y < 0) player.position.y = 0;
    if (player.position.y > game.getWindowHeight() - player.height) player.position.y = game.getWindowHeight() - player.height;
    
    // 自动射击
    Uint32 currentTime = static_cast<Uint32>(SDL_GetTicks());
    if (currentTime - player.lastShootTime > player.coolDown) {
        shootPlayer();
        player.lastShootTime = currentTime;
    }
}

void SceneBoss::shootPlayer()
{
    auto* projectile = playerBulletPool.create();
    if (projectile == nullptr) {
        return;
    }
    
    *projectile = projectilePlayerTemplate;
    
    // 始终向右发射，移除翻转判断
    projectile->position.x = player.position.x + player.width;
    projectile->direction = {1, 0}; // 始终向右
    
    projectile->position.y = player.position.y + player.height / 2 - projectile->height / 2;
    projectile->bounceCount = 0;
    projectilesPlayer.push_back(projectile);
    Mix_PlayChannel(0, sounds["player_shoot"], 0);
}

void SceneBoss::updateBoss(float deltaTime)
{
    auto& game = Game::getInstance();
    
    // 处理Boss出场动画
    if (bossEntering) {
        if (boss.position.x > bossTargetX) {
            boss.position.x -= bossEnterSpeed * deltaTime;
            if (boss.position.x <= bossTargetX) {
                boss.position.x = bossTargetX;
                bossEntering = false; // 出场动画完成
            }
        }
        return; // 出场期间不进行其他更新
    }
    
    // 在updateBoss函数中
    if (boss.currentHealth <= 0) {
        if (!bossDefeated && !bossExploding) {
            bossExplode();
            bossDefeated = true;
        }
        return; // Boss死亡后直接返回，不执行后续射击逻辑
    }
    
    Uint32 currentTime = static_cast<Uint32>(SDL_GetTicks());
    
    // 切换弹幕模式
    if (currentTime - boss.patternChangeTime > 5000) {
        boss.shootPattern = (boss.shootPattern + 1) % 3;
        boss.patternChangeTime = currentTime;
    }
    
    // Boss射击
    if (currentTime - boss.lastShootTime > boss.coolDown) {
        switch (boss.shootPattern) {
            case 0:
                shootBossPattern1();
                break;
            case 1:
                shootBossPattern2();
                break;
            case 2:
                shootBossPattern3();
                break;
        }
        boss.lastShootTime = currentTime;
    }
    
    boss.shootAngle += 2.0f * deltaTime;
}

void SceneBoss::shootBossPattern1()
{
    int bulletCount = 16;
    for (int i = 0; i < bulletCount; i++) {
        auto* projectile = bossBulletPool.create();
        if (projectile == nullptr) continue;
        
        *projectile = projectileBossTemplate;
        projectile->position.x = boss.position.x + boss.width / 2 - projectile->width / 2;
        projectile->position.y = boss.position.y + boss.height / 2 - projectile->height / 2;
        
        float angle = (2.0f * static_cast<float>(M_PI) * i / bulletCount) + boss.shootAngle;
        projectile->direction.x = static_cast<float>(cos(angle));
        projectile->direction.y = static_cast<float>(sin(angle));
        projectile->rotationAngle = angle * 180.0f / static_cast<float>(M_PI);
        
        projectilesBoss.push_back(projectile);
    }
    Mix_PlayChannel(-1, sounds["boss_shoot"], 0);
}

void SceneBoss::shootBossPattern2()
{
    int bulletCount = 8;
    for (int i = 0; i < bulletCount; i++) {
        auto* projectile = bossBulletPool.create();
        if (projectile == nullptr) continue;
        
        *projectile = projectileBossTemplate;
        projectile->position.x = boss.position.x + boss.width / 2 - projectile->width / 2;
        projectile->position.y = boss.position.y + boss.height / 2 - projectile->height / 2;
        
        float angle = (2.0f * static_cast<float>(M_PI) * i / bulletCount) + boss.shootAngle * 3.0f;
        projectile->direction.x = static_cast<float>(cos(angle));
        projectile->direction.y = static_cast<float>(sin(angle));
        projectile->rotationAngle = angle * 180.0f / static_cast<float>(M_PI);
        
        projectilesBoss.push_back(projectile);
    }
}

void SceneBoss::shootBossPattern3()
{
    int bulletCount = 12;
    float spreadAngle = static_cast<float>(M_PI) / 3.0f;
    float playerAngle = static_cast<float>(atan2(player.position.y - boss.position.y, player.position.x - boss.position.x));
    
    for (int i = 0; i < bulletCount; i++) {
        auto* projectile = bossBulletPool.create();
        if (projectile == nullptr) continue;
        
        *projectile = projectileBossTemplate;
        projectile->position.x = boss.position.x + boss.width / 2 - projectile->width / 2;
        projectile->position.y = boss.position.y + boss.height / 2 - projectile->height / 2;
        
        float angle = playerAngle - spreadAngle / 2 + (spreadAngle * i / (bulletCount - 1));
        projectile->direction.x = static_cast<float>(cos(angle));
        projectile->direction.y = static_cast<float>(sin(angle));
        projectile->rotationAngle = angle * 180.0f / static_cast<float>(M_PI);
        
        projectilesBoss.push_back(projectile);
    }
}

void SceneBoss::updatePlayerProjectiles(float deltaTime)
{
    auto& game = Game::getInstance();
    auto it = projectilesPlayer.begin();
    while (it != projectilesPlayer.end()) {
        auto projectile = *it;
        
        projectile->position.x += projectile->direction.x * projectile->speed * deltaTime;
        projectile->position.y += projectile->direction.y * projectile->speed * deltaTime;
        
        // 检查边界
        if (projectile->position.x < -32 || projectile->position.x > game.getWindowWidth() + 32 ||
            projectile->position.y < -32 || projectile->position.y > game.getWindowHeight() + 32) {
            playerBulletPool.release(projectile);
            it = projectilesPlayer.erase(it);
            continue;
        }
        
        // 检查与Boss的碰撞
        if (boss.currentHealth > 0 &&
            projectile->position.x < boss.position.x + boss.width &&
            projectile->position.x + projectile->width > boss.position.x &&
            projectile->position.y < boss.position.y + boss.height &&
            projectile->position.y + projectile->height > boss.position.y) {
            
            boss.currentHealth -= 10;
            Mix_PlayChannel(-1, sounds["hit"], 0);
            
            playerBulletPool.release(projectile);
            it = projectilesPlayer.erase(it);
            continue;
        }
        
        ++it;
    }
}

void SceneBoss::updateBossProjectiles(float deltaTime)
{
    auto& game = Game::getInstance();
    auto it = projectilesBoss.begin();
    while (it != projectilesBoss.end()) {
        auto projectile = *it;
        
        projectile->position.x += projectile->direction.x * projectile->speed * deltaTime;
        projectile->position.y += projectile->direction.y * projectile->speed * deltaTime;
        
        // 检查边界
        if (projectile->position.x < -32 || projectile->position.x > game.getWindowWidth() + 32 ||
            projectile->position.y < -32 || projectile->position.y > game.getWindowHeight() + 32) {
            bossBulletPool.release(projectile);
            it = projectilesBoss.erase(it);
            continue;
        }
        
        // 检查与玩家的碰撞 - 玩家碰撞体积减少到20%
        float playerCollisionReduction = 0.8f; // 玩家碰撞体积减少80%，只保留20%
        float playerCollisionWidth = player.width * (1.0f - playerCollisionReduction);
        float playerCollisionHeight = player.height * (1.0f - playerCollisionReduction);
        float playerCollisionX = player.position.x + (player.width - playerCollisionWidth) / 2;
        float playerCollisionY = player.position.y + (player.height - playerCollisionHeight) / 2;
    
        
        // Boss子弹的碰撞体积
        float bulletCollisionReduction = 0.3f;
        float bulletCollisionWidth = projectile->width * (1.0f - bulletCollisionReduction);
        float bulletCollisionHeight = projectile->height * (1.0f - bulletCollisionReduction);
        float bulletCollisionX = projectile->position.x + (projectile->width - bulletCollisionWidth) / 2;
        float bulletCollisionY = projectile->position.y + (projectile->height - bulletCollisionHeight) / 2;
        
        if (!isDead &&
            bulletCollisionX < playerCollisionX + playerCollisionWidth &&
            bulletCollisionX + bulletCollisionWidth > playerCollisionX &&
            bulletCollisionY < playerCollisionY + playerCollisionHeight &&
            bulletCollisionY + bulletCollisionHeight > playerCollisionY) {
            
            if (player.currentShield > 0) {
                player.currentShield--;
            } else {
                player.currentHealth--;
                if (player.currentHealth <= 0) {
                    isDead = true;
                    Mix_PlayChannel(-1, sounds["player_explode"], 0);
                }
            }
            
            bossBulletPool.release(projectile);
            it = projectilesBoss.erase(it);
            continue;
        }
        
        ++it;
    }
}

void SceneBoss::updatePlayer(float deltaTime)
{
    auto& game = Game::getInstance();
    if (isDead && !bossDefeated) {
        game.setFinalScore(score);
    }
}

void SceneBoss::updateExplosions(float deltaTime)
{
    Uint32 currentTime = static_cast<Uint32>(SDL_GetTicks());
    auto it = explosions.begin();
    while (it != explosions.end()) {
        auto explosion = *it;
        
        Uint32 frameTime = 1000 / explosion->FPS;
        explosion->currentFrame = (currentTime - explosion->startTime) / frameTime;
        
        if (explosion->currentFrame >= explosion->totlaFrame) {
            explosionPool.release(*it);
            it = explosions.erase(it);
        } else {
            ++it;
        }
    }
}

void SceneBoss::renderPlayerProjectiles()
{
    auto& game = Game::getInstance();
    for (auto projectile : projectilesPlayer) {
        SDL_FRect projectileRect = {projectile->position.x, projectile->position.y, projectile->width, projectile->height};
        SDL_RenderTexture(game.getRenderer(), projectile->texture, NULL, &projectileRect);
    }
}

void SceneBoss::renderBossProjectiles()
{
    auto& game = Game::getInstance();
    for (auto projectile : projectilesBoss) {
        SDL_FRect projectileRect = {projectile->position.x, projectile->position.y, projectile->width, projectile->height};
        SDL_RenderTextureRotated(game.getRenderer(), projectile->texture, NULL, &projectileRect, projectile->rotationAngle, NULL, SDL_FLIP_NONE);
    }
}

void SceneBoss::renderBoss()
{
    auto& game = Game::getInstance();
    if (boss.currentHealth > 0) {
        SDL_FRect bossRect = {boss.position.x, boss.position.y, boss.width, boss.height};
        SDL_RenderTexture(game.getRenderer(), boss.texture, NULL, &bossRect);
        
        // 渲染Boss血条
        float healthRatio = static_cast<float>(boss.currentHealth) / boss.maxHealth;
        SDL_FRect healthBarBg = {boss.position.x, boss.position.y - 20, boss.width, 10};
        SDL_FRect healthBar = {boss.position.x, boss.position.y - 20, boss.width * healthRatio, 10};
        
        SDL_SetRenderDrawColor(game.getRenderer(), 255, 0, 0, 255);
        SDL_RenderFillRect(game.getRenderer(), &healthBarBg);
        SDL_SetRenderDrawColor(game.getRenderer(), 0, 255, 0, 255);
        SDL_RenderFillRect(game.getRenderer(), &healthBar);
    }
}

void SceneBoss::renderExplosions()
{
    auto& game = Game::getInstance();
    for (auto explosion : explosions) {
        SDL_FRect srcRect = {
            static_cast<float>(explosion->currentFrame * explosion->height),
            0,
            explosion->height,
            explosion->height
        };
        SDL_FRect destRect = {explosion->position.x, explosion->position.y, explosion->width, explosion->height};
        SDL_RenderTexture(game.getRenderer(), explosion->texture, &srcRect, &destRect);
    }
}

void SceneBoss::renderUI()
{
    auto& game = Game::getInstance();
    // 渲染玩家血量（与SceneMain完全一致）
    float x = 10;
    float y = 10;
    float size = 32;
    float offset = 40;
    SDL_SetTextureColorMod(uiHealth, 100, 100, 100); // 颜色减淡
    for (int i = 0; i < player.maxHealth; i++) {
        SDL_FRect rect = {x + i * offset, y, size, size};
        SDL_RenderTexture(game.getRenderer(), uiHealth, NULL, &rect);
    }
    SDL_SetTextureColorMod(uiHealth, 255, 255, 255); // 恢复颜色
    for (int i = 0; i < player.currentHealth; i++) {
        SDL_FRect rect = {x + i * offset, y, size, size};
        SDL_RenderTexture(game.getRenderer(), uiHealth, NULL, &rect);
    }
    
    // 渲染护盾（修改为与SceneMain一致的样式）
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
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
    

    
    // 渲染Boss血量文字
    if (boss.currentHealth > 0) {
        game.renderTextPos("Boss HP: " + std::to_string(boss.currentHealth) + "/" + std::to_string(boss.maxHealth), 
                          game.getWindowWidth() - 350, 10, true);
    }
}

void SceneBoss::renderPauseOverlay()
{
    auto& game = Game::getInstance();
    
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

void SceneBoss::bossExplode()
{
    // 开始连续爆炸序列
    bossExploding = true;
    explosionTimer = 0.0f;
    explosionCount = 0;
    
    // 立即创建第一个爆炸
    createSingleExplosion();
}

void SceneBoss::createSingleExplosion()
{
    auto explosion = explosionPool.create();
    if (explosion != nullptr) {
        // 在Boss周围随机位置创建爆炸
        float randomX = boss.position.x + (dis(gen) - 0.5f) * boss.width * 1.5f;
        float randomY = boss.position.y + (dis(gen) - 0.5f) * boss.height * 1.5f;
        
        explosion->position.x = randomX - explosion->width / 2;
        explosion->position.y = randomY - explosion->height / 2;
        explosion->startTime = static_cast<Uint32>(SDL_GetTicks());
        explosions.push_back(explosion);
    }
    
    // 播放爆炸音效
    Mix_PlayChannel(-1, sounds["boss_explode"], 0);
    explosionCount++;
}

void SceneBoss::changeSceneDelayed(float deltaTime, float delay)
{
    auto& game = Game::getInstance();
    timerEnd += deltaTime;
    if (timerEnd >= delay) {
        if (bossDefeated) {
            // Boss被击败，设置分数为9999并进入胜利场景
            game.setFinalScore(9999);
            game.changeScene(new SceneEnd(true));  // 传递胜利状态
        } else {
            // 玩家死亡，设置当前分数并进入失败场景
            game.setFinalScore(score);
            game.changeScene(new SceneEnd(false)); // 传递失败状态
        }
    }
}

void SceneBoss::clean()
{
    // 清理资源
    if (uiHealth != nullptr) {
        SDL_DestroyTexture(uiHealth);
        uiHealth = nullptr;
    }
    if (uiShield != nullptr) {
        SDL_DestroyTexture(uiShield);
        uiShield = nullptr;
    }
    if (scoreFont != nullptr) {
        TTF_CloseFont(scoreFont);
        scoreFont = nullptr;
    }
    
    // 清理模板纹理
    if (player.texture != nullptr) {
        SDL_DestroyTexture(player.texture);
    }
    if (boss.texture != nullptr) {
        SDL_DestroyTexture(boss.texture);
    }
    if (projectilePlayerTemplate.texture != nullptr) {
        SDL_DestroyTexture(projectilePlayerTemplate.texture);
    }
    if (projectileBossTemplate.texture != nullptr) {
        SDL_DestroyTexture(projectileBossTemplate.texture);
    }
    if (explosionTemplate.texture != nullptr) {
        SDL_DestroyTexture(explosionTemplate.texture);
    }
    
    // 清理音效
    for (auto& sound : sounds) {
        if (sound.second != nullptr) {
            Mix_FreeChunk(sound.second);
        }
    }
    sounds.clear();
    
    // 清理对象列表
    for (auto projectile : projectilesPlayer) {
        playerBulletPool.release(projectile);
    }
    for (auto projectile : projectilesBoss) {
        bossBulletPool.release(projectile);
    }
    for (auto explosion : explosions) {
        explosionPool.release(explosion);
    }
    
    projectilesPlayer.clear();
    projectilesBoss.clear();
    explosions.clear();
}

