#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#define NOMINMAX
#include <windows.h>

const uint8_t XOR_KEY = 0xAA;

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

bool patch_stub_placeholders(std::vector<uint8_t>& stub, DWORD64 encrypted_va, DWORD encrypted_size, DWORD64 original_oep) {
    std::cout << "\n=== Patching Placeholders ===" << std::endl;
    std::cout << "  NOT encrypting - just testing stub jump" << std::endl;
    std::cout << "  Original OEP: 0x" << std::hex << original_oep << std::endl;
    
    // 只替換 OEP 佔位符，不處理加密相關的佔位符
    const uint8_t oep_pattern[] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
    bool found_oep = false;
    
    for (size_t i = 0; i <= stub.size() - 8; ++i) {
        if (memcmp(stub.data() + i, oep_pattern, 8) == 0) {
            memcpy(stub.data() + i, &original_oep, 8);
            std::cout << "  ✓ Updated original OEP at offset: 0x" << std::hex << i << std::endl;
            found_oep = true;
            break;
        }
    }
    
    if (!found_oep) {
        std::cout << "  ✗ OEP placeholder not found!" << std::endl;
    }
    
    return found_oep;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: packer <input.exe> <output.exe>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    std::vector<uint8_t> binary;

    std::cout << "=== NO ENCRYPTION Test Packer ===" << std::endl;

    if (!read_file(input_path, binary)) {
        std::cerr << "Failed to read input file." << std::endl;
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

    auto section_header = IMAGE_FIRST_SECTION(nt_header);
    DWORD64 original_oep = nt_header->OptionalHeader.ImageBase + nt_header->OptionalHeader.AddressOfEntryPoint;
    
    std::cout << "ImageBase: 0x" << std::hex << nt_header->OptionalHeader.ImageBase << std::endl;
    std::cout << "Original Entry Point: 0x" << nt_header->OptionalHeader.AddressOfEntryPoint << std::endl;
    std::cout << "Number of sections: " << std::dec << nt_header->FileHeader.NumberOfSections << std::endl;

    // 找到 .text 段但不加密它
    bool text_found = false;
    DWORD64 text_section_va = 0;
    DWORD text_section_size = 0;

    for (int i = 0; i < nt_header->FileHeader.NumberOfSections; ++i) {
        if (strncmp((char*)section_header[i].Name, ".text", 5) == 0) {
            text_section_va = nt_header->OptionalHeader.ImageBase + section_header[i].VirtualAddress;
            text_section_size = section_header[i].SizeOfRawData;
            
            std::cout << "Found .text section at VA 0x" << std::hex << text_section_va 
                      << " size 0x" << text_section_size << std::endl;
            std::cout << "  ** NOT ENCRYPTING for this test **" << std::endl;
            
            text_found = true;
            break;
        }
    }
    
    if (!text_found) {
        std::cerr << ".text section not found!" << std::endl;
        return 1;
    }

    // Read stub
    std::vector<uint8_t> stub;
    if (!read_file("stub.bin", stub)) {
        std::cerr << "Failed to load stub.bin" << std::endl;
        return 1;
    }

    std::cout << "Stub size: " << stub.size() << " bytes" << std::endl;

    // 只修補 OEP，不修補加密相關的佔位符
    if (!patch_stub_placeholders(stub, text_section_va, text_section_size, original_oep)) {
        std::cerr << "Failed to patch stub placeholders!" << std::endl;
        return 1;
    }

    // 添加新段的邏輯保持不變
    DWORD file_align = nt_header->OptionalHeader.FileAlignment;
    DWORD section_align = nt_header->OptionalHeader.SectionAlignment;
    
    DWORD current_sections = nt_header->FileHeader.NumberOfSections;
    DWORD headers_size = dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS64) + 
                        ((current_sections + 1) * sizeof(IMAGE_SECTION_HEADER));
    DWORD aligned_headers = (headers_size + file_align - 1) & ~(file_align - 1);
    
    DWORD first_section_start = section_header[0].PointerToRawData;
    if (aligned_headers > first_section_start) {
        std::cerr << "No space for additional section header!" << std::endl;
        return 1;
    }

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

    std::cout << "New section: .stub" << std::endl;
    std::cout << "  RVA: 0x" << std::hex << new_section.VirtualAddress << std::endl;
    std::cout << "  File offset: 0x" << new_section.PointerToRawData << std::endl;

    size_t new_file_size = new_section.PointerToRawData + new_section.SizeOfRawData;
    binary.resize(new_file_size, 0);
    memcpy(binary.data() + new_section.PointerToRawData, stub.data(), stub.size());

    // 更新 PE 頭
    nt_header->FileHeader.NumberOfSections++;
    nt_header->OptionalHeader.AddressOfEntryPoint = new_section.VirtualAddress;
    nt_header->OptionalHeader.SizeOfImage = new_section.VirtualAddress + 
        ((new_section.Misc.VirtualSize + section_align - 1) & ~(section_align - 1));
    nt_header->OptionalHeader.SizeOfHeaders = aligned_headers;

    std::cout << "Updated headers:" << std::endl;
    std::cout << "  Entry Point: 0x" << std::hex << nt_header->OptionalHeader.AddressOfEntryPoint << std::endl;
    std::cout << "  Will jump to original OEP: 0x" << (original_oep - nt_header->OptionalHeader.ImageBase) << std::endl;

    if (!write_file(output_path, binary)) {
        std::cerr << "Failed to write output file." << std::endl;
        return 1;
    }

    std::cout << "=== NO ENCRYPTION Test Completed ===" << std::endl;
    std::cout << "This version should work if PE structure is correct!" << std::endl;
    std::cout << "Output: " << output_path << " (" << binary.size() << " bytes)" << std::endl;
    
    return 0;
}