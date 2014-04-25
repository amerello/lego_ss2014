#!/bin/sh
#
# This file was automatically generated.
#
# It can be overwritten by nios2-flash-programmer-generate or nios2-flash-programmer-gui.
#

#
# Converting ELF File: C:\Users\Andreas\Documents\_Studium\LegoCar\NewUart\software\new_Uart\new_Uart.elf to: "..\flash/new_Uart_epcs.flash"
#
elf2flash --input="C:/Users/Andreas/Documents/_Studium/LegoCar/NewUart/software/new_Uart/new_Uart.elf" --output="../flash/new_Uart_epcs.flash" --epcs --verbose 

#
# Programming File: "..\flash/new_Uart_epcs.flash" To Device: epcs
#
nios2-flash-programmer "../flash/new_Uart_epcs.flash" --base=0x4000000 --epcs --sidp=0x5000060 --id=0x0 --timestamp=1386756594 --device=1 --instance=0 '--cable=USB-Blaster on localhost [USB-0]' --program --verbose 

