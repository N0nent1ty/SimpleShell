#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <algorithm>

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

bool patch_stub_info(std::vector<uint8_t>& stub, DWORD text_rva, DWORD text_size, uint8_t xor_key) {
    uint32_t pattern_rva  = 0xAAAAAAAA;
    uint32_t pattern_size = 0xBBBBBBBB;
    uint8_t  pattern_key  = 0xCC;

    bool found_rva = false, found_size = false, found_key = false;

    for (size_t i = 0; i + 4 <= stub.size(); ++i) {
        if (!found_rva && memcmp(&stub[i], &pattern_rva, 4) == 0) {
            memcpy(&stub[i], &text_rva, 4);
            found_rva = true;
        }
        if (!found_size && memcmp(&stub[i], &pattern_size, 4) == 0) {
            memcpy(&stub[i], &text_size, 4);
            found_size = true;
        }
    }
    for (size_t i = 0; i < stub.size(); ++i) {
        if (!found_key && stub[i] == pattern_key) {
            stub[i] = xor_key;
            found_key = true;
        }
    }

    return found_rva && found_size && found_key;
}

int pack(const std::string& input_path, const std::string& output_path) {
    std::vector<uint8_t> binary;
    if (!read_file(input_path, binary)) return 1;

    auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(binary.data());
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return 1;

    auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS64*>(binary.data() + dos_header->e_lfanew);
    if (nt_header->Signature != IMAGE_NT_SIGNATURE) return 1;

    IMAGE_SECTION_HEADER* section_header = IMAGE_FIRST_SECTION(nt_header);
    IMAGE_SECTION_HEADER* text_section = nullptr;

    for (int i = 0; i < nt_header->FileHeader.NumberOfSections; ++i) {
        if (strcmp((char*)section_header[i].Name, ".text") == 0) {
            text_section = &section_header[i];
            section_header[i].Characteristics |= IMAGE_SCN_MEM_WRITE;
            break;
        }
    }
    if (!text_section) return 1;

    DWORD text_rva  = text_section->VirtualAddress;
    DWORD text_size = text_section->SizeOfRawData;
    DWORD text_raw  = text_section->PointerToRawData;
    //text_section->Characteristics |= IMAGE_SCN_MEM_WRITE;
    uint8_t xor_key = 0xAA;

    for (DWORD i = 0; i < text_size; ++i)
        binary[text_raw + i] ^= xor_key;

    std::vector<uint8_t> stub;
    if (!read_file("stub.bin", stub)) return 1;

    DWORD original_oep_rva = nt_header->OptionalHeader.AddressOfEntryPoint;

    const uint64_t oep_pattern = 0x3333333333333333ULL;
    for (size_t i = 0; i + 8 <= stub.size(); ++i) {
        if (memcmp(stub.data() + i, &oep_pattern, 8) == 0) {
            uint64_t oep64 = original_oep_rva;
            memcpy(stub.data() + i, &oep64, 8);
            break;
        }
    }

    if (!patch_stub_info(stub, text_rva, text_size, xor_key)) return 1;

    DWORD file_align = nt_header->OptionalHeader.FileAlignment;
    DWORD section_align = nt_header->OptionalHeader.SectionAlignment;

    DWORD current_sections = nt_header->FileHeader.NumberOfSections;
    IMAGE_SECTION_HEADER& new_section = section_header[current_sections];
    memset(&new_section, 0, sizeof(IMAGE_SECTION_HEADER));
    memcpy(new_section.Name, ".stub", 5);

    DWORD last_raw_end = section_header[current_sections - 1].PointerToRawData +
                         section_header[current_sections - 1].SizeOfRawData;
    DWORD last_va_end  = section_header[current_sections - 1].VirtualAddress +
                         section_header[current_sections - 1].Misc.VirtualSize;

    new_section.PointerToRawData = (last_raw_end + file_align - 1) & ~(file_align - 1);
    new_section.VirtualAddress   = (last_va_end + section_align - 1) & ~(section_align - 1);
    new_section.SizeOfRawData    = (DWORD)((stub.size() + file_align - 1) & ~(file_align - 1));
    new_section.Misc.VirtualSize = (DWORD)stub.size();
    new_section.Characteristics  = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;

    size_t new_file_size = new_section.PointerToRawData + new_section.SizeOfRawData;
    binary.resize(new_file_size, 0);
    memcpy(binary.data() + new_section.PointerToRawData, stub.data(), stub.size());

    nt_header->FileHeader.NumberOfSections++;
    nt_header->OptionalHeader.AddressOfEntryPoint = new_section.VirtualAddress;
    nt_header->OptionalHeader.SizeOfImage = new_section.VirtualAddress +
        ((new_section.Misc.VirtualSize + section_align - 1) & ~(section_align - 1));

    return write_file(output_path, binary) ? 0 : 1;
}