// WS281x Signal Generation PRU Program Template
//
// Drives up to 24 strips using a single PRU. LEDscape (in userspace) writes rendered frames into shared DDR memory
// and sets a flag to indicate how many pixels are to be written.  The PRU then bit bangs the signal out the
// 24 GPIO pins and sets a "complete" flag.
//
// To stop, the ARM can write a 0xFF to the command, which will cause the PRU code to exit.
//
// At 800 KHz the ws281x signal is:
//  ____
// |  | |______|
// 0  250 600  1250 offset
//    250 350   650 delta
//
// each pixel is stored in 4 bytes in the order GRBA (4th byte is ignored)
//
// while len > 0:
//    for bit# = 23 down to 0:
//        write out bits
//    increment address by 32
//

//
//

// Mapping lookup

.origin 0
.entrypoint START

#include "common.p.h"
START:
	// Enable OCP master port
	// clear the STANDBY_INIT bit in the SYSCFG register,
	// otherwise the PRU will not be able to write outside the
	// PRU memory space and to the BeagleBon's pins.
	LBCO	r0, C4, 4, 4
	CLR		r0, r0, 4
	SBCO	r0, C4, 4, 4

	// Configure the programmable pointer register for PRU0 by setting
	// c28_pointer[15:0] field to 0x0120.  This will make C28 point to
	// 0x00012000 (PRU shared RAM).
	MOV		r0, 0x00000120
	MOV		r1, CTPPR_0
	ST32	r0, r1

	// Configure the programmable pointer register for PRU0 by setting
	// c31_pointer[15:0] field to 0x0010.  This will make C31 point to
	// 0x80001000 (DDR memory).
	MOV		r0, 0x00100000
	MOV		r1, CTPPR_1
	ST32	r0, r1

	// Write a 0x1 into the response field so that they know we have started
	MOV r2, #0x1
	SBCO r2, CONST_PRUDRAM, 12, 4


	MOV r20, 0xFFFFFFFF

	// Wait for the start condition from the main program to indicate
	// that we have a rendered frame ready to clock out.  This also
	// handles the exit case if an invalid value is written to the start
	// start position.
_LOOP:
	// Let ledscape know that we're starting the loop again. It waits for this
	// interrupt before sending another frame
	RAISE_ARM_INTERRUPT

	// Load the pointer to the buffer from PRU DRAM into r0 and the
	// length (in bytes-bit words) into r1.
	// start command into r2
	LBCO      r_data_addr, CONST_PRUDRAM, 0, 12

	// Wait for a non-zero command
	QBEQ _LOOP, r2, #0

	// Reset the sleep timer
	RESET_COUNTER

	// Zero out the start command so that they know we have received it
	// This allows maximum speed frame drawing since they know that they
	// can now swap the frame buffer pointer and write a new start command.
	MOV r3, 0
	SBCO r3, CONST_PRUDRAM, 8, 4

	// Command of 0xFF is the signal to exit
	QBEQ EXIT, r2, #0xFF

	// Output work would go here....but this is NOP

	// Write out that we are done!
	// Store a non-zero response in the buffer so that they know that we are done
	// aso a quick hack, we write the counter so that we know how
	// long it took to write out.
	MOV r8, PRU_CONTROL_ADDRESS // control register
	LBBO r2, r8, 0xC, 4
	SBCO r2, CONST_PRUDRAM, 12, 4

	// Go back to waiting for the next frame buffer
	QBA _LOOP

EXIT:
	// Write a 0xFF into the response field so that they know we're done
	MOV r2, #0xFF
	SBCO r2, CONST_PRUDRAM, 12, 4

	HALT
