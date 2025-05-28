#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include "../../version.h"

bool read_file(const std::string& path, std::vector<uint8_t>& buffer) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    buffer = std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
    return true;
}

bool write_file(const std::string& path, const std::vector<uint8_t>& buffer) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

bool patch_oep_only(std::vector<uint8_t>& stub, DWORD original_oep_rva) {
    std::cout << "Patching OEP placeholder with RVA: 0x" << std::hex << original_oep_rva << std::endl;
    
    // 只替换OEP占位符 (3333333333333333h)
    const uint64_t oep_pattern = 0x3333333333333333ULL;
    
    for (size_t i = 0; i <= stub.size() - 8; ++i) {
        if (memcmp(stub.data() + i, &oep_pattern, 8) == 0) {
            uint64_t oep_rva_64 = original_oep_rva;
            memcpy(stub.data() + i, &oep_rva_64, 8);
            std::cout << "* Updated OEP RVA at offset: 0x" << std::hex << i << std::endl;
            
            // 显示修改后的8字节
            std::cout << "* New bytes: ";
            for (int j = 0; j < 8; j++) {
                printf("%02X ", stub[i + j]);
            }
            std::cout << std::endl;
            return true;
        }
    }
    
    std::cout << "x OEP placeholder not found!" << std::endl;
    return false;
}

int main(int argc, char* argv[]) {
    std::cout << "=== SIMPLE NO ENCRYPTION TEST PACKER ===" << std::endl;
    std::cout << "SimpleShell Simple Test Packer v" << VERSION << std::endl;
    
    if (argc != 3) {
        std::cerr << "Usage: simple_pack <input.exe> <output.exe>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    std::vector<uint8_t> binary;

    if (!read_file(input_path, binary)) {
        std::cerr << "Failed to read input file: " << input_path << std::endl;
        return 1;
    }

    std::cout << "Original file size: " << binary.size() << " bytes" << std::endl;

    // Basic PE validation
    if (binary.size() < sizeof(IMAGE_DOS_HEADER)) {
        std::cerr << "File too small to be a valid PE." << std::endl;
        return 1;
    }

    auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(binary.data());
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Invalid DOS signature." << std::endl;
        return 1;
    }

    auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS64*>(binary.data() + dos_header->e_lfanew);
    if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Invalid PE signature." << std::endl;
        return 1;
    }

    DWORD original_oep_rva = nt_header->OptionalHeader.AddressOfEntryPoint;
    
    std::cout << "ImageBase: 0x" << std::hex << nt_header->OptionalHeader.ImageBase << std::endl;
    std::cout << "Original Entry Point RVA: 0x" << original_oep_rva << std::endl;
    std::cout << "Machine type: 0x" << nt_header->FileHeader.Machine << std::endl;
    
    // 验证是64位程序
    if (nt_header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        std::cerr << "This packer only supports x64 executables!" << std::endl;
        return 1;
    }

    // Read stub
    std::vector<uint8_t> stub;
    if (!read_file("stub.bin", stub)) {
        std::cerr << "Failed to load stub.bin" << std::endl;
        std::cout << "Make sure stub.bin exists in the current directory" << std::endl;
        return 1;
    }

    std::cout << "Stub size: " << stub.size() << " bytes" << std::endl;
    
    // 显示stub的前32字节用于调试
    std::cout << "Stub first 32 bytes: ";
    for (size_t i = 0; i < std::min((size_t)32, stub.size()); ++i) {
        printf("%02X ", stub[i]);
    }
    std::cout << std::endl;

    // 只修补OEP占位符
    if (!patch_oep_only(stub, original_oep_rva)) {
        std::cerr << "Failed to patch stub!" << std::endl;
        return 1;
    }

    // 显示修补后的stub前32字节
    std::cout << "Stub after patching (first 32 bytes): ";
    for (size_t i = 0; i < std::min((size_t)32, stub.size()); ++i) {
        printf("%02X ", stub[i]);
    }
    std::cout << std::endl;

    // 添加新段
    auto section_header = IMAGE_FIRST_SECTION(nt_header);
    DWORD file_align = nt_header->OptionalHeader.FileAlignment;
    DWORD section_align = nt_header->OptionalHeader.SectionAlignment;
    
    DWORD current_sections = nt_header->FileHeader.NumberOfSections;
    
    // 检查是否有空间添加新的段头
    DWORD headers_size = dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS64) + 
                        ((current_sections + 1) * sizeof(IMAGE_SECTION_HEADER));
    DWORD aligned_headers = (headers_size + file_align - 1) & ~(file_align - 1);
    
    DWORD first_section_start = section_header[0].PointerToRawData;
    if (aligned_headers > first_section_start) {
        std::cerr << "No space for additional section header!" << std::endl;
        return 1;
    }

    // 计算新段的位置
    DWORD last_idx = current_sections - 1;
    DWORD last_raw_end = section_header[last_idx].PointerToRawData + section_header[last_idx].SizeOfRawData;
    DWORD last_va_end = section_header[last_idx].VirtualAddress + section_header[last_idx].Misc.VirtualSize;
    
    IMAGE_SECTION_HEADER& new_section = section_header[current_sections];
    memset(&new_section, 0, sizeof(IMAGE_SECTION_HEADER));
    memcpy(new_section.Name, ".stub", 5);
    
    new_section.PointerToRawData = (last_raw_end + file_align - 1) & ~(file_align - 1);
    new_section.VirtualAddress = (last_va_end + section_align - 1) & ~(section_align - 1);
    new_section.SizeOfRawData = (static_cast<DWORD>(stub.size()) + file_align - 1) & ~(file_align - 1);
    new_section.Misc.VirtualSize = static_cast<DWORD>(stub.size());
    new_section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    std::cout << "\nAdding new section: .stub" << std::endl;
    std::cout << "  RVA: 0x" << std::hex << new_section.VirtualAddress << std::endl;
    std::cout << "  File offset: 0x" << new_section.PointerToRawData << std::endl;
    std::cout << "  Size: 0x" << new_section.SizeOfRawData << std::endl;

    // 扩展文件大小并复制stub
    size_t new_file_size = new_section.PointerToRawData + new_section.SizeOfRawData;
    binary.resize(new_file_size, 0);
    memcpy(binary.data() + new_section.PointerToRawData, stub.data(), stub.size());

    // 更新PE头
    nt_header->FileHeader.NumberOfSections++;
    nt_header->OptionalHeader.AddressOfEntryPoint = new_section.VirtualAddress;
    nt_header->OptionalHeader.SizeOfImage = new_section.VirtualAddress + 
        ((new_section.Misc.VirtualSize + section_align - 1) & ~(section_align - 1));
    nt_header->OptionalHeader.SizeOfHeaders = aligned_headers;

    std::cout << "\nUpdated PE headers:" << std::endl;
    std::cout << "  Old Entry Point RVA: 0x" << std::hex << original_oep_rva << std::endl;
    std::cout << "  New Entry Point RVA: 0x" << std::hex << nt_header->OptionalHeader.AddressOfEntryPoint << std::endl;
    std::cout << "  Image Size: 0x" << nt_header->OptionalHeader.SizeOfImage << std::endl;
    std::cout << "  Number of sections: " << std::dec << nt_header->FileHeader.NumberOfSections << std::endl;

    if (!write_file(output_path, binary)) {
        std::cerr << "Failed to write output file: " << output_path << std::endl;
        return 1;
    }

    std::cout << "\n=== SIMPLE TEST PACKING COMPLETED ===" << std::endl;
    std::cout << "* NO encryption - just testing stub jump logic" << std::endl;
    std::cout << "* Stub should jump from RVA 0x" << std::hex << new_section.VirtualAddress 
              << " to original OEP at RVA 0x" << original_oep_rva << std::endl;
    std::cout << "Output: " << output_path << " (" << std::dec << binary.size() << " bytes)" << std::endl;
    std::cout << "\nTry running the packed executable now." << std::endl;
    
    return 0;
}