# 🛡️ SimpleShell

**SimpleShell** is a high-performance, customizable software protector (shell/packer) designed to resist reverse engineering through modern techniques such as section encryption, runtime decryption, anti-debugging, and VM-aware defenses.

> ✅ Inspired by the dream of building the ultimate shell since university days, now brought to life.

---

> **Note:** The current version supports only 64-bit (x64) executables.

---

## 🛠️ Build Dependencies

- [vcpkg](https://github.com/microsoft/vcpkg)
- MinGW
- Visual Studio toolchain

---

## ✨ Features

### 1. PE Header Verification and Analysis

- ✅ DOS header verification (MZ signature)
- ✅ PE header verification (PE signature)
- ✅ x64 PE file support
- ✅ Section table parsing and verification
- ✅ Entry Point identification

### 2. Code Section Encryption

- ✅ Automatic `.text` section identification
- ✅ XOR encryption algorithm (key: 0xAA)
- ✅ Full section content encryption
- ✅ Section permission modification (add write permission)

### 3. Dynamic Decryption Stub

- ✅ Dynamic base address acquisition (ASLR compatible)
- ✅ PE header search algorithm (locate MZ signature)
- ✅ Runtime address calculation
- ✅ XOR decryption loop
- ✅ Register state protection

### 4. PE Structure Modification

- ✅ Add new section (.stub)
- ✅ Section table expansion
- ✅ Entry Point redirection
- ✅ PE header update (section count, SizeOfImage, SizeOfHeaders)
- ✅ File alignment handling

### 5. Memory Protection Handling

- ✅ Section permission setting (Execute + Read + Write)
- ✅ Runtime memory access
- ✅ Safe memory operations

### 6. ASLR Compatibility

- ✅ Address Space Layout Randomization support
- ✅ Dynamic base address calculation
- ✅ Correct handling of relative addresses (RVA)
- ✅ Adaptation to arbitrary load addresses

### 7. Placeholder System

- ✅ Three placeholder types supported:
    - `1111111111111111h` - Encrypted section virtual address
    - `22222222h` - Encrypted section size
    - `3333333333333333h` - Original entry point address

- ✅ Automatic search and replacement
- ✅ Binary pattern matching

---

## 📦 Project Structure