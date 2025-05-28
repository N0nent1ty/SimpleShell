#include <fstream>
#include <string>



bool IsValidPEFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        return false;

    // 檢查 DOS 標頭 "MZ"
    char mz[2];
    file.read(mz, 2);
    if (mz[0] != 'M' || mz[1] != 'Z')
        return false;

    // 讀取 PE header offset
    file.seekg(0x3C, std::ios::beg);
    uint32_t pe_offset = 0;
    file.read(reinterpret_cast<char*>(&pe_offset), sizeof(pe_offset));

    // 檢查 PE 標頭 "PE\0\0"
    file.seekg(pe_offset, std::ios::beg);
    char pe[4];
    file.read(pe, 4);
    if (pe[0] != 'P' || pe[1] != 'E' || pe[2] != 0 || pe[3] != 0)
        return false;

    return true;
}