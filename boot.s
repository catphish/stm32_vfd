/* Simple startup file for Cortex-M3 */

/* Thumb-2 instructions are only supported in unified syntax mode */
.syntax unified

/* Vector table definition */
.section ".isr_vector", "a"
.long  _sram_end                   /* Top of Stack                 */
.long  Reset_Handler               /* Reset Handler                */
.long  NMI_Handler                 /* NMI Handler                  */
.long  HardFault_Handler           /* Hard Fault Handler           */
.long  MemManage_Handler           /* MPU Fault Handler            */
.long  BusFault_Handler            /* Bus Fault Handler            */
.long  UsageFault_Handler          /* Usage Fault Handler          */
.long  0                           /* Reserved                     */
.long  0                           /* Reserved                     */
.long  0                           /* Reserved                     */
.long  0                           /* Reserved                     */
.long  SVC_Handler                 /* SVCall Handler               */
.long  DebugMon_Handler            /* Debug Monitor Handler        */
.long  0                           /* Reserved                     */
.long  PendSV_Handler              /* PendSV Handler               */
.long  SysTick_Handler             /* SysTick Handler              */

/* Program section */
.section ".text"

/* Declare as thumb function. Otherwise it will not be linked
* correctly
*/
.thumb_func
/* Export the symbol so linker can see this */
.global Reset_Handler
Reset_Handler:
CopyDataInitializersStart:
  ldr   r0, =_sdata   /* write to this addr */
  ldr   r1, =_edata   /* until you get to this addr */
  ldr   r2, =_sidata  /* reading from this addr */
  b     CopyDataInitializersEnterLoop
CopyDataInitializersLoop:
  ldmia r2!, {r3}
  stmia r0!, {r3}
CopyDataInitializersEnterLoop:
  cmp   r0, r1
  bcc   CopyDataInitializersLoop
WriteZeroToBssStart:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
  b WriteZeroToBssEnterLoop
WriteZeroToBssLoop:
  stmia r0!, {r2}
WriteZeroToBssEnterLoop:
  cmp r0, r1
  bcc WriteZeroToBssLoop
/* Jump to main(), a thumb function */
  LDR     R0, =setup
  BLX     R0
  LDR     R0, =main
  BLX     R0
/* If main() ever exit, this should hold MCU from running wild */
  B       .

/* All standard interrupts will land here */
.thumb_func
NMI_Handler:
.thumb_func
HardFault_Handler:
.thumb_func
MemManage_Handler:
.thumb_func
BusFault_Handler:
.thumb_func
UsageFault_Handler:
.thumb_func
SVC_Handler:
.thumb_func
DebugMon_Handler:
.thumb_func
PendSV_Handler:
.thumb_func
SysTick_Handler:
 B    . /* while(1); */
