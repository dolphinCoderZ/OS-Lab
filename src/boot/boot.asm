; boot加载到内存中的起始位置
[org 0x7c00]

; 设置屏幕模式为文本模式，清除屏幕
mov ax, 3
int 0x10

; 初始化段寄存器
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov fs, ax
mov sp, 0x7c00 ; 设置栈顶指针


; 打印booting
mov si, booting
call print

mov edi, 0x1000; 读取loader，存放的目标内存起始地址
; 起始扇区
mov ecx, 2
; 扇区数量
mov bl, 4
call read_disk

cmp word [0x1000], 0x55aa
jnz error

; 跳转到loader
jmp 0:0x1002

; 阻塞，原地跳转
jmp $

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


booting:
    ; 10:\n  13"\r  0:字符串结束
    db "Booting Conix...", 10, 13, 0

error:
    mov si, .msg
    call print
    hlt ; 让CPU停止
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0


; 初始化前510个字节
times 510 - ($ - $$) db 0

; 主引导扇区最后两个字节(511和512)标识
db 0x55, 0xaa