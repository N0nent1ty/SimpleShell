import pefile

def extract_pe_info(path):
    pe = pefile.PE(path)
    return {
        "Path": path,
        "EntryPoint": hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint),
        "ImageBase": hex(pe.OPTIONAL_HEADER.ImageBase),
        "NumberOfSections": pe.FILE_HEADER.NumberOfSections,
        "SectionNames": [s.Name.decode('utf-8', errors='ignore').strip('\x00') for s in pe.sections],
        "DllCharacteristics": hex(pe.OPTIONAL_HEADER.DllCharacteristics),
        "Subsystem": pe.OPTIONAL_HEADER.Subsystem,
        "Magic": hex(pe.OPTIONAL_HEADER.Magic),
        "SizeOfImage": hex(pe.OPTIONAL_HEADER.SizeOfImage),
        "SizeOfHeaders": hex(pe.OPTIONAL_HEADER.SizeOfHeaders),
    }

original_info = extract_pe_info("../output/notepad.exe")
packed_info = extract_pe_info("../output/notepad_packed.exe")

from pprint import pprint
print("Original:")
pprint(original_info)
print("\nPacked:")
pprint(packed_info)