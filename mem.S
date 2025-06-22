get_stack_available:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 4192
        mov     rax, rsp
        mov     qword ptr [rbp - 80], rax
        mov     dword ptr [rbp - 2128], 10
        mov     rax, rsp
        mov     qword ptr [rbp - 80], rax
        call    pthread_self@PLT
        mov     rdi, rax
        lea     rsi, [rbp - 56]
        call    pthread_getattr_np@PLT
        lea     rdi, [rbp - 56]
        lea     rsi, [rbp - 64]
        lea     rdx, [rbp - 72]
        call    pthread_attr_getstack@PLT
        lea     rdi, [rbp - 56]
        call    pthread_attr_destroy@PLT
        mov     rax, qword ptr [rbp - 80]
        mov     rcx, qword ptr [rbp - 64]
        sub     rax, rcx
        mov     qword ptr [rbp - 4184], rax
        mov     rax, qword ptr [rbp - 4184]
        add     rsp, 4192
        pop     rbp
        ret

main:
        push    rbp
        mov     rbp, rsp
        call    get_stack_available
        mov     rsi, rax
        lea     rdi, [rip + .L.str]
        mov     al, 0
        sub     rsp, 1040
        call    printf@PLT
        lea     rax, [rbp - 1040]
        mov     qword ptr [rbp - 8], rax
        call    get_stack_available
        mov     rsi, rax
        lea     rdi, [rip + .L.str]
        mov     al, 0
        call    printf@PLT
        xor     eax, eax
        add     rsp, 1040
        pop     rbp
        ret

.L.str:
        .asciz  "stack avail: %zu\n"