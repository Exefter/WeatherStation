#ifndef IR_H
#define IR_H

#include <stdint.h>

#define IR_CODE_1      (0x00FF6897UL)
#define IR_CODE_2      (0x00FF9867UL)
#define IR_CODE_OK     (0x00FF02FDUL)
#define IR_CODE_LEFT   (0x00FF22DDUL)
#define IR_CODE_RIGHT  (0x00FFC23DUL)

void irInit(void);
uint8_t irHasCode(void);
uint32_t irGetCode(void);

#endif