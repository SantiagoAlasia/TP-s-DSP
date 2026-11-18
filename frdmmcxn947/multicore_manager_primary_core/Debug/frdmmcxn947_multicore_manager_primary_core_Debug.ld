/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * Generated linker script file for MCXN947
 * Created from linkscript.ldt by FMCreateLinkLibraries
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v25.6 [Build 136] [2025-06-27] on 17 nov. 2026 21:20:43
 */

INCLUDE "frdmmcxn947_multicore_manager_primary_core_Debug_library.ld"
INCLUDE "frdmmcxn947_multicore_manager_primary_core_Debug_memory.ld"

ENTRY(ResetISR)

SECTIONS
{
     /* MAIN TEXT SECTION */
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2_core_m33slave_text));
        LONG(    ADDR(.data_RAM2_core_m33slave_text));
        LONG(  SIZEOF(.data_RAM2_core_m33slave_text));
        LONG(LOADADDR(.data_RAM2_core_m33slave_ARM_extab));
        LONG(    ADDR(.data_RAM2_core_m33slave_ARM_extab));
        LONG(  SIZEOF(.data_RAM2_core_m33slave_ARM_extab));
        LONG(LOADADDR(.data_RAM2_core_m33slave_ARM_exidx));
        LONG(    ADDR(.data_RAM2_core_m33slave_ARM_exidx));
        LONG(  SIZEOF(.data_RAM2_core_m33slave_ARM_exidx));
        LONG(LOADADDR(.data_RAM2_core_m33slave_data));
        LONG(    ADDR(.data_RAM2_core_m33slave_data));
        LONG(  SIZEOF(.data_RAM2_core_m33slave_data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        LONG(LOADADDR(.data_RAM3));
        LONG(    ADDR(.data_RAM3));
        LONG(  SIZEOF(.data_RAM3));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM3));
        LONG(  SIZEOF(.bss_RAM3));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */

        *(.after_vectors*)

       *(.text*)
       *(.rodata .rodata.* .constdata .constdata.*)
       . = ALIGN(4);
    } > PROGRAM_FLASH
    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > PROGRAM_FLASH

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > PROGRAM_FLASH
 
    _etext = .;
        
    /* DATA section for Ram1 */
    .data_RAM2_core_m33slave_text : 
    {
       FILL(0xff)
       PROVIDE(__start_data_RAM2 = .) ;
       __core_m33slave_START__ = .; /* start of slave image */
       KEEP(*(.core_m33slave))
    } > Ram1 AT>PROGRAM_FLASH

    /* M33SLAVE extab and exidx sections */
    .data_RAM2_core_m33slave_ARM_extab :
    {
        FILL(0xff)
        KEEP(*(.core_m33slave.ARM.extab))
    } > Ram1 AT>PROGRAM_FLASH 

    .data_RAM2_core_m33slave_ARM_exidx :
    {
        FILL(0xff)
        KEEP(*(.core_m33slave.ARM.exidx))
    } > Ram1 AT>PROGRAM_FLASH 

    /* M33SLAVE data section */
    .data_RAM2_core_m33slave_data :
    {
        FILL(0xff)
        KEEP(*(.core_m33slave.data_*)) KEEP(*(.core_m33slave.data))
        __core_m33slave_END__ = .; /* end of slave image */

        /* perform some simple sanity checks */
        ASSERT(!(__core_m33slave_START__ == __core_m33slave_END__), "No slave code for _core_m33slave");
        ASSERT( (ABSOLUTE(__core_m33slave_START__) == __vectors_start___core_m33slave), "M33SLAVE execute address differs from address provided in source image");
    } > Ram1 AT>PROGRAM_FLASH 

    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
        *(.ramfunc.$RAM2)
        *(.ramfunc.$Ram1)
        *(.data.$RAM2)
        *(.data.$Ram1)
        *(.data.$RAM2.*)
        *(.data.$Ram1.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
        PROVIDE(__end_data_Ram1 = .) ;
     } > Ram1 AT>PROGRAM_FLASH

    /* DATA section for rpmsg_sh_mem */

    .data_RAM3 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM3 = .) ;
        PROVIDE(__start_data_rpmsg_sh_mem = .) ;
        *(.ramfunc.$RAM3)
        *(.ramfunc.$rpmsg_sh_mem)
        *(.data.$RAM3)
        *(.data.$rpmsg_sh_mem)
        *(.data.$RAM3.*)
        *(.data.$rpmsg_sh_mem.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
        PROVIDE(__end_data_rpmsg_sh_mem = .) ;
     } > rpmsg_sh_mem AT>PROGRAM_FLASH

    /* MAIN DATA SECTION */
    .uninit_RESERVED (NOLOAD) : ALIGN(4)
    {
        _start_uninit_RESERVED = .;
        KEEP(*(.bss.$RESERVED*))
       . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > Ram0 AT> Ram0

    /* Main DATA section (Ram0) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       PROVIDE(__start_data_RAM = .) ;
       PROVIDE(__start_data_Ram0 = .) ;
       *(vtable)
       *(.ramfunc*)
       KEEP(*(CodeQuickAccess))
       KEEP(*(DataQuickAccess))
       *(RamFunction)
       *(.data*)
       . = ALIGN(4) ;
       _edata = . ;
       PROVIDE(__end_data_RAM = .) ;
       PROVIDE(__end_data_Ram0 = .) ;
    } > Ram0 AT>PROGRAM_FLASH

    /* BSS section for Ram1 */
    .bss_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       PROVIDE(__start_bss_Ram1 = .) ;
       *(.bss.$RAM2)
       *(.bss.$Ram1)
       *(.bss.$RAM2.*)
       *(.bss.$Ram1.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
       PROVIDE(__end_bss_Ram1 = .) ;
    } > Ram1 AT> Ram1

    /* BSS section for rpmsg_sh_mem */
    .bss_RAM3 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM3 = .) ;
       PROVIDE(__start_bss_rpmsg_sh_mem = .) ;
       *(.bss.$RAM3)
       *(.bss.$rpmsg_sh_mem)
       *(.bss.$RAM3.*)
       *(.bss.$rpmsg_sh_mem.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM3 = .) ;
       PROVIDE(__end_bss_rpmsg_sh_mem = .) ;
    } > rpmsg_sh_mem AT> rpmsg_sh_mem

    /* MAIN BSS SECTION */
    .bss (NOLOAD) : ALIGN(4)
    {
        _bss = .;
        PROVIDE(__start_bss_RAM = .) ;
        PROVIDE(__start_bss_Ram0 = .) ;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(__end_bss_RAM = .) ;
        PROVIDE(__end_bss_Ram0 = .) ;
        PROVIDE(end = .);
    } > Ram0 AT> Ram0

    /* NOINIT section for Ram1 */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM2 = .) ;
       PROVIDE(__start_noinit_Ram1 = .) ;
       *(.noinit.$RAM2)
       *(.noinit.$Ram1)
       *(.noinit.$RAM2.*)
       *(.noinit.$Ram1.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM2 = .) ;
       PROVIDE(__end_noinit_Ram1 = .) ;
    } > Ram1 AT> Ram1

    /* NOINIT section for rpmsg_sh_mem */
    .noinit_RAM3 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM3 = .) ;
       PROVIDE(__start_noinit_rpmsg_sh_mem = .) ;
       *(.noinit.$RAM3)
       *(.noinit.$rpmsg_sh_mem)
       *(.noinit.$RAM3.*)
       *(.noinit.$rpmsg_sh_mem.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM3 = .) ;
       PROVIDE(__end_noinit_rpmsg_sh_mem = .) ;
    } > rpmsg_sh_mem AT> rpmsg_sh_mem

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        PROVIDE(__start_noinit_RAM = .) ;
        PROVIDE(__start_noinit_Ram0 = .) ;
        *(.noinit*)
         . = ALIGN(4) ;
        _end_noinit = .;
       PROVIDE(__end_noinit_RAM = .) ;
       PROVIDE(__end_noinit_Ram0 = .) ;        
    } > Ram0 AT> Ram0

    /* Reserve and place Heap within memory map */
    _HeapSize = 0x1000;
    .heap (NOLOAD) :  ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > Ram0

     _StackSize = 0x1000;
     /* Reserve space in memory for Stack */
    .heap2stackfill (NOLOAD) :
    {
        . += _StackSize;
    } > Ram0
    /* Locate actual Stack in memory map */
    .stack ORIGIN(Ram0) + LENGTH(Ram0) - _StackSize - 0 (NOLOAD) :  ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > Ram0

    /* Provide basic symbols giving location and size of main text
     * block, including initial values of RW data sections. Note that
     * these will need extending to give a complete picture with
     * complex images (e.g multiple Flash banks).
     */
    _image_start = LOADADDR(.text);
    _image_end = LOADADDR(.data) + SIZEOF(.data);
    _image_size = _image_end - _image_start;
}