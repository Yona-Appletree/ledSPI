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

	////////////////
	// PREAMBLE

	PREP_GPIO_MASK_NAMED(all)

	// LOW for 220us
	PREP_GPIO_ADDRS_FOR_CLEAR()
	GPIO_APPLY_MASK_TO_ADDR()
	//SLEEPNS 220000, 4, wait_preamble_low
	WAITNS 220000, wait_preamble_low

	// HIGH for 113us
	PREP_GPIO_ADDRS_FOR_SET()
	GPIO_APPLY_MASK_TO_ADDR()
	//SLEEPNS 113000, 4, wait_preamble_high1
	WAITNS (220000+113000), wait_preamble_high1

	///////////////
	// ZERO FRAME

	// LOW 36us
	PREP_GPIO_ADDRS_FOR_CLEAR()
	GPIO_APPLY_MASK_TO_ADDR()

	//SLEEPNS 32500, 4, wait_zeroframe_low //4us happens in the beginning of the loop
	WAITNS (220000+113000+32000), wait_zeroframe_low //4us happens in the beginning of the loop

	// HIGH for 8us is handled by the first byte
	RESET_COUNTER

l_word_loop:
	// for bit in 24 to 0
	MOV r_bit_num, 0

	l_bit_loop:
		// Zero out the registers
		RESET_GPIO_ZEROS()

		///////////////////////////////////////////////////////////////////////
		// LOAD/BUILD DATA MASK
		LOAD_CHANNEL_DATA(24, 0, 16)

		TEST_BIT_ZERO(r_data0,  0)
		TEST_BIT_ZERO(r_data1,  1)
		TEST_BIT_ZERO(r_data2,  2)
		TEST_BIT_ZERO(r_data3,  3)
		TEST_BIT_ZERO(r_data4,  4)
		TEST_BIT_ZERO(r_data5,  5)
		TEST_BIT_ZERO(r_data6,  6)
		TEST_BIT_ZERO(r_data7,  7)
		TEST_BIT_ZERO(r_data8,  8)
		TEST_BIT_ZERO(r_data9,  9)
		TEST_BIT_ZERO(r_data10, 10)
		TEST_BIT_ZERO(r_data11, 11)
		TEST_BIT_ZERO(r_data12, 12)
		TEST_BIT_ZERO(r_data13, 13)
		TEST_BIT_ZERO(r_data14, 14)
		TEST_BIT_ZERO(r_data15, 15)

		LOAD_CHANNEL_DATA(24, 16, 8)

		TEST_BIT_ZERO(r_data0, 16)
		TEST_BIT_ZERO(r_data1, 17)
		TEST_BIT_ZERO(r_data2, 18)
		TEST_BIT_ZERO(r_data3, 19)
		TEST_BIT_ZERO(r_data4, 20)
		TEST_BIT_ZERO(r_data5, 21)
		TEST_BIT_ZERO(r_data6, 22)
		TEST_BIT_ZERO(r_data7, 23)

		// DATA MASK CREATED
		///////////////////////////////////////////////////////////////////////

		// Load the address(es) of the GPIO devices
		PREP_GPIO_MASK_NAMED(all)


		//////////////////////////
		// POTENTIAL STOP BIT
		// Stop bits happen after bits 0, 8, 16
		AND r_temp1, r_bit_num, 7
		QBNE skip_stop_bits, r_temp1, 0
			// Wait for last bit to finish
			WAITNS 4000, wait_lastframe_in_stop_end

			// HIGH (all)
			PREP_GPIO_ADDRS_FOR_SET()
			GPIO_APPLY_MASK_TO_ADDR()

			// Wait for 8us
			//SLEEPNS 8000, 4, wait_stop_bit
			WAITNS 12000, wait_stop_bit

			// Reset the counter so the bit-loop can get the 4us wait
			RESET_COUNTER

			// LOW (all) leading 0
			PREP_GPIO_ADDRS_FOR_CLEAR()
			GPIO_APPLY_MASK_TO_ADDR()

			// Wait for 4us
			// Don't actually wait here... reset the counter so the next bit handles the wait for better timing

		skip_stop_bits:

		// FRAME END
		//////////////////////////


		//////////////////////////
		// FRAME START
		INCREMENT r_bit_num

		// LOW (0s only):
//		PREP_GPIO_ADDRS_FOR_CLEAR()
//		GPIO_APPLY_ZEROS_TO_ADDR()
//
//		// Invert zeros to set ones
//		XOR r_gpio0_zeros, r_gpio0_zeros, r_gpio0_mask
//		XOR r_gpio1_zeros, r_gpio1_zeros, r_gpio1_mask
//		XOR r_gpio2_zeros, r_gpio2_zeros, r_gpio2_mask
//		XOR r_gpio3_zeros, r_gpio3_zeros, r_gpio3_mask
//
//		// HIGH (1s only):
//		PREP_GPIO_ADDRS_FOR_SET()
//		GPIO_APPLY_ZEROS_TO_ADDR()


		// Prep addresses for zero bits
		PREP_GPIO_ADDRS_FOR_CLEAR()

		// Prep a second set of addresses for the one bits so we don't have to spend extra time loading these
		// addresses between the 0 and 1 bit sends
		MOV r_data0, GPIO0 | GPIO_SETDATAOUT;
		MOV r_data1, GPIO1 | GPIO_SETDATAOUT;
		MOV r_data2, GPIO2 | GPIO_SETDATAOUT;
		MOV r_data3, GPIO3 | GPIO_SETDATAOUT;

		// Invert zeros (but only within the mask) and store them in a second set of registers to send the ones
		XOR r_data4, r_gpio0_zeros, r_gpio0_mask
		XOR r_data5, r_gpio1_zeros, r_gpio1_mask
		XOR r_data6, r_gpio2_zeros, r_gpio2_mask
		XOR r_data7, r_gpio3_zeros, r_gpio3_mask

		// Wait for last bit to finish
		WAITNS 4000, wait_lastframe_end

		// Start of the next timing frame
		RESET_COUNTER

		// Apply data to GPIO banks one at a time to reduce latency between the 0 and 1 switch per line
		SBBO r_gpio0_zeros, r_gpio0_addr, 0, 4;
		SBBO r_data4, r_data0, 0, 4;

		SBBO r_gpio1_zeros, r_gpio1_addr, 0, 4;
		SBBO r_data5, r_data1, 0, 4;

		SBBO r_gpio2_zeros, r_gpio2_addr, 0, 4;
		SBBO r_data6, r_data2, 0, 4;

		SBBO r_gpio3_zeros, r_gpio3_addr, 0, 4;
		SBBO r_data7, r_data3, 0, 4;


		// The bits finish in the next iteration
		QBNE l_bit_loop, r_bit_num, 24

	// The RGB streams have been clocked out
	// Move to the next pixel on each row
	ADD r_data_addr, r_data_addr, 48 * 4
	DECREMENT r_data_len
	QBNE l_word_loop, r_data_len, #0

	// Write the trailing high
	PREP_GPIO_MASK_NAMED(all)
	PREP_GPIO_ADDRS_FOR_SET()
	GPIO_APPLY_MASK_TO_ADDR()

	// HIGH for about 2ms
	SLEEPNS 2504800, 4, wait_end_high

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

#ifdef AM33XX
	// Send notification to Host for program completion
	MOV R31.b0, PRU_ARM_INTERRUPT+16
#else
	MOV R31.b0, PRU_ARM_INTERRUPT
#endif

	HALT
