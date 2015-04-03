// APA102 Signal Generation PRU Program Template
//
// Drives up to 12 strips using a single PRU. LEDscape (in userspace) writes rendered frames into shared DDR memory
// and sets a flag to indicate how many pixels are to be written.  The PRU then bit bangs the signal out the
// 24 GPIO pins and sets a "complete" flag.
//
// To stop, the ARM can write a 0xFF to the command, which will cause the PRU code to exit.
//
// Implementation does not try and stick to any specific clock speed, just pushes out data as fast as it can (about 1.6mhz).
// 
// [ start frame ][   LED1   ][   LED2   ]...[   LEDN   ][ end frame ]
// [ 32bit x 0   ][0xFF 8 8 8][0xFF 8 8 8]...[0xFF 8 8 8][ (n/2) * 1 ]
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

// send the start frame
l_start_frame:
	MOV r_bit_num, 32
	
	// store number of leds in r29
	MOV r29, r_data_len

	RESET_GPIO_ONES()

	// 32 bits of 0
	l_start_bit_loop:

		DECREMENT r_bit_num

		// Clocks HIGH
		PREP_GPIO_ADDRS_FOR_SET()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()
			
		// lower clock and data
		PREP_GPIO_ADDRS_FOR_CLEAR()
		PREP_GPIO_MASK_NAMED(all)
		GPIO_APPLY_MASK_TO_ADDR()
		
		QBNE l_start_bit_loop, r_bit_num, #0


l_word_loop:
	// first 8 bits will be 0xFF (global brightness always at maximum)
	MOV r_bit_num, 8

	RESET_GPIO_ONES()

	l_header_bit_loop:
		DECREMENT r_bit_num

		// Clocks HIGH
		PREP_GPIO_ADDRS_FOR_SET()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()
		
		// lower clock and raise data
		PREP_GPIO_ADDRS_FOR_SET()
		PREP_GPIO_MASK_NAMED(even)
		GPIO_APPLY_MASK_TO_ADDR()

		PREP_GPIO_ADDRS_FOR_CLEAR()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()
		
		QBNE l_header_bit_loop, r_bit_num, #0


	// for bit in 24 to 0
	MOV r_bit_num, 24

	l_bit_loop:
		DECREMENT r_bit_num

		// Zero out the registers
		RESET_GPIO_ONES()

		///////////////////////////////////////////////////////////////////////
		// Load 12 registers of data into r10-r21

		LOAD_CHANNEL_DATA(12, 0, 12)

		// Test for ones
		TEST_BIT_ONE(r_data0,  0)
		TEST_BIT_ONE(r_data1,  2)
		TEST_BIT_ONE(r_data2,  4)
		TEST_BIT_ONE(r_data3,  6)
		TEST_BIT_ONE(r_data4,  8)
		TEST_BIT_ONE(r_data5,  10)
		TEST_BIT_ONE(r_data6,  12)
		TEST_BIT_ONE(r_data7,  14)
		TEST_BIT_ONE(r_data8,  16)
		TEST_BIT_ONE(r_data9,  18)
		TEST_BIT_ONE(r_data10, 20)
		TEST_BIT_ONE(r_data11, 22)

		// Data loaded
		///////////////////////////////////////////////////////////////////////

		///////////////////////////////////////////////////////////////////////
		// Send the bits
		
		// Clocks HIGH
		PREP_GPIO_ADDRS_FOR_SET()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()

		// set all data LOW
		PREP_GPIO_ADDRS_FOR_CLEAR()
		PREP_GPIO_MASK_NAMED(even)
		GPIO_APPLY_MASK_TO_ADDR()

		// Data 1s HIGH
		PREP_GPIO_ADDRS_FOR_SET()
		GPIO_APPLY_ONES_TO_ADDR()

		// Clocks LOW
		PREP_GPIO_ADDRS_FOR_CLEAR()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()

		// Bits sent
		///////////////////////////////////////////////////////////////////////

		QBNE l_bit_loop, r_bit_num, #0
	//end l_bit_loop

	// The RGB streams have been clocked out
	// Move to the next pixel on each row
	ADD r_data_addr, r_data_addr, 48 * 4
	DECREMENT r_data_len
	QBNE l_word_loop, r_data_len, #0
	

l_end_frame:
	// send end frame bits
	MOV r_bit_num, r29
	LSR r_bit_num, r_bit_num, 1
	ADD r_bit_num, r_bit_num, 1

	RESET_GPIO_ONES()

	// numleds / 2 bits of 1
	

	l_end_bit_loop:
		DECREMENT r_bit_num

		// Clocks HIGH
		PREP_GPIO_ADDRS_FOR_SET()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()
	
		// raise data
	        PREP_GPIO_ADDRS_FOR_SET()
        	PREP_GPIO_MASK_NAMED(even)
	        GPIO_APPLY_MASK_TO_ADDR()
		
		// Clocks LOW
		PREP_GPIO_ADDRS_FOR_CLEAR()
		PREP_GPIO_MASK_NAMED(odd)
		GPIO_APPLY_MASK_TO_ADDR()

		QBNE l_end_bit_loop, r_bit_num, #0
	
	PREP_GPIO_ADDRS_FOR_CLEAR()
	PREP_GPIO_MASK_NAMED(all)
	GPIO_APPLY_MASK_TO_ADDR()


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

	RAISE_ARM_INTERRUPT

	HALT

