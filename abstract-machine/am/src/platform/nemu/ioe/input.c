#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t temp = inl(KBD_ADDR);
  kbd->keydown = temp & KEYDOWN_MASK ? true : false;
  kbd->keycode = temp & ~KEYDOWN_MASK;
}
