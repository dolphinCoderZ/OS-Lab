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
    ; ȡloaderѹ���ards_count��ħ��
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
    mov ss, ax; ��ʼ���μĴ���

    call memory_init    ; �ڴ��ʼ��
    mov esp, 0x10000; �޸�ջ��
    call kernel_init    ; �ں˳�ʼ��

    jmp $; ����
