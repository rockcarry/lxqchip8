#ifndef __CHIP8VM_H__
#define __CHIP8VM_H__

#ifdef __cplusplus
extern "C" {
#endif

void* chip8vm_init  (char *rom);
void  chip8vm_exit  (void *vm );
void  chip8vm_run   (void *vm );
void  chip8vm_stop  (void *vm );
void  chip8vm_key   (void *vm, int key);
void  chip8vm_render(void *vm, void *buf);

#ifdef __cplusplus
}
#endif

#endif

