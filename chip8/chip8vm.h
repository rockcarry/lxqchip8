#ifndef __CHIP8VM_H__
#define __CHIP8VM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP8VM_RENDER_WIDTH   (64 * 5)
#define CHIP8VM_RENDER_HEIGHT  (32 * 5)

void* chip8vm_init  (char *rom);
void  chip8vm_exit  (void *vm );
void  chip8vm_run   (void *vm, int vsync);
void  chip8vm_stop  (void *vm );
void  chip8vm_key   (void *vm, int key);
int   chip8vm_render(void *vm, void *buf);

enum {
    CHIP8VM_PARAM_SOUND_TIMER = 0,
};
void  chip8vm_getparam(void *vm, int id, void *param);

#ifdef __cplusplus
}
#endif

#endif

