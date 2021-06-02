; test program

    jmp _main

_dotdata:
    0x8000

_main:
    mov  v0, 32
    mov  v1, 16
    mov  i , _dotdata

_loop1:
    draw v0, v1, 1
    mov  v3, 8
    sku  v3
    add  v1,-1
    mov  v3, 5
    sku  v3
    add  v1, 1
    mov  v3, 4
    sku  v3
    add  v0,-1
    mov  v3, 6
    sku  v3
    add  v0, 1
    jmp  _loop1

