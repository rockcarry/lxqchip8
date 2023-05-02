jmp start
; include "16bit.asm"

set_16bit_addr_r23:
    mov I,set_inc_blank
    mov vd,0xa0
    or v2,vd
    store v3
    jmp set_inc_r23
set_inc_blank:
    0x0000
set_inc_r23:
    0x0000
    ret

add_16bit_rBC:
    add v2,v4
    sne vf,0
    jmp NC_rBC
    add v3,1
NC_rBC:
    add v3,v5
    ret

add_16bit_r01:
    add v0,v2
    sne vf,0
    jmp NC_r01
    add v1,1
NC_r01:
    add v1,v3
    ret

shl_16bit:
    shl v0,vf
    sne vf,0
    jmp F0_shl
    shl v1,vf
    add v1,1
    ret
F0_shl:
    shl v1,vf
    ret

shl_16bit_B:
    shl v2,vf
    sne vf,0
    jmp F0_shl_B
    shl v3,vf
    add v3,1
    ret
F0_shl_B:
    shl v3,vf
    ret

; #include "stack.asm"
stack_init:
    mov ve,0
    ret

protect_regs:
    mov I,0
    add I,ve
    store vd
    add ve,16
    ret

recover_regs:
    add ve,-16
    mov I,0
    add I,ve
    load vd
    ret

;#mainfile
;head_xy   :256 (x:256,y:257)
;q_head    :258
;q_tail    :260
;food_xy   :262 (x:262,y:263)
;eat_food  :264
;error_key :266
;dead      :268
;snake_q   :0x400

add_put: ; ²ÎÊý£ºq_point:x:v0,y:v1
    call protect_regs

    mov v4,v0
    mov v5,v1 ;mov C,A

    mov v6,v0
    mov v7,v1 ;mov D,A

    call shl_16bit ;shl A
    mov v2,0
    mov v3,0x4 ;mov B,0x400
    call add_16bit_r01 ;add A,B

    mov v2,v1
    mov v3,v0
    call set_16bit_addr_r23 ;mov I,A
    load v1 ;mov A,[A]

    mov I,dt
    draw v0,v1,1 ;print a dot at {x: v0, Y: v1} in screen

    mov v0,v4
    mov v1,v5 ;mov A,C

    mov v0,v6
    mov v1,v7 ;mov A,D

    mov v2,1
    mov v3,0
    call add_16bit_r01 ;add A,1

    mov v1,4
    mov vc,3
    and v1,vc ;add A,0x3ff

    mov I,add_put_ret
    store v1 ;mov [ret],A

    call recover_regs
    ret

dt: 0x8000
add_put_ret: 0x0000

key_press:
    call protect_regs

    mov I,256
    load v1 ;mov A,[head_xy]
    gkey vd ;getkey

key_8:
    seq vd,8
    jmp key_5
    add v1,-1
    jmp key_press_die

key_5:
    seq vd,5
    jmp key_4
    add v1,1
    jmp key_press_die

key_4:
    seq vd,4
    jmp key_6
    add v0,-1
    jmp key_press_die

key_6:
    seq vd,6
    jmp key_ndef
    add v0,1

key_press_die:
    sne v0,0xff
    jmp _die
    sne v1,0xff
    jmp _die
    sne v0,32
    jmp _die
    sne v1,32
    jmp _die

    mov I,256
    store v1 ;mov [head_xy],A

    mov v2,v0
    mov v3,v1 ;mov B,A

    mov I,262
    load v1 ;mov A,[food_xy]
key_press_eat:
    seq v0,v2
    jmp key_press_end
    seq v1,v3
    jmp key_press_end
    jmp eat_food

key_press_end:
    call recover_regs
    ret

eat_food:
    mov I,262
    load v1
    mov I,dt
    draw v0,v1,1

    call set_food
    mov v0,1
    mov I,264
    store v0 ;mov [eat_food],1
    jmp key_press_end
key_ndef:
    call protect_regs
    mov I,266
    mov v0,1
    mov v1,0
    store v1 ;mov [error_key],word ptr 1
    call recover_regs
    jmp key_press_die
_die:
    mov v0,20
    beep v0
    gkey vd

    mov I,268
    mov v0,1
    mov v1,0
    store v1 ;mov [dead],word ptr 1

    clr

    ret
stop:
    jmp stop

set_food:
     rand v0,31
     rand v1,31

     mov I,dt
     draw v0,v1,1 ;print a dot

     mov I,262
     store v1 ;mov [food_xy],A
     ret

print_wall:
    mov v0,32
    mov v1,0
    mov I,dt
print_loop:
    draw v0,v1,1
    add v1,1
    seq v1,32
    jmp print_loop
    ret

init:
    mov I,0x400
    mov v0,16
    mov v1,16
    store v1 ;mov [snake_q + 0],{16, 16}

    mov I,258
    mov v0,1
    mov v1,0
    store v1 ;q_head = 1

    mov I,260
    mov v0,0
    mov v1,0
    store v1 ;q_tail = 0

    mov I,256
    mov v0,16
    mov v1,16
    store v1 ;mov head_xy,{16, 16}

    mov I,dt
    draw v0,v1,1 ;print snake_head

    call set_food

    call stack_init

    call print_wall

    ret

put_newhead:
    call protect_regs

    mov I,256
    load v3 ;mov A,[head_xy] mov B,[q_head]

    mov v4,0
    mov v5,0x4
    call shl_16bit_B
    call add_16bit_rBC
    mov vc,v2
    mov v2,v3
    mov v3,vc
    call set_16bit_addr_r23
    store v1 ;mov snake_q[q_head],A

    call recover_regs
    ret

delay:
    call protect_regs

    mov v0,10
    stm v0 ;set 100ms's delay

delay_loop:
    gtm v0
    seq v0,0
    jmp delay_loop

    call recover_regs
    ret

loop:
    call delay

    call key_press
    mov I,268
    load v1 ;mov A,[dead]
    sne v0,0
    jmp err_key

been_dead:
    call init

    mov I,268
    mov v0,0
    mov v1,0
    store v1 ;mov [dead],word ptr 0

    jmp loop

err_key:
    mov I,266
    load v1 ;mov A,[error_key]
    sne v0,0
    jmp loop_next

continue:
    mov v0,0
    mov v1,0
    store v0 ;mov [error_key],word ptr 0
    jmp loop

loop_next:
    call put_newhead

    mov I,258
    load v1 ;mov A,[q_head]
    call add_put
    mov I,add_put_ret
    load v1 ;mov A,[add_put_ret]
    mov I,258
    store v1 ;q_head = put_add(q_head);

    mov I,264
    load v1 ;mov A,[eat_food]
    seq v0,0 ;if(eat_food == 0)
    jmp loop_eat_food

    mov I,260
    load v1 ;mov A,[q_tail]
    call add_put
    mov I,add_put_ret
    load v1 ;mov A,[add_put_ret]
    mov I,260
    store v1 ;q_tail = put_add(q_tail);

    jmp loop
;then {
;    q_tail = put_add(q_tail);
;    eat_food = 0;
;}

loop_eat_food:
    mov I,264
    mov v0,0
    mov v1,0
    store v1 ;mov [eat_food],word ptr 0
    jmp loop

start:
    call init

    jmp loop
st: jmp st
