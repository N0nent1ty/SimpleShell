.code

_start PROC
    ; 最简单的stub - 不解密，只跳转
    ; 保存必要的寄存器
    push    rax
    push    rbx
    
    ; 获取当前模块基地址
    call    get_base_addr
    
get_base_addr:
    pop     rax                     ; 获取返回地址
    and     rax, 0FFFFFFFFFFFFF000h ; 对齐到页边界
    
    ; 向后搜索MZ签名
find_mz_sig:
    cmp     word ptr [rax], 5A4Dh   ; "MZ"
    je      found_module_base
    sub     rax, 1000h              ; 向前一页
    cmp     rax, 10000h             ; 防止搜索过远
    jb      error_exit
    jmp     find_mz_sig
    
found_module_base:
    ; rax = 模块基地址
    mov     rbx, rax                ; 保存基地址
    
    ; 加载原始OEP RVA（这个占位符会被packer替换）
    mov     rax, 3333333333333333h  ; 原始OEP RVA占位符
    
    ; 计算原始OEP的绝对地址 = 基地址 + RVA
    add     rax, rbx
    
    ; 恢复寄存器
    pop     rbx
    add     rsp, 8                  ; 跳过保存的rax，因为我们要用新的rax跳转
    
    ; 直接跳转到原始OEP
    jmp     rax

error_exit:
    ; 如果出错，尝试退出进程
    pop     rbx
    pop     rax
    mov     rcx, 1                  ; 退出码
    ret

_start ENDP

END