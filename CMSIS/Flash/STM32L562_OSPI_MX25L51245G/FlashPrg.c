/* -----------------------------------------------------------------------------
 * Copyright (c) 2020 ARM Ltd.
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
 * $Date:        25. May 2020
 * $Revision:    V1.0.0
 *
 * Project:      Flash Programming Functions for
 *               ST STM32L562 (STM32L562E-DK) with OSPI MX25LM51245G (Macronix)
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include <string.h>
#include "..\FlashOS.h"                /* FlashOS Structures */

#include "stm32l5xx_hal.h"

#if   defined (STM32L552E_EVAL)
  #include "stm32l552e_eval_ospi.h"
#elif defined (STM32L562E_DK)
  #include "stm32l562e_discovery_ospi.h"
#else
  #error no board selected!
#endif

BSP_OSPI_NOR_Init_t ospi_flash;


/* Private variables ---------------------------------------------------------*/
extern void SystemInit(void);
extern void SystemClock_Config(void);

extern OSPI_HandleTypeDef hospi_nor[1];
extern OSPI_NOR_Ctx_t Ospi_Nor_Ctx[1];


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
  int32_t rc;

  __disable_irq();

  memset(&hospi_nor,0,sizeof(hospi_nor));
  memset(&Ospi_Nor_Ctx,0,sizeof(Ospi_Nor_Ctx));

  ospi_flash.InterfaceMode = BSP_OSPI_NOR_OPI_MODE;
  ospi_flash.TransferRate  = BSP_OSPI_NOR_DTR_TRANSFER;

  SystemInit();
  SystemClock_Config();          /* configure system core clock */
//  SystemCoreClockUpdate();

  rc = BSP_OSPI_NOR_Init(0, &ospi_flash);
  return ((rc == BSP_ERROR_NONE) ? 0 : 1);
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {
  int32_t rc;

  rc = BSP_OSPI_NOR_DeInit(0);

  return ((rc == BSP_ERROR_NONE) ? 0 : 1);
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseChip (void) {
  int32_t rc;

  rc = BSP_OSPI_NOR_Erase_Chip(0);

  if (rc != BSP_ERROR_NONE) {
    return (1);
  }

  /* Wait the end of the current operation on memory side */
  do
  {
    rc = BSP_OSPI_NOR_GetStatus(0);
  } while((rc != BSP_ERROR_NONE) && (rc != BSP_ERROR_COMPONENT_FAILURE));

  return ((rc == BSP_ERROR_NONE) ? 0 : 1);
}


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr) {
  int32_t rc;

  rc = BSP_OSPI_NOR_Erase_Block(0, (uint32_t)(adr & 0x0FFFFFFF),  MX25LM51245G_ERASE_64K);

  if (rc != BSP_ERROR_NONE) {
    return (1);
  }

  /* Wait the end of the current operation on memory side */
  do
  {
    rc = BSP_OSPI_NOR_GetStatus(0);
  } while((rc != BSP_ERROR_NONE) && (rc != BSP_ERROR_COMPONENT_FAILURE));

  return ((rc == BSP_ERROR_NONE) ? 0 : 1);
}


/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {
  int32_t rc;

  rc = BSP_OSPI_NOR_Write(0, (uint8_t*)buf, (uint32_t)(adr & 0x0FFFFFFF), (uint32_t)sz);

  return ((rc == BSP_ERROR_NONE) ? 0 : 1);
}


 /*
  *  Verify Flash Contents
  *    Parameter:      adr:  Start Address
  *                    sz:   Size (in bytes)
  *                    buf:  Data
  *    Return Value:   (adr+sz) - OK, Failed Address
 */
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf){
  uint8_t * ptr = (uint8_t *)adr;
  uint32_t i;
  int32_t rc;

  rc = BSP_OSPI_NOR_EnableMemoryMappedMode(0);
  if (rc != BSP_ERROR_NONE) {
    return (adr);
  }

  for(i = 0; i < sz; i++)
  {
    if (ptr[i] != buf[i])
      return (adr + i);                /* Verification Failed (return address) */
  }

  rc = BSP_OSPI_NOR_DisableMemoryMappedMode(0);
  if (rc != BSP_ERROR_NONE) {
    return (adr);
  }
  rc = BSP_OSPI_NOR_ConfigFlash(0, ospi_flash.InterfaceMode, ospi_flash.TransferRate);
  if (rc != BSP_ERROR_NONE) {
    return (adr);
  }


  return (adr + sz);                   /* Done successfully */
}


/*  Blank Check Block in Flash Memory
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */
int BlankCheck  (unsigned long adr, unsigned long sz, unsigned char pat) {
  uint8_t * ptr = (uint8_t *)adr;
  uint32_t i = 0;
  int result = 0;
  int32_t rc;


  rc = BSP_OSPI_NOR_EnableMemoryMappedMode(0);

  for (i = 0; i < sz; i++)
  {
    if(ptr[i] != pat) {
      result = 1;
      break;
    }
  }

  rc = BSP_OSPI_NOR_DisableMemoryMappedMode(0);
  rc = BSP_OSPI_NOR_ConfigFlash(0, ospi_flash.InterfaceMode, ospi_flash.TransferRate);

  return (result);
}


/* -- helper functions for test application -- */
void SetOSPIMemMode(void) {

  ospi_flash.InterfaceMode = BSP_OSPI_NOR_OPI_MODE;
  ospi_flash.TransferRate  = BSP_OSPI_NOR_DTR_TRANSFER;

  BSP_OSPI_NOR_DeInit(0);
  BSP_OSPI_NOR_Init(0, &ospi_flash);
  BSP_OSPI_NOR_EnableMemoryMappedMode(0);
}

void ReSetOSPIMemMode(void) {

//ospi_flash.InterfaceMode = BSP_OSPI_NOR_OPI_MODE;
//ospi_flash.TransferRate  = BSP_OSPI_NOR_DTR_TRANSFER;

  BSP_OSPI_NOR_DeInit(0);
//BSP_OSPI_NOR_Init(0, &ospi_flash);
}
