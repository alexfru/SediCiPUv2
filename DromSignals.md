# SediCiPU 2: Control signals from decoder ROM

The decoder/control ROM produces 32 bits of control signals for every
instruction, some signals are shared/multipurpose:

- OP
  - 4-bit ALU operation code.
    ALU operation and relevant signals within ALU:

        0000 sr          SEXT=0 ROT=0 LEFT=0
        0001 sl          SEXT=0 ROT=0 LEFT=1
        0010 rr          SEXT=0 ROT=1 LEFT=0
        0011 rl          SEXT=0 ROT=1 LEFT=1
        0100 asr         SEXT=1 ROT=0 LEFT=0
        0101 reserved
        0110 reserved
        0111 xor
        1000 add         CI=0 SUB=0
        1001 sub/cmp     CI=0 SUB=1
        1010 adc         CI=I SUB=0
        1011 sbb         CI=I SUB=1
        1100 zxt
        1101 sxt
        1110 and
        1111 or

  - Instructions: all.
- RL
  - Feeds RegFile's QSELL.
  - ((QSELL == 7) | (QSELR == 7)) drives MiscIntMem's D1.
  - Instructions: many.
- RLOE
  - Drives RegFile's OEL.
  - Drives DelayReg's nOE.
  - ((RLOE & CNZ & Flags' CO) | !CNZ) widened to 16 bits, ANDed with
    output of selection between RegFile's RL and DelayReg's Q, feeds
    ALU's DIL. RL vs 0 vs DelayReg selection at ALU's DIL:
    - We need to choose between 3 values: RL vs 0 vs DelayReg, e.g.:
      - RLOE selects between RL (RLOE==1) and DelayReg (RLOE==0)
      - each bit of selector's output is ANDed with F:

            CNZ  Flags' CO  RLOE -> F  Result of selection
              0          X     0    1  DelayReg
              0          X     1    1  RL
              1          X     0    0  0
              1          0     1    0  0  \ That is, RL or 0,
              1          1     1    1  RL / depending on Flags' CO
            
            F = !CNZ | (CNZ & Flags' CO & RLOE) =
                !(CNZ & !(CNZ & Flags' CO & RLOE))

  - Instructions: many.
- RR
  - One of two inputs to the selector that feeds RegFile's QSELR
    (the other being instruction's qqq field).
  - Instructions: many.
- RROE
  - Drives selection of ALU's DIR between RegFile's QR (RROE==1) and
    immediate (RROE==0).
  - Instructions: many.
- RI
  - Feeds RegFile's DSEL.
  - Instructions: many.
- RIWE
  - Drives RegFile's WE.
  - Instructions: many.
- IMM
  - Feeds ImmDecoder's SEL. IMM encoded and decoded values and some examples:

        000 -1                               ; (d)incm/(d)decm, add, sb/sw, ls5r/ss5r, last, jal, mrs/msr, stc, cpl, mf0
        001 imm7                             ; and/or/xor, l/s, last, sac/sr/sl/asr/rl, (d)incm/(d)decm
        010 simm7                            ; add/cmp, push, l/s, swi, incm/decm
        011 (simm7 * condition_is_true) << 1 ; j<cond>
        100 imm9 << 7                        ; addu, lurpc
        101 simm9                            ; li, j
        110 simm11 << 1                      ; jal
        111 -2                               ; push/pop, dincm/ddecm, swi/reti, ls5r/ss5r

  - Instructions: many.
- RRBUSOE
  - Drives connection of RegFile's QR to the data bus.
  - Instructions: push r, sb/sw, ss5r, add odd_imm/link, jal, msr.
- ALUOE
  - Drives connection of ALU's DO to the data bus.
  - Instructions: many.
- FLAGSOE
  - Drives connection of Flags' DO to the data bus.
  - Instructions: m0f.
- FLAGSWE
  - Drives Flags' nWE.
  - Instructions: (d)incm/(d)decm, addm/subm, adcz, and/or/xor,
    add/sub/adc/sbb/cmp, addu, stc, neg, sr/sl/asr/rl/rr, sac, mf0,
    add00adc11, cadd02, cadd02adc1z, csub12.
- IADDRSEL
  - Drives selection of the address from either ALU's DO (IADDRSEL==0) or
    DelayReg's Q (IADDRSEL==1).
  - Drives selection of:
    - RegFile's PC0 (0 (IADDRSEL==0) vs MiscIntMem's Q0 (IADDRSEL==1))
    - MiscIntMem's D0 (SELIFLAGSSEL (IADDRSEL==0) vs data bus bit 0
      (IADDRSEL==1))
  - Drives MiscIntMem's DEL1.
  - Instructions: (d)incm/(d)decm, swi, pushi, reti, sb2/sw2.
- IWE
  - Drives MiscIntMem's WE0.
  - Instructions: swi, di/ei, reti.
- SELE
  - Drives SelRegFile's OWE.
  - Instructions: mrs/msr.
- SELIFLAGSSEL
  - One bit value to select MiscIntMem's D0 from.
  - Drives SelRegFile's OnW.
  - Drives Flags' SEL, which selects the source of Flags' new value
    (ALU's flag outputs (only flags updated; SELIFLAGSSEL==0) vs
    data bus (flags and interrupt state updated; SELIFLAGSSEL==1)).
  - Instructions: mrs, mf0, ei.
- CNZ
  - Input to ((RLOE & CNZ & Flags' CO) | !CNZ), see RLOE.
  - Instructions: adcz, swi, push simm7, li, neg, cadd02, cadd02adc1z,
    csub12, mov.
- MWE
  - Enables writing a byte or word to memory from the data bus.
  - Instructions: (d)incm/(d)decm, push, sb/sw, ss5r, swi.
- MOE
  - Enables reading a byte or word from memory to the data bus.
  - Instructions: (d)incm/(d)decm, pop, lb/lw, ls5r, addm/subm, reti.
- W16
  - Selects 16-bit word as the size of memory read/write.
  - Instructions: (d)incm/(d)decm, lw, sw, ls5r, ss5r, addm/subm, push,
    pop, swi, reti.
- CRST
  - Drives synchronous reset of the counter that selects the instruction
    cycle (fetch, execute1, execute2-optional)
  - Instructions: all.
