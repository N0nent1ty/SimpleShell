.code

_start PROC
    ; Simplest stub - no decryption, just jump
    ; Save necessary registers
    push    rax
    push    rbx
    
    ; Get current module base address
    call    get_base_addr
    
get_base_addr:
    pop     rax                     ; Get return address
    and     rax, 0FFFFFFFFFFFFF000h ; Align to page boundary
    
    ; Search backward for MZ signature
find_mz_sig:
    cmp     word ptr [rax], 5A4Dh   ; "MZ"
    je      found_module_base
    sub     rax, 1000h              ; Go back one page
    cmp     rax, 10000h             ; Prevent searching too far
    jb      error_exit
    jmp     find_mz_sig
    
found_module_base:
    ; rax = module base address
    mov     rbx, rax                ; Save base address
    
    ; Load original OEP RVA (this placeholder will be replaced by packer)
    mov     rax, 3333333333333333h  ; Original OEP RVA placeholder
    
    ; Calculate absolute address of original OEP = base address + RVA
    add     rax, rbx
    
    ; Restore registers
    pop     rbx
    add     rsp, 8                  ; Skip saved rax, since we want to use new rax for jump
    
    ; Directly jump to original OEP
    jmp     rax

error_exit:
    ; If error, try to exit process
    pop     rbx
    pop     rax
    mov     rcx, 1                  ; Exit code
    ret

_start ENDP

END