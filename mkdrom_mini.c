/*
Copyright (c) 2024, Alexey Frunze
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
  SediCiPU2 (the MINI variant) decoder ROM generator.

  How to compile: gcc -std=c99 -O2 -Wall mkdrom_mini.c -o mkdrom_mini.exe
*/

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define STATIC_ASSERT(x) extern char StAtIcAsSeRt[(x) ? 1 : -1]

STATIC_ASSERT(CHAR_BIT == 8);

enum
{
  CLK_BITS   = 1,
  INSTR_BITS = 8,
  IN_BITS    = CLK_BITS + INSTR_BITS,
  IN_CNT     = 1 << IN_BITS
};

enum
{
  POS_OP,
  POS_RL           = POS_OP + 4,
  POS_RLOE         = POS_RL + 3,
  POS_RR,
  POS_RROE         = POS_RR + 3,
  POS_RI,
  POS_RIWE         = POS_RI + 3,
  POS_IMM,
  POS_RRBUSOE      = POS_IMM + 3,
  POS_ALUOE,
  POS_FLAGSOE,
  POS_FLAGSWE,
  POS_IADDRSEL,
  POS_IWE,
  POS_SELE,
  POS_SELIFLAGSSEL,
  POS_CNZ,
  POS_MWE,
  POS_MOE,
  POS_W16,
  POS_CRST
};

typedef unsigned short ushort;
typedef unsigned uint;
typedef unsigned long ulong;

ulong rom[IN_CNT];

uint idx0_6(uint clk, uint n0_6, uint rrr, uint L/*bit 0*/)
{
  return (clk << INSTR_BITS) | (0 << (INSTR_BITS - 1)) |
         (n0_6 << (3 + 1)) | (rrr << 1) | L;
}

uint idx7(uint clk, uint b6/*bit 6*/, uint b54/*bits 5,4*/, uint b30/*bits 3-0*/)
{
  return (clk << INSTR_BITS) | (1 << (INSTR_BITS - 1)) |
         (b6 << 6) | (b54 << 4) | b30;
}

void fill(uint idx,
          ulong CRST,
          ulong MOE, ulong MWE, ulong W16,
          ulong RI, ulong RIWE, ulong RL, ulong RLOE, ulong RR, ulong RROE, ulong IMM,
          ulong OP, ulong ALUOE, ulong FLAGSWE, ulong IADDRSEL, ulong FLAGSOE, ulong CNZ,
          ulong RRBUSOE,
          ulong IWE, ulong SELE, ulong SELIFLAGSSEL)
{
  rom[idx] = (CRST << POS_CRST) |
             (MOE << POS_MOE) | (MWE << POS_MWE) | (W16 << POS_W16) |
             (RI << POS_RI) | (RIWE << POS_RIWE) |
             (RL << POS_RL) | (RLOE << POS_RLOE) |
             (RR << POS_RR) | (RROE << POS_RROE) | (IMM << POS_IMM) |
             (OP << POS_OP) |
             (ALUOE << POS_ALUOE) | (FLAGSWE << POS_FLAGSWE) |
             (IADDRSEL << POS_IADDRSEL) | (FLAGSOE << POS_FLAGSOE) | (CNZ << POS_CNZ) |
             (RRBUSOE << POS_RRBUSOE) |
             (IWE << POS_IWE) | (SELE << POS_SELE) | (SELIFLAGSSEL << POS_SELIFLAGSSEL);
}

FILE* startup(int argc, char* argv[], char** outname, int* bigendian)
{
  FILE* f;

  if (argc < 2 || argc > 3)
  {
lusage:
    fprintf(stderr,
            "Usage:\n"
            "  mkdrom [options] <output_file>\n"
            "Options:\n"
            "  -be   big-endian output\n");
    exit(EXIT_FAILURE);
  }

  if ((*bigendian = !strcmp(argv[1], "-be")) != 0)
  {
    if (argc < 3)
      goto lusage;
    *outname = argv[2];
  }
  else
  {
    if (argc > 2)
      goto lusage;
    *outname = argv[1];
  }

  if ((f = fopen(*outname, "wb")) == NULL)
  {
    fprintf(stderr, "Can't create file \"%s\"\n", *outname);
    exit(EXIT_FAILURE);
  }

  return f;
}

void writebytes(FILE* f, char* outname, int bigendian, int nbytes, ulong value)
{
  int i;
  if (bigendian)
  {
    ulong tmp = 0;
    for (i = 0; i < nbytes; i++)
    {
      tmp = (tmp << 8) | (value & 0xFFU);
      value >>= 8;
    }
    value = tmp;
  }
  for (i = 0; i < nbytes; i++)
  {
    if (fputc(value & 0xFFU, f) == EOF)
    {
      fprintf(stderr, "Can't write to file \"%s\"\n", outname);
      exit(EXIT_FAILURE);
    }
    value >>= 8;
  }
}

int main(int argc, char* argv[])
{
  int bigendian = 0;
  char* outname = NULL;
  FILE* f = startup(argc, argv, &outname, &bigendian);
  uint rrr, w, L, b6, b30;

#if 0
  idx0_6(/*clk*/, /*n0_6*/, /*rrr*/, /*L=bit 0*/)
  idx7(/*clk*/, /*b6*/, /*b54*/, /*b30*/)
  fill(/*idx*/,
       /*CRST*/,
       /*MOE*/, /*MWE*/, /*W16*/,
       /*RI*/, /*RIWE*/, /*RL*/, /*RLOE*/, /*RR*/, /*RROE*/, /*IMM*/0,
       /*OP*/, /*ALUOE*/, /*FLAGSWE*/, /*IADDRSEL*/, /*FLAGSOE*/, /*CNZ*/,
       /*RRBUSOE*/,
       /*IWE*/, /*SELE*/, /*SELIFLAGSSEL*/);
#endif

  // Fill the unused space with "code" for infinite loop.
  {
    uint idx;
    for (idx = 0; idx < IN_CNT; idx++)
    {
      // pc = pc + (-2)
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/7, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }

/*
7/8:vvvvvv         v: 7 bits to decode
    00wrrrRRR7777777 lb/lw            rrr, (RRR + simm7) ; rrr!=sp,pc when lb
    000rrrRRR7777777   and            RRR, imm7          ; when lb, rrr==sp
    000rrrRRR7777777   or             RRR, imm7          ; when lb, rrr==pc
*/
  for (w = 0; w <= 1; w++)
  for (rrr = 0; rrr <= 7; rrr++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/w, /*rrr*/rrr, /*L=bit 0*/L);
    if (rrr >= 6 && w == 0) // and/or
    {
      int is_or = rrr & 1;
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/1, /*RIWE*/1, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
           /*OP*/14 + is_or, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // lb/lw
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/1, /*MWE*/0, /*W16*/w,
           /*RI*/0, /*RIWE*/1, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2,
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
    01wrrrRRR7777777 sb/sw            rrr, (RRR + simm7)  ; rrr!=sp,pc when sb ; rrr!=pc when sw
    010rrrRRR7777777   xor            RRR, imm7           ; when sb, rrr==sp
    010rrrRRR7777777   cmp            RRR, simm7 ; stc    ; when sb, rrr==pc
    011rrr???7777777   push           simm7               ; when sw, rrr==pc
*/
  for (w = 0; w <= 1; w++)
  for (rrr = 0; rrr <= 7; rrr++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/2 + w, /*rrr*/rrr, /*L=bit 0*/L);
    if (rrr >= 6 && w == 0) // xor/cmp
    {
      int is_xor = (rrr == 6);
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/1, /*RIWE*/is_xor, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/(is_xor ? 1 : 2),
           /*OP*/(is_xor ? 7 : 9), /*ALUOE*/is_xor, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (rrr == 7 && w) // push simm7
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/0, /*MWE*/1, /*W16*/1,
           /*RI*/0, /*RIWE*/0, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/2,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/1, // mem[DelayReg=sp + (-2)] = 0 + simm7
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/6, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sp = sp + (-2)
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // sb/sw
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/1, /*W16*/w,
           /*RI*/0, /*RIWE*/0, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2,
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/1,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
    100rrrRRR7777777 add              rrr, RRR, simm7 ; sl ; except {sp,pc,imm},{pc,sp,imm},{pc,pc,imm} ; `add pc, RRR, simm7` preserves flags (ditto `sp,RRR,simm7`)
    100rrr???7777771   swi            simm6*2         ; when rrr==sp, odd-valued simm7 ; invokes ISR at (simm7&(-2)), disabling interrupts
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/4, /*rrr*/rrr, /*L=bit 0*/L);
    if (rrr == 6 && L) // swi
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/0, /*MWE*/1, /*W16*/1,
           /*RI*/0, /*RIWE*/0, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg=sp + (-2)] = pc + (-2) + interrupt enable flag
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/7, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/2,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // pc = 0 + simm7
           /*RRBUSOE*/0,
           /*IWE*/1, /*SELE*/0, /*SELIFLAGSSEL*/0); // interrupt enable flag = 0
    }
    else // add
    {
      if (L && rrr == 7) // add with link
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/5, /*RIWE*/1, /*RL*/1, /*RLOE*/1, /*RR*/7, /*RROE*/0, /*IMM*/2, // r5 = pc
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = RRR + simm7
             /*RRBUSOE*/1,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/7, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/0,
             /*OP*/14, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = DelayReg & -1
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // add without link
      {
        int is_quiet = (rrr >= 6);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/!is_quiet, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
  }
/*
    101rrr9999999990 li               rrr, simm9          ; when rrr!=sp,pc ; rrr = simm9
    101rrr???7777770   last           imm6*2              ; when rrr==sp
    101rrrRRR7777770   decs           RRR, (sp + simm6*2) ; when rrr==pc
    101rrr9999999991 addu             rrr, imm9           ; when rrr!=pc ; rrr += imm9 << 7 ; `addu sp, imm9` preserves flags
    101rrr99999999?1   j              simm8*2             ; when rrr==pc
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/5, /*rrr*/rrr, /*L=bit 0*/L);
    if (L) // addu, j
    {
      if (rrr == 7) // j
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/7, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/5,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = pc + simm9
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // addu
      {
        int is_quiet = (rrr == 6);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/4,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/!is_quiet, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = rrr + (imm9 << 7)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else // li, last, decs
    {
      if (rrr == 6) // last
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/6, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sp = sp + imm7
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/7, /*RIWE*/1, /*RL*/5, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
             /*OP*/14, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = r5 & -1
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else if (rrr == 7) // decs
      {
        int dec = 1;
        int dbl = 0;
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/1, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2, // DelayReg = sp + simm7
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // RRR = mem[sp + simm7]
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/(dbl ? 7 : 0),
             /*OP*/9 - dec, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = RRR +/- (-1)/(-2)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // li
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/5,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + simm9
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
  }
/*
    110rreeeeeeeeeee jal              simm11*2  ; when rrr==sp,pc
    110rrr9999999990   lurpc          rrr, imm9 ; when rrr!=sp,pc ; rrr = pc + (imm9 << 7)
    110rrr?????????1   add22adc33               ; when rrr==0
    110rrr?????????1   cadd24                   ; when rrr==1
    110rrr?????????1   cadd24adc3z              ; when rrr==2
    110rrr?????????1   csub34                   ; when rrr==3
    110rrr?????????1   mf2                      ; when rrr==4
    110rrr?????????1   m2f                      ; when rrr==5
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/6, /*rrr*/rrr, /*L=bit 0*/L);
    if (rrr >= 6) // jal
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/5, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/7, /*RROE*/0, /*IMM*/6, // r5 = pc
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = pc + simm11 * 2
           /*RRBUSOE*/1,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/7, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/0,
           /*OP*/14, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = DelayReg & -1
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (L) // mul/div helpers, mf2/m2f
    {
      switch (rrr)
      {
        default:
        case 0: // add22adc33
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/2, /*RIWE*/1, /*RL*/2, /*RLOE*/1, /*RR*/2, /*RROE*/1, /*IMM*/0,
               /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // add r2, r2
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          fill(/*idx*/idx + (1u << INSTR_BITS),
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/3, /*RIWE*/1, /*RL*/3, /*RLOE*/1, /*RR*/3, /*RROE*/1, /*IMM*/0,
               /*OP*/10, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // adc r3, r3
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          break;
        case 1: // cadd24
          fill(/*idx*/idx,
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/2, /*RIWE*/1, /*RL*/4, /*RLOE*/1, /*RR*/2, /*RROE*/1, /*IMM*/0,
               /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // add r2, (Carry ? r4 : 0)
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          break;
        case 2: // cadd24adc3z
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/2, /*RIWE*/1, /*RL*/4, /*RLOE*/1, /*RR*/2, /*RROE*/1, /*IMM*/0,
               /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // add r2, (Carry ? r4 : 0)
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          fill(/*idx*/idx + (1u << INSTR_BITS),
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/3, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/3, /*RROE*/1, /*IMM*/0,
               /*OP*/10, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // adcz r3
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          break;
        case 3: // csub34
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/3, /*RIWE*/1, /*RL*/3, /*RLOE*/1, /*RR*/4, /*RROE*/1, /*IMM*/0,
               /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sub r3, r4
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          fill(/*idx*/idx + (1u << INSTR_BITS),
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/3, /*RIWE*/1, /*RL*/4, /*RLOE*/1, /*RR*/3, /*RROE*/1, /*IMM*/0,
               /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // add r3, (Carry ? r4 : 0)
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          break;
        case 4: // mf2
        case 5: // m2f
          {
            int is_mff = (rrr == 5); // move from flags
            fill(/*idx*/idx,
                 /*CRST*/1,
                 /*MOE*/0, /*MWE*/0, /*W16*/0,
                 /*RI*/2, /*RIWE*/is_mff, /*RL*/2, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
                 /*OP*/14, /*ALUOE*/!is_mff, /*FLAGSWE*/!is_mff, /*IADDRSEL*/0, /*FLAGSOE*/is_mff, /*CNZ*/0,
                 /*RRBUSOE*/0,
                 /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/!is_mff); // mf2 updates arithmetic flags and interrupt control bits
          }
          break;
      }
    }
    else // lurpc
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/4,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = pc + (imm9 << 7)
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }

/*
1/8:         vvvvvvv: 7 bits to decode
    111777777700cccc j<cond0-13>      simm7*2
    111rrr???0001110 zxt              rrr
    111rrr???1001110 sxt              rrr
    111rrr???0001111 cpl              rrr
    111rrr???1001111 neg              rrr
*/
  for (b6 = 0; b6 <= 1; b6++)
  for (b30 = 0; b30 <= 15; b30++)
  {
    uint idx = idx7(/*clk*/0, /*b6*/b6, /*b54*/0, /*b30*/b30);
    if (b30 == 14) // zxt/sxt
    {
      int is_sxt = b6;
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/1, /*RR*/0, /*RROE*/1, /*IMM*/0,
           /*OP*/12 + is_sxt, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (b30 == 15) // cpl/neg
    {
      int is_neg = b6;
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/!is_neg, /*RR*/0, /*RROE*/is_neg, /*IMM*/0,
           /*OP*/(is_neg ? 9 : 7), /*ALUOE*/1, /*FLAGSWE*/is_neg, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/is_neg,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // j<cond>
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/7, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/3,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = pc + simm7 * condition_is_true * 2
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
    111rrrRRR001oooo ALU<op0-13>      rrr, RRR
    111rrrRRR0011110 msr              RRR, rrr ; set RRR-reg-indexed selector from reg rrr
    111rrrRRR0011111 mrs              rrr, RRR ; get RRR-reg-indexed selector to reg rrr
    111rrrRRR1014444 sac              rrr, RRR, imm4 ; when imm4!=0
    111rrr???1010000   adcz           rrr            ; when imm4==0
*/
  for (b6 = 0; b6 <= 1; b6++)
  for (b30 = 0; b30 <= 15; b30++)
  {
    uint idx = idx7(/*clk*/0, /*b6*/b6, /*b54*/1, /*b30*/b30);
    if (b6) // sac, adcz
    {
      if (b30) // sac
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/0, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
             /*OP*/1, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = RRR << imm4
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = DelayReg + rrr
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // adcz
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/1, /*IMM*/0,
             /*OP*/10, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + rrr + Carry
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else // ALU<op0-13>, msr/mrs
    {
      if (b30 <= 12) // ALU<op0-12>
      {
        uint a[13] =
        {
          /*sr*/0,
          /*sl*/1,
          /*rr*/2,
          /*rl*/3,
          /*asr*/4,
          /*xor*/7,
          /*and*/14,
          /*or*/15,
          /*adc*/10,
          /*sbb*/11,
          /*add*/8,
          /*sub*/9,
          /*cmp*/9
        };
        uint op = a[b30];
        int is_cmp = (b30 == 12);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/!is_cmp, /*RL*/0, /*RLOE*/1, /*RR*/1, /*RROE*/1, /*IMM*/0,
             /*OP*/op, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else if (b30 == 13) // ALU<op13> AKA mov
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/1, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + RRR
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // msr/mrs
      {
        if (b30 & 1) // mrs
        {
          fill(/*idx*/idx,
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/0, /*RIWE*/1, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
               /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = sel[RRR & -1]
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/1, /*SELIFLAGSSEL*/1);
        }
        else // msr
        {
          fill(/*idx*/idx,
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/0, /*RIWE*/0, /*RL*/1, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
               /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sel[RRR & -1] = rrr
               /*RRBUSOE*/1,
               /*IWE*/0, /*SELE*/1, /*SELIFLAGSSEL*/0);
        }
      }
    }
  }
/*
    111rrr???0104444 asr              rrr, imm4 ; when imm4!=0
    111rrr???0100000   push           rrr       ; when imm4==0
    111rrr???1104444 rl               rrr, imm4  ; when imm4!=0
    111rrr???1100000   pop            rrr        ; when imm4==0
*/
  for (b6 = 0; b6 <= 1; b6++)
  for (b30 = 0; b30 <= 15; b30++)
  {
    uint idx = idx7(/*clk*/0, /*b6*/b6, /*b54*/2, /*b30*/b30);
    if (b30) // asr/rl
    {
      int is_rl = b6;
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
           /*OP*/(is_rl ? 3 : 4), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // push/pop
    {
      if (b6) // pop
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/6, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
             /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sp = sp - (-2)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/0, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = mem[sp + (-2)]
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // push
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = rrr
             /*RRBUSOE*/1,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/6, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sp = sp + (-2)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
  }
/*
    111rrr???0114444 sr               rrr, imm4 ; when imm4!=0
    111??????0110000   reti                     ; when imm4==0 ; returns from ISR, restoring interrupt enable/disable
    111rrr???1114444 sl               rrr, imm4       ; when 2<=imm4<=15
    111rrrRRR1110000   lw             rrr, (pc + RRR) ; when imm4==0 ; lw in program/code space
    111rrrRRR1110001   sw             rrr, (pc + RRR) ; when imm4==1 ; sw in program/code space
*/
  for (b6 = 0; b6 <= 1; b6++)
  for (b30 = 0; b30 <= 15; b30++)
  {
    uint idx = idx7(/*clk*/0, /*b6*/b6, /*b54*/3, /*b30*/b30);
    if (b30) // sr/sl, sw
    {
      if (b6 && b30 == 1) // sw
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/0, /*RL*/7, /*RLOE*/1, /*RR*/1, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = pc + RRR
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/0, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
             /*OP*/14, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = rrr & -1
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // sr/sl
      {
        int is_sl = b6;
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/1, /*RL*/0, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
             /*OP*/(is_sl ? 1 : 0), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else // reti, lw
    {
      if (b6) // lw
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/0, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/1, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // reti
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/7, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/0,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // pc = mem[DelayReg=sp + (-2)]
             /*RRBUSOE*/0,
             /*IWE*/1, /*SELE*/0, /*SELIFLAGSSEL*/0); // interrupt enable flag = mem[DelayReg=sp + (-2)] & 1
      }
    }
  }

  {
    uint idx;
    for (idx = 0; idx < IN_CNT; idx++)
      writebytes(f, outname, bigendian, 4, rom[idx]);
    if (fclose(f))
    {
      fprintf(stderr, "Can't write to \"%s\"\n", outname);
      exit(EXIT_FAILURE);
    }
  }
  return 0;
}
