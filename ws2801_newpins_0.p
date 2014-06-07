// \file
 //* WS2801 LED strip driver for the BeagleBone Black.
 //*
 //* Outputs 12 parallel lines of SPI-like data to 24 GPIO pins on the beaglebone black from PRU0. Designed to be used
 //* conjunction with another similar program on PRU1.
 //*/


.origin 0
.entrypoint START

#include "ws281x.hp"

#define NOP       mov r0, r0

/*
First 24 pins in new layoutfrom pinmap.js
00: gpio0_bit0 11
01: gpio2_bit0 25
02: gpio2_bit1  1
03: gpio1_bit0 14
04: gpio0_bit1 26
05: gpio1_bit1 12
06: gpio2_bit2 4
07: gpio2_bit3 3
08: gpio2_bit4 6
09: gpio2_bit5 7
10: gpio2_bit5 9
11: gpio2_bit6  11
12: gpio2_bit7 13
13: gpio2_bit8 15
14: gpio2_bit9 16
15: gpio2_bit10 17
16: gpio2_bit11 23
17: gpio0_bit2 10
18: gpio0_bit3 9
19: gpio0_bit4 8
20: gpio2_bit12 14
21: gpio2_bit13 12
22: gpio2_bit14 10
23: gpio2_bit15 8
*/

//===============================
// GPIO Pin Mapping

// Clock and data pins in the order they should be taken from the data. The pins are interleaved, but separated here
// for clarity.

#define gpio0_bit0 11  // DATA
// First CLOCK...
#define gpio2_bit1  1  // DATA
#define gpio0_bit1 26  // DATA
#define gpio2_bit2 4   // DATA
#define gpio2_bit4 6   // DATA
#define gpio2_bit5 9   // DATA
#define gpio2_bit7 13  // DATA
#define gpio2_bit9 16  // DATA
#define gpio2_bit11 23 // DATA
#define gpio0_bit3 9   // DATA
#define gpio2_bit12 14 // DATA
#define gpio2_bit14 10 // DATA
// Last CLOCK....

// First Data
#define gpio2_bit0 25  // CLOCK
#define gpio1_bit0 14  // CLOCK
#define gpio1_bit1 12  // CLOCK
#define gpio2_bit3 3   // CLOCK
#define gpio2_bit5 7   // CLOCK
#define gpio2_bit6  11 // CLOCK
#define gpio2_bit8 15  // CLOCK
#define gpio2_bit10 17 // CLOCK
#define gpio0_bit2 10  // CLOCK
#define gpio0_bit4 8   // CLOCK
#define gpio2_bit13 12 // CLOCK
// Last Data
#define gpio2_bit15 8  // CLOCK

#define GPIO0_DATA_MASK (0\
|(1<<gpio0_bit0)\
|(1<<gpio0_bit1)\
|(1<<gpio0_bit3)\
)

#define GPIO1_DATA_MASK (0)

#define GPIO2_DATA_MASK (0\
|(1<<gpio2_bit1)\
|(1<<gpio2_bit2)\
|(1<<gpio2_bit4)\
|(1<<gpio2_bit5)\
|(1<<gpio2_bit7)\
|(1<<gpio2_bit9)\
|(1<<gpio2_bit11)\
|(1<<gpio2_bit12)\
|(1<<gpio2_bit14)\
)



#define GPIO0_CLOCK_MASK (0\
|(1<<gpio0_bit2)\
|(1<<gpio0_bit4)\
)

#define GPIO1_CLOCK_MASK (0\
|(1<<gpio1_bit0)\
|(1<<gpio1_bit1)\
)

#define GPIO2_CLOCK_MASK (0\
|(1<<gpio2_bit0)\
|(1<<gpio2_bit3)\
|(1<<gpio2_bit5)\
|(1<<gpio2_bit6)\
|(1<<gpio2_bit8)\
|(1<<gpio2_bit10)\
|(1<<gpio2_bit13)\
|(1<<gpio2_bit15)\
)


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
    MOV r8, 0x22000 // control register
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
		MOV addr_reg, 0x22000 // control register
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

WORD_LOOP:
	// for bit in 24 to 0
	MOV bit_num, 24

	BIT_LOOP:
		SUB bit_num, bit_num, 1
		/** Macro to generate the mask of which bits are zero.
		 * For each of these registers, set the
		 * corresponding bit in the gpio0_one register if
		 * the current bit is set in the strided register.
		 */
		#define TEST_BIT(regN,gpioN,bitN) \
			QBBC gpioN##_##regN##_skip, regN, bit_num; \
			SET gpioN##_ones, gpioN##_ones, gpioN##_##bitN ; \
			gpioN##_##regN##_skip: ;

		///////////////////////////////////////////////////////////////////////
		// Load 12 registers of data into r10-r21

		LBBO r10, r0, 0, 12*4
		MOV gpio0_ones, 0
		MOV gpio2_ones, 0

		TEST_BIT(r10, gpio0, bit0)      // Bit 0
		TEST_BIT(r11, gpio2, bit1)      // Bit 1
		TEST_BIT(r12, gpio0, bit1)      // Bit 2
		TEST_BIT(r13, gpio2, bit2)      // Bit 3
		TEST_BIT(r14, gpio2, bit4)      // Bit 4
		TEST_BIT(r15, gpio2, bit5)      // Bit 5
		TEST_BIT(r16, gpio2, bit7)      // Bit 6
		TEST_BIT(r17, gpio2, bit9)      // Bit 7
		TEST_BIT(r18, gpio2, bit11)     // Bit 8
		TEST_BIT(r19, gpio0, bit3)      // Bit 9
		TEST_BIT(r20, gpio2, bit12)     // Bit 10
		TEST_BIT(r21, gpio2, bit14)     // Bit 11

		// Data loaded
		///////////////////////////////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////////
		// Send the bits

		// Everything LOW
		MOV r23, GPIO0 | GPIO_CLEARDATAOUT
		MOV r24, GPIO1 | GPIO_CLEARDATAOUT
		MOV r25, GPIO2 | GPIO_CLEARDATAOUT

		MOV r20, GPIO0_DATA_MASK
		MOV r21, GPIO1_DATA_MASK
		MOV r22, GPIO2_DATA_MASK

		SBBO r20, r23, 0, 4
		SBBO r21, r24, 0, 4
		SBBO r22, r25, 0, 4

		// Data 1s HIGH
		MOV r23, GPIO0 | GPIO_SETDATAOUT
		MOV r24, GPIO1 | GPIO_SETDATAOUT
		MOV r25, GPIO2 | GPIO_SETDATAOUT
		SBBO gpio0_ones, r23, 0, 4
		SBBO gpio2_ones, r25, 0, 4

		// Wait for a moment before raising the clocks
		NOP

		// Clocks HIGH
		MOV r20, GPIO0_CLOCK_MASK
		MOV r21, GPIO1_CLOCK_MASK
		MOV r22, GPIO2_CLOCK_MASK
		SBBO r20, r23, 0, 4
		SBBO r21, r24, 0, 4
		SBBO r22, r25, 0, 4

		// Bits sent
		///////////////////////////////////////////////////////////////////////

		// We're done.
		QBNE BIT_LOOP, bit_num, 0

	// The RGB streams have been clocked out
	// Move to the next pixel on each row
	ADD data_addr, data_addr, 48 * 4
	SUB data_len, data_len, 1
	QBNE WORD_LOOP, data_len, #0

	// Final clear for the word
	MOV r20, GPIO0_DATA_MASK
	MOV r21, GPIO1_DATA_MASK
	MOV r10, GPIO0 | GPIO_CLEARDATAOUT
	MOV r11, GPIO1 | GPIO_CLEARDATAOUT

	SBBO r20, r10, 0, 4
	SBBO r21, r11, 0, 4

    // Delay at least 500 usec; this is the required reset
    // time for the LED strip to update with the new pixels.
    SLEEPNS 1000000, 1, reset_time

    // Write out that we are done!
    // Store a non-zero response in the buffer so that they know that we are done
    // aso a quick hack, we write the counter so that we know how
    // long it took to write out.
    MOV r8, 0x22000 // control register
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
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT
