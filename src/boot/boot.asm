; boot���ص��ڴ��е���ʼλ��
[org 0x7c00]

; ������ĻģʽΪ�ı�ģʽ�������Ļ
mov ax, 3
int 0x10

; ��ʼ���μĴ���
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov fs, ax
mov sp, 0x7c00 ; ����ջ��ָ��


; ��ӡbooting
mov si, booting
call print

mov edi, 0x1000; ��ȡloader����ŵ�Ŀ���ڴ���ʼ��ַ
; ��ʼ����
mov ecx, 2
; ��������
mov bl, 4
call read_disk

cmp word [0x1000], 0x55aa
jnz error

; ��ת��loader
jmp 0:0x1002

; ������ԭ����ת
jmp $

read_disk:
    ; ���ö�д����������
    mov dx, 0x1f2 ; 0x1f2��д���������Ķ˿�
    mov al, bl
    out dx, al

    inc dx ; 0x1f3
    mov al, cl ; ��ʼ������ǰ��λ
    out dx, al

    inc dx ; 0x1f4
    shr ecx, 8
    mov al, cl ; ��ʼ�������а�λ
    out dx, al

    inc dx ; 0x1f5
    shr ecx, 8
    mov al, cl ; ��ʼ�����ĸ߰�λ
    out dx, al

    inc dx ; 0x1f6
    shr ecx, 8
    and cl, 0b1111 ; ������λ��0

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
    ; 10:\n  13"\r  0:�ַ�������
    db "Booting Conix...", 10, 13, 0

error:
    mov si, .msg
    call print
    hlt ; ��CPUֹͣ
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0


; ��ʼ��ǰ510���ֽ�
times 510 - ($ - $$) db 0

; ������������������ֽ�(511��512)��ʶ
db 0x55, 0xaa