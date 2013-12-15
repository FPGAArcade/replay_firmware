/*****************************************************************************
 *   swi_handler.s: SWI handler file ARM 
 *   tested with ARM77TDMI and ARM77TDMI-S 
 *
 *   by Martin Thomas 
 *   http://www.siwawi.arubi.uni-kl.de/avr_projects
 *   based on information from ARM documents
 *
 *****************************************************************************/
 
/* 
  20060902 - SWI-handler in section .init if vectors in ROM
             SWI-handler in section .fastrun if vectors in RAM
  20070827 - changed register restore after user-swi call
  20070830 - a lot of modifications - diff with old version for changes
*/

 
#include "swi_numbers.h"

.set SWI_IRQ_DIS,	SWI_NUM_IRQ_DIS
.set SWI_IRQ_EN,	SWI_NUM_IRQ_EN
.set SWI_FIQ_DIS,	SWI_NUM_FIQ_DIS
.set SWI_FIQ_EN,	SWI_NUM_FIQ_EN
.set SWI_GET_CPSR,	SWI_NUM_GET_CPSR
.set SWI_IRQ_REST,	SWI_NUM_IRQ_REST
.set SWI_FIQ_REST,	SWI_NUM_FIQ_REST

.set I_Bit, 0x80
.set F_Bit, 0x40
.set T_Bit, 0x20

#ifdef ROM_RUN
#ifdef VECTORS_IN_RAM
	.set VECTREMAPPED, 1
#else
	.set VECTREMAPPED, 0
#endif
#endif

#ifdef RAM_RUN
	.set VECTREMAPPED, 0
#endif


.text
.arm

/* Handler located in RAM if exception-vector in RAM */
.if (VECTREMAPPED)
	.print "SWI-Handler in section .fastrun -> .data"
	.section .fastrun, "ax"
.else
	.print "SWI-Handler in section .init -> .text"
	.section .init, "ax"
.endif


/* for external c-function (dummy provided at EOF if no 
   c-function should be used) */
.extern SWI_Handler_User

  
.global SoftwareInterruptASM
.func   SoftwareInterruptASM
SoftwareInterruptASM:
	STMFD   sp!, {r4, r5, lr}       /* store regs. */
	MRS     r5, spsr               /* load status-reg */
	TST     r5, #T_Bit             /* test for thumb */
	LDRNEH  r4, [lr, #-2]          /* NE->thumb - get swi instruction code */
	BICNE   r4, r4, #0xff00        /* NE->thumb - clear top 8 bits leaving swi "comment field"=number */
	LDREQ   r4, [lr, #-4]          /* EQ->arm - get swi instruction code */
	BICEQ   r4, r4, #0xff000000    /* EQ->arm - clear top 8 bits leaving swi "comment field"=number */
	CMP     r4, #MAX_SWI           /* range-check */
	LDRLS   pc, [pc, r4, LSL #2]   /* jump to routine if <= MAX (LS) */

/* if none from the predefined SWIs match call the user´s-handler */
	B       UserSWIHandler

/* Jump-Table */
SwiTableStart:
	.word IRQDisable	// 0
	.word IRQEnable		// 1
	.word FIQDisable	// 2
	.word FIQEnable		// 3
	.word CPSRget		// 4
	.word IRQRestore	// 5
	.word FIQRestore	// 6
SwiTableEnd:
	.set MAX_SWI, ((SwiTableEnd-SwiTableStart)/4)-1

IRQDisable:
	MOV     r0, r5                /* return value (SPSR on function entry) */
	ORR     r5, r5, #I_Bit        /* I_Bit set */
	B       EndofSWI_loc

IRQEnable:
	MOV     r0, r5                /* return value */
	BIC     r5, r5, #I_Bit        /* I_Bit clear */
	MSR     SPSR_c, r4            /* Set SPSR */
	B       EndofSWI_loc
 
FIQDisable:
	MOV     r0, r5                /* return value */
	ORR     r5, r5, #F_Bit
	B       EndofSWI_loc

FIQEnable:
	MOV     r0, r5                /* return value */
	BIC     r5, r5, #F_Bit
	B       EndofSWI_loc

CPSRget:
	// LDR r0, =0xdeadbeef
	MOV     r0, r5
	B       EndofSWI_loc

IRQRestore:
	AND     r0, r0, #I_Bit         /* Mask I-Bit in input */
	TST     r0, #I_Bit             /* Test input for I_Bit */
	BICEQ   r5, r5, #I_Bit
	ORRNE   r5, r5, #I_Bit
	B       EndofSWI_loc

FIQRestore:
	AND     r0, r0, #F_Bit         /* Mask F-Bit in input */
	TST     r0, #F_Bit             /* Test input for F_Bit */
	BICEQ   r5, r5, #F_Bit
	ORRNE   r5, r5, #F_Bit
	B       EndofSWI_loc


UserSWIHandler:
	stmfd   sp!,{r1-r12}
	ldr     r12, =SWI_Handler_User
	mov     r3, r4 /* pass SWI-Number as Param */
	mov     lr, pc
	bx      r12    /* call c-function */
	ldmfd   sp!, {r1-r12}  /* wrong: ldmfd sp!, {r1-r12,pc}^ */  /* return-value in r0 */
/*	b       EndofSWI*/

EndofSWI_loc:
	MSR     SPSR_c, r5             /* Set SPSR */
	LDMFD   sp!, {r4, r5, pc}^     /* restore regs and return */

.endfunc


/* empty handler used if no C-function is provided */
.weak SWI_Handler_User
SWI_Handler_User:
	bx lr


.end
