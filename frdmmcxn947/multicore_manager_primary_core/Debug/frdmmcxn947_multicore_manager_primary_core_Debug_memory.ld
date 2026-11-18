/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * Generated linker script file for MCXN947
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v25.6 [Build 136] [2025-06-27] on 17 nov. 2026 21:20:43
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0xc0000 /* 768K bytes (alias Flash) */  
  Ram0 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x4c000 /* 304K bytes (alias RAM) */  
  Ram1 (rwx) : ORIGIN = 0x2004e000, LENGTH = 0x1a000 /* 104K bytes (alias RAM2) */  
  rpmsg_sh_mem (rwx) : ORIGIN = 0x2004c000, LENGTH = 0x2000 /* 8K bytes (alias RAM3) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH = 0x0  ; /* PROGRAM_FLASH */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_PROGRAM_FLASH = 0x0 + 0xc0000 ; /* 768K bytes */  
  __top_Flash = 0x0 + 0xc0000 ; /* 768K bytes */  
  __base_Ram0 = 0x20000000  ; /* Ram0 */  
  __base_RAM = 0x20000000 ; /* RAM */  
  __top_Ram0 = 0x20000000 + 0x4c000 ; /* 304K bytes */  
  __top_RAM = 0x20000000 + 0x4c000 ; /* 304K bytes */  
  __base_Ram1 = 0x2004e000  ; /* Ram1 */  
  __base_RAM2 = 0x2004e000 ; /* RAM2 */  
  __top_Ram1 = 0x2004e000 + 0x1a000 ; /* 104K bytes */  
  __top_RAM2 = 0x2004e000 + 0x1a000 ; /* 104K bytes */  
  __base_rpmsg_sh_mem = 0x2004c000  ; /* rpmsg_sh_mem */  
  __base_RAM3 = 0x2004c000 ; /* RAM3 */  
  __top_rpmsg_sh_mem = 0x2004c000 + 0x2000 ; /* 8K bytes */  
  __top_RAM3 = 0x2004c000 + 0x2000 ; /* 8K bytes */  
