.CODE

ShellCode PROC
    mov     rax, 4141414141414141h
    mov     rbx, rax
    mov     rcx, rax
    mov     rdx, rax
    mov     rsi, rax
    mov     rdi, rax
    mov     rbp, rax
    mov     r8, rax
    mov     r9, rax
    mov     r10, rax
    mov     r11, rax
    mov     r12, rax
    mov     r13, rax
    mov     r14, rax
    mov     r15, rax
    jmp     rax
ShellCode ENDP

ShellCodeEnd PROC
    int     3
ShellCodeEnd ENDP

END
