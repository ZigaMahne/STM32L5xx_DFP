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
 * Project:      Flash Device Description for ST STM32L5xx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.2.0
 *    Added OPT Algorithms
 *  Version 1.1.0
 *   the sector size is changed to 2 kB to be used for Dual/Single Bank Flash configuration
 *  Version 1.00
 *    Initial release
 */

#include "..\FlashOS.h"        /* FlashOS Structures */

extern struct FlashDevice const FlashDevice;

#ifdef FLASH_MEM

#ifdef STM32L5xx_512
  struct FlashDevice const FlashDevice  =  {
    FLASH_DRV_VERS,                /* Driver Version, do not modify! */
    "STM32L5x_512_NSecure_Flash",  /* Device Name (512kB) */
    ONCHIP,                        /* Device Type */
    0x08000000,                    /* Device Start Address */
    0x00080000,                    /* Device Size in Bytes (512kB) */
    1024,                          /* Programming Page Size */
    0,                             /* Reserved, must be 0 */
    0xFF,                          /* Initial Content of Erased Memory */
    400,                           /* Program Page Timeout 400 mSec */
    400,                           /* Erase Sector Timeout 400 mSec */

    /* Specify Size and Address of Sectors */
    0x0800, 0x000000,              /* Sector Size  2kB (256 Sectors) */
    SECTOR_END
  };
#endif


#ifdef STM32L5xx_512_0x0C
  struct FlashDevice const FlashDevice  =  {
    FLASH_DRV_VERS,                /* Driver Version, do not modify! */
    "STM32L5x_512_Secure_Flash",   /* Device Name (512kB) */
    ONCHIP,                        /* Device Type */
    0x0C000000,                    /* Device Start Address */
    0x00080000,                    /* Device Size in Bytes (512kB) */
    1024,                          /* Programming Page Size */
    0,                             /* Reserved, must be 0 */
    0xFF,                          /* Initial Content of Erased Memory */
    400,                           /* Program Page Timeout 400 mSec */
    400,                           /* Erase Sector Timeout 400 mSec */

    /* Specify Size and Address of Sectors */
    0x0800, 0x000000,              /* Sector Size  2kB (256 Sectors) */
    SECTOR_END
  };
#endif

#endif /* FLASH_MEM */


#if defined FLASH_OPT

  struct FlashDevice const FlashDevice  =  {
    FLASH_DRV_VERS,                /* Driver Version, do not modify! */
    "STM32L5xx Flash Options",     /* Device Name */
    ONCHIP,                        /* Device Type */
    0x1FF00000,                    /* Device Start Address (virtual address) */
    0x00000030,                    /* Device Size in Bytes (48) */
    48,                            /* Programming Page Size */
    0,                             /* Reserved, must be 0 */
    0xFF,                          /* Initial Content of Erased Memory */
    3000,                          /* Program Page Timeout 3 Sec */
    3000,                          /* Erase Sector Timeout 3 Sec */

    /* Specify Size and Address of Sectors */
    0x0030, 0x000000,              /* Sector Size 48B */
    SECTOR_END
  };

#endif /* FLASH_OPT */

