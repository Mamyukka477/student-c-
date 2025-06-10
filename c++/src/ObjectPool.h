#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include "Object.h"
#include <vector>
#include <SDL3/SDL.h>

// 模版类
template<typename T>
class ObjectPool{
private:
    size_t poolSize;
    std::vector<T> pool;
    T* firstAvailable = nullptr;
    size_t activeCount = 0;
    bool initialized = false;  // 添加初始化标志

public:
    // 添加默认构造函数
    ObjectPool() : poolSize(0), activeCount(0), initialized(false) {
        // 空的默认构造函数，稍后通过 initialize 方法初始化
    }
    
    // 原有的构造函数
    ObjectPool(const T& prototype, size_t pool_size = 20) : poolSize(pool_size), activeCount(0), initialized(true) {
        if (pool_size == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Object pool size cannot be zero");
            return;
        }
        
        // 使用resize并用原型对象初始化
        pool.resize(poolSize, prototype);
        
        // 初始化链表
        firstAvailable = &pool[0];
        for (size_t i = 0; i < poolSize - 1; i++) {
            pool[i].next = &pool[i + 1];
        }
        pool[poolSize - 1].next = nullptr;
        
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Object pool initialized with %zu objects", poolSize);
    }
    
    // 添加初始化方法
    void initialize(const T& prototype, size_t pool_size = 20) {
        if (initialized) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Object pool already initialized");
            return;
        }
        
        poolSize = pool_size;
        activeCount = 0;
        
        if (pool_size == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Object pool size cannot be zero");
            return;
        }
        
        pool.resize(poolSize, prototype);
        
        firstAvailable = &pool[0];
        for (size_t i = 0; i < poolSize - 1; i++) {
            pool[i].next = &pool[i + 1];
        }
        pool[poolSize - 1].next = nullptr;
        
        initialized = true;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Object pool initialized with %zu objects", poolSize);
    }
    
    // 修改 create 方法，添加初始化检查
    T* create() {
        if (!initialized) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Object pool not initialized");
            return nullptr;
        }
        
        if (firstAvailable == nullptr) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Object pool exhausted! Active objects: %zu/%zu", activeCount, poolSize);
            return nullptr;
        }
        
        T* result = firstAvailable;
        firstAvailable = firstAvailable->next;
        result->next = nullptr;
        activeCount++;
        
        return result;
    }
    
    // 修改其他方法也添加初始化检查
    void release(T* object) {
        if (!initialized || object == nullptr) {
            return;
        }
        
        // 验证对象是否属于这个池子
        if (object < &pool[0] || object > &pool[poolSize - 1]) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Attempting to release object not from this pool");
            return;
        }
        
        resetObject(object);
        object->next = firstAvailable;
        firstAvailable = object;
        activeCount--;
    }
    
    // 获取池子状态信息
    size_t getPoolSize() const { return poolSize; }
    size_t getActiveCount() const { return activeCount; }
    size_t getAvailableCount() const { return poolSize - activeCount; }
    bool isEmpty() const { return firstAvailable == nullptr; }
    
    // 重置整个池子
    void reset() {
        firstAvailable = &pool[0];
        for (size_t i = 0; i < poolSize - 1; i++) {
            resetObject(&pool[i]);
            pool[i].next = &pool[i + 1];
        }
        resetObject(&pool[poolSize - 1]);
        pool[poolSize - 1].next = nullptr;
        activeCount = 0;
    }
    
private:
    // 重置对象状态的辅助函数
    void resetObject(T* object) {
        // 这里可以根据不同类型的对象进行特定的重置操作
        // 例如重置位置、状态等，但保留纹理等资源
        if constexpr (std::is_same_v<T, Enemy>) {
            object->position = {0, 0};
            object->currentHealth = 0;
            object->lastShootTime = 0;
        } else if constexpr (std::is_same_v<T, ProjectilePlayer>) {
            object->position = {0, 0};
            object->bounceCount = 0;
            object->direction = {1, 0};
        } else if constexpr (std::is_same_v<T, ProjectileEnemy>) {
            object->position = {0, 0};
            object->direction = {0, 0};
        } else if constexpr (std::is_same_v<T, Explosion>) {
            object->position = {0, 0};
            object->currentFrame = 0;
            object->startTime = 0;
        } else if constexpr (std::is_same_v<T, Item>) {
            object->position = {0, 0};
            object->direction = {0, 0};
            object->bounceCount = 3;
            object->currentFrame = 0;
            object->startTime = 0;
        }
    }
};

#endif // OBJECT_POOL_H
