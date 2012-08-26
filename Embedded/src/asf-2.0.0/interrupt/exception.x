/* This file is part of the AVR Software Framework 2.0.0 release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief Exception and interrupt vectors.
 *
 * This file maps all events supported by an AVR32.
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an INTC module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */

#if !__AVR32_UC__ && !__AVR32_AP__
  #error Implementation of the AVR32 architecture not supported by the INTC driver.
#endif


#include <avr32/io.h>


//! @{
//! \verbatim


  .section  .evba, "ax", @progbits

//  (.balign 0x200) alignment has been declared in linker script!

// Start of Exception Vector Table.
  // Export symbol.
  .global _evba
  .type _evba, @function
_evba:

        .org  0x000
        // Unrecoverable Exception.
_handle_Unrecoverable_Exception:
        rjmp default_exception_handler
        
        .org  0x004
        // TLB Multiple Hit.
_handle_TLB_Multiple_Hit:
        rjmp default_exception_handler

        .org  0x008
        // Bus Error Data Fetch.
_handle_Bus_Error_Data_Fetch:
        rjmp default_exception_handler

        .org  0x00C
         // Bus Error Instruction Fetch.
_handle_Bus_Error_Instruction_Fetch:
        rjmp default_exception_handler

        .org  0x010
        // NMI.
_handle_NMI:
        rjmp default_exception_handler

        .org  0x014
        // Instruction Address.
_handle_Instruction_Address:
        rjmp default_exception_handler

        .org  0x018
        // ITLB Protection.
_handle_ITLB_Protection:
        rjmp default_exception_handler

        .org  0x01C
        // Breakpoint.
_handle_Breakpoint:
        rjmp default_exception_handler

        .org  0x020
        // Illegal Opcode.
_handle_Illegal_Opcode:
        rjmp default_exception_handler

        .org  0x024
        // Unimplemented Instruction.
_handle_Unimplemented_Instruction:
        rjmp default_exception_handler

        .org  0x028
        // Privilege Violation.
_handle_Privilege_Violation:
        rjmp default_exception_handler

        .org  0x02C
        // Floating-Point: UNUSED IN AVR32UC and AVR32AP.
_handle_Floating_Point:
        rjmp default_exception_handler

        .org  0x030
        // Coprocessor Absent: UNUSED IN AVR32UC.
_handle_Coprocessor_Absent:
        rjmp default_exception_handler

        .org  0x034
        // Data Address (Read).
_handle_Data_Address_Read:
        rjmp default_exception_handler

        .org  0x038
        // Data Address (Write).
_handle_Data_Address_Write:
        rjmp default_exception_handler

        .org  0x03C
        // DTLB Protection (Read).
_handle_DTLB_Protection_Read:
        rjmp default_exception_handler

        .org  0x040
        // DTLB Protection (Write).
_handle_DTLB_Protection_Write:
        rjmp default_exception_handler

        .org  0x044
        // DTLB Modified: UNUSED IN AVR32UC.
_handle_DTLB_Modified:
        rjmp default_exception_handler

        .org  0x050
        // ITLB Miss.
_handle_ITLB_Miss:
        rjmp default_exception_handler

        .org  0x060
        // DTLB Miss (Read).
_handle_DTLB_Miss_Read:
        rjmp default_exception_handler

        .org  0x070
        // DTLB Miss (Write).
_handle_DTLB_Miss_Write:
        rjmp default_exception_handler

        .org  0x100
        // Supervisor Call.
_handle_Supervisor_Call:
        lda.w   pc, SCALLYield

		
//! \endverbatim
//! @}
