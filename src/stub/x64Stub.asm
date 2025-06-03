.code
PUBLIC _start

_start PROC
    ; reserver registers for use
    ; rbx: PEB base
    push rbx
    push rcx
    push rdx

    ; === Step 1: Get ImageBase from PEB ===
    mov     rbx, gs:[60h]        ; PEB base
    mov     rbx, [rbx + 10h]     ; PEB->ImageBaseAddress

    ; === Step 2: Load encrypted text section info ===
    mov     ecx, 0AAAAAAAAh      ; placeholder -> text RVA
    mov     edx, 0BBBBBBBBh      ; placeholder -> text size
    mov     al,  0CCh            ; placeholder -> XOR key

    ; === Step 3: Decrypt .text section ===
    add     rcx, rbx             ; rcx = VA of .text
    xor     r8d, r8d             ; counter = 0

decrypt_loop:
    cmp     r8d, edx
    jge     decrypt_done
    xor     byte ptr [rcx + r8], al
    inc     r8d
    jmp     decrypt_loop

decrypt_done:
    ; === Step 4: Jump to OEP ===
    mov     rax, 3333333333333333h  ; placeholder -> original OEP
    add     rax, rbx

    ; === Step 5: Restore registers and jump to OEP ===
    pop     rdx
    pop     rcx
    pop     rbx
    jmp     rax

_start ENDP
END
