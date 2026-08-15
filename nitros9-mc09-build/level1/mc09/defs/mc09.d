MC09.D SET 1

********************************************************************
* mc09.d - NitrOS-9 System Definitions for the Multicomp09
*
* When the primary boot loader passes control to the "track34" code
* the system memory map looks like this:
*
* $0000----> ==================================
* | |
* | |
* | RAM |
* | |
* | |
* $FFD0---->|==================================|
* | I/O |
* $FFE0---->|==================================|
* | RAM |
* |==================================|
* $FFFF---->
*
* The exception vectors at $FFF2 and onwards are initialised to vector to RAM
* as per coco1, just as krn expects. Specifically:
*
* $FFF2 holds vector to $0100 for SWI3
* $FFF4 holds vector to $0103 for SWI2
* $FFF6 holds vector to $010F for FIRQ
* $FFF8 holds vector to $010C for IRQ
* $FFFA holds vector to $0106 for SWI
* $FFFC holds vector to $0109 for NMI
*
* The 16-location I/O space and the interrupt hook-up are described below.
*
* Edt/Rev YYYY/MM/DD Modified by
* Comment
* ------------------------------------------------------------------
* 0.1 2015/10/17 Neal Crook. Tidy-up for first commit.
*
 NAM Multicomp09Defs
 TTL NitrOS-9 System Definitions for the Multicomp09


********************************************************************
* CPU Type Definitions
*
Color SET 1
Color3 SET 2
CPUType SET Color


********************************************************************
* Clock Speed Type Definitions
*
OneMHz EQU 1
TwoMHz EQU 2
CPUSpeed SET OneMHz


********************************************************************
* Power Line Frequency Definitions
* Multicomp09 has no dependency on power line frequency but setting
* it to Hz50 is the simplest way to make TkPerSec 50, which is important
* [NAC HACK 2015Aug31] actually no need to change it - gets applied at
* command line in modules/makefile to generate a 50Hz and 60Hz version.
*
Hz50 EQU 50 Assemble clock for 50 hz power
Hz60 EQU 60 Assemble clock for 60 hz power
PwrLnFrq SET Hz50 Set to Appropriate freq


********************************************************************
* Ticks per second
*
TkPerSec SET 50


********************************************************************
* Special character Bit position equates
*
* [NAC HACK 2017Jun04] used in level1/modules/sysgo.asm
SHIFTBIT EQU %00000001


********************************************************************
* Multicomp09 Interrupts:
*
* NMI - wired to single-step logic
* FIRQ - (currently) unused
* IRQ - wired to timer interrupt, VDU/KBD virtual ACIA and serial
* ports 1 and 2.

********************************************************************
* Multicomp09 I/O space is $FFD0-$FFDF (16 locations). The equates
* below describe the I/O registers.

********************************************************************
* VDU/KBD (VIRTUAL ACIA)
VDUSTA EQU $FFD0
VDUDAT EQU $FFD1

* SERIAL PORT 1
UARTSTA1 EQU $FFD2
UARTDAT1 EQU $FFD3

* SERIAL PORT 2
UARTSTA2 EQU $FFD4
UARTDAT2 EQU $FFD5

********************************************************************
* GPIO device
* SEE VHDL HEADER FOR PROG GUIDE
GPIOADR EQU $FFD6
GPIODAT EQU $FFD7

* values supported by GPIOADR register
GPDAT0 EQU 0
GPDDR1 EQU 1
GPDAT2 EQU 2
GPDDR3 EQU 3


********************************************************************
* SDCARD CONTROL REGISTERS
* SEE VHDL HEADER FOR PROG GUIDE
SDDATA EQU $FFD8
SDCTL EQU $FFD9
SDLBA0 EQU $FFDA
SDLBA1 EQU $FFDB
SDLBA2 EQU $FFDC

********************************************************************
* 50Hz TIMER INTERRUPT
* TIMER (READ/WRITE)
*
* AT RESET, THE TIMER IS DISABLED AND THE INTERRUPT IS DEASSERTED. TIMER READS AS 0.
* BIT[1] IS READ/WRITE, TIMER ENABLE.
* BIT[7] IS READ/WRITE-1-TO-CLEAR, INTERRUPT.
*
* IN AN ISR THE TIMER CAN BE SERVICED BY PERFORMING AN INC ON ITS ADDRESS
*
* READ WRITE COMMENT
* N/A $02 ENABLE TIMER
* $00 $01 TIMER WAS/REMAINS DISABLED. N=0.
* $02 $03 TIMER WAS/REMAINS ENABLED, NO INTERRUPT. N=0.
* $80 $81 TIMER WAS/REMAINS DISABLED, OLD PENDING INTERRUPT CLEARED. N=1.
* $82 $83 TIMER WAS/REMAINS DISABLED, OLD PENDING INTERRUPT CLEARED. N=1.
*
TIMER EQU $FFDD

********************************************************************
* MEM_MAPPER2 CONTROL REGISTERS
* MMUADR (WRITE-ONLY)
* 7 - ROMDIS (RESET TO 0)
* 6 - TR
* 5 - MMUEN (RESET TO 0)
* 4 - NMI
* 3:0 - MAPSEL
*
* MMUDAT (WRITE-ONLY)
* 7 - WRPROT
* 6:0 - PHYSICAL BLOCK FOR CURRENT MAPSEL
*
*
* For NitrOS-9 Level 2, want a fixed 512byte region of r/w memory
* at the top of the address space. There is no space to provide
* an enable for this behaviour (which I call FRT for FixedRamTop)
* and so some special magic is used, as follows:
*
* IF ROMDIS=1 & MMUEN=1 then a write with b4=0 (see NMI behaviour
* below) and b7=0 and b5=1 does NOT enable the ROM but actually
* sets FRT=1. Any write with MMUEN=0 sets FRT=0 again. In summary:
*
* Current Action End State
* -----------------+-------------+-----------------
* ROMDIS MMUEn FRT ROMDIS MMUEn ROMDIS MMUEn FRT
* x x x RESET 0 0 0
* x x x 0 1 0 1 x
* x x x 1 1 1 1 x
* x x x x 0 x 0 0
* 1 1 x 0 1 1 1 1

MMUADR EQU $FFDE
MMUDAT EQU $FFDF

* BIT-FIELDS
MMUADR_ROMDIS EQU $80 0 after reset, 1 when OS boots. Leave at 1.
MMUADR_TR EQU $40 0 after reset, 0 when OS boots. 0=user, 1=sys
MMUADR_MMUEN EQU $20 0 after reset, 1 when OS boots. Leave at 1.
MMUADR_NMI EQU $10 0 after reset, 0 when OS boots. Do not write 1.
MMUADR_MAPSEL EQU $0f last-written value is UNDEFINED.
* the only two useful values for the upper nibble
MMU_TR0 EQU (MMUADR_ROMDIS|MMUADR_MMUEN)
MMU_TR1 EQU (MMUADR_ROMDIS|MMUADR_MMUEN|MMUADR_TR)
* one-time write to enable FRT
MMU_TR0FRT EQU (MMUADR_MMUEN)
MMU_TR1FRT EQU (MMUADR_MMUEN|MMUADR_TR)


********************************************************************
* Coco stuff that's needed to allow other files to compile

* [NAC HACK 2017Jun04] needed by level1/modules/boot_sdc.asm
DPort SET $FF40 Disk controller base address
* [NAC HACK 2017Jun04] needed by level1/modules/sysgo.asm
PIA0Base EQU $FF00
PIA1Base EQU $FF20


********************************************************************
* Needed for building generic tools (eg cobbler.asm)
Bt.Size EQU $1080 Maximum size of bootfile
Bt.Track EQU 34 Boot track
Bt.Sec EQU 0 Start LSN of boot area on boot track



********************************************************************
*
* NitrOS-9 Level 1 Section
*
********************************************************************

* Needed for boot
Bt.Start EQU $EE00 Address of boot track in memory

HW.Page SET $FF Device descriptor hardware page

