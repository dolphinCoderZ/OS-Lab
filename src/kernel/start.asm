[bits 32]

extern device_init
extern console_init
extern gdt_init
extern gdt_ptr
extern memory_init
extern kernel_init

code_selector equ (1 << 3)
data_selector equ (2 << 3)

section .text
global _start
_start:
    ; 取loader压入的ards_count和魔数
    push ebx ; ards_count
    push eax ; magic

    call device_init
    call console_init
    call gdt_init

    lgdt [gdt_ptr]

    jmp dword code_selector:_next

_next:
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax; 初始化段寄存器

    call memory_init    ; 内存初始化
    mov esp, 0x10000; 修改栈顶
    call kernel_init    ; 内核初始化

    jmp $; 阻塞
