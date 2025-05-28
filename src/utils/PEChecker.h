#pragma once
#include <fstream>
#include <string>

// 檢查檔案是否為合法的 PE 格式（EXE/DLL）
bool IsValidPEFile(const std::string& filepath);