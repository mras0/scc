        org 0x100

        mov ax, 123
        cmp ax, 32
        jz ERR
        jng ERR
        jl ERR

        mov ax, -123
        cmp ax, 32
        jz ERR
        jg ERR
        jnl ERR

        mov ax, 42
        cmp ax, 42
        jnz ERR
        jg ERR
        jl ERR

        mov ax, 0x8000
        cmp ax, 0x7fff
        jz ERR
        jg ERR
        jnl ERR

        mov ax, 0x7fff
        cmp ax, 0x8000
        jz ERR
        jng ERR
        jl ERR

        mov ax, 0x1111
        cbw
        cmp ax, 0x0011
        jne ERR

        mov ax, 0x1180
        cbw
        cmp ax, 0xFF80
        jne ERR

        mov ax, 0xFFC1
        push ax
        cbw
        pop cx
        cmp ax, cx
        jne ERR
        jg ERR
        jl ERR

        mov ax, 0x0020
        cmp ax, 0x0020
        jnz ERR
        jg ERR
        jl ERR
        jng L1
        jmp short ERR
L1:
        jnl L2
        jmp short ERR
L2:

        mov ax, -42
        xor ah, ah
        jnz ERR
        cmp ax, 214
        jne ERR
        mov ah, 2
        cmp ax, 0x2D6

        mov cx, 0x1234
        sub ch, 0x14
        jz ERR
        cmp cx, 0xFE34
        jne ERR

        mov cx, 0xABCD
        shr cx, 1
        jnc ERR
        cmp cx, 0x55E6
        jne ERR

        mov ax, 0x4c00
        int 0x21
ERR:
        mov ax, 0x4c01
        int 0x21
