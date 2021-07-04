#include <stdlib.h>
#include <stdio.h>
#include "stdafx.h"
#include "chip8vm.h"

#ifdef WIN32
typedef   signed int    int32_t;
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef   signed short  int16_t;
typedef unsigned char  uint8_t;
typedef   signed char   int8_t;
#define usleep(t) Sleep((t)/1000);
#else
#include <stdint.h>
#endif

#define CHIP8_TYPE_ORIGINAL 0
#define CHIP8_TYPE_CHIP48   1
#define CHIP8_TYPE_SCHIP8   2
#define CONFIG_CHIP8_TYPE   CHIP8_TYPE_SCHIP8

int log_printf(char *format, ...)
{
    char buf[1024];
    int  retv;
    va_list valist;
    va_start(valist, format);
    retv = vsnprintf(buf, sizeof(buf), format, valist);
    va_end(valist);
    OutputDebugStringA(buf);
    return retv;
}

const static uint8_t g_chip8_fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F

    // high-res sprites
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // "0"
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // "1"
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // "2"
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // "3"
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // "4"
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // "5"
    0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // "6"
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // "7"
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // "8"
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // "9"
};

typedef struct {
    uint8_t  mem[4096];
    uint8_t  v[16];
    uint16_t stack[128];
    uint8_t  delay_timer;
    uint8_t  sound_timer;
    uint16_t pc;
    uint8_t  sp;
    uint16_t i;
    uint16_t key;
    #define FLAG_EXIT   (1 << 0)
    #define FLAG_RENDER (1 << 1)
    #define FLAG_HIRES  (1 << 2)
    uint8_t  flags;
    uint8_t  schip8_flag_regs[8];
    uint8_t  schip8_vram[1024];
} CHIP8;

#define VX   (chip8->v[(opcode0 >> 0) & 0xF])
#define VY   (chip8->v[(opcode1 >> 4) & 0xF])
#define V0   (chip8->v[0x0])
#define VF   (chip8->v[0xF])
#define X    (opcode0 & 0xF)
#define N    (opcode1 & 0xF)
#define NN   (opcode1)
#define NNN  (((opcode0 & 0xF) << 8) | opcode1)
#define VRAM (chip8->schip8_vram)
#define UNKNOWN_OPCODE(op0, op1) do { log_printf("unknown opcode: %02X%02X\n", op0, op1); } while (0)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void* chip8vm_init(char *rom)
{
    int    len   = 0;
    FILE  *fp    = NULL;
    CHIP8 *chip8 = (CHIP8*)calloc(1, sizeof(CHIP8));
    if (!chip8) return NULL;
    fp  = fopen(rom, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(chip8->mem + 0x200, 1, MIN(sizeof(chip8->mem) - 0x200, len), fp);
        fclose(fp);
    }
    chip8->pc = 0x200;
    memcpy(chip8->mem, g_chip8_fontset, sizeof(g_chip8_fontset));
    return chip8;
}

void chip8vm_exit(void *vm)
{
    if (vm) free(vm);
}

static void chip8vm_scroll_screen(CHIP8 *chip8, int scrollx, int scrolly)
{
    int sw = (chip8->flags & FLAG_HIRES) ? CHIP8VM_SCHIP8_WIDTH : CHIP8VM_CHIP8_WIDTH;
    int sh = (chip8->flags & FLAG_HIRES) ? CHIP8VM_SCHIP8_HEIGHT: CHIP8VM_CHIP8_HEIGHT;
    int stride = sw / 8, i, j;
    if (scrolly > 0) {
        for (i=sh-1; i>=scrolly; i--) {
            memcpy(chip8->schip8_vram + i * stride, chip8->schip8_vram + (i - scrolly) * stride, stride);
        }
        for (; i>=0; i--) {
            memset(chip8->schip8_vram + i * stride, 0, stride);
        }
    }
    if (scrolly < 0) {
        for (i=0; i<sh-scrolly; i++) {
            memcpy(chip8->schip8_vram + i * stride, chip8->schip8_vram + (i - scrolly) * stride, stride);
        }
        for (; i<sh; i++) {
            memset(chip8->schip8_vram + i * stride, 0, stride);
        }
    }
    if (scrollx > 0) {
        uint8_t *p = chip8->schip8_vram + stride * sh - 1;
        for (i=0; i<sh; i++) {
            for (j=0; j<stride-1; j++) {
                p[0] = (p[0] >> 4) | (p[-1] << 4);
                *p--;
            }
            *p-- >>= 4;
        }
    }
    if (scrollx < 0) {
        uint8_t *p = chip8->schip8_vram;
        for (i=0; i<sh; i++) {
            for (j=0; j<stride-1; j++) {
                p[0] = (p[0] << 4) | (p[1] >> 4);
                *p++;
            }
            *p++ <<= 4;
        }
    }
}

void chip8vm_run(void *vm, int vsync)
{
    CHIP8   *chip8  = (CHIP8*)vm;
    uint8_t  opcode0= chip8->mem[(chip8->pc + 0) & 0xFFF];
    uint8_t  opcode1= chip8->mem[(chip8->pc + 1) & 0xFFF];
    int      tmp, i, n;
    switch (opcode0 >> 4) {
    case 0x0:
        switch (NNN) {
        case 0x0EE: chip8->pc = chip8->stack[--chip8->sp]; goto _done;
        case 0x0E0: memset(VRAM, 0, sizeof(VRAM)); chip8->flags |= FLAG_RENDER; break;
        case 0x0FF: chip8->flags |= FLAG_HIRES|FLAG_RENDER; break;
        case 0x0FE: chip8->flags &=~FLAG_HIRES|FLAG_RENDER; break;
        case 0x0FD: chip8->flags |= FLAG_EXIT; break;
        case 0x0FC: chip8vm_scroll_screen(chip8, -4, 0); chip8->flags |= FLAG_RENDER; break;
        case 0x0FB: chip8vm_scroll_screen(chip8,  4, 0); chip8->flags |= FLAG_RENDER; break;
        default:
            if ((opcode1 & 0xF0) == 0xB0) {
                chip8vm_scroll_screen(chip8, 0,-N);
                chip8->flags |= FLAG_RENDER;
            } else if ((opcode1 & 0xF0) == 0xC0) {
                chip8vm_scroll_screen(chip8, 0, N);
                chip8->flags |= FLAG_RENDER;
            } else {
                UNKNOWN_OPCODE(opcode0, opcode1);
            }
            break;
        }
        break;
    case 0x1: chip8->pc = NNN; goto _done;
    case 0x2: chip8->stack[chip8->sp++] = chip8->pc + 2; chip8->pc = NNN; goto _done;
    case 0x3: chip8->pc += (VX == NN) ? 2 : 0; break;
    case 0x4: chip8->pc += (VX != NN) ? 2 : 0; break;
    case 0x5: chip8->pc += (VX == VY) ? 2 : 0; break;
    case 0x6: VX  = NN; break;
    case 0x7: VX += NN; break;
    case 0x8:
        switch (N) {
        case 0x0: VX  = VY; break;
        case 0x1: VX |= VY; break;
        case 0x2: VX &= VY; break;
        case 0x3: VX ^= VY; break;
        case 0x4: tmp = VX + VY; VX = (uint8_t)tmp; VF = tmp > 255; break;
        case 0x5: tmp = VX - VY; VX = (uint8_t)tmp; VF = tmp > 0  ; break;
        case 0x7: tmp = VY - VX; VX = (uint8_t)tmp; VF = tmp > 0  ; break;
#if (CONFIG_CHIP8_TYPE == CHIP8_TYPE_ORIGINAL)
        case 0x6: VF = (VX >> 0) & 1; VY = VX >> 1; break;
        case 0xE: VF = (VX >> 7) & 1; VY = VX << 1; break;
#else
        case 0x6: VF = (VX >> 0) & 1; VX >>= 1; break;
        case 0xE: VF = (VX >> 7) & 1; VX <<= 1; break;
#endif
        default : UNKNOWN_OPCODE(opcode0, opcode1); break;
        }
        break;
    case 0x9:
        if (N == 0) {
            chip8->pc+= (VX != VY) ? 2 : 0;
        } else {
            UNKNOWN_OPCODE(opcode0, opcode1);
        }
        break;
    case 0xA: chip8->i  = NNN; break;
    case 0xB: chip8->pc = NNN + V0; goto _done;
    case 0xC: VX = rand() & NN; break;
    case 0xD:
        n = N == 0 ? 16 : N;
        if (N == 0 && chip8->flags & FLAG_HIRES) { // draw 16xN sprite
            for (i=0; i<n; i++) {
                uint32_t line = ((chip8->mem[chip8->i + i * 2 + 0] << 16) | (chip8->mem[chip8->i + i * 2 + 1] << 8)) >> (VX & 0x7);
                uint8_t *byte = VRAM + 16 * ((VY + i) & 0x3F) + (VX & 0x7F) / 8;
                if (!VF) VF = !!(((byte[0] << 16) | (byte[1] << 8) | (byte[2] << 0)) & line);
                byte[0] ^= (line >> 16) & 0xFF;
                byte[1] ^= (line >> 8 ) & 0xFF;
                byte[2] ^= (line >> 0 ) & 0xFF;
            }
        } else { // draw 8xN sprite
            for (i=0,VF=0; i<n; i++) {
                uint32_t line = (chip8->mem[chip8->i + i] << 8) >> (VX & 0x7);
                uint8_t *byte = (chip8->flags & FLAG_HIRES) ? VRAM + 16 * ((VY + i) & 0x3F) + (VX & 0x7F) / 8 : VRAM + 8 * ((VY + i) & 0x1F) + (VX & 0x3F) / 8;
                if (!VF) VF = !!(((byte[0] << 8) | (byte[1] << 0)) & line);
                byte[0] ^= (line >> 8) & 0xFF;
                byte[1] ^= (line >> 0) & 0xFF;
            }
        }
        chip8->flags |= FLAG_RENDER;
        break;
    case 0xE:
        if (NN == 0x9E) {
            chip8->pc += (chip8->key & (1 << VX)) ? 2 : 0;
        } else if (NN == 0xA1) {
            chip8->pc += (chip8->key & (1 << VX)) ? 0 : 2;
        } else {
            UNKNOWN_OPCODE(opcode0, opcode1);
        }
        break;
    case 0xF:
        switch (NN) {
        case 0x07: VX = chip8->delay_timer; break;
        case 0x0A:
            while (!(chip8->flags & FLAG_EXIT)) {
                for (i=0; i<16 && (chip8->key & (1<<i))==0; i++);
                if (i != 16) { VX = i; break; }
                usleep(10*1000);
            }
            break;
        case 0x15: chip8->delay_timer = VX; break;
        case 0x18: chip8->sound_timer = VX; break;
        case 0x1E: VF = chip8->i + VX > 0xFFF; chip8->i += VX; break;
        case 0x29: chip8->i = VX * 5; break;
        case 0x30: chip8->i = 0xA0 + VX * 10; break;
        case 0x33:
            chip8->mem[chip8->i + 0] = (VX % 1000) / 100;
            chip8->mem[chip8->i + 1] = (VX % 100 ) / 10;
            chip8->mem[chip8->i + 2] = (VX % 10  ) / 1;
            break;
#if (CONFIG_CHIP8_TYPE == CHIP8_TYPE_SCHIP8)
        case 0x55: for (i=0; i<=X; i++) chip8->mem[chip8->i + i] = chip8->v[i]; break;
        case 0x65: for (i=0; i<=X; i++) chip8->v[i] = chip8->mem[chip8->i + i]; break;
#else
        case 0x55: for (i=0; i<=X; i++) chip8->mem[chip8->i + i] = chip8->v[i]; chip8->i += X + 1; break;
        case 0x65: for (i=0; i<=X; i++) chip8->v[i] = chip8->mem[chip8->i + i]; chip8->i += X + 1; break;
#endif
        case 0x75: if (X <= 7) memcpy(chip8->schip8_flag_regs, chip8->v, X + 1); break;
        case 0x85: if (X <= 7) memcpy(chip8->v, chip8->schip8_flag_regs, X + 1); break;
        default  : UNKNOWN_OPCODE(opcode0, opcode1); break;
        }
        break;
    }
    chip8->pc += 2;
_done:
    if (vsync)  {
        if (chip8->delay_timer > 0) chip8->delay_timer--;
        if (chip8->sound_timer > 0) chip8->sound_timer--;
    }
}

void chip8vm_stop(void *vm)
{
    CHIP8 *chip8 = (CHIP8*)vm;
    if (chip8) chip8->flags |= FLAG_EXIT;
}

void chip8vm_key(void *vm, int key)
{
    CHIP8 *chip8 = (CHIP8*)vm;
    if (chip8) chip8->key = key;
}

static void pixel(uint32_t *buf, int x, int y, int c)
{
    int gx = x * 5, gy = y * 5, i, j;
    uint32_t *p = buf + (y * CHIP8VM_RENDER_DOTSIZE1) * CHIP8VM_RENDER_WIDTH + (x * CHIP8VM_RENDER_DOTSIZE1);
    for (i=0; i<CHIP8VM_RENDER_DOTSIZE2; i++) {
        for (j=0; j<CHIP8VM_RENDER_DOTSIZE2; j++) {
            *p++ = c ? CHIP8VM_RENDER_DOTCOLOR : 1;
        }
        p -= CHIP8VM_RENDER_DOTSIZE2;
        p += CHIP8VM_RENDER_WIDTH;
    }
}

int chip8vm_render(void *vm, void *buf)
{
    int x = 0, y = 0, w, h, i, j;
    CHIP8   *chip8 = (CHIP8*)vm;
    uint8_t *vram  = VRAM;

    if (!chip8 || !(chip8->flags & FLAG_RENDER)) return 0;
    chip8->flags &= ~FLAG_RENDER;

    w = (chip8->flags & FLAG_HIRES) ? CHIP8VM_SCHIP8_WIDTH  : CHIP8VM_CHIP8_WIDTH ;
    h = (chip8->flags & FLAG_HIRES) ? CHIP8VM_SCHIP8_HEIGHT : CHIP8VM_CHIP8_HEIGHT;
    for (i=0; i<w*h/8; i++) {
        for (j=7; j>=0; j--) {
            pixel((uint32_t*)buf, x++, y, *vram & (1 <<j));
        }
        vram++;
        if (chip8->flags & FLAG_HIRES) {
            if ((i & 0xF) == 0xF) { x = 0; y++; }
        } else {
            if ((i & 0x7) == 0x7) { x = 0; y++; }
        }
    }
    return 1;
}

void chip8vm_getparam(void *vm, int id, void *param)
{
    CHIP8   *chip8 = (CHIP8*)vm;
    uint8_t *vram  = VRAM;
    if (!chip8) return;
    switch (id) {
    case CHIP8VM_PARAM_SOUND_TIMER:
        *(uint32_t*)param = chip8->sound_timer;
        break;
    case CHIP8VM_PARAM_CHIP8_LRES:
        *(uint32_t*)param = !(chip8->flags & FLAG_HIRES);
        break;
    }
}


