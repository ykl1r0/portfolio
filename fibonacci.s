/*
 * File: fibonacci_7seg.s
 * Description: Computes Fibonacci numbers in Decimal across six displays.
 *              Once overflowed, loops the "OVAFLO" alert infinitely.
 * Target: CPulator ARMv7-A Architecture (DE1-SoC Core Registers)
 */

    .syntax unified
    .arm                            // Standard 32-bit ARM instruction mode

    // --- HARDWARE PERIPHERAL ADDRESSES ---
    .equ HEX_BASE_L,    0xFF200020  // Controls HEX3, HEX2, HEX1, HEX0
    .equ HEX_BASE_H,    0xFF200030  // Controls HEX5, HEX4
    .equ DELAY_VAL,     0x1FFFFF    // Master delay to stabilize visuals at clock speed
    .equ ALERT_DELAY,   0x3FFFFF    // Blinking speed for the infinite overflow loop

    .text
    .global _start
    .align 2

_start:
    // Reset sequence registers to initial values
    MOV R0, #0                      // F(n-2) = 0
    MOV R1, #1                      // F(n-1) = 1

fib_loop:
    // 1. Compute next Fibonacci term: F(n) = F(n-1) + F(n-2)
    ADD R2, R1, R0                  // R2 = New Fibonacci number

    // 2. Check if value exceeds 6 digits (999,999)
    LDR R3, =999999
    CMP R2, R3
    BHI infinite_overflow_loop      // Lock into infinite error loop once overflowed

    // 3. Update registers for the next step iteration
    MOV R0, R1                      // F(n-2) becomes old F(n-1)
    MOV R1, R2                      // F(n-1) becomes computed F(n)

    // 4. Save current context state before conversion routine
    PUSH {R0-R2}
    MOV R0, R2                      // Pass computed number into R0 for processing
    BL display_decimal
    POP {R0-R2}

    // 5. Native CPU delay loop to control display refresh sequence
    LDR R3, =DELAY_VAL
delay_loop:
    SUBS R3, R3, #1
    BNE delay_loop

    B fib_loop                      // Advance to next Fibonacci number


// --- INFINITE OVERFLOW LOOP (NEVER RESETS TO 0) ---
infinite_overflow_loop:
    LDR R4, =HEX_BASE_L
    LDR R5, =HEX_BASE_H
    
    // Step A: Load the bit patterns to display "OVAFLO"
    LDR R8, =0x7771383F             // "AFLO" -> HEX3 to HEX0
    LDR R9, =0x00003F3E             // "OV"   -> HEX5 and HEX4
    STR R8, [R4]                    // Turn on lower displays
    STR R9, [R5]                    // Turn on upper displays

    // Pause while text is fully visible
    LDR R3, =ALERT_DELAY
visible_delay:
    SUBS R3, R3, #1
    BNE visible_delay

    // Step B: Clear all displays (turn them dark) to create a flash effect
    MOV R8, #0
    MOV R9, #0
    STR R8, [R4]
    STR R9, [R5]

    // Pause while screen is blank
    LDR R3, =ALERT_DELAY
blank_delay:
    SUBS R3, R3, #1
    BNE blank_delay

    B infinite_overflow_loop        // Loop back up to maintain infinite blinking state


// --- BINARY TO DECIMAL 7-SEGMENT CONVERSION FUNCTION ---
display_decimal:
    PUSH {R4, R5, R6, R7, R8, R9, R10, R11, LR} 
    LDR R4, =HEX_BASE_L             // Base pointer for lower bank
    LDR R11, =HEX_BASE_H            // Base pointer for upper bank
    MOV R5, #0                      // Display loop index (0 to 5)
    MOV R8, #0                      // Accumulates lower 32-bits (HEX3-HEX0)
    MOV R10, #0                     // Accumulates upper 16-bits (HEX5-HEX4)

unpack_digits_loop:
    CMP R5, #6
    BEQ write_to_hardware           // Stop once all 6 displays are constructed

    // Call hardware subtraction routine to calculate R0 / 10
    MOV R6, #10
    MOV R7, #0                      // Initialize quotient accumulator to 0

divide_by_10_loop:
    CMP R0, R6
    BLO divide_done                 // If remaining value < 10, division complete
    SUB R0, R0, R6                  // Subtract 10
    ADD R7, R7, #1                  // Increment quotient count
    B divide_by_10_loop

divide_done:
    MOV R3, R0                      // Remainder is our single base-10 digit
    MOV R0, R7                      // Set quotient back into R0 for the next scale position

    // Convert raw single digit number to active segment byte configuration
    BL get_segment_pattern          // Returns byte map in R3
    
    // Sort the unpacked byte into the lower register bank or upper register bank
    CMP R5, #4
    BGE pack_upper_bank

    // Lower register pack (HEX3 - HEX0) -> Displays 0, 1, 2, 3
    LSL R9, R5, #3                  // Shift = index * 8
    LSL R3, R3, R9
    ORR R8, R8, R3
    B advance_loop

pack_upper_bank:
    // Upper register pack (HEX5 - HEX4) -> Displays 4, 5
    SUB R6, R5, #4                  // Normalize index offset to 0 or 1
    LSL R9, R6, #3                  // Shift = normalized index * 8
    LSL R3, R3, R9
    ORR R10, R10, R3

advance_loop:
    ADD R5, R5, #1                  // Advance to next display index position
    B unpack_digits_loop

write_to_hardware:
    STR R8, [R4]                    // Explicit write to lower bank address (0xFF200020)
    STR R10, [R11]                  // Explicit write to upper bank address (0xFF200030)
    POP {R4, R5, R6, R7, R8, R9, R10, R11, PC} 


// --- 7-SEGMENT HARDWARE BIT PATTERN LOOKUP FUNCTION ---
get_segment_pattern:
    PUSH {R6}                      
    ADR R6, PATTERN_TABLE
    LDRB R3, [R6, R3]
    POP {R6}
    BX LR

    .align 2
PATTERN_TABLE:
    //      0     1     2     3     4     5     6     7     8     9
    .byte 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
