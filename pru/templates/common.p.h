#ifndef _ws281x_HP_
#define _ws281x_HP_


#define AM33XX

// ***************************************
// *     Global Register Assignments     *
// ***************************************

#define r_data_addr r0
#define r_data_len r1

#define r_bit_num r6
#define r_sleep_counter r7
#define r_temp_addr r8
#define r_temp1 r9

#define r_gpio0_zeros r2
#define r_gpio1_zeros r3
#define r_gpio2_zeros r4
#define r_gpio3_zeros r5

#define r_gpio0_ones r2
#define r_gpio1_ones r3
#define r_gpio2_ones r4
#define r_gpio3_ones r5

#define r_data0 r10
#define r_data1 r11
#define r_data2 r12
#define r_data3 r13
#define r_data4 r14
#define r_data5 r15
#define r_data6 r16
#define r_data7 r17
#define r_data8 r18
#define r_data9 r19
#define r_data10 r20
#define r_data11 r21
#define r_data12 r22
#define r_data13 r23
#define r_data14 r24
#define r_data15 r25

#define r_gpio0_mask r20
#define r_gpio1_mask r21
#define r_gpio2_mask r22
#define r_gpio3_mask r23

#define r_gpio0_addr r24
#define r_gpio1_addr r25
#define r_gpio2_addr r26
#define r_gpio3_addr r27

#define r_temp2 r28

// ***************************************
// *      Global Macro definitions       *
// ***************************************

#ifdef AM33XX

// Refer to this mapping in the file - \prussdrv\include\pruss_intc_mapping.h
#define PRU0_PRU1_INTERRUPT     17
#define PRU1_PRU0_INTERRUPT     18
#define PRU0_ARM_INTERRUPT      19
#define PRU1_ARM_INTERRUPT      20
#define ARM_PRU0_INTERRUPT      21
#define ARM_PRU1_INTERRUPT      22

#define CONST_PRUDRAM   C24
#define CONST_SHAREDRAM C28
#define CONST_L3RAM     C30
#define CONST_DDR       C31

// Address for the Constant table Programmable Pointer Register 0(CTPPR_0)
#define CTBIR_0         0x22020
// Address for the Constant table Programmable Pointer Register 0(CTPPR_0)
#define CTBIR_1         0x22024

// Address for the Constant table Programmable Pointer Register 0(CTPPR_0)
#define CTPPR_0         0x22028
// Address for the Constant table Programmable Pointer Register 1(CTPPR_1)
#define CTPPR_1         0x2202C

#else

// Refer to this mapping in the file - \prussdrv\include\pruss_intc_mapping.h
#define PRU0_PRU1_INTERRUPT     32
#define PRU1_PRU0_INTERRUPT     33
#define PRU0_ARM_INTERRUPT      34
#define PRU1_ARM_INTERRUPT      35
#define ARM_PRU0_INTERRUPT      36
#define ARM_PRU1_INTERRUPT      37

#define CONST_PRUDRAM   C3
#define CONST_HPI       C15
#define CONST_DSPL2     C28
#define CONST_L3RAM     C30
#define CONST_DDR       C31

// Address for the Constant table Programmable Pointer Register 0(CTPPR_0)      
#define CTPPR_0         0x7028
// Address for the Constant table Programmable Pointer Register 1(CTPPR_1)      
#define CTPPR_1         0x702C

#endif

.macro  LD32
.mparam dst,src
    LBBO    dst,src,#0x00,4
.endm

.macro  LD16
.mparam dst,src
    LBBO    dst,src,#0x00,2
.endm

.macro  LD8
.mparam dst,src
    LBBO    dst,src,#0x00,1
.endm

.macro ST32
.mparam src,dst
    SBBO    src,dst,#0x00,4
.endm

.macro ST16
.mparam src,dst
    SBBO    src,dst,#0x00,2
.endm

.macro ST8
.mparam src,dst
    SBBO    src,dst,#0x00,1
.endm


#if PRU_NUM == 0
	#define PRU_CONTROL_ADDRESS 0x22000
	#define PRU_ARM_INTERRUPT PRU0_ARM_INTERRUPT
#elif PRU_NUM == 1
	#define PRU_CONTROL_ADDRESS 0x24000
	#define PRU_ARM_INTERRUPT PRU1_ARM_INTERRUPT
#else
	#error Invalid #PRU_NUM: PRU_NUM; must be 0 or 1
#endif


#define sp r0
#define lr r23
#define STACK_TOP       (0x2000 - 4)
#define STACK_BOTTOM    (0x2000 - 0x200)

#define NOP       mov r0, r0

.macro stack_init
    mov     sp, STACK_BOTTOM
.endm

.macro push
.mparam reg, cnt
    sbbo    reg, sp, 0, 4*cnt
    add     sp, sp, 4*cnt
.endm

.macro pop
.mparam reg, cnt
    sub     sp, sp, 4*cnt
    lbbo    reg, sp, 0, 4*cnt
.endm

.macro INCREMENT
.mparam reg
    add reg, reg, 1
.endm

.macro DECREMENT
.mparam reg
    sub reg, reg, 1
.endm


/** Sleep a given number of nanoseconds with 10 ns resolution.
 *
 * This busy waits for a given number of cycles.  Not for use
 * with things that must happen on a tight schedule.
 */
.macro SLEEPNS
.mparam ns,inst,lab
	MOV r_sleep_counter, (ns/10)-1-inst // ws2811 -- high speed
lab:
	SUB r_sleep_counter, r_sleep_counter, 1
	QBNE lab, r_sleep_counter, 0
.endm


/** Wait for the cycle counter to reach a given value */
.macro WAITNS
.mparam ns,lab
	MOV r_temp_addr, PRU_CONTROL_ADDRESS // control register

	// Instructions take 5ns and RESET_COUNTER takes about 20 instructions
	// this value was found through trial and error on the DMX signal
	// generation
	MOV r_temp2, (ns)/5 - 20
lab:
	LBBO r_temp1, r_temp_addr, 0xC, 4 // read the cycle counter
//	SUB r9, r9, r_sleep_counter
	QBGT lab, r_temp1, r_temp2
.endm

/** Used after WAITNS to jump to a label if too much time has elapsed */
.macro WAIT_TIMEOUT
.mparam timeoutNs, timeoutLabel
    // Check that we haven't waited too long (waiting for memory, etc...) and if we have, jump to a timeout label
    MOV r_temp2, ((timeoutNs)/5 - 20)
    QBGT timeoutLabel, r_temp2, r_temp1
.endm

/** Reset the cycle counter */
.macro RESET_COUNTER
		// Disable the counter and clear it, then re-enable it
		MOV r_temp_addr, PRU_CONTROL_ADDRESS // control register
		LBBO r9, r_temp_addr, 0, 4
		CLR r9, r9, 3 // disable counter bit
		SBBO r9, r_temp_addr, 0, 4 // write it back

		MOV r_temp2, 0
		SBBO r_temp2, r_temp_addr, 0xC, 4 // clear the timer

		SET r9, r9, 3 // enable counter bit
		SBBO r9, r_temp_addr, 0, 4 // write it back

		// Read the current counter value
		// Should be zero.
		// LBBO r_sleep_counter, r_temp_addr, 0xC, 4
.endm

/* Send an interrupt to the ARM*/
.macro RAISE_ARM_INTERRUPT
	#ifdef AM33XX
		MOV R31.b0, PRU_ARM_INTERRUPT+16
	#else
		MOV R31.b0, PRU_ARM_INTERRUPT
	#endif
.endm

// ***************************************
// *         LED Mapping Macros          *
// ***************************************

#define PREP_GPIO_ADDRS_FOR_CLEAR()     MOV r_gpio0_addr, GPIO0 | GPIO_CLEARDATAOUT; \
                                        MOV r_gpio1_addr, GPIO1 | GPIO_CLEARDATAOUT; \
                                        MOV r_gpio2_addr, GPIO2 | GPIO_CLEARDATAOUT; \
                                        MOV r_gpio3_addr, GPIO3 | GPIO_CLEARDATAOUT;

#define PREP_GPIO_ADDRS_FOR_SET()       MOV r_gpio0_addr, GPIO0 | GPIO_SETDATAOUT; \
                                        MOV r_gpio1_addr, GPIO1 | GPIO_SETDATAOUT; \
                                        MOV r_gpio2_addr, GPIO2 | GPIO_SETDATAOUT; \
                                        MOV r_gpio3_addr, GPIO3 | GPIO_SETDATAOUT;

#define PREP_GPIO_MASK_NAMED(maskName)  MOV r_gpio0_mask, GPIO_MASK(0, maskName); \
                                        MOV r_gpio1_mask, GPIO_MASK(1, maskName); \
                                        MOV r_gpio2_mask, GPIO_MASK(2, maskName); \
                                        MOV r_gpio3_mask, GPIO_MASK(3, maskName);

#define GPIO_APPLY_MASK_TO_ADDR()       SBBO r_gpio0_mask, r_gpio0_addr, 0, 4; \
                                        SBBO r_gpio1_mask, r_gpio1_addr, 0, 4; \
                                        SBBO r_gpio2_mask, r_gpio2_addr, 0, 4; \
                                        SBBO r_gpio3_mask, r_gpio3_addr, 0, 4;
                                        
#define GPIO_APPLY_ZEROS_TO_ADDR()      SBBO r_gpio0_zeros, r_gpio0_addr, 0, 4; \
                                        SBBO r_gpio1_zeros, r_gpio1_addr, 0, 4; \
                                        SBBO r_gpio2_zeros, r_gpio2_addr, 0, 4; \
                                        SBBO r_gpio3_zeros, r_gpio3_addr, 0, 4;
                                        
#define GPIO_APPLY_ONES_TO_ADDR()       SBBO r_gpio0_ones, r_gpio0_addr, 0, 4; \
                                        SBBO r_gpio1_ones, r_gpio1_addr, 0, 4; \
                                        SBBO r_gpio2_ones, r_gpio2_addr, 0, 4; \
                                        SBBO r_gpio3_ones, r_gpio3_addr, 0, 4;

#define _IMPL_CONCAT2(a,b) a ## b
#define CONCAT2(a,b) _IMPL_CONCAT2(a,b)

#define _IMPL_CONCAT3(a,b,c) a ## b ## c
#define CONCAT3(a,b,c) _IMPL_CONCAT3(a,b,c)

#define _IMPL_CONCAT4(a,b,c,d) a ## b ## c ## d
#define CONCAT4(a,b,c,d) _IMPL_CONCAT4(a,b,c,d)

#define PRU_CONSTANT(name) CONCAT4(pru, PRU_NUM, _, name)

#define CHANNEL_INFO(channelIndex,infoType) PRU_CONSTANT(channel##channelIndex##_##infoType)
#define CHANNEL_BANK_NAME(channelIndex) CONCAT2(gpio, CHANNEL_INFO(channelIndex, bank))
#define CHANNEL_BIT(channelIndex) CHANNEL_INFO(channelIndex, bit)
#define GPIO_MASK(gpioNum,maskName) PRU_CONSTANT(gpio##gpioNum##_##maskName##_mask)

/**
 * Zeros the registers used to store which bits should be zero for each GPIO bank
 */
#define RESET_GPIO_ZEROS() MOV r_gpio0_zeros, 0; \
                         MOV r_gpio1_zeros, 0; \
                         MOV r_gpio2_zeros, 0; \
                         MOV r_gpio3_zeros, 0;

/**
 * Zeros the registers used to store which bits should be one for each GPIO bank
 */
#define RESET_GPIO_ONES()  MOV r_gpio0_ones, 0; \
                         MOV r_gpio1_ones, 0; \
                         MOV r_gpio2_ones, 0; \
                         MOV r_gpio3_ones, 0;

/**
 * Checks if the bit indexed by the r_bit_num register in the regN register is a zero, and if so, sets the bit in the
 * corresponding _zeros register. CHANNEL_BANK_NAME and CHANNEL_BIT are used to lookup the bank and bit num.
 *
 * @param regN The data register to check. Generally one of the r_dataN registers.
 * @param channelIndex Which channel this check is for. Used to determine GPIO bank and bit.
 */
#define TEST_BIT_ZERO(regN,channelIndex) QBBS CONCAT3(channel_,channelIndex,_zero_skip), regN, r_bit_num; \
                                        SET CONCAT3(r_,CHANNEL_BANK_NAME(channelIndex),_zeros), CONCAT3(r_,CHANNEL_BANK_NAME(channelIndex),_zeros), CHANNEL_BIT(channelIndex); \
                                        CONCAT3(channel_,channelIndex,_zero_skip): ;

/**
 * Checks if the bit indexed by the r_bit_num register in the regN register is a one, and if so, sets the bit in the
 * corresponding _ones register. CHANNEL_BANK_NAME and CHANNEL_BIT are used to lookup the bank and bit num.
 *
 * @param regN The data register to check. Generally one of the r_dataN registers.
 * @param channelIndex Which channel this check is for. Used to determine GPIO bank and bit.
 */
#define TEST_BIT_ONE(regN,channelIndex) QBBC CONCAT3(channel_,channelIndex,_one_skip), regN, r_bit_num; \
                                        SET CONCAT3(r_,CHANNEL_BANK_NAME(channelIndex),_ones), CONCAT3(r_,CHANNEL_BANK_NAME(channelIndex),_ones), CHANNEL_BIT(channelIndex); \
                                        CONCAT3(channel_,channelIndex,_one_skip): ;

/**
 * Loads more LED channel data into the r_dataN registers.
 *
 * @param firstChannel The first channel index to load data for. Data for this channel will be loaded into r_data0
 * @param channelCount The number of channels to load data for. Must be <= 16
 */
#define LOAD_CHANNEL_DATA(channelsPerPru, firstChannel,channelCount) LBBO r_data0, r_data_addr, PRU_NUM*channelsPerPru*4+firstChannel*4, channelCount*4

// ***************************************
// *    Global Structure Definitions     *
// ***************************************

/** Mappings of the GPIO devices */
#define GPIO0 0x44E07000
#define GPIO1 0x4804c000
#define GPIO2 0x481AC000
#define GPIO3 0x481AE000

/** Offsets for the clear and set registers in the devices */
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194

#define NOP       mov r0, r0




#endif //_ws281x_HP_
