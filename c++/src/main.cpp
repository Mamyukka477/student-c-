#include <iostream>          // 标准输入输出流库
#include "Game.h"            // 游戏主类头文件
#include <windows.h>         // Windows API头文件

// Windows程序入口点函数
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,        // 当前实例句柄
    _In_opt_ HINSTANCE hPrevInstance, // 前一个实例句柄（通常为NULL）
    _In_ LPSTR lpCmdLine,            // 命令行参数字符串
    _In_ int nCmdShow                // 窗口显示状态
) {
    // 获取Game单例对象的引用
    Game& game = Game::getInstance();
    // 初始化游戏系统（SDL、音频、图形等）
    game.init();
    // 运行游戏主循环（事件处理、更新、渲染）
    game.run();
    
    return 0; // 程序正常退出
}