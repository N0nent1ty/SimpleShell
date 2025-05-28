cmake_minimum_required(VERSION 3.15)
project(SimpleShell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制64位架构
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_VS_PLATFORM_NAME x64)

# 确保所有目标都是64位
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Building for 64-bit architecture")
else()
    message(FATAL_ERROR "This project requires 64-bit architecture!")
endif()

#=========================================================
# Versioning
#=========================================================
set(USER_VERSION_MAJOR "1.0" CACHE STRING "Major version number")

# Get version from git tag or commit count
execute_process(
  COMMAND git describe --tags --abbrev=0
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

execute_process(
  COMMAND git rev-list --count HEAD
  OUTPUT_VARIABLE GIT_REV_COUNT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

# 如果git命令失败，使用默认值
if(NOT GIT_REV_COUNT)
    set(GIT_REV_COUNT "1")
endif()

# 拼接版本號，例如 1.0.42
string(CONCAT PROJECT_VERSION "${USER_VERSION_MAJOR}." "${GIT_REV_COUNT}")
message(STATUS "Generated Version: ${PROJECT_VERSION}")

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/version.h"
"#pragma once\n#define VERSION \"${PROJECT_VERSION}\"\n")

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
    src/packer/pack.cpp
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
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && cd /d \"${CMAKE_CURRENT_SOURCE_DIR}/src/stub\" && ml64 /c x64Stub.asm /Fo:x64Stub.obj /nologo"
    
    # 步驟3: 檢查 obj 文件是否生成
    COMMAND ${CMAKE_COMMAND} -E echo "Checking for x64Stub.obj..."
    COMMAND powershell -Command "if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/src/stub/x64Stub.obj') { echo '✓ Object file created' } else { echo '✗ Object file missing!'; exit 1 }"
    
    # 步驟4: 鏈接 - 使用正確的文件名
    COMMAND ${CMAKE_COMMAND} -E echo "Linking..."
    COMMAND cmd /c "call \"${VSVARS_PATH}\" && cd /d \"${CMAKE_CURRENT_SOURCE_DIR}/src/stub\" && link x64Stub.obj /entry:_start /subsystem:native /out:x64Stub.exe /filealign:512 /nologo"
    
    # 步驟5: 檢查 exe 文件
    COMMAND powershell -Command "if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/src/stub/x64Stub.exe') { echo '✓ Executable created' } else { echo '✗ Executable missing!'; exit 1 }"
    
    # 步驟6: 使用 objcopy 提取 .text 段
    COMMAND ${CMAKE_COMMAND} -E echo "Extracting .text section with objcopy..."
    COMMAND objcopy -O binary --only-section=.text ${CMAKE_CURRENT_SOURCE_DIR}/src/stub/x64Stub.exe ${CMAKE_CURRENT_SOURCE_DIR}/src/stub/x64Stub.bin
    
    # 步驟7: 確保 output 目錄存在並複製文件
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/output
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/stub/x64Stub.bin ${CMAKE_CURRENT_SOURCE_DIR}/output/stub.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Copied x64Stub.bin to output/stub.bin"
    
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/stub
    DEPENDS create_output_dir
    COMMENT "Building stub with objcopy extraction"
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
# Compile & link sample code for testing - 修复链接问题
#=========================================================

add_executable(sample_hello WIN32  # 添加WIN32标志
    src/samples/sample.cpp
)

# 正确配置Windows GUI程序的链接选项 - 强制64位
set_target_properties(sample_hello PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /MACHINE:X64 /ENTRY:WinMainCRTStartup"
)

# 链接必要的Windows库 - 确保包含所有必需的运行时库
target_link_libraries(sample_hello
    user32.lib
    gdi32.lib
    kernel32.lib
    shell32.lib
    comctl32.lib
)

# 添加编译器标志处理UTF-8和字符编码
target_compile_options(sample_hello PRIVATE
    /utf-8          # 使用UTF-8编码
    /wd4819         # 禁用字符编码警告
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
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output') { ls '${CMAKE_CURRENT_SOURCE_DIR}/output' | Where-Object { $_.Extension -eq '.exe' -or $_.Extension -eq '.bin' } | Format-Table Name, Length } else { echo 'Output directory not found' } }"
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
    COMMAND powershell -Command "& { if (Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/output') { ls '${CMAKE_CURRENT_SOURCE_DIR}/output' | Where-Object { $_.Extension -eq '.exe' } | ForEach-Object { echo \"$($_.Name): $($_.Length) bytes\" } } }"
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

# 简单的控制台测试程序 - 避免GUI问题，强制64位
add_executable(sample_console
    src/samples/console_sample.cpp
)

# 确保控制台程序也是64位
set_target_properties(sample_console PROPERTIES
    LINK_FLAGS "/MACHINE:X64"
)

add_custom_command(TARGET sample_console POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sample_console>
        ${CMAKE_CURRENT_SOURCE_DIR}/output/
)

# 测试控制台版本的打包
add_custom_target(pack_console
    COMMAND ${CMAKE_COMMAND} -E echo "Packing console sample..."
    COMMAND $<TARGET_FILE:simpleshell> sample_console.exe packed_console.exe
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output
    DEPENDS simpleshell sample_console build_stub
    COMMENT "Packing console sample executable"
)