#include <iostream>
#include <windows.h>

int main() {
    std::cout << "=== Console Sample Program ===" << std::endl;
    std::cout << "This is a simple console application for testing the packer." << std::endl;
    
    // 显示一些基本信息
    std::cout << "Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "Thread ID: " << GetCurrentThreadId() << std::endl;
    
    // 简单的计算
    int sum = 0;
    for (int i = 1; i <= 100; ++i) {
        sum += i;
    }
    std::cout << "Sum of 1-100: " << sum << std::endl;
    
    std::cout << "Press Enter to continue..." << std::endl;
    std::cin.get();
    
    std::cout << "Console sample completed successfully!" << std::endl;
    return 0;
}