.code

_start PROC
    ; 保存寄存器
    pushfq
    push    rax
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    r8
    push    r9
    
    ; 動態獲取實際基地址
    call    get_current_base
get_current_base:
    pop     rax                     ; 獲取當前指令地址
    and     rax, 0FFFFFFFFFFFFF000h ; 對齊到頁邊界
    
    ; 向後搜尋找到 MZ 簽名
find_pe_header:
    cmp     word ptr [rax], 5A4Dh   ; "MZ"
    je      found_pe_base
    sub     rax, 1000h              ; 向前一頁
    jmp     find_pe_header
    
found_pe_base:
    ; rax = 實際模組基地址
    ; 計算實際的 .text 段地址
    lea     rsi, [rax + 1000h]      ; .text 段 RVA = 0x1000
    
    ; 這些佔位符會被 packer 替換，但我們用動態計算的值
    mov     r8, 1111111111111111h   ; 加密段地址佔位符（會被替換）
    mov     ecx, 22222222h          ; 加密段大小佔位符（會被替換）
    
    ; 使用動態計算的地址而不是佔位符
    ; rsi 已經是正確的動態地址
    mov     al, 0AAh                ; XOR 密鑰
    
    ; 檢查大小是否有效
    test    ecx, ecx
    jz      cleanup
    
    ; 嘗試解密
decrypt_loop:
    xor     byte ptr [rsi], al
    inc     rsi
    dec     ecx
    jnz     decrypt_loop
    
cleanup:
    ; 恢復寄存器
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rax
    popfq
    
    ; 動態計算 OEP 並跳轉
    call    get_oep_base
get_oep_base:
    pop     rax
    and     rax, 0FFFFFFFFFFFFF000h
find_pe_for_oep:
    cmp     word ptr [rax], 5A4Dh
    je      found_oep_base
    sub     rax, 1000h
    jmp     find_pe_for_oep
found_oep_base:
    ; 計算實際 OEP
    lea     rax, [rax + 1181h]      ; OEP = 基地址 + 原始 Entry Point RVA
    
    ; 這個佔位符也會被 packer 替換，但我們忽略它
    mov     rdx, 3333333333333333h  ; 原始 OEP 佔位符（會被替換）
    
    ; 使用動態計算的地址
    jmp     rax

_start ENDP

END