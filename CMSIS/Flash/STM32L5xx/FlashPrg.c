/* -----------------------------------------------------------------------------
 * Copyright (c) 2019 - 2020 ARM Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        27. February 2020
 * $Revision:    V1.2.0
 *
 * Project:      Flash Programming Functions for ST STM32L5xx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.2.0
 *    Algorithm rework.
 *    Added OPT Algorithms
 *  Version 1.1.0
 *   we use one FLM for both config D/S Bank
 *   Flash has 2K sector size.
 *  Version 1.0.0
 *    Initial release
 */

#include "..\FlashOS.h"        /* FlashOS Structures */

typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;

#define M32(adr) (*((vu32 *) (adr)))

// Peripheral Memory Map
#define FLASH_BASE       (0x40022000)
#define DBGMCU_BASE      (0xE0044000)
#define FLASHSIZE_BASE   (0x0BFA05E0)

#define FLASH           ((FLASH_TypeDef  *) FLASH_BASE)
#define DBGMCU          ((DBGMCU_TypeDef *) DBGMCU_BASE)

// Debug MCU
typedef struct {
  vu32 IDCODE;
} DBGMCU_TypeDef;

// Flash Registers
typedef struct
{
  vu32 ACR;              /* Offset: 0x00  Flash access control register */
  vu32 PDKEYR;           /* Offset: 0x04  Flash power-down key register */
  vu32 NSKEYR;           /* Offset: 0x08  Flash non-secure key register */
  vu32 SECKEYR;          /* Offset: 0x0C  Flash secure key register */
  vu32 OPTKEYR;          /* Offset: 0x10  Flash option key register */
  vu32 LVEKEYR;          /* Offset: 0x14  Flash low voltage key register */
  vu32 RESERVED0[2];
  vu32 NSSR;             /* Offset: 0x20  Flash non-secure status register */
  vu32 SECSR;            /* Offset: 0x24  Flash secure status register */
  vu32 NSCR;             /* Offset: 0x28  Flash non-secure control register */
  vu32 SECCR;            /* Offset: 0x2C  Flash secure control register */
  vu32 ECCR;             /* Offset: 0x30  Flash ECC register */
  vu32 RESERVED1[3];
  vu32 OPTR;             /* Offset: 0x40  Flash option register */
  vu32 NSBOOTADD0R;      /* Offset: 0x44  Flash non-secure boot address 0 register */
  vu32 NSBOOTADD1R;      /* Offset: 0x48  Flash non-secure boot address 1 register */
  vu32 SECBOOTADD0R;     /* Offset: 0x4C  Flash secure boot address 0 register */
  vu32 SECWM1R1;         /* Offset: 0x50  Flash bank 1 secure watermak1 register */
  vu32 SECWM1R2;         /* Offset: 0x54  Flash bank 1 secure watermak2 register */
  vu32 WRP1AR;           /* Offset: 0x58  Flash WPR1 area A address register */
  vu32 WRP1BR;           /* Offset: 0x5C  Flash WPR1 area B address register */
  vu32 SECWM2R1;         /* Offset: 0x60  Flash secure watermak2 register */
  vu32 SECWM2R2;         /* Offset: 0x64  Flash secure watermak2 register 2 */
  vu32 WRP2AR;           /* Offset: 0x68  Flash WPR2 area A address register */
  vu32 WRP2BR;           /* Offset: 0x6C  Flash WPR2 area B address register */
  vu32 RESERVED2[4];
  vu32 SECBBA0;          /* Offset: 0x80  FLASH secure block based bank 1 register 0 */
  vu32 SECBBA1;          /* Offset: 0x84  FLASH secure block based bank 1 register 1 */
  vu32 SECBBA2;          /* Offset: 0x88  FLASH secure block based bank 1 register 2 */
  vu32 SECBBA3;          /* Offset: 0x8C  FLASH secure block based bank 1 register 3 */
  vu32 RESERVED3[4];
  vu32 SECBBB0;          /* Offset: 0xA0  FLASH secure block based bank 2 register 0 */
  vu32 SECBBB1;          /* Offset: 0xA4  FLASH secure block based bank 2 register 1 */
  vu32 SECBBB2;          /* Offset: 0xA8  FLASH secure block based bank 2 register 2 */
  vu32 SECBBB3;          /* Offset: 0xAC  FLASH secure block based bank 2 register 3 */
  vu32 RESERVED4[4];
  vu32 SECHDPCR;         /* Offset: 0xC0  FLASH secure HDP control register */
  vu32 PRIVCFGR;         /* Offset: 0xC4  FLASH privilege configuration register */
} FLASH_TypeDef;


// Flash Keys
#define RDPRT_KEY                0x00A5
#define FLASH_KEY1               0x45670123
#define FLASH_KEY2               0xCDEF89AB
#define FLASH_OPTKEY1            0x08192A3B
#define FLASH_OPTKEY2            0x4C5D6E7F

// Flash Control Register definitions
#define FLASH_CR_PG             ((u32)(  1U      ))
#define FLASH_CR_PER            ((u32)(  1U <<  1))
#define FLASH_CR_MER1           ((u32)(  1U <<  2))
#define FLASH_CR_PNB_MSK        ((u32)(0xFF <<  3))
#define FLASH_CR_BKER           ((u32)(  1U << 11))
#define FLASH_CR_MER2           ((u32)(  1U << 15))
#define FLASH_CR_STRT           ((u32)(  1U << 16))
#define FLASH_CR_OPTSTRT        ((u32)(  1U << 17))
#define FLASH_CR_OBL_LAUNCH     ((u32)(  1U << 27))
#define FLASH_CR_OPTLOCK        ((u32)(  1U << 30))
#define FLASH_CR_LOCK           ((u32)(  1U << 31))


// Flash Status Register definitions
#define FLASH_SR_EOP            ((u32)(  1U      ))
#define FLASH_SR_OPERR          ((u32)(  1U <<  1))
#define FLASH_SR_PROGERR        ((u32)(  1U <<  3))
#define FLASH_SR_WRPERR         ((u32)(  1U <<  4))
#define FLASH_SR_PGAERR         ((u32)(  1U <<  5))
#define FLASH_SR_SIZERR         ((u32)(  1U <<  6))
#define FLASH_SR_PGSERR         ((u32)(  1U <<  7))
#define FLASH_SR_OPTWERR        ((u32)(  1U << 13))
#define FLASH_SR_BSY            ((u32)(  1U << 16))


// Flash option register definitions
#define FLASH_OPTR_RDP          ((u32)(0xFF      ))
#define FLASH_OPTR_RDP_NO       ((u32)(0xAA      ))
#define FLASH_OPTR_DBANK        ((u32)(  1U << 22))
#define FLASH_OPTR_TZEN         ((u32)(  1U << 31))



#define FLASH_PGERR             (FLASH_SR_OPERR  | FLASH_SR_PROGERR | FLASH_SR_WRPERR  | \
                                 FLASH_SR_PGAERR | FLASH_SR_SIZERR  | FLASH_SR_PGSERR  | FLASH_SR_OPTWERR)

#if defined FLASH_MEM
static u32 gFlashBase;                  /* Flash base address */
static u32 gFlashSize;                  /* Flash size in bytes */

static vu32 *pFlashCR;                  /* Pointer to Flash Control register */
static vu32 *pFlashSR;                  /* Pointer to Flash Status register */
#endif /* FLASH_MEM */

static void DSB(void) {
    __asm("DSB");
}


/*
 * Get Flash security Mode
 *    Return Value:   0 = non-secure Flash
 *                    1 = secure Flash
 */

static u32 GetFlashSecureMode (void) {
  u32 flashSecureMode;

  flashSecureMode = (FLASH->OPTR & FLASH_OPTR_TZEN) ? 1U : 0U;

  return (flashSecureMode);
}


/*
 * Get Flash Type
 *    Return Value:   0 = Single-Bank flash
 *                    1 = Dual-Bank Flash (configurable)
 */

#if defined FLASH_MEM
static u32 GetFlashType (void) {
//  u32 flashType = 0U;
//
//  switch ((DBGMCU->IDCODE & 0xFFFU))
//  {
//    case 0x472:             /* Flash Category 3 devices, 2k or 4k sectors (STM32L552xx, STM32L562xx) */
//    default:                /* devices have a dual bank flash, configurable via FLASH_OPTR.DBANK */
//      flashType = 1U;       /* Dual-Bank Flash type */
//    break;
//  }
//
//  return (flashType);
  return (1U);              /* always Dual-Bank Flash */
}
#endif /* FLASH_MEM */


/*
 * Get Flash Bank Mode
 *    Return Value:   0 = Single-Bank mode
 *                    1 = Dual-Bank mode
 */

#if defined FLASH_MEM
static u32 GetFlashBankMode (void) {
  u32 flashBankMode;

  flashBankMode = (FLASH->OPTR & FLASH_OPTR_DBANK) ? 1U : 0U;

  return (flashBankMode);
}
#endif /* FLASH_MEM */


/*
 * Get Flash Bank Number
 *    Parameter:      adr:  Sector Address
 *    Return Value:   Bank Number (0..1)
 *                    Flash bank size is always the half of the Flash size
 */

#if defined FLASH_MEM
static u32 GetFlashBankNum(u32 adr) {
  u32 flashBankNum;

  if (GetFlashType() == 1U)
  {
    /* Dual-Bank Flash */
    if (GetFlashBankMode() == 1U)
    {
      /* Dual-Bank Flash configured as Dual-Bank */
      if (adr >= (gFlashBase + (gFlashSize >> 1)))
      {
        flashBankNum = 1U;
      }
      else
      {
        flashBankNum = 0U;
      }
    }
    else
    {
      /* Dual-Bank Flash configured as Single-Bank */
      flashBankNum = 0U;
    }
  }
  else
  {
    /* Single-Bank Flash */
    flashBankNum = 0U;
  }

  return (flashBankNum);
}
#endif /* FLASH_MEM */


/*
 * Get Flash Page Number
 *    Parameter:      adr:  Page Address
 *    Return Value:   Page Number (0..127)
 */

#if defined FLASH_MEM
static u32 GetFlashPageNum (unsigned long adr) {
  u32 flashPageNum;

  if (GetFlashType() == 1U)
  {
    /* Dual-Bank Flash */
    if (GetFlashBankMode() == 1U)
    {
      /* Dual-Bank Flash configured as Dual-Bank */
      flashPageNum = (((adr & ((gFlashSize >> 1) - 1U)) ) >> 11); /* 2K sector size */
    }
    else
    {
      /* Dual-Bank Flash configured as Single-Bank */
      flashPageNum = (((adr & (gFlashSize        - 1U)) ) >> 12); /* 4K sector size */
    }
  }
  else
  {
    /* Single-Bank Flash */
        flashPageNum = (((adr & (gFlashSize      - 1U)) ) >> 11); /* 2K sector size */
  }

  return (flashPageNum);
}
#endif /* FLASH_MEM */


/*
 * Get Flash Page Size
 *    Return Value:   flash page size (in Bytes)
 */

#if 0 //defined FLASH_MEM
static u32 GetFlashPageSize (void)
{
  u32 flashPageSize;

  if (GetFlashType() == 1U)
  {
    /* Dual-Bank Flash */
    if (GetFlashBankMode() == 1U)
    {
      /* Dual-Bank Flash configured as Dual-Bank */
      flashPageSize = 0x0800;                            /* 2K sector size */
    }
    else
    {
      /* Dual-Bank Flash configured as Single-Bank */
      flashPageSize = 0x1000;                            /* 4K sector size */
    }
  }
  else
  {
    /* Single-Bank Flash */
      flashPageSize = 0x0800;                            /* 2K sector size */
  }

  return (flashPageSize);
}
#endif /* FLASH_MEM */


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int Init (unsigned long adr, unsigned long clk, unsigned long fnc)
{
  (void)clk;
  (void)fnc;

#if defined FLASH_MEM
  if (GetFlashSecureMode() == 0U)
  {                                                      /* Flash non-secure */
    /* set used Control, Status register */
    pFlashCR = &FLASH->NSCR;
    pFlashSR = &FLASH->NSSR;

    /* unlock FLASH_NSCR */
    FLASH->NSKEYR = FLASH_KEY1;
    FLASH->NSKEYR = FLASH_KEY2;
    DSB();
    while (FLASH->NSSR & FLASH_SR_BSY);                  /* Wait until operation is finished */
  }
  else
  {                                                      /* Flash secure */
    /* set used Control, Status register */
    pFlashCR = &FLASH->SECCR;
    pFlashSR = &FLASH->SECSR;

    /* unlock FLASH_SECCR */
    FLASH->SECKEYR = FLASH_KEY1;
    FLASH->SECKEYR = FLASH_KEY2;
    DSB();
    while (FLASH->SECSR & FLASH_SR_BSY);                 /* Wait until operation is finished */

    /* Flash block-based secure bank1 */
    FLASH->SECBBA0 = 0xFFFFFFFF;
    FLASH->SECBBA1 = 0xFFFFFFFF;
    FLASH->SECBBA2 = 0xFFFFFFFF;
    FLASH->SECBBA3 = 0xFFFFFFFF;

    if (GetFlashBankMode() == 1U)                        /* Flash secure DUAL BANK */
    {
      /* Flash block-based secure bank2 */
      FLASH->SECBBB0 = 0xFFFFFFFF;
      FLASH->SECBBB1 = 0xFFFFFFFF;
      FLASH->SECBBB2 = 0xFFFFFFFF;
      FLASH->SECBBB3 = 0xFFFFFFFF;
    }
  }

  while (*pFlashSR & FLASH_SR_BSY);                      /* Wait until operation is finished */

  gFlashBase = adr;
  gFlashSize = (M32(FLASHSIZE_BASE) & 0x0000FFFF) << 10;
#endif /* FLASH_MEM */

#if defined FLASH_OPT
  (void)adr;

  /* unlock FLASH_NSCR */
  FLASH->NSKEYR = FLASH_KEY1;
  FLASH->NSKEYR = FLASH_KEY2;
  DSB();
  while (FLASH->NSSR & FLASH_SR_BSY);                    /* Wait until operation is finished */

  /* Unlock Option Bytes operation */
  FLASH->OPTKEYR = FLASH_OPTKEY1;
  FLASH->OPTKEYR = FLASH_OPTKEY2;
  DSB();
  while (FLASH->NSSR & FLASH_SR_BSY);                    /* Wait until operation is finished */

  /* load option bytes */
//FLASH->NSCR = FLASH_CR_OBL_LAUNCH;
//DSB();
//while (FLASH->NSCR & FLASH_CR_OBL_LAUNCH);             /* Wait until option bytes are updated */
#endif /* FLASH_OPT */

  return (0);
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc)
{
  (void)fnc;

#if defined FLASH_MEM
  /* Lock Flash operation */
  *pFlashCR = FLASH_CR_LOCK;
  DSB();
  while (*pFlashSR & FLASH_SR_BSY);                      /* Wait until operation is finished */
#endif /* FLASH_MEM */

#if defined FLASH_OPT
  /* Lock option bytes operation */
  FLASH->NSCR = FLASH_CR_OPTLOCK;
  DSB();
  while (FLASH->NSCR & FLASH_SR_BSY);                    /* Wait until operation is finished */

  /* Load option bytes */
//FLASH->NSCR  = FLASH_CR_OBL_LAUNCH;
//DSB();
//while (FLASH->NSCR & FLASH_CR_OBL_LAUNCH);             /* Wait until option bytes are updated */

  /* Lock FLASH CR */
  FLASH->NSCR = FLASH_CR_LOCK;
  DSB();
  while (FLASH->NSCR & FLASH_SR_BSY);                    /* Wait until operation is finished */
#endif /* FLASH_OPT */

  return (0);
}


/*
 *  Blank Check Checks if Memory is Blank
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_OPT
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat) {
  /* For OPT algorithm Flash is always erased */

  (void)adr;
  (void)sz;
  (void)pat;

  return (0);
}
#endif /* FLASH_OPT */


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_MEM
int EraseChip (void)
{
  *pFlashSR = FLASH_PGERR;                               /* Reset Error Flags */

  *pFlashCR  = (FLASH_CR_MER1 | FLASH_CR_MER2);          /* Bank A/B mass erase enabled */
  *pFlashCR |=  FLASH_CR_STRT;                           /* Start erase */
  DSB();

  while (*pFlashSR & FLASH_SR_BSY);                      /* Wait until operation is finished */

  return (0);                                            /* Done */
}
#endif /* FLASH_MEM */

#ifdef FLASH_OPT
int EraseChip (void) {

  FLASH->NSSR = FLASH_PGERR;                             /* Reset Error Flags */

  FLASH->OPTR         = (FLASH->OPTR & 0x80000000) | 0x7FEFF8AA;  /* TZEN is unchanged */
  FLASH->NSBOOTADD0R  = 0x0800007F;
  FLASH->NSBOOTADD1R  = 0x0BF9007F;
  FLASH->WRP1AR       = 0xFF80FFFF;
  FLASH->WRP1BR       = 0xFF80FFFF;
  FLASH->WRP2AR       = 0xFF80FFFF;
  FLASH->WRP2BR       = 0xFF80FFFF;
  if (GetFlashSecureMode() == 1U)
  {                                                      /* Flash secure */
  FLASH->SECBOOTADD0R = 0x0C00007C;
  FLASH->SECWM1R2     = 0x7F807F80;
  FLASH->SECWM2R2     = 0x7F807F80;
  FLASH->SECWM1R1     = 0xFFFFFF80;
  FLASH->SECWM2R1     = 0xFFFFFF80;
  }

  FLASH->NSCR = FLASH_CR_OPTSTRT;                        /* Program values */
  DSB();

  while (FLASH->NSSR & FLASH_SR_BSY);                    /* Wait until operation is finished */

  if (FLASH->NSSR & FLASH_PGERR) {                       /* Check for Error */
    FLASH->NSSR  = FLASH_PGERR;                          /* Reset Error Flags */
    return (1);                                          /* Failed */
  }

  return (0);                                            /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_MEM
int EraseSector (unsigned long adr)
{
  u32 b, p;

  adr &= 0x08FFFFFF;                                     /* map 0x0C000000 to 0x08000000 */
  b = GetFlashBankNum(adr);                              /* Get Bank Number 0..1  */
  p = GetFlashPageNum(adr);                              /* Get Page Number 0..127 */

  *pFlashSR  = FLASH_PGERR;                              /* Reset Error Flags */

  *pFlashCR  = (FLASH_CR_PER |                           /* Page Erase Enabled */
                 (p <<  3)   |                           /* page Number. 0 to 127 for each bank */
                 (b << 11)    );
  *pFlashCR |=  FLASH_CR_STRT;                           /* Start Erase */
  DSB();

  while (*pFlashSR & FLASH_SR_BSY);                      /* Wait until operation is finished */

  if (*pFlashSR & FLASH_PGERR) {                         /* Check for Error */
    *pFlashSR  = FLASH_PGERR;                            /* Reset Error Flags */
    return (1);                                          /* Failed */
  }

  return (0);                                            /* Done */
}
#endif /* FLASH_MEM */

#if defined FLASH_OPT
int EraseSector (unsigned long adr) {
  /* erase sector is not needed for Flash Option bytes */

  (void)adr;

  return (0);                                              /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_MEM
int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{

  sz = (sz + 7) & ~7U;                                   /* Adjust size for two words */

  *pFlashSR = FLASH_PGERR;                               /* Reset Error Flags */

  *pFlashCR = FLASH_CR_PG ;	                             /* Programming Enabled */

  while (sz)
  {
//    M32(adr    ) = *((u32 *)(buf + 0));                  /* Program the first word of the Double Word */
//    M32(adr + 4) = *((u32 *)(buf + 4));                  /* Program the second word of the Double Word */
    M32(adr    ) = (u32)((*(buf+0)      ) |
                         (*(buf+1) <<  8) |
                         (*(buf+2) << 16) |
                         (*(buf+3) << 24) );             /* Program the first word of the Double Word */
    M32(adr + 4) = (u32)((*(buf+4)      ) |
                         (*(buf+5) <<  8) |
                         (*(buf+6) << 16) |
                         (*(buf+7) << 24) );             /* Program the second word of the Double Word */
    DSB();

    while (*pFlashSR & FLASH_SR_BSY);                    /* Wait until operation is finished */

    if (*pFlashSR & FLASH_PGERR) {                       /* Check for Error */
      *pFlashSR  = FLASH_PGERR;                          /* Reset Error Flags */
      return (1);                                        /* Failed */
    }

    adr += 8;                                            /* Next DoubleWord */
    buf += 8;
    sz  -= 8;
  }

  *pFlashCR = 0U;                                       /* Reset CR */

  return (0);
}
#endif /* FLASH_MEM */


#ifdef FLASH_OPT
int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 secwm1r2;
  u32 secwm2r2;
  u32 nsbootadd0r;
  u32 nsbootadd1r;
  u32 secbootadd0r;
  u32 secwm1r1;
  u32 wrp1ar;
  u32 wrp1br;
  u32 secwm2r1;
  u32 wrp2ar;
  u32 wrp2br;

  (void)adr;
  (void)sz;

/* Option Bytes order in buf
         secure                   non-secure
    0  FLASH_OPTR                FLASH_OPTR
    4  FLASH_SECWM1R2            DUMMY0 (0)
    8  FLASH_SECWM2R2            DUMMY1 (0)
   12  FLASH_NSBOOTADD0R         FLASH_NSBOOTADD0R
   16  FLASH_NSBOOTADD1R         FLASH_NSBOOTADD1R
   20  FLASH_SECBOOTADD0R        FLASH_SECBOOTADD0R
   24  FLASH_SECWM1R1            DUMMY2 (0)
   28  FLASH_WRP1AR              FLASH_WRP1AR
   32  FLASH_WRP1BR              FLASH_WRP1BR
   36  FLASH_SECWM2R1            DUMMY3 (0)
   40  FLASH_WRP2AR              FLASH_WRP2AR
   44  FLASH_WRP2BR              FLASH_WRP2BR
 */

  optr         = (u32)((*(buf   )) | (*(buf   +1) <<  8) | (*(buf   +2) << 16) | (*(buf   +3) << 24) );
  secwm1r2     = (u32)((*(buf+ 4)) | (*(buf+ 4+1) <<  8) | (*(buf+ 4+2) << 16) | (*(buf+ 4+3) << 24) );
  secwm2r2     = (u32)((*(buf+ 8)) | (*(buf+ 8+1) <<  8) | (*(buf+ 8+2) << 16) | (*(buf+ 8+3) << 24) );
  nsbootadd0r  = (u32)((*(buf+12)) | (*(buf+12+1) <<  8) | (*(buf+12+2) << 16) | (*(buf+12+3) << 24) );
  nsbootadd1r  = (u32)((*(buf+16)) | (*(buf+16+1) <<  8) | (*(buf+16+2) << 16) | (*(buf+16+3) << 24) );
  secbootadd0r = (u32)((*(buf+20)) | (*(buf+20+1) <<  8) | (*(buf+20+2) << 16) | (*(buf+20+3) << 24) );
  secwm1r1     = (u32)((*(buf+24)) | (*(buf+24+1) <<  8) | (*(buf+24+2) << 16) | (*(buf+24+3) << 24) );
  wrp1ar       = (u32)((*(buf+28)) | (*(buf+28+1) <<  8) | (*(buf+28+2) << 16) | (*(buf+28+3) << 24) );
  wrp1br       = (u32)((*(buf+32)) | (*(buf+32+1) <<  8) | (*(buf+32+2) << 16) | (*(buf+32+3) << 24) );
  secwm2r1     = (u32)((*(buf+36)) | (*(buf+36+1) <<  8) | (*(buf+36+2) << 16) | (*(buf+36+3) << 24) );
  wrp2ar       = (u32)((*(buf+40)) | (*(buf+40+1) <<  8) | (*(buf+40+2) << 16) | (*(buf+40+3) << 24) );
  wrp2br       = (u32)((*(buf+44)) | (*(buf+44+1) <<  8) | (*(buf+44+2) << 16) | (*(buf+44+3) << 24) );

  FLASH->NSSR  = FLASH_PGERR;                              /* Reset Error Flags */

  FLASH->OPTR         = (optr         & 0x9F7F77FFU) | ~(0x9F7F77FFU);
  FLASH->NSBOOTADD0R  = (nsbootadd0r  & 0xFFFFFF80U) | ~(0xFFFFFF80U);
  FLASH->NSBOOTADD1R  = (nsbootadd1r  & 0xFFFFFF80U) | ~(0xFFFFFF80U);
  FLASH->WRP1AR       = (wrp1ar       & 0x007F007FU) | ~(0x007F007FU);
  FLASH->WRP1BR       = (wrp1br       & 0x007F007FU) | ~(0x007F007FU);
  FLASH->WRP2AR       = (wrp2ar       & 0x007F007FU) | ~(0x007F007FU);
  FLASH->WRP2BR       = (wrp2br       & 0x007F007FU) | ~(0x007F007FU);
  if (GetFlashSecureMode() == 1U)
  {                                                        /* Flash secure */
  FLASH->SECBOOTADD0R = (secbootadd0r & 0xFFFFFF83U) | ~(0xFFFFFF83U);  /* not sure if BOOT_LOCK is 1 ore 2 bits. docu says 1 but it seems to be 2 */
  FLASH->SECWM1R2     = (secwm1r2     & 0x807F0000U) | ~(0x807F0000U);
  FLASH->SECWM2R2     = (secwm2r2     & 0x807F0000U) | ~(0x807F0000U);
  FLASH->SECWM1R1     = (secwm1r1     & 0x007F007FU) | ~(0x007F007FU);
  FLASH->SECWM2R1     = (secwm2r1     & 0x007F007FU) | ~(0x007F007FU);
  }
  DSB();

  FLASH->NSCR  = FLASH_CR_OPTSTRT;                       /* Program values */
  DSB();

  while (FLASH->NSSR & FLASH_SR_BSY);                    /* Wait until operation is finished */

  if (FLASH->NSSR & FLASH_PGERR) {                       /* Check for Error */
    FLASH->NSSR |= FLASH_PGERR;                          /* Reset Error Flags */
    return (1);                                          /* Failed */
  }

  return (0);                                            /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Verify Flash Contents
 *    Parameter:      adr:  Start Address
 *                    sz:   Size (in bytes)
 *                    buf:  Data
 *    Return Value:   (adr+sz) - OK, Failed Address
 */

#ifdef FLASH_OPT
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 secwm1r2;
  u32 secwm2r2;
  u32 nsbootadd0r;
  u32 nsbootadd1r;
  u32 secbootadd0r;
  u32 secwm1r1;
  u32 wrp1ar;
  u32 wrp1br;
  u32 secwm2r1;
  u32 wrp2ar;
  u32 wrp2br;

  (void)adr;
  (void)sz;

  optr         = (u32)((*(buf   )) | (*(buf   +1) <<  8) | (*(buf   +2) << 16) | (*(buf   +3) << 24) );
  secwm1r2     = (u32)((*(buf+ 4)) | (*(buf+ 4+1) <<  8) | (*(buf+ 4+2) << 16) | (*(buf+ 4+3) << 24) );
  secwm2r2     = (u32)((*(buf+ 8)) | (*(buf+ 8+1) <<  8) | (*(buf+ 8+2) << 16) | (*(buf+ 8+3) << 24) );
  nsbootadd0r  = (u32)((*(buf+12)) | (*(buf+12+1) <<  8) | (*(buf+12+2) << 16) | (*(buf+12+3) << 24) );
  nsbootadd1r  = (u32)((*(buf+16)) | (*(buf+16+1) <<  8) | (*(buf+16+2) << 16) | (*(buf+16+3) << 24) );
  secbootadd0r = (u32)((*(buf+20)) | (*(buf+20+1) <<  8) | (*(buf+20+2) << 16) | (*(buf+20+3) << 24) );
  secwm1r1     = (u32)((*(buf+24)) | (*(buf+24+1) <<  8) | (*(buf+24+2) << 16) | (*(buf+24+3) << 24) );
  wrp1ar       = (u32)((*(buf+28)) | (*(buf+28+1) <<  8) | (*(buf+28+2) << 16) | (*(buf+28+3) << 24) );
  wrp1br       = (u32)((*(buf+32)) | (*(buf+32+1) <<  8) | (*(buf+32+2) << 16) | (*(buf+32+3) << 24) );
  secwm2r1     = (u32)((*(buf+36)) | (*(buf+36+1) <<  8) | (*(buf+36+2) << 16) | (*(buf+36+3) << 24) );
  wrp2ar       = (u32)((*(buf+40)) | (*(buf+40+1) <<  8) | (*(buf+40+2) << 16) | (*(buf+40+3) << 24) );
  wrp2br       = (u32)((*(buf+44)) | (*(buf+44+1) <<  8) | (*(buf+44+2) << 16) | (*(buf+44+3) << 24) );

  /* Fail address returns the number of the OPT word passed with the assembler file */
  if ((FLASH->OPTR         & 0x9F7F77FF) != (optr         & 0x9F7F77FF)) {    /* Check OPTR values */
    return (adr + 0);
  }

  if ((FLASH->NSBOOTADD0R  & 0xFFFFFF80) != (nsbootadd0r  & 0xFFFFFF80)) {    /* Check NSBOOTADD0R values */
    return (adr + 3);
  }

  if ((FLASH->NSBOOTADD1R  & 0xFFFFFF80) != (nsbootadd1r  & 0xFFFFFF80)) {    /* Check NSBOOTADD1R values */
    return (adr + 4);
  }

  if ((FLASH->WRP1AR       & 0x007F007F) != (wrp1ar       & 0x007F007F)) {    /* Check WRP1AR values */
    return (adr + 7);
  }

  if ((FLASH->WRP1BR       & 0x007F007F) != (wrp1br       & 0x007F007F)) {    /* Check WRP1BR values */
    return (adr + 8);
  }

  if ((FLASH->WRP2AR       & 0x007F007F) != (wrp2ar       & 0x007F007F)) {    /* Check WRP2AR values */
    return (adr + 10);
  }

  if ((FLASH->WRP2BR       & 0x007F007F) != (wrp2br       & 0x007F007F)) {    /* Check WRP2BR values */
    return (adr + 11);
  }

if (GetFlashSecureMode() == 1U)
{                                                      /* Flash secure */
  if ((FLASH->SECBOOTADD0R & 0xFFFFFF83) != (secbootadd0r & 0xFFFFFF83)) {    /* Check SECBOOTADD0R values */
    return (adr + 5);
  }

  if ((FLASH->SECWM1R2     & 0x807F0000) != (secwm1r2     & 0x807F0000)) {    /* Check SECWM1R2 values */
    return (adr + 1);
  }

  if ((FLASH->SECWM2R2     & 0x807F0000) != (secwm2r2     & 0x807F0000)) {    /* Check SECWM2R2 values */
    return (adr + 2);
  }

  if ((FLASH->SECWM1R1     & 0x007F007F) != (secwm1r1     & 0x007F007F)) {    /* Check SECWM1R1 values */
    return (adr + 6);
  }

  if ((FLASH->SECWM2R1     & 0x007F007F) != (secwm2r1     & 0x007F007F)) {    /* Check SECWM2R1 values */
    return (adr + 9);
  }
}

  return (adr + sz);
}
#endif /* FLASH_OPT */
