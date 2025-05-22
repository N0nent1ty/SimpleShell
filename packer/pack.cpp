int main(int argc, char* argv[]) {
    // 1. 讀取目標 PE 檔
    // 2. 找到 .text 區段
    // 3. XOR 加密 .text 區段
    // 4. 插入 stub（重新對齊 section、加入 stub 程式碼）
    // 5. 記錄原始 EntryPoint，修改到 stub 起始點
    // 6. 輸出新的 packed 檔案
}