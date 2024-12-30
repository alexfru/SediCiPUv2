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
  SediCiPU2 decoder ROM generator.

  How to compile: gcc -std=c99 -O2 -Wall mkdrom.c -o mkdrom.exe
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
  INSTR_BITS = 11,
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

uint idx0_6(uint clk, uint n0_6, uint rrr, uint RRR, uint L/*bit 0*/)
{
  return (clk << INSTR_BITS) | (0 << (INSTR_BITS - 1)) |
         (n0_6 << (3 + 3 + 1)) |
         (rrr << (3 + 1)) | (RRR << 1) | L;
}

uint idx7(uint clk, uint rrr, uint m/*3 bits*/, uint PPP, uint Q/*1 bit*/)
{
  return (clk << INSTR_BITS) | (1 << (INSTR_BITS - 1)) |
         (rrr << (3 + 3 + 1)) | (m << (3 + 1)) | (PPP << 1) | Q;
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
  uint rrr, RRR, w, L, PPP, Q;

#if 0
  idx0_6(/*clk*/, /*n0_6*/, /*rrr*/, /*RRR*/, /*L=bit 0*/)
  idx7(/*clk*/, /*rrr*/, /*m=3 bits*/, /*PPP*/, /*Q=1 bit*/)
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
00wrrrRRR7777777 lb/lw            rrr, (RRR + ximm7) ; rrr!=sp,pc when lb
000rrrRRR7777777   and            RRR, imm7          ; when lb, rrr==sp, RRR!=sp,pc
000rrrRRR7777777   or             RRR, imm7          ; when lb, rrr==pc, RRR!=sp,pc
000rrrRRR7777777   addm0          r, (R+ximm7)       ; when lb, rrr==sp, RRR==sp
000rrrRRR7777777   addm1          r, (R+ximm7)       ; when lb, rrr==sp, RRR==pc
000rrrRRR7777777   addm2          r, (R+ximm7)       ; when lb, rrr==pc, RRR==sp
000rrrRRR7777777   addm3          r, (R+ximm7)       ; when lb, rrr==pc, RRR==pc
001rrrRRR7777771   incm/decm      (rrr), simm7       ; when lw, rrr!=sp,pc, RRR==sp, odd-valued simm7
001rrrRRR7777771   adcz           rrr[, imm6]        ; when lw, rrr!=sp,pc, RRR==pc, odd-valued imm7
*/
  for (w = 0; w <= 1; w++)
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 7; RRR++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/w, /*rrr*/rrr, /*RRR*/RRR, /*L=bit 0*/L);
    if (rrr <= 5 && w && RRR == 6 && L) // incm/decm
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/1, /*MWE*/0, /*W16*/1,
           /*RI*/rrr, /*RIWE*/1, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0, // DelayReg = rrr & -1
           /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = mem[rrr & -1]
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/1, /*W16*/1,
           /*RI*/0, /*RIWE*/0, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = rrr + simm7
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (rrr <= 5 && w && RRR == 7 && L) // adcz
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/rrr, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/rrr, /*RROE*/1, /*IMM*/0,
           /*OP*/10, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + rrr + Carry
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (w || rrr <= 5) // lb/lw
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/1, /*MWE*/0, /*W16*/w,
           /*RI*/rrr, /*RIWE*/1, /*RL*/RRR, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (RRR == 6),
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (RRR <= 5) // and/or
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/RRR, /*RIWE*/1, /*RL*/RRR, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
           /*OP*/14 + (rrr & 1), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // addm0...3 r, (R+ximm7)
    {
      uint rR = (rrr & 1) * 2 + (RRR & 1);
      uint r = rR / 6;
      uint mod = rR % 6;
      uint R = mod + (mod >= r);
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/1, /*MWE*/0, /*W16*/1,
           /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }

/*
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
*/
  for (w = 0; w <= 1; w++)
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 7; RRR++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/2 + w, /*rrr*/rrr, /*RRR*/RRR, /*L=bit 0*/L);
    if (rrr == RRR && RRR <= 5) // subm0x,subm1x RRR / push r2/r3
    {
      if (rrr == 5 && L) // push r2/r3
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/2 + w, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = r2/r3
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
      else // subm0x,subm1x RRR
      {
        uint r = w;
        uint R = RRR + (RRR >= r);
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
             /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else if (w && rrr == 7 && RRR <= 5) // subm2x RRR / push r4
    {
      if (RRR == 5 && L) // push r4
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/4, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = r4
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
      else // subm2x RRR
      {
        uint r = 2;
        uint R = RRR + (RRR >= r);
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
             /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else if (rrr <= 5 && w && RRR >= 6 && L) // dincm/ddecm
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/1, /*MWE*/0, /*W16*/1,
           /*RI*/rrr, /*RIWE*/1, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0, // DelayReg = rrr & -1
           /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = mem[rrr & -1]
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/1, /*W16*/1,
           /*RI*/0, /*RIWE*/0, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
           /*OP*/9 - (RRR & 1), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = rrr +/- (-2)
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if ((w && rrr <= 6) || (!w && rrr <= 5)) // sb/sw
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/1, /*W16*/w,
           /*RI*/0, /*RIWE*/0, /*RL*/RRR, /*RLOE*/1, /*RR*/rrr, /*RROE*/0, /*IMM*/2 - (RRR == 6),
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/1,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else if (!w) // xor/cmp/j<cond>
    {
      if (RRR <= 5) // xor/cmp
      {
        int is_xor = (rrr == 6);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/RRR, /*RIWE*/is_xor, /*RL*/RRR, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/(is_xor ? 1 : 2),
             /*OP*/(is_xor ? 7 : 9), /*ALUOE*/is_xor, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // addm4...7 r, (R+ximm7) / push r0
      {
        uint rR = 4 + (rrr & 1) * 2 + (RRR & 1);
        uint r = rR / 6;
        uint mod = rR % 6;
        uint R = mod + (mod >= r);
        if (rR == 5 && L) // push r0
        {
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/0, /*MWE*/1, /*W16*/1,
               /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
               /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = r0
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
        else // addm4...7 r, (R+ximm7)
        {
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/1, /*MWE*/0, /*W16*/1,
               /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
               /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          fill(/*idx*/idx + (1u << INSTR_BITS),
               /*CRST*/1,
               /*MOE*/0, /*MWE*/0, /*W16*/0,
               /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
               /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        }
      }
    }
    else // addm12...13 r, (R+ximm7)
    {
      uint rR = 12 + (RRR & 1);
      uint r = rR / 6;
      uint mod = rR % 6;
      uint R = mod + (mod >= r);
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/1, /*MWE*/0, /*W16*/1,
           /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }

/*
100rrrRRR7777777 add              rrr, RRR, simm7 ; except {sp,pc,imm},{pc,sp,imm},{pc,pc,imm} ; `add pc, RRR, simm7` preserves flags (ditto `sp,RRR,simm7`)
100rrrRRR7777771   swi            simm6*2         ; when rrr==sp, RRR==sp, odd-valued simm7 ; invokes ISR at (simm7&(-2)), disabling interrupts
100rrrRRR7777777   push           simm7           ; when rrr==sp, RRR==pc
100rrrRRR7777777   addm8          r, (R+ximm7)    ; when rrr==pc, RRR==sp
100rrrRRR7777777   addm9          r, (R+ximm7)    ; when rrr==pc, RRR==pc
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 7; RRR++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/4, /*rrr*/rrr, /*RRR*/RRR, /*L=bit 0*/L);
    if ((rrr == 6) && (RRR == 6) && L) // swi
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
    else if ((rrr == 6) && (RRR == 7)) // push simm7
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
    else if (((rrr <= 6) && (RRR <= 6)) ||
             ((rrr == 7) && (RRR <= 5)) ||
             ((rrr <= 5) && (RRR == 7))) // add
    {
      if ((rrr == 7) && L) // add with link
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/5, /*RIWE*/1, /*RL*/RRR, /*RLOE*/1, /*RR*/7, /*RROE*/0, /*IMM*/2, // r5 = pc
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
      else if ((rrr != 6) || !L) // add without link
      {
        int is_quiet = (rrr >= 6);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/rrr, /*RIWE*/1, /*RL*/RRR, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/!is_quiet, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else if ((rrr == 7) && (RRR >= 6)) // addm8...9 r, (R+ximm7)
    {
      uint rR = 8 + (RRR & 1);
      uint r = rR / 6;
      uint mod = rR % 6;
      uint R = mod + (mod >= r);
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/1, /*MWE*/0, /*W16*/1,
           /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
           /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }

/*
101rrr0999999999 li               rrr, simm9   ; when rrr!=sp,pc ; rrr = simm9
101rrr1999999999 addu             rrr, imm9    ; when rrr!=pc ; rrr += imm9 << 7 ; `addu sp, imm9` preserves flags
101rrr0999999990   ?              simm8*2      ; when rrr==sp, even-valued imm9
101rrr0999999991   j              simm8*2      ; when rrr==sp, odd-valued imm9
101rrrRRR7777777   subm3x         r, (R+ximm7) ; when rrr==pc, RRR!=sp,pc
101rrrRRR7777771     push         r5           ; when rrr==pc, RRR==5, odd-valued imm7
101rrrRRR7777777   addm10         r, (R+ximm7) ; when rrr==pc, RRR==sp
101rrrRRR7777777   addm11         r, (R+ximm7) ; when rrr==pc, RRR==pc
101rrrRRR7777771     push         r1           ; when rrr==pc, RRR==pc, odd-valued imm7
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 7; RRR++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/5, /*rrr*/rrr, /*RRR*/RRR, /*L=bit 0*/L);
    if ((rrr == 6) && (RRR < 4)) // j/?
    {
      if (L) // j
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/7, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/5,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // pc = pc + simm9
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else if (rrr != 7) // li/addu
    {
      if (RRR >= 4) // addu
      {
        int is_quiet = (rrr == 6);
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/rrr, /*RIWE*/1, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/4,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/!is_quiet, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = rrr + (imm9 << 7)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // li
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/rrr, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/5,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + simm9
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else if (RRR <= 5) // subm3x RRR / push r5
    {
      if (RRR == 5 && L) // push r5
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/5, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = r5
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
      else // subm3x RRR
      {
        uint r = 3;
        uint R = RRR + (RRR >= r);
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
             /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else // addm10...11 r, (R+ximm7) / push r1
    {
      uint rR = 10 + (RRR & 1);
      uint r = rR / 6;
      uint mod = rR % 6;
      uint R = mod + (mod >= r);
      if (rR == 11 && L) // push r1
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/0, /*MWE*/1, /*W16*/1,
             /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/1, /*RROE*/0, /*IMM*/7,
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // mem[sp + (-2)] = r1
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
      else // addm10...11 r, (R+ximm7)
      {
        fill(/*idx*/idx,
             /*CRST*/0,
             /*MOE*/1, /*MWE*/0, /*W16*/1,
             /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
             /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        fill(/*idx*/idx + (1u << INSTR_BITS),
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
  }

/*
110rrr0007777777 incm             rrr, (sp+imm7) ; when rrr!=sp,pc
110rrr0017777777 decm             rrr, (sp+imm7) ; when rrr!=sp,pc
110rrr0107777777 dincm            rrr, (sp+imm7) ; when rrr!=sp,pc
110rrr0117777777 ddecm            rrr, (sp+imm7) ; when rrr!=sp,pc
110rrr0007777771   ls5r           rrr [, imm6]   ; when rrr!=5,sp,pc, odd-valued imm7
110rrr0007777771     last         imm6*2         ; when rrr==5, odd-valued imm7
110rrr0017777771   ss5r           rrr [, imm6]   ; when rrr!=5,sp,pc, odd-valued imm7
110rrr1999999999 lurpc            rrr, imm9      ; when rrr!=sp,pc ; rrr = pc + (imm9 << 7)
110rreeeeeeeeeee   jal            simm11*2       ; when rrr==sp,pc
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 7; RRR++)
  for (L = 0; L <= 1; L++)
  {
    uint idx = idx0_6(/*clk*/0, /*n0_6*/6, /*rrr*/rrr, /*RRR*/RRR, /*L=bit 0*/L);
    if (rrr <= 5) // (d)incm/(d)decm/lurpc
    {
      if (RRR < 4) // (d)incm/(d)decm, ls5r/ss5r, last
      {
        if (L == 0) // (d)incm/(d)decm
        {
          int dec = RRR & 1;
          int dbl = RRR >> 1;
          fill(/*idx*/idx,
               /*CRST*/0,
               /*MOE*/1, /*MWE*/0, /*W16*/1,
               /*RI*/rrr, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1, // DelayReg = sp + imm7
               /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = mem[sp + imm7]
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          fill(/*idx*/idx + (1u << INSTR_BITS),
               /*CRST*/1,
               /*MOE*/0, /*MWE*/1, /*W16*/1,
               /*RI*/0, /*RIWE*/0, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/(dbl ? 7 : 0),
               /*OP*/9 - dec, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = rrr +/- (-1)/(-2)
               /*RRBUSOE*/0,
               /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
        }
        else // ls5r/ss5r, last
        {
          if (RRR == 0 && rrr <= 4) // ls5r
          {
            fill(/*idx*/idx,
                 /*CRST*/0,
                 /*MOE*/1, /*MWE*/0, /*W16*/1,
                 /*RI*/5, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0, // r5 = mem[sp & -1]
                 /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
                 /*RRBUSOE*/0,
                 /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
            fill(/*idx*/idx + (1u << INSTR_BITS),
                 /*CRST*/1,
                 /*MOE*/1, /*MWE*/0, /*W16*/1,
                 /*RI*/rrr, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7, // rrr = mem[sp - (-2)]
                 /*OP*/9, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
                 /*RRBUSOE*/0,
                 /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          }
          else if (RRR == 0 && rrr == 5) // last
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
          else if (RRR == 1 && rrr <= 4) // ss5r
          {
            fill(/*idx*/idx,
                 /*CRST*/0,
                 /*MOE*/0, /*MWE*/1, /*W16*/1,
                 /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/5, /*RROE*/0, /*IMM*/0, // mem[sp & -1] = r5
                 /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
                 /*RRBUSOE*/1,
                 /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
            fill(/*idx*/idx + (1u << INSTR_BITS),
                 /*CRST*/1,
                 /*MOE*/0, /*MWE*/1, /*W16*/1,
                 /*RI*/0, /*RIWE*/0, /*RL*/6, /*RLOE*/1, /*RR*/rrr, /*RROE*/0, /*IMM*/7, // mem[sp - (-2)] = rrr
                 /*OP*/9, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
                 /*RRBUSOE*/1,
                 /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
          }
        }
      }
      else // lurpc
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/rrr, /*RIWE*/1, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/4,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = pc + (imm9 << 7)
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
    }
    else // jal
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
  }

/*
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
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 0; RRR <= 1; RRR++)
  for (PPP = 0; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // j<cond0...13>
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/RRR, /*PPP*/PPP, /*Q=1 bit*/Q);
    int cc = RRR * 8 + rrr;
    if (cc <= 13)
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
11111r001PPPQIII mrs              r, s ; get s-reg-indexed selector to reg r (both reg numbers encoded in 5-bit rPPPQ)
1111110011110III   stc
1111110011111III   ???
*/
  for (rrr = 6; rrr <= 7; rrr++)
  for (RRR = 1; RRR == 1; RRR++)
  for (PPP = 0; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // mrs
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/RRR, /*PPP*/PPP, /*Q=1 bit*/Q);
    uint rPPPQ = (rrr & 1) * 16 + PPP * 2 + Q;
    if (rPPPQ <= 29) // mrs
    {
      uint r = rPPPQ / 5;
      uint mod = rPPPQ % 5;
      uint s = mod + (mod >= r);
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/r, /*RIWE*/1, /*RL*/s, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
           /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // r = sel[s & -1]
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/1, /*SELIFLAGSSEL*/1);
    }
    else if (rPPPQ == 30) // stc
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/0, /*RL*/7, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
           /*OP*/9, /*ALUOE*/0, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // cmp pc, -1
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
111rrr01R7777777 addm14...29      r, (R+ximm7) ; rrrR = 0...15
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (RRR = 2; RRR <= 3; RRR++)
  for (PPP = 0; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // addm14...29 r, (R+ximm7)
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/RRR, /*PPP*/PPP, /*Q=1 bit*/Q);
    uint rrrR = 14 + rrr * 2 + (RRR & 1);
    uint r = rrrR / 6;
    uint mod = rrrR % 6;
    uint R = mod + (mod >= r);
    fill(/*idx*/idx,
         /*CRST*/0,
         /*MOE*/1, /*MWE*/0, /*W16*/1,
         /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
         /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    fill(/*idx*/idx + (1u << INSTR_BITS),
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
         /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr1007777777 subm4x           r, (R+ximm7)  ; when rrr!=sp,pc
*/
  for (rrr = 0; rrr <= 5; rrr++)
  for (PPP = 0; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // subm4x rrr
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/4, /*PPP*/PPP, /*Q=1 bit*/Q);
    uint r = 4;
    uint R = rrr + (rrr >= r);
    fill(/*idx*/idx,
         /*CRST*/0,
         /*MOE*/1, /*MWE*/0, /*W16*/1,
         /*RI*/5, /*RIWE*/1, /*RL*/R, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/2 - (R == 6),
         /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    fill(/*idx*/idx + (1u << INSTR_BITS),
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/r, /*RIWE*/1, /*RL*/r, /*RLOE*/1, /*RR*/5, /*RROE*/1, /*IMM*/0,
         /*OP*/9, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr100PPP0III   zxt            PPP           ; when rrr==sp, PPP!=sp,pc ; doesn't modify flags
111rrr100PPP1III   sxt            PPP           ; when rrr==sp, PPP!=sp,pc ; doesn't modify flags
*/
  for (rrr = 6; rrr == 6; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // zxt/sxt PPP
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/4, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/PPP, /*RIWE*/1, /*RL*/PPP, /*RLOE*/1, /*RR*/PPP, /*RROE*/1, /*IMM*/0,
         /*OP*/12 + Q, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr100PPP0III   cpl            PPP           ; when rrr==pc, PPP!=sp,pc ; doesn't modify flags
111rrr100PPP1III   neg            PPP           ; when rrr==pc, PPP!=sp,pc
*/
  for (rrr = 7; rrr == 7; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // cpl/neg PPP
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/4, /*PPP*/PPP, /*Q=1 bit*/Q);
    int is_neg = Q;
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/PPP, /*RIWE*/1, /*RL*/PPP, /*RLOE*/!is_neg, /*RR*/PPP, /*RROE*/is_neg, /*IMM*/0,
         /*OP*/(is_neg ? 9 : 7), /*ALUOE*/1, /*FLAGSWE*/is_neg, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/is_neg,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr100PPPQIII     pop          rPQ           ; when rrr==sp,pc, PPP==sp,pc, Q==0,1
111rrr100PPP0III     mf2                        ; when rrr==pc, PPP==pc
111rrr100PPP1III     m2f                        ; when rrr==pc, PPP==pc
*/
  for (rrr = 6; rrr <= 7; rrr++)
  for (PPP = 6; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // pop rPQ, mf2, m2f
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/4, /*PPP*/PPP, /*Q=1 bit*/Q);
    uint rPQ = (rrr & 1) * 4 + (PPP & 1) * 2 + (Q & 1);
    if (rPQ <= 5) // pop rPQ
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
           /*RI*/rPQ, /*RIWE*/1, /*RL*/6, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/7,
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rPQ = mem[sp + (-2)]
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
    else // mf2, m2f
    {
      int is_mff = Q; // move from flags
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/2, /*RIWE*/is_mff, /*RL*/2, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
           /*OP*/14, /*ALUOE*/!is_mff, /*FLAGSWE*/!is_mff, /*IADDRSEL*/0, /*FLAGSOE*/is_mff, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/!is_mff); // mf2 updates arithmetic flags and interrupt control bits
    }
  }
/*
111rrr101PPP4444 sac              rrr, PPP, imm4 ; when rrr,PPP!=sp,pc
*/
  for (rrr = 0; rrr <= 5; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // sac rrr, PPP, imm4
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/0,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/0, /*RIWE*/0, /*RL*/PPP, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
         /*OP*/1, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = PPP << imm4
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    fill(/*idx*/idx + (1u << INSTR_BITS),
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/rrr, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/rrr, /*RROE*/1, /*IMM*/0,
         /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // rrr = DelayReg + rrr
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr101PPP4444   sr             PPP, imm4      ; when rrr==sp, PPP!=sp,pc
111rrr101PPP4444   sl             PPP, imm4      ; when rrr==pc, PPP!=sp,pc
*/
  for (rrr = 6; rrr <= 7; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // sr/sl PPP, imm4
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/PPP, /*RIWE*/1, /*RL*/PPP, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
         /*OP*/((rrr == 7) ? 1 : 0), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr101PPP4444   asr            rrr, imm4      ; when rrr!=sp,pc, PPP==sp
111rrr101PPP4444   rl             rrr, imm4      ; when rrr!=sp,pc, PPP==pc
*/
  for (rrr = 0; rrr <= 5; rrr++)
  for (PPP = 6; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // asr/rl rrr, imm4
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/rrr, /*RIWE*/1, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/1,
         /*OP*/((PPP == 7) ? 3 : 4), /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr101PPP0III   di                            ; when rrr==sp, PPP==sp
111rrr101PPP1III   ei                            ; when rrr==sp, PPP==sp
*/
  for (rrr = 6; rrr <= 6; rrr++)
  for (PPP = 6; PPP <= 6; PPP++)
  for (Q = 0; Q <= 1; Q++) // di/ei
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    int is_ei = Q;
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/0, /*RIWE*/0, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/0,
         /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/1, /*SELE*/0, /*SELIFLAGSSEL*/is_ei); // interrupt enable flag = is_ei
  }
/*
111rrr101PPP0III   reti                          ; when rrr==sp, PPP==pc ; returns from ISR, restoring interrupt enable/disable
111rrr101PPP1III   hlt? (reserved)               ; when rrr==sp, PPP==pc
*/
  for (rrr = 6; rrr <= 6; rrr++)
  for (PPP = 7; PPP <= 7; PPP++)
  for (Q = 0; Q == 0; Q++) // reti
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/1, /*MWE*/0, /*W16*/1,
         /*RI*/7, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR*/0, /*RROE*/0, /*IMM*/0,
         /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // pc = mem[DelayReg=sp + (-2)]
         /*RRBUSOE*/0,
         /*IWE*/1, /*SELE*/0, /*SELIFLAGSSEL*/0); // interrupt enable flag = mem[DelayReg=sp + (-2)] & 1
  }
/*
111rrr101PPP0III   add22adc33                    ; when rrr==pc, PPP==sp
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 6; PPP <= 6; PPP++)
  for (Q = 0; Q == 0; Q++) // add22adc33
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
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
  }
/*
111rrr101PPP1III   cadd24                        ; when rrr==pc, PPP==sp
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 6; PPP <= 6; PPP++)
  for (Q = 1; Q == 1; Q++) // cadd24
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/2, /*RIWE*/1, /*RL*/4, /*RLOE*/1, /*RR*/2, /*RROE*/1, /*IMM*/0,
         /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // add r2, (Carry ? r4 : 0)
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr101PPP0III   cadd24adc3z                   ; when rrr==pc, PPP==pc
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 7; PPP <= 7; PPP++)
  for (Q = 0; Q == 0; Q++) // cadd24adc3z
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
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
  }
/*
111rrr101PPP1III   csub34                        ; when rrr==pc, PPP==pc
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 7; PPP <= 7; PPP++)
  for (Q = 1; Q == 1; Q++) // csub34
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/5, /*PPP*/PPP, /*Q=1 bit*/Q);
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
  }
/*
111rrr11wPPP0qqq lb/lw            rrr, (PPP + qqq) ; when PPP!=sp,pc ; rrr!=sp,pc when lb
*/
  for (rrr = 0; rrr <= 7; rrr++)
  for (w = 0; w <= 1; w++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q == 0; Q++) // lb/lw
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/6 + w, /*PPP*/PPP, /*Q=1 bit*/Q);
    if (w || rrr <= 5) // lb/lw
    {
      fill(/*idx*/idx,
           /*CRST*/1,
           /*MOE*/1, /*MWE*/0, /*W16*/w,
           /*RI*/rrr, /*RIWE*/1, /*RL*/PPP, /*RLOE*/1, /*RR:HW substitutes qqq*/PPP, /*RROE*/1, /*IMM*/0,
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
111rrr11wPPP1qqq sb/sw            rrr, (PPP + qqq) ; when PPP!=sp,pc, rrr!=PPP ; rrr!=sp,pc when sb ; rrr!=pc when sw
111rrr110PPP1qqq   mov            rrr, qqq         ; when sb, (rrr==PPP)!=sp,pc
111rrr111PPP1qqq   msr            rrr, qqq         ; set rrr-reg-indexed selector from reg qqq ; when sw, (rrr==PPP)!=sp,pc
*/
  for (rrr = 0; rrr <= 6; rrr++)
  for (w = 0; w <= 1; w++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 1; Q == 1; Q++) // sb/sw
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/6 + w, /*PPP*/PPP, /*Q=1 bit*/Q);
    if (rrr == PPP)
    {
      if (!w) // mov rrr, qqq
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/rrr, /*RIWE*/1, /*RL*/0, /*RLOE*/0, /*RR:HW substitutes qqq*/rrr, /*RROE*/1, /*IMM*/0,
             /*OP*/8, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/1, // rrr = 0 + qqq
             /*RRBUSOE*/0,
             /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      }
      else // msr rrr, qqq
      {
        fill(/*idx*/idx,
             /*CRST*/1,
             /*MOE*/0, /*MWE*/0, /*W16*/0,
             /*RI*/0, /*RIWE*/0, /*RL*/rrr, /*RLOE*/1, /*RR:HW substitutes qqq*/rrr, /*RROE*/0, /*IMM*/0,
             /*OP*/14, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // sel[rrr & -1] = qqq
             /*RRBUSOE*/1,
             /*IWE*/0, /*SELE*/1, /*SELIFLAGSSEL*/0);
      }
    }
    else if (w || rrr <= 5) // sb/sw
    {
      fill(/*idx*/idx,
           /*CRST*/0,
           /*MOE*/0, /*MWE*/0, /*W16*/0,
           /*RI*/0, /*RIWE*/0, /*RL*/PPP, /*RLOE*/1, /*RR:HW substitutes qqq*/PPP, /*RROE*/1, /*IMM*/0,
           /*OP*/8, /*ALUOE*/0, /*FLAGSWE*/0, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0, // DelayReg = PPP + qqq
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
      fill(/*idx*/idx + (1u << INSTR_BITS),
           /*CRST*/1,
           /*MOE*/0, /*MWE*/1, /*W16*/w,
           /*RI*/0, /*RIWE*/0, /*RL*/rrr, /*RLOE*/1, /*RR*/0, /*RROE*/0, /*IMM*/0,
           /*OP*/14, /*ALUOE*/1, /*FLAGSWE*/0, /*IADDRSEL*/1, /*FLAGSOE*/0, /*CNZ*/0, // mem[DelayReg] = rrr & -1
           /*RRBUSOE*/0,
           /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
    }
  }
/*
111rrr110PPP0qqq   sr             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
111rrr110PPP1qqq   sl             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
111rrr111PPP0qqq   rr             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
111rrr111PPP1qqq   rl             rrr, qqq         ; when PPP==sp ; rrr!=sp,pc
111rrr110PPP0qqq   asr            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
111rrr110PPP1qqq   xor            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
111rrr111PPP0qqq   and            rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
111rrr111PPP1qqq   or             rrr, qqq         ; when PPP==pc ; rrr!=sp,pc
*/
  for (rrr = 0; rrr <= 5; rrr++)
  for (w = 0; w <= 1; w++)
  for (PPP = 6; PPP <= 7; PPP++)
  for (Q = 0; Q <= 1; Q++) // <shifts,xor,and,or> rrr, qqq
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/6 + w, /*PPP*/PPP, /*Q=1 bit*/Q);
    uint op = (PPP & 1) * 4 + w * 2 + Q;
    switch (op)
    {
    default:
    case 0: op = 0; break; // sr
    case 1: op = 1; break; // sl
    case 2: op = 2; break; // rr
    case 3: op = 3; break; // rl
    case 4: op = 4; break; // asr
    case 5: op = 7; break; // xor
    case 6: op = 14; break; // and
    case 7: op = 15; break; // or
    }
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/rrr, /*RIWE*/1, /*RL*/rrr, /*RLOE*/1, /*RR:HW substitutes qqq*/rrr, /*RROE*/1, /*IMM*/0,
         /*OP*/op, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr110PPP0qqq   adc            PPP, qqq         ; when lb, rrr==sp, PPP!=sp,pc
111rrr110PPP1qqq   sbb            PPP, qqq         ; when sb, rrr==sp, PPP!=sp,pc
*/
  for (rrr = 6; rrr <= 6; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // adc/sbb PPP, qqq
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/6, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/PPP, /*RIWE*/1, /*RL*/PPP, /*RLOE*/1, /*RR:HW substitutes qqq*/PPP, /*RROE*/1, /*IMM*/0,
         /*OP*/Q + 10, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr110PPP0qqq   add            PPP, qqq         ; when lb, rrr==pc, PPP!=sp,pc
111rrr110PPP1qqq   sub            PPP, qqq         ; when sb, rrr==pc, PPP!=sp,pc
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 0; Q <= 1; Q++) // add/sub PPP, qqq
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/6, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/PPP, /*RIWE*/1, /*RL*/PPP, /*RLOE*/1, /*RR:HW substitutes qqq*/PPP, /*RROE*/1, /*IMM*/0,
         /*OP*/Q + 8, /*ALUOE*/1, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
  }
/*
111rrr111PPP1qqq   cmp            PPP, qqq         ; when sw, rrr==pc, PPP!=sp,pc
*/
  for (rrr = 7; rrr <= 7; rrr++)
  for (PPP = 0; PPP <= 5; PPP++)
  for (Q = 1; Q == 1; Q++) // cmp PPP, qqq
  {
    uint idx = idx7(/*clk*/0, /*rrr*/rrr, /*m=3 bits*/7, /*PPP*/PPP, /*Q=1 bit*/Q);
    fill(/*idx*/idx,
         /*CRST*/1,
         /*MOE*/0, /*MWE*/0, /*W16*/0,
         /*RI*/0, /*RIWE*/0, /*RL*/PPP, /*RLOE*/1, /*RR:HW substitutes qqq*/PPP, /*RROE*/1, /*IMM*/0,
         /*OP*/9, /*ALUOE*/0, /*FLAGSWE*/1, /*IADDRSEL*/0, /*FLAGSOE*/0, /*CNZ*/0,
         /*RRBUSOE*/0,
         /*IWE*/0, /*SELE*/0, /*SELIFLAGSSEL*/0);
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
