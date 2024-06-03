[org 0x1000]

; 校验loader
dw 0x55aa

mov si, loading
call print

detect_memory:
    xor ebx, ebx

    ; es:di 结构体的缓存位置
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer

    ; 固定签名
    mov edx, 0x534d4150

.next:
    ; 子功能号
    mov eax, 0xe820
    ; ards结构的大小(字节)
    mov ecx, 20
    ; boot提供的INT系统调用
    int 0x15

    ; 如果CF置位，表示出错
    jc error

    ; 将缓存指针指向下一个结构体
    add di, cx
    ; 结构体数量-1
    inc dword [ards_count]

    cmp ebx, 0
    jnz .next

    mov si, detecting
    call print

    jmp prepare_protected_mode

prepare_protected_mode:
    cli ; 关中断

    ; 打开A20线
    in al, 0x92
    or al, 0b10
    out 0x92, al

    lgdt [gdt_ptr] ; 加载gdt，目的：标记内存区域

    ; 启动保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 跳转刷新缓存，进入保护模式
    jmp dword code_selector:protect_mode

print:
    mov ah, 0x0e
.next:
    mov al, [si]
    cmp al, 0
    jz .done
    
    int 0x10
    inc si
    jmp .next
.done:
    ret

loading:
    db "loading Conix...", 10, 13, 0
detecting:
    db "Detecting Memory Success...", 10, 13, 0

error:
    mov si, .msg
    call print
    hlt ; 让CPU停止
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0

; 进入保护模式
[bits 32]
protect_mode:
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x10000 ; 修改栈顶

    ; 读取内核，进入内核
    mov edi, 0x10000
    mov ecx, 10
    mov bl, 200
    call read_disk

    mov eax, 0x20220205
    mov ebx, ards_count

    jmp dword code_selector:0x10000

    ; 到这里就意味着没跳转成功，出错
    ud2

read_disk:
    ; 设置读写扇区的数量
    mov dx, 0x1f2 ; 0x1f2读写扇区数量的端口
    mov al, bl
    out dx, al

    inc dx ; 0x1f3
    mov al, cl ; 起始扇区的前八位
    out dx, al

    inc dx ; 0x1f4
    shr ecx, 8
    mov al, cl ; 起始扇区的中八位
    out dx, al

    inc dx ; 0x1f5
    shr ecx, 8
    mov al, cl ; 起始扇区的高八位
    out dx, al

    inc dx ; 0x1f6
    shr ecx, 8
    and cl, 0b1111 ; 将高四位置0

    mov al, 0b1110_0000
    or al, cl
    out dx, al

    inc dx ; 0x1f7
    mov al, 0x20
    out dx, al

    xor ecx, ecx
    mov cl, bl

    .read:
        push cx
        call .waits
        call .reads
        pop cx
        loop .read

    ret

    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx
            jmp $+2
            jmp $+2
            jmp $+2
            and al, 0b1000_1000
            cmp al, 0b0000_1000
            jnz .check
        ret

    .reads:
        mov dx, 0x1f0
        mov cx, 256
        .readw:
            in ax, dx
            jmp $+2
            jmp $+2
            jmp $+2

            mov [edi], ax
            add edi, 2
            loop .readw
        ret

code_selector equ (1 << 3)
data_selector equ (2 << 3)

memory_base equ 0 ; 内存基地址
memory_limit equ ((1024 * 1024 * 1024 * 4)/(1024 * 4)) - 1 ; 内存界限

; 构建GDT
gdt_ptr:
    dw (gdt_end - gdt_base) - 1
    dd gdt_base

gdt_base:
    dd 0, 0
gdt_code:
    dw memory_limit & 0xffff
    dw memory_base & 0xffff
    db (memory_base >> 16) & 0xff
    
    db 0b_1_00_1_1_0_1_0
    db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff
gdt_data:
    dw memory_limit & 0xffff
    dw memory_base & 0xffff
    db (memory_base >> 16) & 0xff
    
    db 0b_1_00_1_0_0_1_0
    db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff
gdt_end:

ards_count:
    dd 0
ards_buffer:


