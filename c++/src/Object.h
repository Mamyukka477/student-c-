#ifndef OBJECT_H    // 防止头文件重复包含
#define OBJECT_H    // 定义头文件标识符
#include <SDL3/SDL.h>  // SDL3核心库
#include <string>      // 标准字符串库
#include <vector>      // 添加vector头文件

// 道具类型枚举，定义游戏中可收集的道具种类
enum class ItemType{
    Life,    // 生命道具，增加玩家生命值
    Shield,  // 护盾道具，增加玩家护盾值
    Time,    // 时间道具，可能影响游戏时间
    Gold     // 金币道具，增加分数或货币
};

// 玩家结构体，存储玩家的所有状态和属性
// 武器类型枚举
enum class WeaponType {
    BASIC,      // 基础武器
    RAPID,      // 快速射击
    BOUNCER,    // 反弹武器
    SPLITTER,   // 分裂武器
    PIERCER     // 穿透武器
};

// 武器升级选项
enum class WeaponUpgrade {
    DAMAGE_UP,      // 伤害提升
    SPEED_UP,       // 射速提升
    BOUNCE_UP,      // 反弹次数提升
    SPLIT_UP,       // 分裂数量提升
    PIERCE_UP       // 穿透能力提升
};

// 武器结构体
struct Weapon {
    WeaponType type = WeaponType::BASIC;    // 武器类型
    int damage = 1;                         // 基础伤害
    float fireRate = 1.0f;                  // 射速倍率
    int bounceCount = 0;                    // 反弹次数
    int splitCount = 1;                     // 分裂数量
    bool piercing = false;                  // 是否穿透
    int level = 1;                          // 武器等级
    
    // 获取实际冷却时间
    Uint64 getCooldown(Uint32 baseCooldown) const {
        return static_cast<Uint64>(baseCooldown / fireRate);
    }
};

struct Player{
    SDL_Texture* texture = nullptr;         // 玩家角色的纹理图像
    SDL_FPoint position = {0, 0};           // 玩家在屏幕上的位置坐标
    float width = 0;                        // 玩家角色的显示宽度
    float height = 0;                       // 玩家角色的显示高度
    int speed = 300;                        // 玩家移动速度（像素/秒）
    int currentHealth = 3;                  // 玩家当前生命值
    int maxHealth = 3;                      // 玩家最大生命值
    int currentShield = 0;                  // 玩家当前护盾值
    Uint32 coolDown = 300;                  // 射击冷却时间（毫秒）
    Uint64 lastShootTime = 0;               // 上次射击的时间戳
    SDL_FlipMode flip = SDL_FLIP_NONE;      // 纹理翻转模式
    Weapon weapon;                          // 玩家武器
    Player* next = nullptr;                 // 对象池链表的下一个节点指针
};

// 敌人结构体，存储敌人的状态和行为属性
struct Enemy{
    SDL_Texture* texture = nullptr;         // 敌人的主要纹理
    SDL_Texture* randomTextures[10] = {nullptr}; // 随机敌人的10种不同纹理
    SDL_FPoint position = {0, 0};           // 敌人在屏幕上的位置
    float width = 0, height = 0;            // 敌人的显示尺寸
    float speed = 0;                        // 敌人移动速度
    int currentHealth = 0;                  // 敌人当前血量
    Uint32 lastShootTime = 0;               // 敌人上次射击时间
    Uint32 coolDown = 0;                    // 敌人射击冷却时间
    int type = 0;                           // 敌人类型：0-基础敌人，1-敌人1，2-敌人2
    float rotationAngle = 0.0f;             // 敌人旋转角度（用于旋转类敌人）
    float moveTimer = 0.0f;                 // 移动计时器（用于控制移动模式）
    int currentTextureIndex = 0;            // 当前使用的纹理索引（随机敌人）
    Enemy* next = nullptr;                  // 对象池链表指针
};

// 玩家子弹结构体
struct ProjectilePlayer{
    SDL_Texture* texture = nullptr;         // 子弹纹理
    SDL_FRect position = {0, 0, 0, 0};      // 子弹位置和尺寸
    float width = 0, height = 0;            // 子弹尺寸
    int speed = 800;                        // 子弹移动速度
    int damage = 1;                         // 子弹伤害值
    SDL_FPoint direction = {1, 0};          // 子弹移动方向
    int bounceCount = 0;                    // 反弹次数
    int maxBounces = 3;                     // 最大反弹次数
    std::vector<Enemy*> hitEnemies;         // 已击中的敌人列表（防止穿透子弹帧伤）
    ProjectilePlayer* next = nullptr;       // 对象池链表指针
};

// 敌人子弹结构体
struct ProjectileEnemy{
    SDL_Texture* texture = nullptr;         // 敌人子弹纹理
    SDL_FPoint position = {0, 0};           // 子弹位置
    SDL_FPoint direction = {0, 0};          // 子弹移动方向
    float width = 0;                        // 子弹宽度
    float height = 0;                       // 子弹高度
    int speed = 400;                        // 子弹速度
    int damage = 1;                         // 子弹伤害
    ProjectileEnemy* next = nullptr;        // 对象池链表指针
};

// 爆炸特效结构体
struct Explosion{
    SDL_Texture* texture = nullptr;         // 爆炸动画纹理
    SDL_FPoint position = {0, 0};           // 爆炸发生位置
    float width = 0;                        // 爆炸效果宽度
    float height = 0;                       // 爆炸效果高度
    int currentFrame = 0;                   // 当前播放帧
    int totlaFrame = 0;                     // 总帧数
    Uint32 startTime = 0;                   // 爆炸开始时间
    Uint32 FPS = 10;                        // 动画播放帧率
    Explosion* next = nullptr;              // 对象池链表指针
};

// 道具结构体
struct Item{
    SDL_Texture* texture = nullptr;         // 道具纹理图像
    SDL_FPoint position = {0, 0};           // 道具位置
    SDL_FPoint direction = {0, 0};          // 道具移动方向
    float width = 0;                        // 道具宽度
    float height = 0;                       // 道具高度
    int speed = 200;                        // 道具移动速度
    int bounceCount = 3;                    // 道具反弹次数
    ItemType type = ItemType::Life;         // 道具类型
    
    // 动画相关属性（用于有动画效果的道具如金币）
    int currentFrame = 0;                   // 当前动画帧
    int totlaFrame = 1;                     // 总动画帧数
    Uint32 startTime = 0;                   // 动画开始时间
    Uint32 FPS = 8;                         // 动画播放帧率
    Item* next = nullptr;                   // 对象池链表指针
};

// 背景结构体，用于滚动背景效果
struct Background{
    SDL_Texture* texture = nullptr;         // 背景纹理
    SDL_FPoint position = {0, 0};           // 背景位置
    float offset = 0;                       // 滚动偏移量
    float width = 0;                        // 背景宽度
    float height = 0;                       // 背景高度
    int speed = 30;                         // 背景滚动速度
};

// 按钮结构体，用于UI界面
struct Button {
    SDL_FRect rect = {0, 0, 0, 0};          // 按钮矩形区域
    std::string text;                       // 按钮显示文字
    bool isHovered = false;                 // 鼠标是否悬停在按钮上
    bool isPressed = false;                 // 按钮是否被按下
    SDL_Color normalColor = {100, 100, 100, 255};   // 正常状态颜色
    SDL_Color hoverColor = {150, 150, 150, 255};    // 悬停状态颜色
    SDL_Color pressedColor = {135, 206, 250, 255};  // 按下状态颜色（浅蓝色）
};

// 滑动条结构体，用于设置界面
struct Slider {
    SDL_FRect rect;                         // 滑动条轨道矩形
    SDL_FRect handle;                       // 滑块矩形
    std::string label;                      // 滑动条标签文字
    int value;                              // 当前值 (0-100)
    int minValue = 0;                       // 最小值
    int maxValue = 100;                     // 最大值
    bool isDragging = false;                // 是否正在拖拽滑块
    SDL_Color trackColor = {100, 100, 100, 255};    // 轨道颜色
    SDL_Color handleColor = {200, 200, 200, 255};   // 滑块颜色
    SDL_Color activeColor = {255, 255, 255, 255};   // 激活状态颜色
};

// Boss结构体，存储Boss敌人的属性
struct Boss{
    SDL_Texture* texture = nullptr;         // Boss纹理
    SDL_FPoint position = {0, 0};           // Boss位置
    float width = 0;                        // Boss宽度
    float height = 0;                       // Boss高度
    int currentHealth = 1000;               // Boss当前血量
    int maxHealth = 1000;                   // Boss最大血量
    Uint32 lastShootTime = 0;               // Boss上次射击时间
    Uint32 coolDown = 100;                  // Boss射击冷却时间
    float shootAngle = 0.0f;                // Boss射击角度
    int shootPattern = 0;                   // Boss弹幕模式
    Uint32 patternChangeTime = 0;           // 弹幕模式切换时间
    Boss* next = nullptr;                   // 对象池链表指针
};

// Boss子弹结构体
struct ProjectileBoss{
    SDL_Texture* texture = nullptr;         // Boss子弹纹理
    SDL_FPoint position = {0, 0};           // 子弹位置
    SDL_FPoint direction = {0, 0};          // 子弹移动方向
    float width = 0;                        // 子弹宽度
    float height = 0;                       // 子弹高度
    int speed = 200;                        // 子弹速度
    int damage = 1;                         // 子弹伤害
    float rotationAngle = 0.0f;             // 子弹旋转角度
    ProjectileBoss* next = nullptr;         // 对象池链表指针
};

#endif // OBJECT_H  // 头文件结束标记