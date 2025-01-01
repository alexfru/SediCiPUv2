# SediCiPU 2: ISA

sedici - 16 in Italian

Specification version: 0.9

CC BY 2024 Alexey Frunze  
This text is licensed under a [Creative Commons Attribution
4.0 International license.](https://creativecommons.org/)


## Table of contents

[1 Intro](#1-intro)

[2 ISA features at a glance](#2-isa-features-at-a-glance)

[3 Registers](#3-registers)

[4 Instruction set](#4-instruction-set)

[4.1 Instruction descriptions](#41-instruction-descriptions)

[4.1.1 Memory loads and stores: lb, lw, sb, sw](#411-memory-loads-and-stores-lb-lw-sb-sw)

[4.1.2 Stack loads and stores: pop, push, ls5r, ss5r](#412-stack-loads-and-stores-pop-push-ls5r-ss5r)

[4.1.3 Other memory instructions: addm, subm, incm, decm, dincm, ddecm](#413-other-memory-instructions-addm-subm-incm-decm-dincm-ddecm)

[4.1.4 Non-memory data moves: li, mov, add, m2f, mf2](#414-non-memory-data-moves-li-mov-add-m2f-mf2)

[4.1.5 Carry flag manipulation: stc](#415-carry-flag-manipulation-stc)

[4.1.6 Single-register ALU operations: zxt, sxt, cpl, neg, adcz](#416-single-register-alu-operations-zxt-sxt-cpl-neg-adcz)

[4.1.7 Regular ALU: add, addu, adc, sub, sbb, cmp, and, or, xor](#417-regular-alu-add-addu-adc-sub-sbb-cmp-and-or-xor)

[4.1.8 Shifts and rotates: sr, sl, asr, rl, rr](#418-shifts-and-rotates-sr-sl-asr-rl-rr)

[4.1.9 Address formation: lurpc](#419-address-formation-lurpc)

[4.1.10 Multiplication and division: sac, add22adc33, cadd24, cadd24adc3z, csub34](#4110-multiplication-and-division-sac-add22adc33-cadd24-cadd24adc3z-csub34)

[4.1.11 Jumps, nop, subroutines](#4111-jumps-nop-subroutines)

[4.1.12 Interrupts: swi, external/HW ints, reti, di, ei](#4112-interrupts-swi-externalhw-ints-reti-di-ei)

[4.1.13 Logical to physical mapping/translation](#4113-logical-to-physical-mappingtranslation)

[4.2 Code examples](#42-code-examples)

[4.2.1 Loading a 16-bit const](#421-loading-a-16-bit-const)

[4.2.2 Adding a 16-bit const](#422-adding-a-16-bit-const)

[4.2.3 Multiplying by a constant (using "shift and accumulate")](#423-multiplying-by-a-constant-using-shift-and-accumulate)

[4.2.4 8-bit rotation](#424-8-bit-rotation)

[4.2.5 Clearing low bits while preserving high bits](#425-clearing-low-bits-while-preserving-high-bits)

[4.2.6 Resetting Carry flag](#426-resetting-carry-flag)

[4.2.7 Setting Carry flag](#427-setting-carry-flag)

[4.2.8 Loading/storing with larger offset/immediate, unaligned load/store](#428-loadingstoring-with-larger-offsetimmediate-unaligned-loadstore)

[4.2.9 Jump (unconditional) with larger distance and indirection](#429-jump-unconditional-with-larger-distance-and-indirection)

[4.2.10 Jump and link with larger distance and indirection](#4210-jump-and-link-with-larger-distance-and-indirection)

[4.2.11 Conditional jump with larger distance](#4211-conditional-jump-with-larger-distance)

[4.2.12 16-bit multiplication](#4212-16-bit-multiplication)

[4.2.13 Widening unsigned 16-bit multiplication](#4213-widening-unsigned-16-bit-multiplication)

[4.2.14 Unsigned 16-bit division and modulo/remainder](#4214-unsigned-16-bit-division-and-moduloremainder)

[4.2.15 16-bit binary to decimal conversion](#4215-16-bit-binary-to-decimal-conversion)

[4.2.16 Subroutine prolog and epilog](#4216-subroutine-prolog-and-epilog)

[4.2.17 ISRs](#4217-isrs)

[4.2.17.1 Hardware ISRs](#42171-hardware-isrs)

[4.2.17.2 Software ISRs](#42172-software-isrs)

[4.3 Instruction encodings](#43-instruction-encodings)

[4.3.1 Overall encodings](#431-overall-encodings)

[4.3.2 Further encodings in special cases](#432-further-encodings-in-special-cases)

[4.3.2.1 addm dst, (src + ximm7)](#4321-addm-dst-src--ximm7)

[4.3.2.2 subm dst, (src + ximm7)](#4322-subm-dst-src--ximm7)

[4.3.2.3 mrs r, s](#4323-mrs-r-s)

[4.4 Instruction set summary](#44-instruction-set-summary)

[4.5 The mini](#45-the-mini)

[4.5.1 Overall mini encodings](#451-overall-mini-encodings)

[4.5.2 What didn't make it into the mini](#452-what-didnt-make-it-into-the-mini)

[5 Memory accesses](#5-memory-accesses)

[5.1 Logical and physical addresses](#51-logical-and-physical-addresses)

[5.2 Program/code and data spaces](#52-programcode-and-data-spaces)

[5.3 Mapping/translation](#53-mappingtranslation)

[5.4 Multitasking](#54-multitasking)

[6 Proposed ABI](#6-proposed-abi)

[7 Proposed assembler macro instructions](#7-proposed-assembler-macro-instructions)


## 1 Intro

This non-pipelined 16-bit architecture is relatively simple and
straightforward for implementation. In some respects it's even simpler than
the earlier SediCiPU. But it is quite powerful and it comes with a
proof-of-concept implementation using Logisim-evolution and its library of
the 7400 series of chips.

It would be nice to build a microcomputer around this, possibly using an
FPGA board with a timer (for multitasking and time keeping), VGA output,
keyboard input and non-volatile storage, maybe a serial port, too.


## 2 ISA features at a glance

- mostly RISC instructions, every instruction being 16 bits long
- mostly orthogonal/uniform, rarely with hard-coded operands
- 6 16-bit GPRs: r0 through r5 (no hardwired zero register)
- 16-bit stack pointer (AKA sp/r6), 16-bit program counter (AKA pc/r7)
- flags register (arithmetic flags (O(verflow),S(ign),Z(ero),C(arry)),
  interrupt status/control)
- byte and word memory addressing (16-bit words naturally aligned,
  little-endian)
- memory addresses in loads/stores: register+immediate, register+register
  (both forms include pc-relative addressing, helping create position-
  independent code)
- 8 memory selector registers to map/translate logical addresses to physical
  memory addresses (64KB program/code space and 64KB data space can be mapped
  independently to anywhere in a maximum of 4MB of physical RAM in blocks
  of 16KB)
- composable load, store, jump instructions for full 16-bit range of
  value/address/distance (possibly with pc-relative addressing, also helping
  create position-independent code)
- common binary ALU ops on register & register, register & immediate (with
  few exceptions): add, adc, sub, sbb, cmp, and, or, xor, sr, sl, rr, rl, asr
- common unary ALU ops: zxt, sxt, cpl, neg, also adcz
- 14 conditional jumps: { <, <=, >=, > } x { signed, unsigned }, ==, !=,
  < 0, >= 0, overflow, no overflow
- special instructions to speed up multiplication and division
- special memory-accessing instructions for faster and denser code
- software interrupts (AKA SWIs), a few dozens of entry points
- external/hardware interrupts (6 IRQs; for multitasking and possibly nested
  IRQ ISRs), external/hardware interrupt enable flag
- I/O is MMIO (although it's possible to extend the set of system registers
  manipulated by the msr/mrs instructions that currently control only the
  memory selector registers)
- no protection, but it should be easy at the very least to write-protect
  the system memory from writes originating outside of the system memory


## 3 Registers

These are the architectural registers:
- r0...r4: 16-bit GPRs
- r5 = 16-bit GPR (also serves as the link/return address register and
  a temporary register)
- r6 = 16-bit sp (bit 0 is always 0)
- r7 = 16-bit pc (bit 0 is always 0), 0 upon reset
- flags register:  

      bits 15-10: IRQ5 through IRQ0 masks: a 0 doesn't let the corresponding
                  IRQ to generate an interrupt and invoke the ISR, while a 1
                  lets this happen (provided, the external/hardware interrupt
                  enable flag is set to 1, see below)
      
      bits 9-4: when read: indicate whether an IRQ (IRQ5 through IRQ0)
                is requested;
      bits 9-4: when written: used to clear requested IRQs (IRQ5 through
                IRQ0) in negative logic (that is, 1 preserves the IRQ
                requested flag, 0 clears it)
      
      bits 3-0: arithmetic flags: O(verflow), S(ign), Z(ero), C(arry)

- external/hardware interrupt enable flag (separate from flags register),  
  0 upon reset (interrupts disabled from all IRQs (IRQ5 through IRQ0))
- sel0...sel3: instruction fetches and pc-relative memory accesses map the 4
               16KB subregions of the 64KB program/code space to 16KB blocks
               of physical memory through these selector registers;  
               sel0 = 0 upon reset (suitable for BOOT ROM)
- sel4...sel7: non-pc-relative memory accesses map the 4 16KB subregions
               of the 64KB data space to 16KB blocks of physical memory
               through these selector registers;  
               sel4 = 0 upon reset


## 4 Instruction set

Every instruction is 16 bits long and starts at an even memory address.

There are approximately 85 instructions.

Most instructions take 2 clock cycles (fetch and execute), some take 3
(fetch and 2 executes).


### 4.1 Instruction descriptions

#### 4.1.1 Memory loads and stores: lb, lw, sb, sw

- lb/lw r, (r + ximm7/r)

  Loads an 8-bit byte (with zero extension to 16 bits) or a 16-bit word to
  a register from memory at address that is the sum of a register and
  either a 7-bit immediate or another register.

  The 7-bit immediate is signed (and therefore sign-extended to 16 bits),
  unless it's being added to the stack pointer register (sp), in which case
  it's unsigned (and therefore zero-extended to 16 bits).

  If the address is formed using the program counter register (pc),
  the immediate is added not to the pc of the load instruction but to the
  pc of the immediately following instruction (that is, original pc + 2).

  If the address is formed using the program counter register (pc),
  program/code memory is accessed, otherwise data memory is accessed.

  If the address is formed by adding two registers, the first/leftmost
  of the two cannot be sp or pc, while the last/rightmost can be.

  16-bit words must be naturally aligned, that is, at addresses that are
  even. They are stored in memory in the little-endian byte order.

  Note, the data area at addresses below sp should not be accessed when
  external/hardware interrupts are enabled because ISRs will overwrite it.
  This is why we can use the 7-bit immediate as unsigned when added to
  sp. This lets us access more of local variables and subroutine arguments
  on the stack at or above sp without doing more complex address arithmetic.

- sb/sw r, (r + ximm7/r)

  Stores the least significant 8 bits of a register or the entire 16-bit
  register to memory at address that is the sum of a register and either a
  7-bit immediate or another register.

  The instruction can't store store a register to a location formed using
  that same register, examples:

      sb r1, (r1 + 2)  ; non-encodable instruction
      sw r3, (r3 + r4) ; non-encodable instruction

  Note, the instruction cannot store pc.

  See lb/lw for other details, which apply for sb/sw as well.


#### 4.1.2 Stack loads and stores: pop, push, ls5r, ss5r

- push simm7/r

  Decrements sp by 2 and stores the specified 7-bit immediate
  (sign-extended to 16 bits) or register to memory at the new address
  contained in sp. You cannot push sp or pc.

- pop r

  Loads the specified register from memory at address contained in sp and
  then increments sp by 2. You cannot pop sp or pc.

- ls5r r

  Loads r5 from memory at address contained in sp and loads the specified
  register (r0 through r4) from memory at address that is the sum of sp
  and 2.

- ss5r r

  Stores r5 to memory at address contained in sp and stores the specified
  register (r0 through r4) to memory at address that is the sum of sp
  and 2.


#### 4.1.3 Other memory instructions: addm, subm, incm, decm, dincm, ddecm

- addm/subm r, (r + ximm7)

  Loads a word from memory (like the lw instruction) into r5 and then adds r5
  or subtracts it from another register. That is, it's equivalent to:

      lw      r5, (src + ximm7)
      add/sub dst, r5

  subm can double as a compare instruction.

  The source and destination register operands must be different.

  Note also, the source/address register cannot be pc and the destination
  register cannot be r5, sp or pc.

- (d)incm/(d)decm (r)

  Loads a word from memory at address contained in the specified register
  (similarly to the lw instruction) to that same register, stores the value
  of that register incremented/decremented by 1 (or 2 if dincm/ddecm) back
  to memory.

  The value in the register stays the original pre-incremented/decremented
  value loaded from memory.

  The Z(ero) and S(ign) flags reflect the post-increment/decrement value.
  Other arithmetic flags (C(arry), O(verflow)) are unspecified.

  Note, the register operand of the instruction cannot be sp or pc.

- (d)incm/(d)decm r, (sp + imm7)

  Loads a word from memory at a location within the stack (like the lw
  instruction) to the specified register, stores the value of that register
  incremented/decremented by 1 (or 2 if dincm/ddecm) back to memory.

  The value in the register stays the original pre-incremented/decremented
  value loaded from memory.

  The Z(ero) and S(ign) flags reflect the post-increment/decrement value.
  Other arithmetic flags (C(arry), O(verflow)) are unspecified.

  Note, the destination register operand of the instruction cannot be
  sp or pc.


#### 4.1.4 Non-memory data moves: li, mov, add, m2f, mf2

- li r, simm9

  Loads a signed 9-bit immediate sign-extended to 16 bits into a register.

  The register cannot be sp or pc.

- mov r, r

  Copies (AKA moves) the 16-bit value of one register to another.

  This cannot move/copy to the stack pointer register (sp) or the program
  counter (pc).

- add sp, r, 0

  This can be used to move to sp from another register.

  Note, when the 3-operand add instruction stores the sum in sp (or pc),
  it preserves the arithmetic flags.

- m2f/mf2

  `m2f` copies the flags register to r2. Bits 9 through 4 indicate whether
  an IRQ (IRQ5 through IRQ0) is requested.

  `mf2` copies r2 to the flags register. Bits 9 through 4 are used to
  clear requested IRQs (IRQ5 through IRQ0) in negative logic (that is,
  1 preserves the IRQ requested flag, 0 clears it). IOW, if you aren't
  manipulating the flags register to clear requested IRQs, you must write
  all ones to bits 9 through 4.

  Only bits 9 through 4 have the dual/different function and meanings
  depending on whether you read or write them. Bits 15 through 10 (IRQ
  masks) and bits 3 through 0 (arithmetic flags) do not have such a dual
  function.


#### 4.1.5 Carry flag manipulation: stc

- stc

  Sets the C(arry) flag to 1. Other arithmetic flags change to unspecified
  values.


#### 4.1.6 Single-register ALU operations: zxt, sxt, cpl, neg, adcz

None of these instructions can operate on sp or pc.

- zxt r

  Zero-extends a register, that is, sets bits 15 through 8 to all zeroes.

  Preserves flags.

- sxt r

  Sign-extends a register, that is, sets bits 15 through 8 to the value of
  bit 7.

  Preserves flags.

- cpl r

  Complements (or simply inverts) every bit in a register.

  Preserves flags.

- neg r

  Negates (in two's complement representation) the value in a register.

  The arithmetic flags are set as if the register value is subtracted from 0
  using the sub instruction.

- adcz r

  Adds the C(arry) flag to a register.

  Modifies the arithmetic flags like the add/adc instruction.


#### 4.1.7 Regular ALU: add, addu, adc, sub, sbb, cmp, and, or, xor

- and/or/xor r, imm7/r

  Performs the binary bitwise operation on a register and either a 7-bit
  immediate or another register. The immediate is zero-extended to 16 bits.

  Sets the Z(ero) and S(ign) flags according to the result. Clears the
  C(arry) and O(verflow) flags.

  The destination register operand cannot be sp or pc.

- cmp r, simm7

  Compares a register with a signed 7-bit immediate (that is, sign-extended
  to 16 bits) and sets the arithmetic flags as if it's the sub instruction.
  A conditional jump may follow to divert execution based on the result of
  the comparison.

  The register operand cannot be sp or pc.

- add r, r, simm7

  Adds a register and a signed 7-bit immediate (that is, sign-extended to
  16 bits) and stores the sum in another register.

  When the add stores the sum in pc and the 7-bit immediate is odd-valued
  (that is, bit 0 is 1), not only pc is loaded with the sum, but also
  the pc of the immediately following instruction (original pc + 2) is
  stored in r5. This is called linking and is used to call subroutines.
  Subroutines return to their callers by jumping to the address previously
  stored in r5 (hence r5 is referred to as the link or return address
  register).

  When the add instruction stores the sum in sp or pc, it preserves the
  arithmetic flags. Otherwise, the arithmetic flags are set as usual for
  addition, see below.

  Note, `add sp, pc, simm7`, `add pc, sp, simm7` and `add pc, pc, simm7`
  are disallowed.

- add/sub/adc/sbb/cmp r, r

  `add` adds one register to another.

  `adc` adds the carry flag and a register to another register.

  `sub` subtracts one register from another.

  `sbb` subtracts the carry (AKA borrow) flag and a register from another
  register.

  `cmp` compares two registers by performing subtraction as the sub
  instruction but doesn't store the difference. It only affects the
  arithmetic flags.

  The Z(ero) flag is set if the 16-bit sum/difference is zero. The flag is
  reset otherwise.

  The S(ign) flag is set to bit 15 of the sum/difference and indicates
  whether the result is less than zero.

  The C(arry) flag is set if there's an unsigned overflow/underflow in the
  add/sub operation. The flag is reset otherwise.

  The O(verflow) flag is set if there's a signed overflow/underflow in the
  add/sub operation. The flag is reset otherwise.

  Note, the destination (leftmost) register operand of these instructions
  cannot be sp or pc.

- addu r, imm9 ("add upper")

  Shifts the 9-bit immediate 7 bit positions left and adds it to a register.

  The register cannot be pc.

  The arithmetic flags are updated as usual for the add instruction.


#### 4.1.8 Shifts and rotates: sr, sl, asr, rl, rr

None of these instructions can shift/rotate sp or pc.

- sr/sl/asr/rl/rr r, r

  These shift or rotate a register by the number of bit positions
  (0 through 15) specified in another register. Only the 4 least significant
  bits of the number/count are used, the other bits are ignored.

  `sr` is shift right.

  `sl` is shift left.

  `asr` is arithmetic shift right.

  `rl` is rotate left.

  `rr` is rotate right.

  These set the Z(ero) and S(ign) flags according to the result and
  clear the C(arry) and O(verflow) flags.

- sr/sl/asr/rl r, imm4

  These shift or rotate a register by the number of bit positions specified
  in the 4-bit immediate.

  `rl r, imm4` can double as rotate right, hence there's no distinct
  `rr r, imm4` instruction.

  Note, imm4=0 is reserved. Also reserved is `sl r, 1`, use `add r, r`
  instead.

  These set the Z(ero) and S(ign) flags according to the result and
  clear the C(arry) and O(verflow) flags.


#### 4.1.9 Address formation: lurpc

- lurpc r, imm9 ("load upper relative to pc")

  Shifts the 9-bit immediate 7 bit positions left, adds it to the pc of the
  immediately following instruction (that is, original pc + 2) and stores
  the sum in the specified register.

  This instruction helps create position-independent code.

  The register cannot be sp or pc.

  Preserves flags.


#### 4.1.10 Multiplication and division: sac, add22adc33, cadd24, cadd24adc3z, csub34

- sac r, r, imm4 ("shift and accumulate")

  Shifts left the value of a register by the number of bit positions
  specified in the 4-bit immediate and adds it to another register.

  Neither of the register operands can be sp or pc.

  The arithmetic flags are updated as usual for the add instruction.

- add22adc33, cadd24, cadd24adc3z, csub34

  These instructions can speed up general multiplication and division
  subroutines by 35%-40% compared to using regular ALU operations and
  conditional jumps.

  `add22adc33` is equivalent to the following sequence of instructions:

      add r2, r2
      adc r3, r3

  `cadd24` is equivalent to the following instruction:

      add r2, (Carry ? r4 : 0)

  That is, if the C(arry) flag is set, this adds r4 to r2. Otherwise,
  it adds zero to r2.

  `cadd24adc3z` is equivalent to the following sequence of instructions:

      add r2, (Carry ? r4 : 0)
      adcz r3

  That is, if the C(arry) flag is set, this adds r4 to r2. Otherwise,
  it adds zero to r2. Then it adds the C(arry) flag to r3.

  `csub34` is equivalent to the following sequence of instructions:

      sub r3, r4
      add r3, (Carry ? r4 : 0)

  That is, this first subtracts r4 from r3, then, if the C(arry) flag is set,
  it adds r4 to r3, otherwise, it adds zero to r3.


#### 4.1.11 Jumps, nop, subroutines

- j simm8 ("unconditional jump")

  Advances pc to the address of the immediately following instruction plus
  an 8-bit immediate sign-extended to 16 bits and doubled
  (pc = pc + 2 + sign-extended simm8\*2).

  There's no distinct nop ("no operation") instruction, but `j simm8=0` is
  an equivalent substitution.

  Preserves flags.

- j\<cond\> simm7 ("conditional jump")

  Examines the flags register and if the specified condition is true, jumps
  similarly to "j simm8", otherwise continues to the immediately following
  instruction.

  The conditional jump often follows a compare instruction.

  The 14 conditions are:
  - c/lu: carry/borrow OR unsigned overflow/underflow OR
    unsigned less than
  - z/e: zero OR equal
  - s: signed/negative (< 0)
  - leu: unsigned less than or equal
  - l: signed less than
  - le: signed less than or equal
  - nc/geu: no carry/borrow OR no unsigned overflow/underflow OR
    unsigned greater than or equal
  - nz/ne: non-zero OR unequal
  - ns: not signed OR non-negative (>= 0)
  - gu: unsigned greater than
  - ge: signed greater than or equal
  - g: signed greater
  - o: signed overflow/underflow
  - no: no signed overflow/underflow

  Preserves flags.

- jal simm11 ("jump and link")

  Advances pc to the address of the immediately following instruction plus
  an 11-bit immediate sign-extended to 16 bits and doubled
  (pc = pc + 2 + sign-extended simm11\*2).

  The pc of the immediately following instruction (original pc + 2) is
  stored in r5. This is called linking and is used to call subroutines.
  Subroutines return to their callers by jumping to the address previously
  stored in r5 (hence r5 is referred to as the link or return address
  register).

  Preserves flags.

- add pc, r, odd-valued simm7 ("indirect jump and link")

  Sets pc to the sum of a register and a signed 7-bit immediate
  (the least significant bit of the immediate is ignored here).

  The pc of the immediately following instruction (original pc + 2) is
  stored in r5. This is called linking and is used to call subroutines.
  Subroutines return to their callers by jumping to the address previously
  stored in r5 (hence r5 is referred to as the link or return address
  register).

  Preserves flags.

- add pc, r, even-valued simm7 ("indirect jump")

  Sets pc to the sum of a register and a signed 7-bit immediate.
  This can also be used to return from subroutines.

  Preserves flags.

- last imm7 ("leap linked and adjust stack")

  Increments sp by an unsigned 7-bit immediate (even-valued) and loads pc
  with the value of r5. This is used to return from subroutines while also
  deallocating stack storage used by local variables and on-stack arguments.

  Preserves flags.


#### 4.1.12 Interrupts: swi, external/HW ints, reti, di, ei

- swi simm6 ("software interrupt")

  This instruction does the following:
  1. Saves the pc of the swi instruction together with the interrupt enable
     flag (in pc's bit 0) in memory at address sp-2. It's OK to store below
     sp here because of the following step.
  2. Disables external/hardware interrupts by clearing the interrupt enable
     flag.
  3. Sign-extends the 6-bit immediate, doubles it and loads it into pc.

  The ISR that the instruction invokes should decrement sp by 2 (or more if
  necessary) with `add sp, sp, -2` (which, BTW, preserves the arithmetic
  flags) before enabling interrupts or pushing anything onto the stack.

  Because the return address is stored on the stack, interrupts can be
  recursive/nested.

  This instruction can be used to implement the following:
  - debug breakpoints (the return address conveniently points back to swi)
  - OS system calls
  - common subroutines

- external/hardware interrupts

  When external/hardware interrupts are enabled in the interrupt enable flag,
  these interrupts trigger execution of the interrupt service routine. This
  is done by executing the equivalent of `swi 31` in place of the
  instruction that would otherwise be executed at the current pc.

  Because the return address is stored on the stack, interrupts can be
  recursive/nested.

  `swi 31` sets pc to 0x3E, which is the common entry point to all external/
  hardware ISRs. The ISR code should read and analyze the flags register
  to see which of the 6 IRQs are being requested and which of them are
  unmasked and can be handled. After this the IRQ(s) of interest can be
  handled.

  Note, external/hardware interrupts are edge-triggered, not level-triggered.

- reti

  This instruction returns from a software or hardware interrupt service
  for which it does the following:
  1. Loads pc from the memory word at sp-2, ignoring word's bit 0.
  2. Loads the interrupt enable flag with word's bit 0.

  If the ISR (re)enables external/hardware interrupts, it must disable them
  before executing the reti instruction. This prevents overwriting of the
  return address on the stack at sp-2 by another ISR.

  When the reti instruction is used to return from a software interrupt,
  the ISR must increment the return address by 2 before executing reti.
  This will make sure that the interrupted execution continues at the
  instruction immediately following swi. The return address must not be
  incremented by 2 when returning from a hardware interrupt service routine.

- di/ei

  `di` disables external/hardware interrupts by clearing the interrupt
  enable flag.

  `ei` enables external/hardware interrupts by setting the interrupt
  enable flag.


#### 4.1.13 Logical to physical mapping/translation

- msr s, r / mrs r, s

  These instructions write to or read from the memory selector registers and
  take two general-purpose register operands:
  - the `s` register contains the selector number:
    - 0 through 3 are for the program/code space, they select one of the
      four 16KB subregions of the 64KB space
    - 4 through 7 are for the data space, they select one of the four 16KB
      subregions of the 64KB space
    - other numbers are reserved for possible extension in the future
  - the `r` register is the register that provides the new value for the
    selector (in case of `msr`) or receives selector's value (in case of
    `mrs`); this is an 8-bit value from 0 to 255 that specifies a 16KB block
    of physical memory, which allows up to 256\*16KB = 4MB physical memory
    in the system


### 4.2 Code examples

We'll use the following definitions throughout the examples:

- `lo()` - 7 least significant bits of value, signed/sign-extended if/as
  necessary (`li` has simm9, not simm7)
- `hi()` - 9 most significant bits of value, affected by the sign of the
  corresponding `lo()`


#### 4.2.1 Loading a 16-bit const

- direct, full range, modifies flags, doesn't work with sp:

      li    r4, lo(const)
      addu  r4, hi(const)

- alternative direct, full-range, modifies flags, doesn't work with sp:

      lurpc r4, hi(const)
      add   r4, r4, lo(const) ; note the sign extension

- alternative atomic for sp, uses a temporary register, preserves flags:

      lurpc r5, hi(const)
      add   sp, r5, lo(const)

- indirect from code, preserves flags, atomic for sp:

      lw    r4, (pc + simm7)


#### 4.2.2 Adding a 16-bit const

- in either order, produces unreliable overflow and carry, non-atomic for
  sp:

      addu  r4, hi(const)
      add   r4, r4, lo(const)

- short range (-128...+124) alternative atomic for sp, preserves flags:

      add   sp, sp, const1 ; const1 must be even (-64...+62),
                           ; must be same-sign as const2
      add   sp, sp, const2 ; const2 must be even (-64...+62),
                           ; must be same-sign as const1

- coarse (multiple of 128), full range alternative atomic for sp,
  preserves flags:

      addu  sp, const

- instantaneously coarse (by -64) increment, full range alternative atomic
  for sp, preserves flags, in this exact order (else risking stack
  corruption by ISRs):

      add   sp, sp, lo(const) ; const > 0, lo(const) may be < 0
      addu  sp, hi(const)     ; const > 0

- instantaneously coarse (by -62) decrement, full range alternative atomic
  for sp, preserves flags, in this exact order (else risking stack
  corruption by ISRs):

      addu  sp, hi(const)     ; const < 0
      add   sp, sp, lo(const) ; const < 0, lo(const) may be > 0

- full range alternative atomic for sp, modifies flags and a temporary
  register:

      add   r5, sp, lo(const)
      addu  r5, hi(const)
      add   sp, r5, 0


#### 4.2.3 Multiplying by a constant (using "shift and accumulate")

When the constant has just a few one bits:

    li    r2, 0
    li    r5, 9
    sac   r2, r5, 3 ; r2 = r2 + r5 * 8
    sac   r2, r5, 1 ; r2 = r2 + r5 * 2
                    ; r2 = r5 * 10 = 90


#### 4.2.4 8-bit rotation

    zxt   r2        ; optional / as necessary
    sac   r2, r2, 8 ; assuming zero in high byte of r2, duplicate its low byte
    rl    r2, r1


#### 4.2.5 Clearing low bits while preserving high bits

    or    r4, 7
    xor   r4, 7


#### 4.2.6 Resetting Carry flag

- `cmp r4, r4` ; carry=overflow=sign=0, zero=1
- `and/or r4, r4` ; carry=overflow=0
- `add/shift/rotate r4, 0` ; carry=overflow=0
- `xor r4, r4` ; zeroes r4, carry=overflow=sign=0, zero=1


#### 4.2.7 Setting Carry flag

Options other than `stc`:

- when a register isn't 0xFFFF:

      cmp r4, -1 ; register can't be sp,pc

- when a register isn't 0:

      neg r4
      neg r4     ; register can't be sp,pc

- when a register is between 0x8000 and 0xFFFF:

      add r4, r4 ; register can't be sp,pc; modifies r4


#### 4.2.8 Loading/storing with larger offset/immediate, unaligned load/store

- register-relative, mid-range, modifies temporary register:

      li    r5, ofs ; signed 9-bit offset
      lw/sw r4, (r3 + r5)

- register-relative, full-range, modifies register and flags:

      addu  r3, hi(ofs) ; r3 may be restored with `addu r3, -hi(ofs)`,
                        ; which may need a special relocation type
      lw/sw r4, (r3 + lo(ofs))

- pc-relative, full-range, uses temporary register:

      lurpc r5, hi(ofs)
      lw/sw r4, (r5 + lo(ofs))

- unaligned 16-bit load:

      lb    r4, (r3 + odd-valued ofs)
      lb    r5, (r3 + odd-valued ofs + 1) ; note, `ofs + 1` may overflow
      sac   r4, r5, 8

- unaligned 16-bit store:

      sb    r4, (r3 + odd-valued ofs)
      rl    r4, 8
      sb    r4, (r3 + odd-valued ofs + 1) ; note, `ofs + 1` may overflow
      rl    r4, 8 ; restore r4: optional / as necessary


#### 4.2.9 Jump (unconditional) with larger distance and indirection

- pc-relative, +/-256B range:

      j     simm8

- pc-relative, +/-2KB range, modifies r5:

      jal   simm11

- pc-relative, full-range, uses temporary register:

      lurpc r5, hi(ofs)
      add   pc, r5, lo(ofs)

- indirect, from register:

      add   pc, r3, 0

- indirect, from memory:

      lw    pc, (...)


#### 4.2.10 Jump and link with larger distance and indirection

These store the return address in r5.

- pc-relative, +/-2KB range:

      jal   simm11

- pc-relative, full-range:

      lurpc r5, hi(ofs)
      add   pc, r5, lo(ofs)+1 ; odd-valued constant triggers storing pc to r5

- indirect, from register:

      add   pc, r3, 1 ; odd-valued constant triggers storing pc to r5

- indirect, from memory:

      lw    r5, (...)
      add   pc, r5, 1

- alternative indirect, from memory, modifies flags:

      add   r5, pc, 2
      lw    pc, (...)


#### 4.2.11 Conditional jump with larger distance

- pc-relative, +/-128B range:

      j<cond> simm7

- pc-relative, +/-2KB range, modifies r5:

      j<!cond> simm7=1
      jal      simm11

- pc-relative, full-range, uses temporary register:

      j<!cond> simm7=2
      lurpc    r5, hi(ofs)
      add      pc, r5, lo(ofs)

- absolute, full-range:

      j<!cond> simm7=2
      lw       pc, (pc + 0)
      addr16 ; target address


#### 4.2.12 16-bit multiplication

    ; r2 = r3 * r4
    ; destroyed: r0, r3
        li          r2, 0
        li          r0, 16
    Lrepeat:
        add22adc33             ; `add r2, r2`, `adc r3, r3`
        cadd24                 ; add r2, (Carry ? r4 : 0)
        add         r0, r0, -1
        jnz         Lrepeat

This can be trivially unrolled for speed.


#### 4.2.13 Widening unsigned 16-bit multiplication

    ; r3:r2 = r3 * r4
    ; destroyed: r0
        li          r2, 0
        li          r0, 16
    Lrepeat:
        add22adc33             ; `add r2, r2`, `adc r3, r3`
        cadd24adc3z            ; `add r2, (Carry ? r4 : 0)`, `adcz r3`
        add         r0, r0, -1
        jnz         Lrepeat

This can be trivially unrolled for speed.


#### 4.2.14 Unsigned 16-bit division and modulo/remainder

    ; r2 = r2 / r4
    ; r3 = r2 % r4
    ; destroyed: r0
        li          r3, 0
        li          r0, 16
    Lrepeat:
        add22adc33             ; `add r2, r2`, `adc r3, r3`
        csub34                 ; `sub r3, r4`, `add r3, (Carry ? r4 : 0)`
        adcz        r2         ; collect inverted bit of quotient
        add         r0, r0, -1
        jnz         Lrepeat
        cpl         r2

This can be trivially unrolled for speed.


#### 4.2.15 16-bit binary to decimal conversion

Somewhat similar to the division example, one can convert binary numbers
to their decimal representation suitable for display by subtracting powers
of 10 with the csub34 instruction.

Here's how you can count how many times a number in `r3` contains 10000:

        li          r4, 16
        addu        r4, 78     ; r4 = 16 + 78 * 128 = 10000
        li          r0, -1     ; counter of successful subtractions
    Lrepeat:
        add         r0, r0, 1
        csub34                 ; `sub r3, r4`, `add r3, (Carry ? r4 : 0)`
        jnc         Lrepeat

By repeating this for thousands, hundreds and tens you can collect all
decimal digits of a 16-bit number.


#### 4.2.16 Subroutine prolog and epilog

When a subroutine doesn't call other subroutines (AKA leaf subroutine)
and it doesn't clobber r5 in any other way, the prolog and epilog can
be as simple as:

    ; prolog: nothing
    
    ; subroutine body...
    
    ; epilog: just return
    add     pc, r5, 0

If the return address in r5 is overwritten by subroutine's body, r5 can
be saved and restored:

    ; prolog:
    push    r5
    
    ; subroutine body...
    
    ; epilog:
    pop     r5
    add     pc, r5, 0

If more registers need to be preserved by a subroutine, e.g. r2 through r5,
this becomes:

    ; prolog:
    add     sp, sp, -8
    sw      r2, (sp + 6)
    sw      r3, (sp + 4)
    ss5r    r4           ; `sw r5, (sp + 0)`, `sw r4, (sp + 2)`
    
    ; subroutine body...
    
    ; epilog:
    ls5r    r4           ; `lw r5, (sp + 0)`, `lw r4, (sp + 2)`
    lw      r3, (sp + 4)
    lw      r2, (sp + 6)
    last    8            ; `add sp, sp, 8`, `add pc, r5, 0`

When a subroutine needs to remove its on-stack arguments the `last`
instruction should include the cumulative size of the on-stack arguments
in its immediate operand.

Lastly, subroutines may have local variables and so we may have the
following for a subroutine that
- preserves r2 through r5
- allocates 16 bytes for local variables
- removes 4 bytes of arguments from the stack

      ; prolog:
      add     sp, sp, -24
      ; 16 bytes of local variables start at sp + 8
      sw      r2, (sp + 6)
      sw      r3, (sp + 4)
      ss5r    r4           ; `sw r5, (sp + 0)`, `sw r4, (sp + 2)`
    
      ; subroutine body...
    
      ; epilog:
      ls5r    r4           ; `lw r5, (sp + 0)`, `lw r4, (sp + 2)`
      lw      r3, (sp + 4)
      lw      r2, (sp + 6)
      last    28           ; `add sp, sp, 28`, `add pc, r5, 0`


#### 4.2.17 ISRs

Software and hardware interrupt service routines are all entered with
external/hardware interrupts automatically disabled, which is partially
a microarchitectural artifact, which may often be handy.


##### 4.2.17.1 Hardware ISRs

The generic prolog/entry point in a hardware ISR should look like this
(BTW, this code should be at pc=0x3E since there's one common entry point
for all external/hardware ISRs):

    ; Save regs on stack. N.B. return address is at sp-2.
    add  sp, sp, -16   ; doesn't affect flags
    sw   r5, (sp + 12)
    sw   r4, (sp + 10)
    sw   r3, (sp + 8)
    sw   r2, (sp + 6)
    sw   r1, (sp + 4)
    sw   r0, (sp + 2)
    m2f
    sw   r2, (sp + 0)  ; save flags too

And the matching epilog should look like this:

    ; Restore regs from stack and return.
    di                 ; make sure ints are disabled now to protect
                       ; return address on stack
    lw   r2, (sp + 0)
    
    li   r3, -16
    addu r3, 8         ; r3 = 0000001111110000: ones tell which IRQs
                       ; not to clear;
                       ; change r3 accordingly if any IRQ needs cleared
    or   r2, r3
    
    mf2                ; restore flags
    lw   r0, (sp + 2)
    lw   r1, (sp + 4)
    lw   r2, (sp + 6)
    lw   r3, (sp + 8)
    lw   r4, (sp + 10)
    lw   r5, (sp + 12)
    add  sp, sp, 16    ; doesn't affect flags
    reti

Actual interrupt processing/handling (device communication) should happen
between these prolog and epilog blocks.

If desired, interrupts may be reenabled with `ei` between the prolog and
epilog blocks. This will let other unmasked IRQs to be handled in a
nested/recursive fashion.


##### 4.2.17.2 Software ISRs

Software ISRs may be implemented similarly to hardware ISRs.

SW ISRs don't need to save and restore all registers (specifically, flags),
it's up to you to decide how registers are used with SW ISRs.

However, they must do one thing that hardware ISRs must not: increment the
return address on the stack by 2, so `reti` can return to the next
instruction after `swi`.

So, the minimal prolog and epilog for a SW ISR should look like:

    ; N.B. return address is at sp-2.
    add   sp, sp, -2   ; protect return address

and

    di                 ; make sure ints are disabled now to protect
                       ; return address on stack
    dincm r5, (sp + 0) ; adjust return address by one instruction (2 bytes)
    add   sp, sp, 2    ; unprotect return address
    reti

If desired, interrupts may be reenabled with `ei` between the prolog and
epilog blocks. This will let unmasked IRQs to be handled in a nested/
recursive fashion.

SWIs should not keep external/hardware interrupts disabled for long periods
of time.

The swi instruction can target up to 2**6=64 entry points, 32 of them are
near pc=0 and the other 32 of them are near pc=0xFFFE. `swi 0` (and possibly
`swi 1`) is reserved because pc is zeroed during reset. `swi 31` (which
jumps to pc=0x3E) is reserved because that's where the common entry point
is for all hardware ISRs. The swi's between these two can be used as system
calls. OTOH, `swi -32` through `swi -1` (AKA `swi 32` through `swi 63`) can
be used as user-defined SWIs (their target pc will be in the range 0xFFC0
through 0xFFFE).


### 4.3 Instruction encodings

#### 4.3.1 Overall encodings

    FEDCBA9876543210
    00wrrrRRR7777777 lb/lw            rrr, (RRR + ximm7) ; rrr!=sp,pc when lb
    000rrrRRR7777777   and            RRR, imm7          ; when lb, rrr==sp, RRR!=sp,pc
    000rrrRRR7777777   or             RRR, imm7          ; when lb, rrr==pc, RRR!=sp,pc
    000rrrRRR7777777   addm0          r, (R+ximm7)       ; when lb, rrr==sp, RRR==sp
    000rrrRRR7777777   addm1          r, (R+ximm7)       ; when lb, rrr==sp, RRR==pc
    000rrrRRR7777777   addm2          r, (R+ximm7)       ; when lb, rrr==pc, RRR==sp
    000rrrRRR7777777   addm3          r, (R+ximm7)       ; when lb, rrr==pc, RRR==pc
    001rrrRRR7777771   incm/decm      (rrr), simm7       ; when lw, rrr!=sp,pc, RRR==sp, odd-valued simm7
    001rrrRRR7777771   adcz           rrr[, imm6]        ; when lw, rrr!=sp,pc, RRR==pc, odd-valued imm7
    01wrrrRRR7777777 sb/sw            rrr, (RRR + ximm7)  ; rrr!=RRR ; rrr!=sp,pc when sb ; rrr!=pc when sw
    010rrrRRR7777777   xor            RRR, imm7           ; when sb, rrr==sp, RRR!=sp,pc
    010rrrRRR7777777   cmp            RRR, simm7          ; when sb, rrr==pc, RRR!=sp,pc
    010rrrRRR7777777   addm4          r, (R+ximm7)        ; when sb, rrr==sp, RRR==sp
    010rrrRRR7777777   addm5          r, (R+ximm7)        ; when sb, rrr==sp, RRR==pc
    010rrrRRR7777771     push         r0                  ; when sb, rrr==sp, RRR==pc, odd-valued imm7
    010rrrRRR7777777   addm6          r, (R+ximm7)        ; when sb, rrr==pc, RRR==sp
    010rrrRRR7777777   addm7          r, (R+ximm7)        ; when sb, rrr==pc, RRR==pc
    010rrrRRR7777777   subm0x         r, (R+ximm7)        ; when sb, (rrr==RRR)!=sp,pc
    010rrrRRR7777771     push         r2                  ; when sb, (rrr==RRR)==5, odd-valued imm7
    011rrrRRR7777777   subm1x         r, (R+ximm7)        ; when sw, (rrr==RRR)!=sp,pc
    011rrrRRR7777771     push         r3                  ; when sw, (rrr==RRR)==5, odd-valued imm7
    011rrrRRR7777777   subm2x         r, (R+ximm7)        ; when sw, rrr==pc, RRR!=sp,pc
    011rrrRRR7777771     push         r4                  ; when sw, rrr==pc, RRR==5, odd-valued imm7
    011rrrRRR7777777   addm12         r, (R+ximm7)        ; when sw, rrr==pc, RRR==sp
    011rrrRRR7777777   addm13         r, (R+ximm7)        ; when sw, rrr==pc, RRR==pc
    011rrrRRR7777771   dincm          (rrr) [, imm6]      ; when sw, rrr!=sp,pc, RRR==sp, odd-valued imm7
    011rrrRRR7777771   ddecm          (rrr) [, imm6]      ; when sw, rrr!=sp,pc, RRR==pc, odd-valued imm7
    100rrrRRR7777777 add              rrr, RRR, simm7 ; except {sp,pc,imm},{pc,sp,imm},{pc,pc,imm} ; `add pc, RRR, simm7` preserves flags (ditto `sp,RRR,simm7`)
    100rrrRRR7777771   swi            simm6*2         ; when rrr==sp, RRR==sp, odd-valued simm7 ; invokes ISR at (simm7&(-2)), disabling interrupts
    100rrrRRR7777777   push           simm7           ; when rrr==sp, RRR==pc
    100rrrRRR7777777   addm8          r, (R+ximm7)    ; when rrr==pc, RRR==sp
    100rrrRRR7777777   addm9          r, (R+ximm7)    ; when rrr==pc, RRR==pc
    101rrr0999999999 li               rrr, simm9   ; when rrr!=sp,pc ; rrr = simm9
    101rrr1999999999 addu             rrr, imm9    ; when rrr!=pc ; rrr += imm9 << 7 ; `addu sp, imm9` preserves flags
    101rrr0999999990   ?              simm8*2      ; when rrr==sp, even-valued imm9
    101rrr0999999991   j              simm8*2      ; when rrr==sp, odd-valued imm9
    101rrrRRR7777777   subm3x         r, (R+ximm7) ; when rrr==pc, RRR!=sp,pc
    101rrrRRR7777771     push         r5           ; when rrr==pc, RRR==5, odd-valued imm7
    101rrrRRR7777777   addm10         r, (R+ximm7) ; when rrr==pc, RRR==sp
    101rrrRRR7777777   addm11         r, (R+ximm7) ; when rrr==pc, RRR==pc
    101rrrRRR7777771     push         r1           ; when rrr==pc, RRR==pc, odd-valued imm7
    110rrr0007777777 incm/incs        rrr, (sp+imm7) ; when rrr!=sp,pc
    110rrr0017777777 decm/decs        rrr, (sp+imm7) ; when rrr!=sp,pc
    110rrr0107777777 dincm/dincs      rrr, (sp+imm7) ; when rrr!=sp,pc
    110rrr0117777777 ddecm/ddecs      rrr, (sp+imm7) ; when rrr!=sp,pc
    110rrr0007777771   ls5r           rrr [, imm6]   ; when rrr!=5,sp,pc, odd-valued imm7
    110rrr0007777771     last         imm6*2         ; when rrr==5, odd-valued imm7
    110rrr0017777771   ss5r           rrr [, imm6]   ; when rrr!=5,sp,pc, odd-valued imm7
    110rrr0017777771     ???          [imm6]         ; when rrr==5, odd-valued imm7
    110rrr0107777771   ???            rrr [, imm6]   ; when rrr!=sp,pc, odd-valued imm7
    110rrr0117777771   ???            rrr [, imm6]   ; when rrr!=sp,pc, odd-valued imm7
    110rrr1999999999 lurpc            rrr, imm9      ; when rrr!=sp,pc ; rrr = pc + (imm9 << 7)
    110rreeeeeeeeeee   jal            simm11*2       ; when rrr==sp,pc
    1110000007777777 j<cond0:c/lu>    simm7*2
    1110010007777777 j<cond1:z/e>     simm7*2
    1110100007777777 j<cond2:s>       simm7*2
    1110110007777777 j<cond3:leu>     simm7*2
    1111000007777777 j<cond4:l>       simm7*2
    1111010007777777 j<cond5:le>      simm7*2
    1111100007777777 j<cond6:o>       simm7*2
    1111110007777777 j<cond7:no>      simm7*2
    1110000017777777 j<cond8:nc/geu>  simm7*2
    1110010017777777 j<cond9:nz/ne>   simm7*2
    1110100017777777 j<cond10:ns>     simm7*2
    1110110017777777 j<cond11:gu>     simm7*2
    1111000017777777 j<cond12:ge>     simm7*2
    1111010017777777 j<cond13:g>      simm7*2
    11111r001PPPQIII mrs              r, s ; get s-reg-indexed selector to reg r (both reg numbers encoded in 5-bit rPPPQ)
    1111110011110III   stc
    1111110011111III   ???
    111rrr01R7777777 addm14...29      r, (R+ximm7) ; rrrR = 0...15
    111rrr1007777777 subm4x           r, (R+ximm7)  ; when rrr!=sp,pc
    111rrr100PPP0III   zxt            PPP           ; when rrr==sp, PPP!=sp,pc ; doesn't modify flags
    111rrr100PPP1III   sxt            PPP           ; when rrr==sp, PPP!=sp,pc ; doesn't modify flags
    111rrr100PPP0III   cpl            PPP           ; when rrr==pc, PPP!=sp,pc ; doesn't modify flags
    111rrr100PPP1III   neg            PPP           ; when rrr==pc, PPP!=sp,pc
    111rrr100PPPQIII     pop          rPQ           ; when rrr==sp,pc, PPP==sp,pc, Q==0,1
    111rrr100PPP0III     mf2                        ; when rrr==pc, PPP==pc
    111rrr100PPP1III     m2f                        ; when rrr==pc, PPP==pc
    111rrr101PPP4444 sac              rrr, PPP, imm4 ; when rrr,PPP!=sp,pc
    111rrr101PPP4444   sr             PPP, imm4      ; when rrr==sp, PPP!=sp,pc
    111rrr101PPP4444   sl             PPP, imm4      ; when rrr==pc, PPP!=sp,pc
    111rrr101PPP4444   asr            rrr, imm4      ; when rrr!=sp,pc, PPP==sp
    111rrr101PPP4444   rl             rrr, imm4      ; when rrr!=sp,pc, PPP==pc
    111rrr101PPP0III   di                            ; when rrr==sp, PPP==sp
    111rrr101PPP1III   ei                            ; when rrr==sp, PPP==sp
    111rrr101PPP0III   reti                          ; when rrr==sp, PPP==pc ; returns from ISR, restoring interrupt enable/disable
    111rrr101PPP1III   hlt? (reserved)               ; when rrr==sp, PPP==pc
    111rrr101PPP0III   add22adc33                    ; when rrr==pc, PPP==sp
    111rrr101PPP1III   cadd24                        ; when rrr==pc, PPP==sp
    111rrr101PPP0III   cadd24adc3z                   ; when rrr==pc, PPP==pc
    111rrr101PPP1III   csub34                        ; when rrr==pc, PPP==pc
    111rrr11wPPP0qqq lb/lw            rrr, (PPP + qqq) ; when PPP!=sp,pc ; rrr!=sp,pc when lb
    111rrr11wPPP1qqq sb/sw            rrr, (PPP + qqq) ; when PPP!=sp,pc, rrr!=PPP ; rrr!=sp,pc when sb ; rrr!=pc when sw
    111rrr110PPP0qqq   sr             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
    111rrr110PPP1qqq   sl             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
    111rrr111PPP0qqq   rr             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
    111rrr111PPP1qqq   rl             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
    111rrr110PPP0qqq   asr            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
    111rrr110PPP1qqq   xor            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
    111rrr111PPP0qqq   and            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
    111rrr111PPP1qqq   or             rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
    111rrr110PPP0qqq   adc            PPP, qqq         ; when lb, rrr==sp, PPP!=sp,pc
    111rrr110PPP1qqq   sbb            PPP, qqq         ; when sb, rrr==sp, PPP!=sp,pc
    111rrr110PPP0qqq   add            PPP, qqq         ; when lb, rrr==pc, PPP!=sp,pc
    111rrr110PPP1qqq   sub            PPP, qqq         ; when sb, rrr==pc, PPP!=sp,pc
    111rrr111PPP1qqq   cmp            PPP, qqq         ; when sw, rrr==pc, PPP!=sp,pc
    111rrr110PPP1qqq   mov            rrr, qqq         ; when sb, (rrr==PPP)!=sp,pc
    111rrr111PPP1qqq   msr            rrr, qqq         ; set rrr-reg-indexed selector from reg qqq ; when sw, (rrr==PPP)!=sp,pc
    
    Legend:
    - III=111 (currently ignored)
    - imm = unsigned immediate
    - simm = signed immediate
    - ximm = unsigned immediate if added to sp, signed otherwise
    - 4 = bit of 4-bit immediate
    - 7 = bit of 7-bit immediate
    - 9 = bit of 9-bit immediate
    - e = bit of 11-bit immediate


#### 4.3.2 Further encodings in special cases

##### 4.3.2.1 addm dst, (src + ximm7)

Equivalent to:

    lw  r5, (src + ximm7)
    add dst, r5

Encoding:

    dst\src:
     \ 0  1  2  3  4  5  6  7
    0    00 01 02 03 04 05    addm0, ..., addm5,
    1 06    07 08 09 10 11    addm6, ..., addm11,
    2 12 13    14 15 16 17    addm12, addm13, addm14...29,
    3 18 19 20    21 22 23    addm14...29,
    4 24 25 26 27    28 29    addm14...29
    5
    6
    7

    n = dst * 6 + src - (src > dst)
    dst = n / 6
    src = n % 6 + (n % 6 >= n / 6)


##### 4.3.2.2 subm dst, (src + ximm7)

Equivalent to:

    lw  r5, (src + ximm7)
    sub dst, r5

Encoding:

    dst\src:
     \ 0  1  2  3  4  5  6  7
    0    00 01 02 03 04 05    subm0x
    1 00    01 02 03 04 05    subm1x
    2 00 01    02 03 04 05    subm2x
    3 00 01 02    03 04 05    subm3x
    4 00 01 02 03    04 05    subm4x
    5
    6
    7

    n = src - (src > dst)
    src = n + (n >= dst)


##### 4.3.2.3 mrs r, s

Encoding of registers r and s in 5-bit rPPPQ:

    r\s:
     \ 0  1  2  3  4  5  6  7
    0    00 01 02 03 04
    1 05    06 07 08 09
    2 10 11    12 13 14
    3 15 16 17    18 19
    4 20 21 22 23    24
    5 25 26 27 28 29
    6
    7

    n = r * 5 + s - (s > r)
    r = n / 5
    s = n % 5 + (n % 5 >= n / 5)


### 4.4 Instruction set summary

- lb/lw r, (r + ximm7/r)
- sb/sw r, (r + ximm7/r)
- addm/subm r, (r + ximm7)
- (d)incm/(d)decm (r)
- (d)incm/(d)decm r, (sp + imm7)
- push simm7/r
- pop r
- ls5r/ss5r r
- last imm7
- li r, simm9
- and/or/xor r, imm7/r
- cmp r, simm7
- add r, r, simm7
- addu r, imm9
- lurpc r, imm9
- sac r, r, imm4
- sr/sl/asr/rl r, imm4
- zxt/sxt/cpl/neg/adcz r
- sr/sl/asr/rl/rr r, r
- mov r, r
- add/sub/adc/sbb/cmp r, r
- add22adc33, cadd24, cadd24adc3z, csub34
- stc
- j\<cond\> simm7\*2
  (14 conds: c/lu, z/e, s, leu, l, le, nc/geu, nz/ne, ns, gu, ge, g, o, no)
- j simm8\*2
- jal simm11\*2
- swi simm6\*2
- reti
- di/ei
- mf2/m2f
- msr s, r / mrs r, s


### 4.5 The mini

There's an alternative, minimum ISA, which has a more regular instruction
encoding and a slightly reduced instruction set. The decoder ROM for the
mini is much smaller, 2KB compared to 16KB of the full/maximum ISA.


#### 4.5.1 Overall mini encodings

    FEDCBA9876543210
    00wrrrRRR7777777 lb/lw            rrr, (RRR + simm7) ; rrr!=sp,pc when lb
    000rrrRRR7777777   and            RRR, imm7          ; when lb, rrr==sp
    000rrrRRR7777777   or             RRR, imm7          ; when lb, rrr==pc
    01wrrrRRR7777777 sb/sw            rrr, (RRR + simm7)  ; rrr!=sp,pc when sb ; rrr!=pc when sw
    010rrrRRR7777777   xor            RRR, imm7           ; when sb, rrr==sp
    010rrrRRR7777777   cmp            RRR, simm7 ; stc    ; when sb, rrr==pc
    011rrr???7777777   push           simm7               ; when sw, rrr==pc
    100rrrRRR7777777 add              rrr, RRR, simm7 ; sl ; except {sp,pc,imm},{pc,sp,imm},{pc,pc,imm} ; `add pc, RRR, simm7` preserves flags (ditto `sp,RRR,simm7`)
    100rrr???7777771   swi            simm6*2         ; when rrr==sp, odd-valued simm7 ; invokes ISR at (simm7&(-2)), disabling interrupts
    101rrr9999999990 li               rrr, simm9          ; when rrr!=sp,pc ; rrr = simm9
    101rrr???7777770   last           imm6*2              ; when rrr==sp
    101rrrRRR7777770   decs           RRR, (sp + simm6*2) ; when rrr==pc
    101rrr9999999991 addu             rrr, imm9           ; when rrr!=pc ; rrr += imm9 << 7 ; `addu sp, imm9` preserves flags
    101rrr99999999?1   j              simm8*2             ; when rrr==pc
    110rreeeeeeeeeee jal              simm11*2  ; when rrr==sp,pc
    110rrr9999999990   lurpc          rrr, imm9 ; when rrr!=sp,pc ; rrr = pc + (imm9 << 7)
    110rrr?????????1   MULDIVHELPER<op0-3>      ; when 0<=rrr<=3
    110rrr?????????1   mf2                      ; when rrr==4
    110rrr?????????1   m2f                      ; when rrr==5
    111777777700cccc j<cond0-13>      simm7*2
    111rrr???0001110 zxt              rrr
    111rrr???1001110 sxt              rrr
    111rrr???0001111 cpl              rrr
    111rrr???1001111 neg              rrr
    111rrrRRR001oooo ALU<op0-13>      rrr, RRR
    111rrrRRR0011110 msr              RRR, rrr ; set RRR-reg-indexed selector from reg rrr
    111rrrRRR0011111 mrs              rrr, RRR ; get RRR-reg-indexed selector to reg rrr
    111rrrRRR1014444 sac              rrr, RRR, imm4 ; when imm4!=0
    111rrr???1010000   adcz           rrr            ; when imm4==0
    111rrr???0104444 asr              rrr, imm4 ; when imm4!=0
    111rrr???0100000   push           rrr       ; when imm4==0
    111rrr???1104444 rl               rrr, imm4  ; when imm4!=0
    111rrr???1100000   pop            rrr        ; when imm4==0
    111rrr???0114444 sr               rrr, imm4 ; when imm4!=0
    111??????0110000   reti                     ; when imm4==0 ; returns from ISR, restoring interrupt enable/disable
    111rrr???1114444 sl               rrr, imm4       ; when 2<=imm4<=15
    111rrrRRR1110000   lw/lwp         rrr, (RRR + pc) ; when imm4==0 ; lw in program/code space
    111rrrRRR1110001   sw/swp         rrr, (RRR + pc) ; when imm4==1 ; sw in program/code space

Jump conditions 0 through 13:

- 0000: cond0:c/lu
- 0001: cond1:z/e
- 0010: cond2:s
- 0011: cond3:leu
- 0100: cond4:l
- 0101: cond5:le
- 0110: cond6:o
- 0111: cond7:no
- 1000: cond8:nc/geu
- 1001: cond9:nz/ne
- 1010: cond10:ns
- 1011: cond11:gu
- 1100: cond12:ge
- 1101: cond13:g

`ALU rrr, RRR` ops 0 through 13:

- 0000: sr  rrr, RRR
- 0001: sl  rrr, RRR
- 0010: rr  rrr, RRR
- 0011: rl  rrr, RRR
- 0100: asr rrr, RRR
- 0101: xor rrr, RRR
- 0110: and rrr, RRR
- 0111: or  rrr, RRR
- 1000: adc rrr, RRR
- 1001: sbb rrr, RRR
- 1010: add rrr, RRR
- 1011: sub rrr, RRR
- 1100: cmp rrr, RRR
- 1101: mov rrr, RRR

MULDIVHELPER op/rrr 0 through 3:

- 0: add22adc33
- 1: cadd24
- 2: cadd24adc3z
- 3: csub34


#### 4.5.2 What didn't make it into the mini

What's not in the mini:

- `ei` and `di` instructions. Their functionality is implemented through
  dedicated ISRs and relies on the `swi` and `reti` instructions, which
  save and restore the external/hardware interrupt enable flag to/from the
  stack, where it can be accessed.
- Addressing data on the stack relative to `sp` through
  `lb/lw/sb/sw r, (sp + imm7)` is only half-range:
  `lb/lw/sb/sw r, (sp + simm7)`, that is, the immediate is always signed
  and doesn't become unsigned when used in combination with `sp`.
- `lb/lw/sb/sw r, (r + r)` are limited to only `lw/sw r, (r + pc)`, AKA
  `lwp/swp r, r`.
- `addm` and `subm` instructions.
- `(d)incm/(d)decm` are limited to only `decm/decs r, (sp + simm7)`.
- `ls5r` and `ss5r` instructions.


### 5 Memory accesses

#### 5.1 Logical and physical addresses

As the general-purpose registers r0 through r5, sp (AKA r6) and pc (AKA r7)
are all 16-bit, each of them can hold a 16-bit address. Some instructions
add an immediate to to the value contained in a register or add together the
values of two registers to form a memory address (prime instruction examples
here would be lb/lw and sb/sw) and the sum is truncated to 16 bits. Such a
16-bit address is the logical address and this is the kind of address that
programs manipulate directly and most of the time.

However, the logical address exists only inside the CPU. Outside the CPU,
there's the physical address. The CPU maps/translates each logical address
to the corresponding 22-bit physical address through the so-called selector
registers, of which there are 8: sel0 through sel7.

The physical address can then be decoded by the circuitry external to the
CPU to see which connected device (usually the memory) should respond to
reads and writes in certain ranges of the 22-bit physical address space.


#### 5.2 Program/code and data spaces

A 16-bit logical address can be used to access program's code or data. But
which one of the two is it? It depends on the kind of access. When a new
instruction is read/fetched from memory for execution, the address
contained in pc is used to access the code. When a load/store instruction
forms its logical address using pc, the code is accessed as well. All other
memory accesses (that is, not involving pc) are program's data accesses.

This separation into code and data accesses gives rise to two separate
logical address spaces: the logical program/code space and the logical data
space.

With two spaces, each up to 64KB in size, a program can be as big as 128KB
less any OS size.

While the two 16-bit spaces can be mapped/translated independently of one
another to the 22-bit physical address space, they can also overlap, if so
is desired.


#### 5.3 Mapping/translation

Mapping/translation of logical addresses to physical addresses is done in
16KB-aligned blocks of 16KB.

The program/code space can map/translate to four different 16KB blocks of
memory (through selector registers sel0 through sel3) and so can the data
space (through selector registers sel4 through sel7). There are 256 16KB
blocks in the physical space to select from, allowing for up to 4MB of
memory (some of this 4MB may be occupied by memory-mapped devices instead
of ROM or RAM).

A typical setup might look something like this:

          Logical                Physical                  Logical
          program/code           space                     data
          space                                            space
    
     0KB +----------+ sel0=0    +----------+       sel4=1 +----------+  0KB
         | Sys code |---------->| Sys ROM  |  +-----------| Sys data |
         +----------+           +----------+  |           +----------+
    16KB +----------+ sel1=2    +----------+  |    sel5=5 +----------+ 16KB
         | App code |--------+  |   RAM    |<-+  +--------| App data |
    32KB +----------+ sel2=3 |  +----------+     | sel6=6 +----------+ 32KB
         | App code |-----+  +->|   RAM    |     |  +-----| App data |
    48KB +----------+     |     +----------+     |  |     +----------+ 48KB
         | App code |--+  +---->|   ...    |     |  |  +--| App data |
    64KB +----------+  |        +----------+     |  |  |  +----------+ 64KB
                       +------->|          |     |  |  |
                      sel3=4    +----------+     |  |  |
                                |          |<----+  |  |
                                +----------+        |  |
                                |          |<-------+  |
                                +----------+           |
                                |          |<----------+
                                +----------+       sel7=7
                                |          |
                                +----------+
                                    ...

Example of overlapping spaces:

          Logical                Physical                  Logical
          program/code           space                     data
          space                                            space
    
     0KB +----------+ sel0=0    +----------+       sel4=1 +----------+  0KB
         | Sys code |---------->| Sys ROM  |  +-----------| Sys data |
         +----------+           +----------+  |           +----------+
    16KB +----------+ sel1=2    +----------+  |    sel5=2 +----------+ 16KB
         | App code |--------+  |   RAM    |<-+  +--------|          |
    32KB +----------+ sel2=3 |  +----------+     | sel6=3 +----------+ 32KB
         |          |-----+  +->|   RAM    |<----+  +-----| App data |
    48KB +----------+     |     +----------+        |     +----------+ 48KB
         |          |--+  +---->|   ...    |<-------+  +--| App data |
    64KB +----------+  |        +----------+           |  +----------+ 64KB
                       +------->|          |<----------+
                      sel3=4    +----------+       sel7=4
                                |          |
                                +----------+
                                    ...


#### 5.4 Multitasking

When the system starts up following a reset, execution starts at pc=0 with
sel0=0 and sel4=0, allowing for the system ROM at physical address 0. The
OS kernel (its code) is expected to be entirely contained within the 16KB
ROM. Its data (also up to 16KB) can lie somewhere in RAM at a higher
physical address.

A user/non-system program/application can occupy both the logical program/
code space and the logical data space starting at logical address 16KB.

If a second program/app is already loaded into memory, switching to it
will amount to little more than loading new values into sel1 through sel3
and sel5 through sel7 and loading new values into r0 through r7 and the
flags register.

The switching is performed by the OS kernel that occupies the logical
address spaces below address 16KB and rarely (if ever) changes its
corresponding selector registers sel0 and sel4 after it starts (unless,
it has dynamic parts or the ROM only contains a boot loader, in which
case sel0 should eventually switch to point to RAM with a loaded kernel).

The switching can typically be triggered by only two events: an external/
hardware interrupt (typically, a periodic interrupt from a timer device)
or a software interrupt, more specifically, a system call by means of the
swi instruction (`swi 2` through `swi 30` should be available). Both of
these events transfer control to an ISR located below logical 16KB. The
switching code can therefore safely change sel1 through sel3 and sel5
through sel7 without affecting itself in the process. It only needs to take
care when switching the current stack, which belongs to the program/app and
is expected to be above logical 16KB. The current stack's precise location
differs between different programs at different stages of their execution.


### 6 Proposed ABI

For subroutines we shall have:
- caller-saved: r0, r1, r5, arithmetic flags
- callee-saved: r2, r3, r4, sp
- return value:
  - 8-bit: r0 (if signed 8-bit, sign-extended to 16 bits, else zero-extended
    to 16 bits)
  - 16-bit: r0
  - 32-bit: r0 (least significant), r1 (most significant);
    this is an option, see below for another option
  - all others (larger or structures) via a hidden/implicit pointer argument
    (passed before the first formal argument), r0 set to it on return
    from the subroutine
- arguments: stack (C argument passing: leftmost argument at lowest address,
  rightmost argument at highest address; numeric types wider than 16 bits
  should be stored in the little-endian byte order)

For SWIs we shall have more relaxed rules:
- caller-saved: r5, arithmetic flags
- callee-saved: sp
- return value: register(s)
- arguments: registers and/or stack
- implementation-defined: r0-r4 may be all or partially caller-saved

Anyhow, r5 should normally be used as a temporary register (and, of course,
the link/return address register in subroutines) and the arithmetic flags
can be freely clobbered as well.


### 7 Proposed assembler macro instructions

Prerequisites:
- r5 is reserved for temporary operands in extra/implicit operations
  and r5 can only be used inside macro instructions when the assembler
  is in macro mode
- special kind of value supported, VAL, which may be one of the following:
  - symbol address in code/data
  - 16-bit constant (when the constant is the target of a pc-relative
    instruction, a distinct relocation type is needed to distinguish
    the constant target from the constant distance)
  - sum of the two above

Proposed macro instructions:
- nop
- clc
- li r, VAL ; special/atomic case for sp
- lurpc/addu r, hi(VAL)
- push VAL/r
- add r, [r,] VAL/lo(VAL)/r ; atomic for sp and non-atomic otherwise
- and/or/xor/cmp/sub r, VAL/r
- lb/lw/sb/sw/addm/subm r, (r + VAL/lo(VAL))
- lb/lw/sb/sw/addm/subm r, (r/VAL)
- may have synthetic andm/orm/xorm/cmp/adcm?/sbbm? similar to addm/subm
- (d)incm/(d)decm r, (sp + imm16)
- j\<cond\> VAL
- j/jal VAL/r
  - N.B. generated subroutine prolog and epilog needed to save/restore r5
    if it's used by subroutine in macro instructions (actually, any/all
    instructions)
- last [imm]

Non-macro instructions that can have relocated immediate operand:
- lo(sym + const):
  - li r, simm9 ; lo() sign-extended from 7 to 9 bits
  - add r, r, simm7
  - lb/lw/sb/sw/addm/subm r, (r + simm7)
- hi(sym + const):
  - addu r, imm9
  - lurpc r, imm9 ; sym is relative to pc of next instruction

