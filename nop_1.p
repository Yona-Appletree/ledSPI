// \file
 //* WS2801 LED strip driver for the BeagleBone Black.
 //*
 //* Outputs 12 parallel lines of SPI-like data to 24 GPIO pins on the beaglebone black from PRU0. Designed to be used
 //* conjunction with another similar program on PRU1.
 //*/


.origin 0
.entrypoint START

#include "ws281x.hp"


//===============================
// GPIO Pin Mapping


/** Register map */
#define data_addr r0
#define data_len r1
#define gpio0_ones r2
#define gpio1_ones r3
#define gpio2_ones r4
#define gpio3_ones r5
#define bit_num r6
#define sleep_counter r7
#define addr_reg r8
#define temp_reg r9
#define temp2_reg r27
// r10 - r26 are used for temp storage and bitmap processing


/** Sleep a given number of nanoseconds with 10 ns resolution.
 *
 * This busy waits for a given number of cycles.  Not for use
 * with things that must happen on a tight schedule.
 */
.macro SLEEPNS
.mparam ns,inst,lab
#ifdef CONFIG_WS2812
    MOV sleep_counter, (ns/5)-1-inst // ws2812 -- low speed
#else
    MOV sleep_counter, (ns/10)-1-inst // ws2811 -- high speed
#endif
lab:
    SUB sleep_counter, sleep_counter, 1
    QBNE lab, sleep_counter, 0
.endm


/** Wait for the cycle counter to reach a given value */
.macro WAITNS
.mparam ns,lab
    MOV r8, 0x24000 // control register
lab:
	LBBO r9, r8, 0xC, 4 // read the cycle counter
//	SUB r9, r9, sleep_counter
#ifdef CONFIG_WS2812
	QBGT lab, r9, 2*(ns)/5
#else
	QBGT lab, r9, (ns)/5
#endif
.endm

/** Reset the cycle counter */
.macro RESET_COUNTER
		// Disable the counter and clear it, then re-enable it
		MOV addr_reg, 0x24000 // control register
		LBBO r9, addr_reg, 0, 4
		CLR r9, r9, 3 // disable counter bit
		SBBO r9, addr_reg, 0, 4 // write it back

		MOV temp2_reg, 0
		SBBO temp2_reg, addr_reg, 0xC, 4 // clear the timer

		SET r9, r9, 3 // enable counter bit
		SBBO r9, addr_reg, 0, 4 // write it back

		// Read the current counter value
		// Should be zero.
		LBBO sleep_counter, addr_reg, 0xC, 4
.endm

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
    // Load the pointer to the buffer from PRU DRAM into r0 and the
    // length (in bytes-bit words) into r1.
    // start command into r2
    LBCO      data_addr, CONST_PRUDRAM, 0, 12

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
    MOV r8, 0x24000 // control register
    LBBO r2, r8, 0xC, 4
    SBCO r2, CONST_PRUDRAM, 12, 4

    // Go back to waiting for the next frame buffer
    QBA _LOOP

EXIT:
    // Write a 0xFF into the response field so that they know we're done
    MOV r2, #0xFF
    SBCO r2, CONST_PRUDRAM, 12, 4

#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU1_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU1_ARM_INTERRUPT
#endif

    HALT
