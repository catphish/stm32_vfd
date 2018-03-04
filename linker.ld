/* memory layout for an STM32L476RG */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 0x100000
  SRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 0x18000
}

/* output sections */
SECTIONS
{
  /* program code into FLASH */
  .text :
  {
    *(.vector_table)    /* Vector table */
    *(.text)            /* Program code */
  } >FLASH

  /* uninitialized global and static variables into SRAM */
  .data :
  {
    *(.data)
  } >SRAM
}
