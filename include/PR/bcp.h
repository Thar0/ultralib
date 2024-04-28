#ifndef _BCP_H_
#define _BCP_H_
#ifdef BBPLAYER

#include "rcp.h"

/**
 * MIPS Interface (MI) Additional Registers
 */

//! MI_SK_EXCEPTION_REG ?
#define MI_14_REG (MI_BASE_REG + 0x14)



//! ?
#define MI_30_REG (MI_BASE_REG + 0x30)



//! ?
#define MI_38_REG (MI_BASE_REG + 0x38)



//! MI_HW_INTR_MASK_REG ?
#define MI_3C_REG (MI_BASE_REG + 0x3C)



/**
 * Peripheral Interface (PI) Additional Registers 
 */

//! PI_ATB_UPPER_REG ?
#define PI_40_REG (PI_BASE_REG + 0x40)



//! ?
#define PI_44_REG (PI_BASE_REG + 0x44)



//! PI_CARD_CNT_REG ?
#define PI_48_REG (PI_BASE_REG + 0x48)



//! ?
#define PI_4C_REG (PI_BASE_REG + 0x4C)



//! PI_AES_CNT_REG ?
#define PI_50_REG (PI_BASE_REG + 0x50)



//! PI_ALLOWED_IO_REG ?
#define PI_54_REG (PI_BASE_REG + 0x54)



//! ?
#define PI_58_REG (PI_BASE_REG + 0x58)



//! ?
#define PI_5C_REG (PI_BASE_REG + 0x5C)

/**
 * [31:16] Box ID
 *   [31:30] ?? (osInitialize checks this and sets __osBbIsBb to 2 if != 0)
 *   [29:27] ?? (unused so far)
 *   [26:25] ?? (system clock speed identifier?)
 *   [24:22] ?? (bootrom, checked against MI_10_REG and copied there if mismatch)
 * [ 7: 6] RTC mask
 *     [5] Error LED mask
 *     [4] Power Control mask
 * [ 3: 2] RTC control
 *     [1] Error LED (1=on, 0=off)
 *     [0] Power Control (1=on, 0=off)
 */
#define PI_GPIO_REG (PI_BASE_REG + 0x60)

#define PI_GPIO_GET_BOXID(reg)  ((reg) >> 16)
#define PI_GPIO_GET_PWR(reg)    (((reg) >> 0) & 1)
#define PI_GPIO_GET_LED(reg)    (((reg) >> 1) & 1)

/* Box ID masks */
#define PI_GPIO_BOXID_MASK_30_31 (3 << 30)

/* Enable masks */
#define PI_GPIO_MASK_PWR    (1 << 4)
#define PI_GPIO_MASK_LED    (2 << 4)
#define PI_GPIO_MASK_RTC_0  (4 << 4)
#define PI_GPIO_MASK_RTC_1  (8 << 4)
/* RTC (TODO) */
/* LED */
#define PI_GPIO_LED_ON      (1 << 1)
#define PI_GPIO_LED_OFF     (0 << 1)
/* Power */
#define PI_GPIO_PWR_ON      (1 << 0)
#define PI_GPIO_PWR_OFF     (0 << 0)



//! ?
#define PI_64_REG (PI_BASE_REG + 0x64)



//! PI_CARD_BLK_OFFSET_REG ?
#define PI_70_REG (PI_BASE_REG + 0x70)



//! PI_EX_DMA_BUF ?
#define PI_10000_REG(i) (PI_BASE_REG + 0x10000 + (i))



//! PI_ATB_LOWER_REG ?
#define PI_10500_REG(i) (PI_BASE_REG + 0x10500 + (i) * 4)



/**
 * Serial Interface (SI) Additional Registers
 */

//! ?
#define SI_0C_REG (SI_BASE_REG + 0x0C)



//! ?
#define SI_1C_REG (SI_BASE_REG + 0x1C)



#endif
#endif
