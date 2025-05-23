cmake_minimum_required(VERSION 3.15)
project(SimpleShell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#=========================================================
# CMake configuration
#=========================================================
# Set the path to vcpkg and Visual Studio environment variables
set(VCPKG_ROOT "C:/Users/sheng/vcpkg")
# For getting masm path & linker path
set(VSVARS_PATH "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")

# 啟用 vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

#=========================================================
# Compile & link the main project
#=========================================================
add_executable(simpleshell
    packer/pack.cpp
    #crypto/xor_encrypt.cpp
    #utils/pe_utils.cpp
)

# 讓 simpleshell 依賴於 stub 的生成
add_dependencies(simpleshell build_stub)

add_custom_command(TARGET simpleshell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:simpleshell>
        ${CMAKE_CURRENT_SOURCE_DIR}/output/
)

#=========================================================
# Compile & link stub code - 使用 objcopy
#=========================================================

# 創建 output 目錄
add_custom_target(create_output_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/output
    COMMENT "Creating output directory"
)

add_custom_target(build_stub ALL  # 添加 ALL 關鍵字
    # 步驟1: 確保工作目錄並清理舊文件
    COMMAND ${CMAKE_COMMAND} -E echo "=== Building Stub ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Working directory: ${CMAKE_CURRENT_SOURCE_DIR}/stub"
    COMMAND ${CMAKE_COMMAND} -E remove -f x64Stub.obj x64Stub.exe x64Stub.bin
    
    # 步驟2: 編譯組語 - 修正文件名
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling x64Stub.asm..."
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && cd /d \"${CMAKE_CURRENT_SOURCE_DIR}/stub\" && ml64 /c x64Stub.asm /Fo:x64Stub.obj /nologo"
    
    # 步驟3: 檢查 obj 文件是否生成
    COMMAND ${CMAKE_COMMAND} -E echo "Checking for x64Stub.obj..."
    COMMAND powershell -Command "if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.obj') { echo '✓ Object file created' } else { echo '✗ Object file missing!'; exit 1 }"
    
    # 步驟4: 鏈接 - 使用正確的文件名
    COMMAND ${CMAKE_COMMAND} -E echo "Linking..."
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && cd /d \"${CMAKE_CURRENT_SOURCE_DIR}/stub\" && link x64Stub.obj /entry:_start /subsystem:native /out:x64Stub.exe /filealign:512 /nologo"
    
    # 步驟5: 檢查 exe 文件
    COMMAND powershell -Command "if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.exe') { echo '✓ Executable created' } else { echo '✗ Executable missing!'; exit 1 }"
    
    # 步驟6: 使用 objcopy 提取 .text 段
    COMMAND ${CMAKE_COMMAND} -E echo "Extracting .text section with objcopy..."
    COMMAND objcopy -O binary --only-section=.text ${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.exe ${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.bin
    
    # 步驟7: 確保 output 目錄存在並複製文件
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/output
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.bin ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Copied x64Stub.bin to output/stub.bin"
    
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stub
    DEPENDS create_output_dir
    COMMENT "Building stub with objcopy extraction"
)

# 手動構建 target（用於調試）- 修正文件名
add_custom_target(build_stub_manual
    COMMAND ${CMAKE_COMMAND} -E echo "=== Manual Stub Build Steps ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Please run these commands manually:"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "cd ${CMAKE_CURRENT_SOURCE_DIR}/stub"
    COMMAND ${CMAKE_COMMAND} -E echo "call \"${VSVARS_PATH}\""
    COMMAND ${CMAKE_COMMAND} -E echo "ml64 /c x64Stub.asm /Fo:x64Stub.obj"
    COMMAND ${CMAKE_COMMAND} -E echo "link x64Stub.obj /entry:_start /subsystem:native /out:x64Stub.exe"
    COMMAND ${CMAKE_COMMAND} -E echo "objcopy -O binary --only-section=.text x64Stub.exe x64Stub.bin"
    COMMAND ${CMAKE_COMMAND} -E echo "copy x64Stub.bin ..\\output\\stub.bin"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== End Manual Steps ==="
    COMMENT "Display manual build instructions"
)

# 簡化的手動測試腳本
add_custom_target(test_build_env
    COMMAND ${CMAKE_COMMAND} -E echo "Testing build environment..."
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && echo Visual Studio environment loaded && ml64 /? | findstr \"Microsoft\""
    COMMAND ${CMAKE_COMMAND} -E echo "Checking objcopy..."
    COMMAND objcopy --version
    COMMAND ${CMAKE_COMMAND} -E echo "Checking if stub source exists..."
    COMMAND ${CMAKE_COMMAND} -E echo "Looking for: ${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.asm"
    COMMAND powershell -Command "if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/stub/x64Stub.asm') { echo '✓ x64Stub.asm found' } else { echo '✗ x64Stub.asm not found!' }"
    COMMENT "Testing build environment"
)
add_custom_target(copy_stub
    COMMAND ${CMAKE_COMMAND} -E echo "Copying stub files to output directory..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/output
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${CMAKE_CURRENT_SOURCE_DIR}/stub/stub_x64.bin 
        ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Copy completed!"
    COMMAND ${CMAKE_COMMAND} -E echo "Verifying copied file:"
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin') { $size = (Get-Item '${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin').Length; echo \"✓ stub.bin exists ($size bytes)\" } else { echo '✗ stub.bin not found in output directory' } }"
    COMMENT "Copying stub.bin to output directory"
)
add_custom_target(build_stub_python
    # 步驟1-3: 同樣的編譯和鏈接
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && ml64 /c x64Stub.asm /Fo:stub_x64.obj"
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && link stub_x64.obj /entry:_start /subsystem:native /out:stub_x64.exe /filealign:512"
    # 步驟4: 使用 Python 提取（如果有的話）
    COMMAND python -c "
import struct
try:
    with open('stub_x64.exe', 'rb') as f:
        data = f.read()
    pe_offset = struct.unpack('<I', data[60:64])[0]
    optional_header_size = struct.unpack('<H', data[pe_offset+20:pe_offset+22])[0]
    first_section = pe_offset + 24 + optional_header_size
    raw_offset = struct.unpack('<I', data[first_section+20:first_section+24])[0]
    raw_size = struct.unpack('<I', data[first_section+16:first_section+20])[0]
    text_data = data[raw_offset:raw_offset+raw_size]
    with open('stub_x64.bin', 'wb') as f:
        f.write(text_data)
    print(f'Extracted {len(text_data)} bytes')
except Exception as e:
    print(f'Python extraction failed: {e}')
    exit(1)
"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different stub_x64.bin ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stub
    DEPENDS create_output_dir
    COMMENT "Building stub with Python extraction (backup method)"
)
add_custom_target(build_stub_simple
    # 步驟1: 編譯組語，直接生成 .bin 文件
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && echo Compiling assembly to binary... && ml64 /c x64Stub.asm /Fo:stub_x64.obj"
    # 步驟2: 創建一個簡單的鏈接腳本來生成純二進制
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && link stub_x64.obj /entry:_start /subsystem:native /out:stub_x64.exe /filealign:512"
    # 步驟3: 使用更簡單的方法 - 直接複製機器碼段
    COMMAND python -c "
import struct
with open('stub_x64.exe', 'rb') as f:
    data = f.read()
    # 找到 PE 頭
    pe_offset = struct.unpack('<I', data[60:64])[0]
    # 找到第一個段 (.text)
    sections_offset = pe_offset + 248
    text_rva = struct.unpack('<I', data[sections_offset + 12:sections_offset + 16])[0]
    text_size = struct.unpack('<I', data[sections_offset + 16:sections_offset + 20])[0]
    text_file_offset = struct.unpack('<I', data[sections_offset + 20:sections_offset + 24])[0]
    # 提取 .text 段數據
    text_data = data[text_file_offset:text_file_offset + text_size]
    with open('stub_x64.bin', 'wb') as out:
        out.write(text_data)
    print(f'Extracted {len(text_data)} bytes to stub_x64.bin')
"
    # 步驟4: 複製到 output
    COMMAND ${CMAKE_COMMAND} -E copy_if_different stub_x64.bin ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Copied stub.bin to output directory"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stub
    DEPENDS create_output_dir
    COMMENT "Building stub with Python extraction"
)



# 驗證stub文件的目標
add_custom_target(verify_stub
    COMMAND ${CMAKE_COMMAND} -E echo "=== Verifying stub.bin ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Checking if file exists:"
    COMMAND ${CMAKE_COMMAND} -E echo "File: ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin"
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin') { echo '✓ File exists'; $size = (Get-Item '${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin').Length; echo \"Size: $size bytes\"; if ($size -gt 0) { echo '✓ File is not empty' } else { echo '✗ File is empty!' } } else { echo '✗ File NOT found!' } }"
    COMMAND ${CMAKE_COMMAND} -E echo "File content preview:"
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin') { $bytes = [System.IO.File]::ReadAllBytes('${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin'); if ($bytes.Length -gt 0) { echo \"First 16 bytes: $(($bytes[0..15] | ForEach-Object { $_.ToString('X2') }) -join ' ')\" } } }"
    DEPENDS build_stub
    COMMENT "Verifying generated stub binary"
)

#=========================================================
# Compile & link sample code for testing
#=========================================================

add_executable(sample_hello
    samples/sample.cpp
)

# sample_hello 目標為 x64
set_target_properties(sample_hello PROPERTIES
    LINK_FLAGS "/ENTRY:WinMainCRTStartup /MACHINE:X64"
)

add_custom_command(TARGET sample_hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sample_hello>
        ${CMAKE_CURRENT_SOURCE_DIR}/output/
)

#=========================================================
# 測試和調試目標
#=========================================================

# 打包測試目標 - 修復依賴關係
add_custom_target(pack_sample
    COMMAND ${CMAKE_COMMAND} -E echo "Checking if stub.bin exists..."
    COMMAND ${CMAKE_COMMAND} -E echo "Current directory: ${CMAKE_CURRENT_SOURCE_DIR}/output"
    COMMAND powershell -Command "& { ls *.exe, *.bin | Format-Table Name, Length }"
    COMMAND ${CMAKE_COMMAND} -E echo "Packing sample_hello.exe..."
    COMMAND $<TARGET_FILE:simpleshell> sample_hello.exe packed_sample.exe
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output
    DEPENDS simpleshell sample_hello build_stub
    COMMENT "Packing sample executable"
)

# PE結構檢查目標
add_custom_target(check_pe
    COMMAND ${CMAKE_COMMAND} -E echo "Checking PE structure..."
    COMMAND ${CMAKE_COMMAND} -E echo "Files in output directory:"
    COMMAND powershell -Command "& { ls *.exe | ForEach-Object { echo \"$($_.Name): $($_.Length) bytes\" } }"
    # 如果你有dumpbin，可以取消註釋下面這行
    # COMMAND cmd /c "call \"${VSVARS_PATH}\" && dumpbin /headers packed_sample.exe"
    COMMAND ${CMAKE_COMMAND} -E echo "Manual verification required - use PE-bear or similar tool"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output
    DEPENDS pack_sample
    COMMENT "Checking PE file structure"
)

# 總構建目標 - 構建所有組件
add_custom_target(build_all
    DEPENDS simpleshell sample_hello build_stub
    COMMENT "Building all components including packer, sample, and stub"
)

# 快速設置目標 - 一次性完成所有設置
add_custom_target(setup
    DEPENDS build_stub
    COMMAND ${CMAKE_COMMAND} -E echo "=== Setup Complete ==="
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Stub compiled and copied"
    COMMAND ${CMAKE_COMMAND} -E echo "Next steps:"
    COMMAND ${CMAKE_COMMAND} -E echo "  1. cmake --build . --target build_all"
    COMMAND ${CMAKE_COMMAND} -E echo "  2. cmake --build . --target pack_sample"
    COMMENT "Initial setup - compiles stub"
)
add_custom_target(debug_info
    COMMAND ${CMAKE_COMMAND} -E echo "=== Debug Information ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Current directory structure:"
    COMMAND ${CMAKE_COMMAND} -E echo "stub/ directory:"
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/stub') { ls '${CMAKE_CURRENT_SOURCE_DIR}/stub' } else { echo 'stub directory not found' } }"
    COMMAND ${CMAKE_COMMAND} -E echo "output/ directory:"
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output') { ls '${CMAKE_CURRENT_SOURCE_DIR}/output' } else { echo 'output directory not found' } }"
    COMMENT "Displaying debug information"
)