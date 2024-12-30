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
  SediCiPU2 test generator (and crude assembler).

  How to compile:
    gcc -std=c99 -O2 -Wall mktesti.c -o mktesti.exe

  How to compile for the MINI variant:
    gcc -std=c99 -O2 -Wall -DMINI=1 mktesti.c -o mktesti_mini.exe
*/

/*
  Note, some of the tests here depend on, IOW, require the following,
  otherwise they will not run until completion:
  - this code is contained in 0-16KB of physical memory and it's read-only (ROM)
  - there's read-write physical memory (RAM) between 16KB and 4MB
*/

#ifndef JUST_OPS

#ifndef THIS_FILE
#define THIS_FILE "mktesti.c"
#define THIS_FILE_SANS_EXT "mktesti"
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define STATIC_ASSERT(x) extern char StAtIcAsSeRt[(x) ? 1 : -1]

STATIC_ASSERT(CHAR_BIT == 8);

#define SPEEDUP_MULDIV 01

typedef unsigned short ushort;
typedef unsigned uint;
typedef unsigned long ulong;

enum
{
  r0, r1, r2, r3, r4, r5, sp, pc,
};

#define RQ(V) (sizeof(char[(V)?1:-1])-1) // yields 0, requires non-zero V else fails to compile

#define U(x) ((uint)(x))
#define MU1(x) (U(x)&1U)
#define MU2(x) (U(x)&3U)
#define MU3(x) (U(x)&7U)
#define MU4(x) (U(x)&0xFU)
#define MU7(x) (U(x)&0x7FU)
#define MU8(x) (U(x)&0xFFU)
#define MU9(x) (U(x)&0x1FFU)
#define MUB(x) (U(x)&0x7FFU)
#define SXT7(imm)                (MU7(imm) - ((MU7(imm)&0x40)<<1))

#if !MINI

#define OP067(n0_6,rrr,RRR,imm7) ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | (MU3(RRR)<<7) | MU7(imm7))
#define OP069(n0_6,rrr,R,imm9)   ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | (MU1(R)<<9) | MU9(imm9))
#define OP06B(n0_6,rrr,imm11)    ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | MUB(imm11))
#define OP7(rrr,m,PPP,Q,qqq)     ((7U<<13) | (MU3(rrr)<<10) | (MU3(m)<<7) | (MU3(PPP)<<4) | (MU1(Q)<<3) | MU3(qqq))
#define OP74(rrr,m,PPP,imm4)     ((7U<<13) | (MU3(rrr)<<10) | (MU3(m)<<7) | (MU3(PPP)<<4) | MU4(imm4))
#define OP77(rrr,m,imm7)         ((7U<<13) | (MU3(rrr)<<10) | (MU3(m)<<7) | MU7(imm7))
#define ADDMn(r,R)               ((r) * 6U + (R) - ((R) > (r)))
#define SUBMn(r,R)               ((R) - ((R) > (r)))
#define MRSn(r,s)                ((r) * 5U + (s) - ((s) > (r)))

#define lb(rrr,RRR,simm7)      LINE(OP067(0,rrr,RRR,simm7) + RQ((rrr)<sp))
#define lw(rrr,RRR,simm7)      LINE(OP067(1,rrr,RRR,simm7))
#define andi(RRR,imm7)         LINE(OP067(0,sp,RRR,imm7) + RQ((RRR)<sp))
#define ori(RRR,imm7)          LINE(OP067(0,pc,RRR,imm7) + RQ((RRR)<sp))
#define addm_0(n,simm7)        (((n)==0)?OP067(0,sp,sp,simm7):0)
#define addm_1(n,simm7)        (((n)==1)?OP067(0,sp,pc,simm7):0)
#define addm_2(n,simm7)        (((n)==2)?OP067(0,pc,sp,simm7):0)
#define addm_3(n,simm7)        (((n)==3)?OP067(0,pc,pc,simm7):0)
#define incm(rrr)              LINE(OP067(1,rrr,sp,1) + RQ((rrr)<sp))
#define decm(rrr)              LINE(OP067(1,rrr,sp,-1) + RQ((rrr)<sp))
#define adcz(rrr)              LINE(OP067(1,rrr,pc,1) + RQ((rrr)<sp))
#define sb(rrr,RRR,simm7)      LINE(OP067(2,rrr,RRR,simm7) + RQ((rrr)<sp) + RQ((rrr)!=(RRR)))
#define sw(rrr,RRR,simm7)      LINE(OP067(3,rrr,RRR,simm7) + RQ((rrr)<pc) + RQ((rrr)!=(RRR)))
#define xori(RRR,imm7)         LINE(OP067(2,sp,RRR,imm7) + RQ((RRR)<sp))
#define cmpi(RRR,simm7)        LINE(OP067(2,pc,RRR,simm7) + RQ((RRR)<sp))
#define addm_4(n,simm7)        (((n)==4)?OP067(2,sp,sp,simm7):0)
#define addm_5(n,simm7)        (((n)==5)?OP067(2,sp,pc,simm7):0)
#define push_0(n)              (((n)==0)?addm_5(5,1):0)
#define addm_6(n,simm7)        (((n)==6)?OP067(2,pc,sp,simm7):0)
#define addm_7(n,simm7)        (((n)==7)?OP067(2,pc,pc,simm7):0)
#define subm_0x(r,R,simm7)     (((r)==0)?OP067(2,SUBMn(r,R),SUBMn(r,R),simm7):0)
#define push_2(n)              (((n)==2)?subm_0x(0,6,1):0)
#define subm_1x(r,R,simm7)     (((r)==1)?OP067(3,SUBMn(r,R),SUBMn(r,R),simm7):0)
#define push_3(n)              (((n)==3)?subm_1x(1,6,1):0)
#define subm_2x(r,R,simm7)     (((r)==2)?OP067(3,pc,SUBMn(r,R),simm7):0)
#define push_4(n)              (((n)==4)?subm_2x(2,6,1):0)
#define addm_12(n,simm7)       (((n)==12)?OP067(3,pc,sp,simm7):0)
#define addm_13(n,simm7)       (((n)==13)?OP067(3,pc,pc,simm7):0)
#define dincm(rrr)             LINE(OP067(3,rrr,sp,1) + RQ((rrr)<sp))
#define ddecm(rrr)             LINE(OP067(3,rrr,pc,1) + RQ((rrr)<sp))
#define addi(rrr,RRR,simm7)    LINE(OP067(4,rrr,RRR,simm7) + RQ((rrr)<sp || (RRR)<pc) + RQ((rrr)<pc || (RRR)<sp))
#define swi(simm6)             LINE(OP067(4,sp,sp,(simm6)*2+1))
#define pushi(simm7)           LINE(OP067(4,sp,pc,simm7))
#define addm_8(n,simm7)        (((n)==8)?OP067(4,pc,sp,simm7):0)
#define addm_9(n,simm7)        (((n)==9)?OP067(4,pc,pc,simm7):0)
#define li(rrr,simm9)          LINE(OP069(5,rrr,0,simm9) + RQ((rrr)<sp))
#define addu(rrr,imm9)         LINE(OP069(5,rrr,1,imm9) + RQ((rrr)<pc))
#define j(simm8)               LINE(OP069(5,sp,0,MU8(simm8)*2+1))
#define subm_3x(r,R,simm7)     (((r)==3)?OP067(5,pc,SUBMn(r,R),simm7):0)
#define push_5(n)              (((n)==5)?subm_3x(3,6,1):0)
#define addm_10(n,simm7)       (((n)==10)?OP067(5,pc,sp,simm7):0)
#define addm_11(n,simm7)       (((n)==11)?OP067(5,pc,pc,simm7):0)
#define push_1(n)              (((n)==1)?addm_11(11,1):0)
#define push(r)                LINE(push_0(r)+push_1(r)+push_2(r)+push_3(r)+push_4(r)+push_5(r) + RQ((r)<sp))
#define incs(rrr,imm7)         LINE(OP067(6,rrr,0,imm7) + RQ((rrr)<sp))
#define decs(rrr,imm7)         LINE(OP067(6,rrr,1,imm7) + RQ((rrr)<sp))
#define dincs(rrr,imm7)        LINE(OP067(6,rrr,2,imm7) + RQ((rrr)<sp))
#define ddecs(rrr,imm7)        LINE(OP067(6,rrr,3,imm7) + RQ((rrr)<sp))
#define ls5r(rrr)              LINE(OP067(6,rrr,0,1) + RQ((rrr)<=4))
#define last(imm7)             LINE(OP067(6,5,0,(imm7)|1))
#define ss5r(rrr)              LINE(OP067(6,rrr,1,1) + RQ((rrr)<=4))
#define lurpc(rrr,imm9)        LINE(OP069(6,rrr,1,imm9) + RQ((rrr)<sp))
#define jal(simm11)            LINE(OP06B(6,sp,simm11))
#define jc(simm7)              LINE(OP067(7,0,0,simm7))
#define jz(simm7)              LINE(OP067(7,1,0,simm7))
#define js(simm7)              LINE(OP067(7,2,0,simm7))
#define jleu(simm7)            LINE(OP067(7,3,0,simm7))
#define jl(simm7)              LINE(OP067(7,4,0,simm7))
#define jle(simm7)             LINE(OP067(7,5,0,simm7))
#define jo(simm7)              LINE(OP067(7,6,0,simm7))
#define jno(simm7)             LINE(OP067(7,7,0,simm7))
#define jnc(simm7)             LINE(OP067(7,0,1,simm7))
#define jnz(simm7)             LINE(OP067(7,1,1,simm7))
#define jns(simm7)             LINE(OP067(7,2,1,simm7))
#define jgu(simm7)             LINE(OP067(7,3,1,simm7))
#define jge(simm7)             LINE(OP067(7,4,1,simm7))
#define jg(simm7)              LINE(OP067(7,5,1,simm7))
#define mrs(r,s)               LINE(OP7(6+MRSn(r,s)/16,1,MRSn(r,s)/2%8,MRSn(r,s)%2,7) + RQ((r)<sp) + RQ((s)<sp) + RQ((r)!=(s)))
#define stc()                  LINE(OP7(7,1,7,0,7))
#define addm_14_29(n,simm7)    ((((n)>=14)&&(n<=29))?OP067(7,((n)-14)/2,2+((n)-14)%2,simm7):0)
#define addm_0_3(n,simm7)      (addm_0(n,simm7)+addm_1(n,simm7)+addm_2(n,simm7)+addm_3(n,simm7))
#define addm_4_7(n,simm7)      (addm_4(n,simm7)+addm_5(n,simm7)+addm_6(n,simm7)+addm_7(n,simm7))
#define addm_8_11(n,simm7)     (addm_8(n,simm7)+addm_9(n,simm7)+addm_10(n,simm7)+addm_11(n,simm7))
#define addm_12_13(n,simm7)    (addm_12(n,simm7)+addm_13(n,simm7))
#define addm_0_29(n,simm7)     (addm_0_3(n,simm7)+addm_4_7(n,simm7)+addm_8_11(n,simm7)+addm_12_13(n,simm7)+addm_14_29(n,simm7))
#define addm(r,R,simm7)        LINE(addm_0_29(ADDMn(r,R),simm7) + RQ((R)<pc) + RQ((r)<r5) + RQ((R)!=(r)))
#define subm_4x(r,R,simm7)     (((r)==4)?OP067(7,SUBMn(r,R),4,simm7):0)
#define subm(r,R,simm7)        LINE(subm_0x(r,R,simm7)+subm_1x(r,R,simm7)+subm_2x(r,R,simm7)+subm_3x(r,R,simm7)+subm_4x(r,R,simm7) + RQ((R)<pc) + RQ((r)<r5) + RQ((R)!=(r)))
#define zxt(PPP)               LINE(OP7(sp,4,PPP,0,7) + RQ((PPP)<sp))
#define sxt(PPP)               LINE(OP7(sp,4,PPP,1,7) + RQ((PPP)<sp))
#define cpl(PPP)               LINE(OP7(pc,4,PPP,0,7) + RQ((PPP)<sp))
#define neg(PPP)               LINE(OP7(pc,4,PPP,1,7) + RQ((PPP)<sp))
#define pop(rPQ)               LINE(OP7(sp+(((rPQ)>>2)&1),4,sp+(((rPQ)>>1)&1),(rPQ)&1,7) + RQ((rPQ)<sp))
#define mf2()                  LINE(OP7(pc,4,pc,0,7))
#define m2f()                  LINE(OP7(pc,4,pc,1,7))
#define sac(rrr,PPP,imm4)      LINE(OP74(rrr,5,PPP,imm4) + RQ((rrr)<sp) + RQ((PPP)<sp) + RQ(MU4(imm4)))
#define sri(PPP,imm4)          LINE(OP74(sp,5,PPP,imm4) + RQ((PPP)<sp) + RQ(MU4(imm4)))
#define sli(PPP,imm4)          LINE(OP74(pc,5,PPP,imm4) + RQ((PPP)<sp) + RQ(MU4(imm4)>=2))
#define asri(rrr,imm4)         LINE(OP74(rrr,5,sp,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)))
#define rli(rrr,imm4)          LINE(OP74(rrr,5,pc,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)))
#define di()                   LINE(OP7(sp,5,sp,0,7))
#define ei()                   LINE(OP7(sp,5,sp,1,7))
#define reti()                 LINE(OP7(sp,5,pc,0,7))
#define add22adc33()           LINE(OP7(pc,5,sp,0,7))
#define cadd24()               LINE(OP7(pc,5,sp,1,7))
#define cadd24adc3z()          LINE(OP7(pc,5,pc,0,7))
#define csub34()               LINE(OP7(pc,5,pc,1,7))
#define lb2(rrr,PPP,qqq)       LINE(OP7(rrr,6,PPP,0,qqq) + RQ((PPP)<sp) + RQ((rrr)<sp))
#define lw2(rrr,PPP,qqq)       LINE(OP7(rrr,7,PPP,0,qqq) + RQ((PPP)<sp))
#define sb2(rrr,PPP,qqq)       LINE(OP7(rrr,6,PPP,1,qqq) + RQ((PPP)<sp) + RQ((rrr)<sp) + RQ((rrr)!=(PPP)))
#define sw2(rrr,PPP,qqq)       LINE(OP7(rrr,7,PPP,1,qqq) + RQ((PPP)<sp) + RQ((rrr)<pc) + RQ((rrr)!=(PPP)))
#define sr(rrr,qqq)            LINE(OP7(rrr,6,sp,0,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define sl(rrr,qqq)            LINE(OP7(rrr,6,sp,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define rr(rrr,qqq)            LINE(OP7(rrr,7,sp,0,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define rl(rrr,qqq)            LINE(OP7(rrr,7,sp,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define asr(rrr,qqq)           LINE(OP7(rrr,6,pc,0,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define xor(rrr,qqq)           LINE(OP7(rrr,6,pc,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define and(rrr,qqq)           LINE(OP7(rrr,7,pc,0,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define or(rrr,qqq)            LINE(OP7(rrr,7,pc,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp))
#define adc(PPP,qqq)           LINE(OP7(sp,6,PPP,0,qqq) + RQ((PPP)<sp) + RQ((qqq)<sp))
#define sbb(PPP,qqq)           LINE(OP7(sp,6,PPP,1,qqq) + RQ((PPP)<sp) + RQ((qqq)<sp))
#define add(PPP,qqq)           LINE(OP7(pc,6,PPP,0,qqq) + RQ((PPP)<sp)/* + RQ((qqq)<sp)*/)
#define sub(PPP,qqq)           LINE(OP7(pc,6,PPP,1,qqq) + RQ((PPP)<sp)/* + RQ((qqq)<sp)*/)
#define cmp(PPP,qqq)           LINE(OP7(pc,7,PPP,1,qqq) + RQ((PPP)<sp)/* + RQ((qqq)<sp)*/)
#define mov(rrr,qqq)           LINE(OP7(rrr,6,rrr,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<pc))
#define msr(rrr,qqq)           LINE(OP7(rrr,7,rrr,1,qqq) + RQ((rrr)<sp) + RQ((qqq)<sp) + RQ((rrr)!=(qqq)))

#define lwp(rrr,PPP)           lw2(rrr,PPP,pc)
#define swp(rrr,PPP)           sw2(rrr,PPP,pc)

#else /* MINI */

#define OP067(n0_6,rrr,RRR,imm7) ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | (MU3(RRR)<<7) | MU7(imm7))
#define OP069(n0_6,rrr,imm9,L)   ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | (MU9(imm9)<<1) | MU1(L))
#define OP06B(n0_6,rrr,imm11)    ((MU3(n0_6)<<13) | (MU3(rrr)<<10) | MUB(imm11))
#define OP7J(imm7,hh,cccc)       ((7U<<13) | (MU7(imm7)<<6) | (MU2(hh)<<4) | MU4(cccc))
#define OP7(rrr,RRR,O,hh,oooo)   ((7U<<13) | (MU3(rrr)<<10) | (MU3(RRR)<<7) | (MU1(O)<<6) | (MU2(hh)<<4) | MU4(oooo))

#define lb(rrr,RRR,simm7)      LINE(OP067(0,rrr,RRR,simm7) + RQ((rrr)<sp))
#define lw(rrr,RRR,simm7)      LINE(OP067(1,rrr,RRR,simm7))
#define andi(RRR,imm7)         LINE(OP067(0,sp,RRR,imm7) + RQ((RRR)<sp))
#define ori(RRR,imm7)          LINE(OP067(0,pc,RRR,imm7) + RQ((RRR)<sp))
#define sb(rrr,RRR,simm7)      LINE(OP067(2,rrr,RRR,simm7) + RQ((rrr)<sp) + RQ((rrr)!=(RRR)))
#define sw(rrr,RRR,simm7)      LINE(OP067(3,rrr,RRR,simm7) + RQ((rrr)<pc) + RQ((rrr)!=(RRR)))
#define xori(RRR,imm7)         LINE(OP067(2,sp,RRR,imm7) + RQ((RRR)<sp))
#define cmpi_(RRR,simm7)       OP067(2,pc,RRR,simm7)
#define cmpi(RRR,simm7)        LINE(cmpi_(RRR,simm7) + RQ((RRR)<sp))
#define pushi(simm7)           LINE(OP067(3,pc,7,simm7))
#define addi(rrr,RRR,simm7)    LINE(OP067(4,rrr,RRR,simm7) + RQ((rrr)<sp || (RRR)<pc) + RQ((rrr)<pc || (RRR)<sp))
#define swi(simm6)             LINE(OP067(4,sp,7,(simm6)*2+1))
#define li(rrr,simm9)          LINE(OP069(5,rrr,simm9,0) + RQ((rrr)<sp))
#define last(imm7)             LINE(OP067(5,sp,7,(imm7)&~1U))
#define decs(RRR,imm7)         LINE(OP067(5,pc,RRR,(imm7)&~1U) + RQ((RRR)<sp))
#define addu(rrr,imm9)         LINE(OP069(5,rrr,imm9,1) + RQ((rrr)<pc))
#define j(simm8)               LINE(OP069(5,pc,MU8(simm8)*2,1))
#define jal(simm11)            LINE(OP06B(6,sp,simm11))
#define lurpc(rrr,imm9)        LINE(OP069(6,rrr,imm9,0) + RQ((rrr)<sp))
#define add22adc33()           LINE(OP069(6,0,-1,1))
#define cadd24()               LINE(OP069(6,1,-1,1))
#define cadd24adc3z()          LINE(OP069(6,2,-1,1))
#define csub34()               LINE(OP069(6,3,-1,1))
#define mf2()                  LINE(OP069(6,4,-1,1))
#define m2f()                  LINE(OP069(6,5,-1,1))
#define jc(simm7)              LINE(OP7J(simm7,0,0))
#define jz(simm7)              LINE(OP7J(simm7,0,1))
#define js(simm7)              LINE(OP7J(simm7,0,2))
#define jleu(simm7)            LINE(OP7J(simm7,0,3))
#define jl(simm7)              LINE(OP7J(simm7,0,4))
#define jle(simm7)             LINE(OP7J(simm7,0,5))
#define jo(simm7)              LINE(OP7J(simm7,0,6))
#define jno(simm7)             LINE(OP7J(simm7,0,7))
#define jnc(simm7)             LINE(OP7J(simm7,0,8))
#define jnz(simm7)             LINE(OP7J(simm7,0,9))
#define jns(simm7)             LINE(OP7J(simm7,0,10))
#define jgu(simm7)             LINE(OP7J(simm7,0,11))
#define jge(simm7)             LINE(OP7J(simm7,0,12))
#define jg(simm7)              LINE(OP7J(simm7,0,13))
#define zxt(rrr)               LINE(OP7(rrr,7,0,0,14) + RQ((rrr)<sp))
#define sxt(rrr)               LINE(OP7(rrr,7,1,0,14) + RQ((rrr)<sp))
#define cpl(rrr)               LINE(OP7(rrr,7,0,0,15) + RQ((rrr)<sp))
#define neg(rrr)               LINE(OP7(rrr,7,1,0,15) + RQ((rrr)<sp))
#define sr(rrr,RRR)            LINE(OP7(rrr,RRR,0,1,0) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define sl(rrr,RRR)            LINE(OP7(rrr,RRR,0,1,1) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define rr(rrr,RRR)            LINE(OP7(rrr,RRR,0,1,2) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define rl(rrr,RRR)            LINE(OP7(rrr,RRR,0,1,3) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define asr(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,4) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define xor(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,5) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define and(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,6) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define or(rrr,RRR)            LINE(OP7(rrr,RRR,0,1,7) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define adc(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,8) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define sbb(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,9) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define add(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,10) + RQ((rrr)<sp)/* + RQ((RRR)<sp)*/)
#define sub(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,11) + RQ((rrr)<sp)/* + RQ((RRR)<sp)*/)
#define cmp(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,12) + RQ((rrr)<sp)/* + RQ((RRR)<sp)*/)
#define mov(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,13) + RQ((rrr)<sp) + RQ((RRR)<pc))
#define msr(RRR,rrr)           LINE(OP7(rrr,RRR,0,1,14) + RQ((rrr)<sp) + RQ((RRR)<sp) + RQ((rrr)!=(RRR)))
#define mrs(rrr,RRR)           LINE(OP7(rrr,RRR,0,1,15) + RQ((rrr)<sp) + RQ((RRR)<sp) + RQ((rrr)!=(RRR)))
#define sac(rrr,RRR,imm4)      LINE(OP7(rrr,RRR,1,1,imm4) + RQ((rrr)<sp) + RQ((RRR)<sp) + RQ(MU4(imm4)))
#define adcz(rrr)              LINE(OP7(rrr,7,1,1,0) + RQ((rrr)<sp))
#define asri(rrr,imm4)         LINE(OP7(rrr,7,0,2,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)))
#define push(rrr)              LINE(OP7(rrr,7,0,2,0) + RQ((rrr)<sp))
#define rli(rrr,imm4)          LINE(OP7(rrr,7,1,2,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)))
#define pop(rrr)               LINE(OP7(rrr,7,1,2,0) + RQ((rrr)<sp))
#define sri(rrr,imm4)          LINE(OP7(rrr,7,0,3,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)))
#define reti()                 LINE(OP7(7,7,0,3,0))
#define sli(rrr,imm4)          LINE(OP7(rrr,7,1,3,imm4) + RQ((rrr)<sp) + RQ(MU4(imm4)>=2))
#define lwp(rrr,RRR)           LINE(OP7(rrr,RRR,1,3,0) + RQ((rrr)<sp) + RQ((RRR)<sp))
#define swp(rrr,RRR)           LINE(OP7(rrr,RRR,1,3,1) + RQ((rrr)<sp) + RQ((RRR)<sp) + RQ((rrr)!=(RRR)))

#define di()                   swi(2)
#define ei()                   swi(3)
#define stc()                  LINE(cmpi_(pc,-1))
#define addm(r,R,simm7)        lw(r5,R+RQ((R)<pc),simm7), add(r+RQ((r)<r5)+RQ((R)!=(r)),r5)
#define subm(r,R,simm7)        lw(r5,R+RQ((R)<pc),simm7), sub(r+RQ((r)<r5)+RQ((R)!=(r)),r5)
#define ls5r(r)                lw(r+RQ((r)<=r4),sp,2), lw(r5,sp,0)
#define ss5r(r)                sw(r+RQ((r)<=r4),sp,2), sw(r5,sp,0)

#endif

#define nop()                  j(0)
#define jlu(simm7)             jc(simm7)
#define je(simm7)              jz(simm7)
#define jgeu(simm7)            jnc(simm7)
#define jne(simm7)             jnz(simm7)
#define clc()                  cmp(r0,r0)

#define const16(n)             LINE(n)

#define li16(rrr,imm16)        li(rrr,MU7(imm16)), addu(rrr,U(imm16)>>7)
#define addi16(rrr,imm16)      addi(rrr,rrr,SXT7(imm16)), addu(rrr,(U(imm16)-SXT7(imm16))>>7)
#define pushi16(imm16)         li16(r5,imm16), push(r5)

#define expect_c()             jnc(-1)
#define expect_lu()            jgeu(-1)
#define expect_z()             jnz(-1)
#define expect_e()             jne(-1)
#define expect_s()             jns(-1)
#define expect_leu()           jgu(-1)
#define expect_l()             jge(-1)
#define expect_le()            jg(-1)
#define expect_o()             jno(-1)
#define expect_no()            jo(-1)
#define expect_ge()            jl(-1)
#define expect_g()             jle(-1)
#define expect_nc()            jc(-1)
#define expect_geu()           jlu(-1)
#define expect_nz()            jz(-1)
#define expect_ne()            je(-1)
#define expect_ns()            js(-1)
#define expect_gu()            jleu(-1)

#define expect_r16(rrr,imm16)  addi16(rrr,-U(imm16)), jnz(-1), li16(rrr,imm16)

#else // #else of #ifndef JUST_OPS
  lw(pc, pc, 0),           // 00 // skip all ISR entry points, jump to Start...
  const16(/*Start*/0x17A), // 02 // reserved for a distant jump like this
  jal((0x94 - 0x06) / 2),  // 04 // jump to swi 2 handler
  jal((0xA2 - 0x08) / 2),  // 06 // jump to swi 3 handler
  jal((0xAE - 0x0A) / 2),  // 08 // jump to swi 4 handler
  jal((0xC4 - 0x0C) / 2),  // 0A // jump to swi 5 handler
  jal((0x162 - 0x0E) / 2), // 0C // jump to swi 6 handler
  jal((0x170 - 0x10) / 2), // 0E // jump to swi 7 handler
  reti(), // 10
  reti(), // 12
  reti(), // 14
  reti(), // 16
  reti(), // 18
  reti(), // 1A
  reti(), // 1C
  reti(), // 1E
  reti(), // 20
  reti(), // 22
  reti(), // 24
  reti(), // 26
  reti(), // 28
  reti(), // 2A
  reti(), // 2C
  reti(), // 2E
  reti(), // 30
  reti(), // 32
  reti(), // 34
  reti(), // 36
  reti(), // 38
  reti(), // 3A
  reti(), // 3C

  // Common IRQ ISR at 0x3E:
  // Save regs on stack. N.B. return address is at sp - 2.
  addi(sp, sp, -16), // doesn't affect flags
  sw(r5, sp, 12),
  sw(r4, sp, 10),
  sw(r3, sp, 8),
  sw(r2, sp, 6),
  sw(r1, sp, 4),
  sw(r0, sp, 2),
  m2f(),
  sw(r2, sp, 0), // save flags too

  // r1 = original flags.
  mov(r1, r2),

  // r3 = mask of requested/clear field.
  li16(r3, 0x3F<<4),

  // Ensure there's at least one IRQ being requested.
  mov(r0, r2),
  sri(r0, 10),
  sli(r0, 4),
  and(r2, r0), // mask masked IRQs
  jz(-1),

  // r0 = mask for the lowest-numbered requested IRQ.
  mov(r0, r2),
  neg(r2),
  and(r0, r2),

  // Clear the IRQ and mask the IRQ and all higher-numbered IRQs.
  mov(r2, r1),
  or(r2, r3),
  xor(r2, r0),
  mf2(),
  or(r2, r3),
  mov(r4, r0),
  sli(r4, 6),
  addi(r4, r4, -1),
  and(r2, r4),
  mf2(),

  // Can enable interrupts for other/nested IRQs now.
  ei(),

  // Can now handle the IRQ.

  // Restore regs from stack and return.
  di(), // make sure ints are disabled now to protect return address on stack
  lw(r2, sp, 0),
  or(r2, r3),
  mf2(), // unmask the IRQ, restore flags
  lw(r0, sp, 2),
  lw(r1, sp, 4),
  lw(r2, sp, 6),
  lw(r3, sp, 8),
  lw(r4, sp, 10),
  lw(r5, sp, 12),
  addi(sp, sp, 16), // doesn't affect flags
  reti(),

  // SWI 2 ISR (disable hardware/external interrupts):
  //   outs:
  //     r0 bit 0 = old interrupt enable flag
  addi(r5, sp, -2), // r5 points to return address and interrupt enable flag on stack
  lw(r0, r5, 0), // r0 contains old interrupt enable flag
  addi(r1, r0, 2), // adjust return address by one instruction
  ori(r1, 1),
  xori(r1, 1), // clear interrupt enable flag
  sw(r1, r5, 0), // update return address and interrupt enable flag on stack
  reti(), // must restore interrupt enabledness

  // SWI 3 ISR (enable hardware/external interrupts):
  //   outs:
  //     r0 bit 0 = old interrupt enable flag
  addi(r5, sp, -2), // r5 points to return address and interrupt enable flag on stack
  lw(r0, r5, 0), // r0 contains old interrupt enable flag
  addi(r1, r0, 2), // adjust return address by one instruction
  ori(r1, 1), // set interrupt enable flag
  sw(r1, r5, 0), // update return address and interrupt enable flag on stack
  reti(), // must restore interrupt enabledness

  // SWI 4 ISR (jump):
  //   ins:
  //     r0 = ofs
  //     r1 = block
  // determine 16KB subregion and convert it to program/code space selector index
  addi(sp, sp, -2),
  mov(r5, r0),
  sri(r5, 14),
  jz(1), // skip changing system area's code block
  // set new block
  msr(r5, r1),
  // adjust return address, preserving interrupt enable flag
  lw(r5, sp, 0),
  andi(r5, 1),
  or(r5, r0),
  sw(r5, sp, 0),
  addi(sp, sp, 2),
  reti(),

  // SWI 5 ISR (wcpy):
  //   ins:
  //     r0 = dst_ofs
  //     r1 = dst_block
  //     r2 = src_ofs
  //     r3 = src_block
  //     r4 = word_count

  // save regs
  addi(sp, sp, -20),
  sw(r4, sp, 16), // word_count
  sw(r3, sp, 14), // src_block
  sw(r2, sp, 12), // src_ofs
  sw(r1, sp, 10), // dst_block
  sw(r0, sp, 8),  // dst_ofs

  // adjust return address
#if !MINI
  dincs(r5, 18),
  j(1),  // to preserve the ISR size and
  nop(), // entry point addresses
#else
  lw(r5, sp, 18),
  addi(r5, r5, 2),
  sw(r5, sp, 18),
#endif

  // and reenable interrupts if they were enabled on entry to the swi,
  // to keep interrupts enabled while copying data
  andi(r5, 1),
#if !MINI
  jz(/*.ints*/3),
  ei(),
  j(1),  // to preserve the ISR size and
  nop(), // entry point addresses
#else
  jz(/*.ints*/3),
  ei(), // N.B. this ei (which is swi(3)) clobbers r0 and r1 used below
  lw(r0, sp, 8),  // dst_ofs
  lw(r1, sp, 10), // dst_block
#endif
//.ints:

  // save sel1 block; sel1 will be used for src_block
  li(r5, 1),
  mrs(r4, r5),
  sw(r4, sp, 6), // sel1_block

  // set sel1 block to src_block
  msr(r5, r3),

  // figure out which sel (5 or 7) to use for dst_block,
  // switching that sel's block must not interfere with the stack:
  //
  // |   4   |   5   |   6   |   7   |   data sel
  // +-------+-------+-------+-------+
  // |       |       |  40KB |       |
  // 0KB    16KB    32KB    48KB    64KB data ofs
  // +-------+-------+-------+-------+
  // |  sys  |       |       |       |
  //            dst       StkStkStkStk case 1: stack can safely expand down
  //                                           for another ~8KB
  // StkStkStkStkStkStkStk      dst    case 2: stack is out of the way
  li16(r5, 40960),
  cmp(r5, sp),
  li(r5, 5),
  jlu(/*.sel57*/1),
  li(r5, 7),
//.sel57:

  // save that sel5/7 and its block
  mrs(r4, r5),
  sw(r5, sp, 4), // sel5/7
  sw(r4, sp, 2), // sel5/7_block

  // set that sel5/7 to dst_block
  msr(r5, r1),

  // adjust dst_ofs to use sel5/7
  sli(r0, 2),
  sri(r0, 2),
  sac(r0, r5, 14),

  // adjust src_ofs to use sel1
  sli(r2, 2),
  sri(r2, 2),
  addu(r2, (1<<14)>>7),

  // copy 0-3 words one by one
  lw(r4, sp, 16), // word_count
  andi(r4, 3),
  jz(/*.copy4*/10),

  addi(r2, r2, -2), // -2 to compensate for different pc's in sub & lwp
  sub(r2, pc),
//.repeat1:
  lwp(r1, r2),
  sw(r1, r0, 0),
  addi(r2, r2, 2),
  addi(r0, r0, 2),
  addi(r4, r4, -1),
  jnz(/*.repeat1*/-6),

  // restore r2 to non-pc-relative
  add(r2, pc),
  addi(r2, r2, -6 * 2),

//.copy4:
  // see if multiples of quadwords can be copied
  lw(r4, sp, 16), // word_count
  sri(r4, 2),
  jz(/*.restore_sels*/15),

  sw(r4, sp, 0), // counter of quadwords
  addi(r2, r2, -2), // -2 to compensate for different pc's in sub & lwp
  sub(r2, pc),
//.repeat2:
  lwp(r1, r2),
  lwp(r3, r2),
  lwp(r4, r2),
  lwp(r5, r2),
  sw(r1, r0, 0),
  sw(r3, r0, 2),
  sw(r4, r0, 4),
  sw(r5, r0, 6),
  addi(r2, r2, 8),
  addi(r0, r0, 8),
  decs(r1, 0), // decrement counter of quadwords
  jnz(/*.repeat2*/-12),

//.restore_sels:
  // restore sel 5 or 7 block
  lw(r5, sp, 4), // sel5/7
  lw(r4, sp, 2), // sel5/7_block
  msr(r5, r4),

  // restore sel1 block
  li(r5, 1),
  lw(r4, sp, 6), // sel1_block
  msr(r5, r4),

  // restore regs
  di(),
  lw(r0, sp, 8),  // dst_ofs
  lw(r1, sp, 10), // dst_block
  lw(r2, sp, 12), // src_ofs
  lw(r3, sp, 14), // src_block
  lw(r4, sp, 16), // word_count
  addi(sp, sp, 20),
  reti(),

  // SWI 6 ISR:
  //   outs:
  //     r0 = 0x4321
  //     r1 bit 0 = interrupt enable flag
  li16(r0, 0x4321),
#if 0
#if !MINI
  // N.B. dincm isn't available on the mini.
  addi(r1, sp, -2), // r1 points to return address and interrupt enable flag on stack
  dincm(r1), // adjust return address by one instruction; r1 contains interrupt enable flag
  j(1),  // to preserve the ISR size and
  nop(), // entry point addresses
#else
  // N.B. addressing memory below sp is available on the mini only.
  lw(r1, sp, -2),
  addi(r1, r1, 2),
  sw(r1, sp, -2),
  nop(), // to preserve the ISR size and entry point addresses
#endif
#else
  // Using only common instructions, mini or not.
  addi(r5, sp, -2), // r5 points to return address and interrupt enable flag on stack
  lw(r1, r5, 0),
  addi(r1, r1, 2),
  sw(r1, r5, 0),
#endif
  reti(), // must restore interrupt enabledness

  // SWI 7 ISR:
  //   ins:
  //     r2 = return address and interrupt enable flag for reti
  //   outs:
  //     r2 = flags upon entry to the ISR
  //     r5: clobbered
  //     flags: preserved
  addi(sp, sp, -2), // sp points to return address and interrupt enable flag on stack; preserves flags
  sw(r2, sp, 0),    // update return address
  m2f(),            // read flags
  addi(sp, sp, 2),  // preserves flags
  reti(),           // must restore interrupt enabledness

  // Start:

#if 01
  // Test loading constants, arithmetic flags, comparison and conditional jumps.
  // Comparison and the conditional jumps are the foundation of tests and business logic.
  // N.B. This test will clear and mask all IRQs, but there should be no
  // important IRQs upon reset prior do initialization of interrupt-
  // generating devices.
  li(r2,  0), mf2(), expect_no(), expect_ns(), expect_nz(), expect_nc(), expect_gu(), expect_ge(), expect_g(),
  li(r2,  1), mf2(), expect_no(), expect_ns(), expect_nz(), expect_c(), expect_leu(), expect_ge(), expect_g(),
  li(r2,  2), mf2(), expect_no(), expect_ns(), expect_z(), expect_nc(), expect_leu(), expect_ge(), expect_le(),
  li(r2,  3), mf2(), expect_no(), expect_ns(), expect_z(), expect_c(), expect_leu(), expect_ge(), expect_le(),
  li(r2,  4), mf2(), expect_no(), expect_s(), expect_nz(), expect_nc(), expect_gu(), expect_l(), expect_le(),
  li(r2,  5), mf2(), expect_no(), expect_s(), expect_nz(), expect_c(), expect_leu(), expect_l(), expect_le(),
  li(r2,  6), mf2(), expect_no(), expect_s(), expect_z(), expect_nc(), expect_leu(), expect_l(), expect_le(),
  li(r2,  7), mf2(), expect_no(), expect_s(), expect_z(), expect_c(), expect_leu(), expect_l(), expect_le(),
  li(r2,  8), mf2(), expect_o(), expect_ns(), expect_nz(), expect_nc(), expect_gu(), expect_l(), expect_le(),
  li(r2,  9), mf2(), expect_o(), expect_ns(), expect_nz(), expect_c(), expect_leu(), expect_l(), expect_le(),
  li(r2, 10), mf2(), expect_o(), expect_ns(), expect_z(), expect_nc(), expect_leu(), expect_l(), expect_le(),
  li(r2, 11), mf2(), expect_o(), expect_ns(), expect_z(), expect_c(), expect_leu(), expect_l(), expect_le(),
  li(r2, 12), mf2(), expect_o(), expect_s(), expect_nz(), expect_nc(), expect_gu(), expect_ge(), expect_g(),
  li(r2, 13), mf2(), expect_o(), expect_s(), expect_nz(), expect_c(), expect_leu(), expect_ge(), expect_g(),
  li(r2, 14), mf2(), expect_o(), expect_s(), expect_z(), expect_nc(), expect_leu(), expect_ge(), expect_le(),
  li(r2, 15), mf2(), expect_o(), expect_s(), expect_z(), expect_c(), expect_leu(), expect_ge(), expect_le(),
  // Also test clc and stc...
  clc(), expect_nc(),
  stc(), expect_c(),
  // And cmp for equality/inequality as well...
  li(r1, 3),
  li(r2, 33),
  cmp(r1, r1), expect_e(),
  cmp(r2, r2), expect_e(),
  cmp(r1, r2), expect_ne(),
  cmp(r2, r1), expect_ne(),
#endif

#if 01
  // Test loading and adding constants...
  // Start with just li and addi...
  li(r0, -64), addi(r0, r0, +63), addi(r0, r0, +1), expect_z(),
  li(r1, -63), addi(r1, r1, +63), expect_z(),
  li(r2,  -1), addi(r2, r2,  +1), expect_z(),
  li(r3,  +0), addi(r3, r3,  +0), expect_z(),
  li(r4,  +1), addi(r4, r4,  -1), expect_z(),
  li(r5, +63), addi(r5, r5, -63), expect_z(),

  // Now also use addu...
  li(r0, 0x000), expect_r16(r0, 0x0000),
  li(r1, 0x011), expect_r16(r1, 0x0011),
  li(r2, 0x022), expect_r16(r2, 0x0022),
  li(r3, 0x133), expect_r16(r3, 0xFF33),
  li(r4, 0x144), expect_r16(r4, 0xFF44),
  li(r5, 0x155), expect_r16(r5, 0xFF55),

  li16(r0, 0xEEEE), expect_r16(r0, 0xEEEE),
  li16(r1, 0xCCCC), expect_r16(r1, 0xCCCC),
  li16(r2, 0xAAAA), expect_r16(r2, 0xAAAA),
  li16(r3, 0x8888), expect_r16(r3, 0x8888),
  li16(r4, 0x6666), expect_r16(r4, 0x6666),
  li16(r5, 0x4444), expect_r16(r5, 0x4444),

  li16(r0, 0x1111), expect_r16(r0, 0x1111),
  li16(r1, 0x3333), expect_r16(r1, 0x3333),
  li16(r2, 0x5555), expect_r16(r2, 0x5555),
  li16(r3, 0x7777), expect_r16(r3, 0x7777),
  li16(r4, 0x9999), expect_r16(r4, 0x9999),
  li16(r5, 0xBBBB), expect_r16(r5, 0xBBBB),
#endif

#if 01
  // Test mov...
  li16(r0, 0x1111),
  li16(r1, 0x3333),
  li16(r2, 0x5555),
  li16(r3, 0x7777),
  li16(r4, 0x9999),
  li16(r5, 0xBBBB),
  mov(r0, r1), expect_r16(r0, 0x3333),
  mov(r5, r0), expect_r16(r5, 0x3333),
  mov(r1, r4), expect_r16(r1, 0x9999),
  mov(r2, r3), expect_r16(r2, 0x7777),
  mov(r3, r4), expect_r16(r3, 0x9999),
  mov(r4, r0), expect_r16(r4, 0x3333),
#endif

#if 01
  // Some more tests for conditional jumps...
  li(r0, 17),
  clc(),
  jnc(2),
  addi(r0, r0, 1),
  addi(r0, r0, 2),
  addi(r0, r0, 4),
  expect_r16(r0, 21),

  stc(),
  jnc(2),
  addi(r0, r0, 1),
  addi(r0, r0, 2),
  addi(r0, r0, 4),
  expect_r16(r0, 28),

  clc(),
  jc(2),
  addi(r0, r0, 1),
  addi(r0, r0, 2),
  addi(r0, r0, 4),
  expect_r16(r0, 35),

  stc(),
  jc(2),
  addi(r0, r0, 1),
  addi(r0, r0, 2),
  addi(r0, r0, 4),
  expect_r16(r0, 39),

  stc(),
  jc(2),
  addi(r0, r0, 8),
  clc(),
  jc(-3),
  expect_r16(r0, 47),
#endif

#if 01
  // Test j, nop...
  li(r0, 42),
  j(2),
  addi(r0, r0, 1),
  addi(r0, r0, 2),
  addi(r0, r0, 4),
  expect_r16(r0, 46),

  addi(r0, r0, 8),
  nop(),
  expect_r16(r0, 54),

  j(2),
  addi(r0, r0, 16),
  j(1),
  j(-3),
  expect_r16(r0, 70),
#endif

#if 01
  // Test jal...
  li(r0, 42),
  addi(r3, pc, 2),   // r3 points to first skipped addi()
  jal(2),            // r5 points to first skipped addi()
  addi(r0, r0, 1),   // skipped
  addi(r0, r0, 2),   // skipped
  addi(r0, r0, 4),   // executed
  expect_r16(r0, 46),
  cmp(r5, r3),
  expect_e(),

  addi(r3, pc, 12),  // r3 points to expect_r16()
  jal(4),
  addi(r0, r0, 16),
  cmp(r5, r3),
  expect_e(),
  jal(1),
  jal(-5),           // r5 points to expect_r16()
  expect_r16(r0, 62),
#endif

#if 01
  // Test basic ALU operations (reg & imm)...
  li(r1, -1),
  li(r2, 0xAA),
  li16(r3, 0x7C33),
  mov(r4, r3),

  andi(r1, 0x55), expect_nz(), expect_r16(r1, 0x0055),
  andi(r2, 0x55), expect_z(),  expect_r16(r2, 0x0000),
  ori(r3, 0x45),  expect_nz(), expect_r16(r3, 0x7C77),
  xori(r4, 0x7A), expect_nz(), expect_r16(r4, 0x7C49),
#endif

#if 01
  // Test mrs/msr...
  // Offsets from 0 to 16KB-1 must map first 16 KB of 4 MB upon reset,
  // check the selectors for zero...
  li(r1, -1), li(r4, 0), mrs(r1, r4), expect_r16(r1, 0), // program/code selector
  li(r2, -1), li(r3, 4), mrs(r2, r3), expect_r16(r2, 0), // data selector

  // Just write and read back other selectors...
  li(r1, 0x55), li(r2, 1), msr(r2, r1), mrs(r3, r2), expect_r16(r3, 0x55),
  li(r3, 0xAA), li(r4, 2), msr(r4, r3), mrs(r5, r4), expect_r16(r5, 0xAA),
  li(r5, 0xC3), li(r0, 3), msr(r0, r5), mrs(r1, r0), expect_r16(r1, 0xC3),
  li(r1, 0x3C), li(r0, 5), msr(r0, r1), mrs(r5, r0), expect_r16(r5, 0x3C),
  li(r3, 0xF0), li(r2, 6), msr(r2, r3), mrs(r1, r2), expect_r16(r1, 0xF0),
  li(r5, 0x0F), li(r4, 7), msr(r4, r5), mrs(r3, r4), expect_r16(r3, 0x0F),
#endif

  // Select a 16KB block for boot/system data/stack
  li(r0, 1), // 16KB block 1
  li(r1, 4), // selector 4 (data; offsets 0..16KB-1)
  msr(r1, r0),

  // Set up a usable stack top in the boot/system data/stack block
  li16(r0, 0x1000),
  addi(sp, r0, 0),
  mov(r0, sp),
  expect_r16(r0, 0x1000),

  // Test writing to RAM (stack)...
  addi(sp, sp, -2),
  li16(r0, 0x1234), sw(r0, sp, 0),
  lw(r1, sp, 0), expect_r16(r1, 0x1234),
  li16(r0, 0xEDCB), sw(r0, sp, 0),
  lw(r1, sp, 0), expect_r16(r1, 0xEDCB),
  addi(sp, sp, 2),

#if 01
  // Test swi/reti and the interrupt enable flag...
  // N.B. This test will clear and mask all IRQs, but there should be no
  // important IRQs upon reset prior do initialization of interrupt-
  // generating devices.
  li(r2, 0), mf2(),

  // Check that interrupts are disabled...
  swi(6), // invoke SWI 6 ISR
  expect_r16(r0, 0x4321), // expected from SWI ISR
  andi(r1, 1), // isolate interrupt enable flag
  expect_z(), // expecting interrupts disabled

  // Enable interrupts.
  ei(),

  // Check that interrupts are enabled...
  swi(6), // invoke SWI 6 ISR
  expect_r16(r0, 0x4321), // expected from SWI ISR
  andi(r1, 1), // isolate interrupt enable flag
  expect_nz(), // expecting interrupts enabled

  // Toggle the interrupt enable flag a few times...
  swi(3), swi(6), andi(r1, 1), expect_nz(),
  swi(2), swi(6), andi(r1, 1), expect_z(),
  swi(3), swi(6), andi(r1, 1), expect_nz(),
  swi(2), swi(6), andi(r1, 1), expect_z(),
  swi(2), swi(6), andi(r1, 1), expect_z(),
  swi(3), swi(6), andi(r1, 1), expect_nz(),
  swi(3), swi(6), andi(r1, 1), expect_nz(),
  ei(), swi(6), andi(r1, 1), expect_nz(),
  di(), swi(6), andi(r1, 1), expect_z(),
  ei(), swi(6), andi(r1, 1), expect_nz(),
  di(), swi(6), andi(r1, 1), expect_z(),
  di(), swi(6), andi(r1, 1), expect_z(),
  ei(), swi(6), andi(r1, 1), expect_nz(),
  ei(), swi(6), andi(r1, 1), expect_nz(),

  // Disable interrupts.
  di(),
#endif

#if 01
  // Unmask all IRQs and enable interrupts, so IRQs can be handled.
  li(r2, -1), mf2(),
  ei(),
#endif

#if 01
  // Test push/pop...
  mov(r0, sp),
  expect_r16(r0, 0x1000),
  pushi(-64),
  pushi(63),
  mov(r0, sp),
  expect_r16(r0, 0x1000 - 2 * 2),
  lw(r0, sp, 0), expect_r16(r0, 0x003F),
  lw(r0, sp, 2), expect_r16(r0, 0xFFC0),
  pop(r0), // 003F
  pop(r1), // FFC0
  expect_r16(r0, 0x003F),
  expect_r16(r1, 0xFFC0),
  mov(r0, sp),
  expect_r16(r0, 0x1000),

  li16(r0, 0x1111),
  li16(r1, 0x3333),
  li16(r2, 0x5555),
  li16(r3, 0x7777),
  li16(r4, 0x9999),
  li16(r5, 0xBBBB),
  push(r0),
  push(r1),
  push(r2),
  push(r3),
  push(r4),
  push(r5),
  mov(r0, sp),
  expect_r16(r0, 0x1000 - 6 * 2),
  lw(r1, sp, 0), expect_r16(r1, 0xBBBB),
  lw(r1, sp, 5 * 2), expect_r16(r1, 0x1111),
  pop(r0),
  pop(r1),
  pop(r2),
  pop(r3),
  pop(r4),
  pop(r5),
  expect_r16(r0, 0xBBBB),
  expect_r16(r1, 0x9999),
  expect_r16(r2, 0x7777),
  expect_r16(r3, 0x5555),
  expect_r16(r4, 0x3333),
  expect_r16(r5, 0x1111),
  mov(r0, sp),
  expect_r16(r0, 0x1000),
#endif

#if 01
  // Test unary operations...
  li(r0, 0x7F),
  li16(r1, 0x5555),
  li16(r2, 0xFF7F),
  mov(r3, r2),
  li(r4, 0x80),
  mov(r5, r4),
  neg(r0), expect_r16(r0, 0xFF81),
  cpl(r1), expect_r16(r1, 0xAAAA),
  zxt(r2), expect_r16(r2, 0x007F),
  sxt(r3), expect_r16(r3, 0x007F),
  zxt(r4), expect_r16(r4, 0x0080),
  sxt(r5), expect_r16(r5, 0xFF80),
#endif

#if 01
  // Test basic ALU operations (reg & reg)...
  li16(r1, 0x3FFC),
  li16(r0, 0xCFF3),
  mov(r2, r0),
  mov(r3, r0),
  mov(r4, r0),
  mov(r5, r0),
  add(r0, r1), expect_r16(r0, 0x0FEF),
  sub(r2, r1), expect_r16(r2, 0x8FF7),
  and(r3, r1), expect_r16(r3, 0x0FF0),
  or(r4, r1),  expect_r16(r4, 0xFFFF),
  xor(r5, r1), expect_r16(r5, 0xF00F),
#endif

#if 01
  // Test add/sub, adc/sbb, addi, adcz with different carry-ins...
  li(r1, 1),
  li(r2, 3),
  mov(r3, r2),
  mov(r4, r2),
  mov(r5, r2),

  stc(),
  add(r2, r1), expect_r16(r2, 0x0004),
  clc(),
  add(r2, r1), expect_r16(r2, 0x0005),
  stc(),
  adc(r3, r1), expect_r16(r3, 0x0005),
  clc(),
  adc(r3, r1), expect_r16(r3, 0x0006),
  stc(),
  sub(r4, r1), expect_r16(r4, 0x0002),
  clc(),
  sub(r4, r1), expect_r16(r4, 0x0001),
  stc(),
  sbb(r5, r1), expect_r16(r5, 0x0001),
  clc(),
  sbb(r5, r1), expect_r16(r5, 0x0000),

  li(r2, 3),
  mov(r3, r2),
  mov(r4, r2),
  mov(r5, r2),

  stc(),
  addi(r2, r2, 1), expect_r16(r2, 0x0004),
  clc(),
  addi(r2, r2, 1), expect_r16(r2, 0x0005),

  li(r1, 3),
  mov(r2, r1),
  stc(),
  adcz(r1), expect_r16(r1, 0x0004),
  clc(),
  adcz(r2), expect_r16(r2, 0x0003),
#endif

#if 01
  // Test cmp and conditional jumps...
  li(r1, -1),
  li(r2, 0),
  li(r3, 1),
  cmp(r1, r1), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),
  cmp(r1, r2), expect_ne(), expect_l(),  expect_le(), expect_gu(),  expect_geu(),
  cmp(r1, r3), expect_ne(), expect_l(),  expect_le(), expect_gu(),  expect_geu(),
  cmp(r2, r1), expect_ne(), expect_g(),  expect_ge(), expect_lu(),  expect_leu(),
  cmp(r2, r2), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),
  cmp(r2, r3), expect_ne(), expect_l(),  expect_le(), expect_lu(),  expect_leu(),
  cmp(r3, r1), expect_ne(), expect_g(),  expect_ge(), expect_lu(),  expect_leu(),
  cmp(r3, r2), expect_ne(), expect_g(),  expect_ge(), expect_gu(),  expect_geu(),
  cmp(r3, r3), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),

  cmpi(r1, -1), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),
  cmpi(r1, +0), expect_ne(), expect_l(),  expect_le(), expect_gu(),  expect_geu(),
  cmpi(r1, +1), expect_ne(), expect_l(),  expect_le(), expect_gu(),  expect_geu(),
  cmpi(r2, -1), expect_ne(), expect_g(),  expect_ge(), expect_lu(),  expect_leu(),
  cmpi(r2, +0), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),
  cmpi(r2, +1), expect_ne(), expect_l(),  expect_le(), expect_lu(),  expect_leu(),
  cmpi(r3, -1), expect_ne(), expect_g(),  expect_ge(), expect_lu(),  expect_leu(),
  cmpi(r3, +0), expect_ne(), expect_g(),  expect_ge(), expect_gu(),  expect_geu(),
  cmpi(r3, +1), expect_e(),  expect_le(), expect_ge(), expect_leu(), expect_geu(),
#endif

#if 01
  // Test shifts...
  li(r0, -1),

  li16(r1, 0x8765),
  mov(r2, r1),

  li(r3, 8),
  rl(r1, r3), expect_r16(r1, 0x6587),
  rr(r2, r3), expect_r16(r2, 0x6587),

  li(r3, 4),
  rl(r1, r3), expect_r16(r1, 0x5876),
  rr(r2, r3), expect_r16(r2, 0x7658),

  sl(r1, r3), expect_r16(r1, 0x8760),
  sr(r2, r3), expect_r16(r2, 0x0765),

  li16(r4, 0x8888),
  mov(r5, r4),
  li(r3, 1),
  sl(r4, r3), expect_r16(r4, 0x1110),
  sr(r5, r3), expect_r16(r5, 0x4444),

  li(r3, 2),
  sl(r4, r3), expect_r16(r4, 0x4440),
  sr(r5, r3), expect_r16(r5, 0x1111),

  li16(r1, 0x8765),
  mov(r2, r1),
  li(r3, 4),
  asr(r1, r3), expect_r16(r1, 0xF876),
  sr(r2, r3),  expect_r16(r2, 0x0876),
  li(r3, 8),
  asr(r1, r3), expect_r16(r1, 0xFFF8),
  sr(r2, r3),  expect_r16(r2, 0x0008),
  li(r3, 2),
  asr(r1, r3), expect_r16(r1, 0xFFFE),
  sr(r2, r3),  expect_r16(r2, 0x0002),
  li(r3, 1),
  asr(r1, r3), expect_r16(r1, 0xFFFF),
  sr(r2, r3),  expect_r16(r2, 0x0001),

  li16(r1, 0x8765),
  mov(r2, r1),
  mov(r3, r1),
  mov(r4, r1),
  rli(r1, 4),  expect_r16(r1, 0x7658),
  sli(r2, 4),  expect_r16(r2, 0x7650),
  sri(r3, 4),  expect_r16(r3, 0x0876),
  asri(r4, 4), expect_r16(r4, 0xF876),
#endif

#if 01
  // Test sac...
  li(r1, 3),
  li(r5, 5),
  sac(r1, r5, 7), expect_r16(r1, 0x0283),
  li(r4, 7),
  li(r2, 3),
  sac(r4, r2, 5), expect_r16(r4, 0x0067),
  li(r0, 3),
  sac(r0, r0, 5), expect_r16(r0, 0x0063),
  li(r3, 0x55),
  sac(r3, r0, 8), expect_r16(r3, 0x6355),
  li(r2, 10),
  li(r5, 9),
  sac(r2, r5, 3),
  sac(r2, r5, 1), expect_r16(r2, 0x0064),
#endif

#if 01
  // Test `add r, r, simm7` with pc being one of the operands,
  // also test linking...
  li(r5, 1),
  addi(r1, pc, 2),   // r1 points to expect_e()
  cmp(r1, pc),
  expect_e(),
  expect_r16(r5, 0x0001),

  li(r2, 0),
  addi(r1, pc, 0),   // r1 points to next addi()
  addi(pc, r1, 4),   // jump to executed
  addi(r2, r2, 1),   // skipped
  addi(r2, r2, 2),   // executed
  expect_r16(r2, 0x0002),
  expect_r16(r5, 0x0001),

  li(r2, 0),
  addi(r1, pc, 0),     // r1 points to next addi()
  addi(r3, pc, 2),     // r3 points to skipped addi()
  addi(pc, r1, 6 + 1), // jump to executed and link to skipped
  addi(r2, r2, 1),     // skipped
  addi(r2, r2, 2),     // executed
  expect_r16(r2, 0x0002),
  cmp(r5, r3),
  expect_e(),
#endif

#if 01
  // Test pc-relative addressing in loads and stores...
  lb(r1, pc, -2), expect_r16(r1, 0x00FE),
  lb(r1, pc, -1), expect_r16(r1, 0x0007),
  lw(r1, pc, -2), expect_r16(r1, 0x27FE),
  sw(r1, pc, -2), // Encoded as 67FE; ROM shouldn't be writable
  lw(r1, pc, -4), expect_r16(r1, 0x67FE),
#if !MINI
  li(r0, -2),
  li(r2, -1),
  lb2(r1, r0, pc), expect_r16(r1, 0x0007),
  lb2(r1, r2, pc), expect_r16(r1, 0x00E7),
  lwp(r1, r0), expect_r16(r1, 0xE787),
  swp(r1, r0), // Encoded as E78F; ROM shouldn't be writable
  lw(r1, pc, -4), expect_r16(r1, 0xE78F),
#else
  li(r0, -2),
  lwp(r1, r0), expect_r16(r1, 0xE470),
  swp(r1, r0), // Encoded as E471; ROM shouldn't be writable
  lw(r1, pc, -4), expect_r16(r1, 0xE471),
#endif
#endif

#if 01
  // Test addm/subm...
  pushi16(2048), pushi16(1024), pushi16(512), pushi16(256), pushi(8), pushi(4), pushi(2), pushi(1), pushi(0),
  addi(r1, sp, 8), // r1 points to 8
  li(r2, 0),
  addm(r2, r1, -8), expect_z(),
  addm(r2, r1, -6), expect_nz(),
  addm(r2, r1, -4), expect_nz(),
  addm(r2, r1, -2), expect_nz(),
  addm(r2, r1, 0), expect_nz(),
  addm(r2, r1, 2), expect_nz(),
  addm(r2, r1, 4), expect_nz(),
  addm(r2, r1, 6), expect_nz(),
  addm(r2, r1, 8), expect_nz(),
  expect_r16(r2, 0x0F0F),
  addi(sp, sp, 18),

  // 0,6 0,1  1,0 1,2  2,1 2,3  3,2 3,4  4,3 4,5
  //   1   2    3   5    7  11   13  17   19  23
  pushi(23), pushi(19), pushi(17), pushi(13), pushi(11), pushi(7), pushi(5), pushi(3), pushi(2), pushi(1),

  li(r0, -1),
  addi(sp, sp, 0),
  addm(r0, sp, 0),
  addi(r1, sp, -2),
  addm(r0, r1, 4),
  expect_r16(r0, 2),
  li(r1, -1),
  addi(r0, sp, 6),
  addm(r1, r0, -2),
  addi(r2, sp, 0),
  addm(r1, r2, 6),
  expect_r16(r1, 7),
  li(r2, -1),
  addi(r1, sp, 8),
  addm(r2, r1, 0),
  addi(r3, sp, 9),
  addm(r2, r3, 1),
  expect_r16(r2, 17),
  li(r3, -1),
  addi(r2, sp, -4),
  addm(r3, r2, 16),
  addi(r4, sp, 7),
  addm(r3, r4, 7),
  expect_r16(r3, 29),
  li(r4, -1),
  addi(r3, sp, 1),
  addm(r4, r3, 15),
  addi(r5, sp, 20),
  addm(r4, r5, -2),
  expect_r16(r4, 41),

  li(r0, 1),
  addi(sp, sp, 0),
  subm(r0, sp, 0),
  addi(r1, sp, -2),
  subm(r0, r1, 4),
  expect_r16(r0, -2),
  li(r1, 1),
  addi(r0, sp, 6),
  subm(r1, r0, -2),
  addi(r2, sp, 0),
  subm(r1, r2, 6),
  expect_r16(r1, -7),
  li(r2, 1),
  addi(r1, sp, 8),
  subm(r2, r1, 0),
  addi(r3, sp, 9),
  subm(r2, r3, 1),
  expect_r16(r2, -17),
  li(r3, 1),
  addi(r2, sp, -4),
  subm(r3, r2, 16),
  addi(r4, sp, 7),
  subm(r3, r4, 7),
  expect_r16(r3, -29),
  li(r4, 1),
  addi(r3, sp, 1),
  subm(r4, r3, 15),
  addi(r5, sp, 20),
  subm(r4, r5, -2),
  expect_r16(r4, -41),

  addi(sp, sp, 20),
#endif

#if !MINI
  // Test (d)incm/(d)decm...
  pushi(-10),
  addi(r1, sp, 0), incm(r1), expect_nz(),
  addi(r1, sp, 0), incm(r1), expect_nz(),
  addi(r1, sp, 0), incm(r1), expect_nz(),
  addi(r1, sp, 0), incm(r1), expect_nz(), expect_r16(r1, -7),
  addi(r2, sp, 0), dincm(r2), expect_nz(),
  addi(r2, sp, 0), dincm(r2), expect_nz(),
  addi(r2, sp, 0), dincm(r2), expect_z(), expect_r16(r2, -2),
  addi(r2, sp, 0), dincm(r2), expect_nz(),
  addi(r2, sp, 0), ddecm(r2), expect_z(),
  addi(r3, sp, 0), decm(r3), expect_nz(),
  addi(r3, sp, 0), incm(r3), expect_z(),
  addi(r3, sp, 0), incm(r3), expect_nz(),
  addi(r3, sp, 0), decm(r3), expect_z(),
  addi(r3, sp, 0), decm(r3), expect_nz(),
  addi(r3, sp, 0), decm(r3), expect_nz(),
  addi(r3, sp, 0), decm(r3), expect_nz(),
  addi(r3, sp, 0), decm(r3), expect_nz(), expect_r16(r3, -3),
  addi(r4, sp, 0), ddecm(r4), expect_nz(),
  addi(r4, sp, 0), ddecm(r4), expect_nz(),
  addi(r4, sp, 0), ddecm(r4), expect_nz(), expect_r16(r4, -8),
  pop(r1), expect_r16(r1, -10),
#endif

#if !MINI
  // Test (d)incs/(d)decs...
  pushi(-1),
  pushi(1),
  pushi(-2),
  pushi(2),
  incs(r1, 6), expect_z(),  expect_r16(r1, -1), lw(r1, sp, 6), expect_r16(r1,  0),
  incs(r1, 6), expect_nz(), expect_r16(r1,  0), lw(r1, sp, 6), expect_r16(r1,  1),
  decs(r3, 4), expect_z(),  expect_r16(r3,  1), lw(r3, sp, 4), expect_r16(r3,  0),
  decs(r3, 4), expect_nz(), expect_r16(r3,  0), lw(r3, sp, 4), expect_r16(r3, -1),
  dincs(r2, 2), expect_z(),  expect_r16(r2, -2), lw(r2, sp, 2), expect_r16(r2,  0),
  dincs(r2, 2), expect_nz(), expect_r16(r2,  0), lw(r2, sp, 2), expect_r16(r2,  2),
  ddecs(r4, 0), expect_z(),  expect_r16(r4,  2), lw(r4, sp, 0), expect_r16(r4,  0),
  ddecs(r4, 0), expect_nz(), expect_r16(r4,  0), lw(r4, sp, 0), expect_r16(r4, -2),
  addi(sp, sp, 8),
#else
  // Test decs...
  pushi(2),
  decs(r3, 0), expect_nz(), expect_r16(r3, 2), lw(r3, sp, 0), expect_r16(r3,  1),
  decs(r3, 0), expect_z(),  expect_r16(r3, 1), lw(r3, sp, 0), expect_r16(r3,  0),
  decs(r3, 0), expect_nz(), expect_r16(r3, 0), lw(r3, sp, 0), expect_r16(r3, -1),
  addi(sp, sp, 2),
#endif

#if 01
  // Test subroutine prolog/epilog helpers...
  li(r0, 0),
  li(r1, 0),
  li(r2, 222),
  li(r3, 0333),
  mov(r4, sp),
  pushi16(0xBB11),
  pushi16(0xAA00),
  jal(23),
  expect_r16(r0, 0xAA00),
  expect_r16(r1, 0xBB11),
  expect_r16(r2, 222),
  expect_r16(r3, 0333),
  cmp(r4, sp), expect_e(),
#if !MINI
  j(14),
#else
  j(16),
#endif
  addi(sp, sp, -8),
  sw(r2, sp, 6),
  sw(r3, sp, 4),
  ss5r(r4),
  lw(r0, sp, 8),
  lw(r1, sp, 10),
  neg(r2),
  neg(r3),
  neg(r4),
  neg(r5),
  ls5r(r4),
  lw(r3, sp, 4),
  lw(r2, sp, 6),
  last(12),
#endif

#if 01
  // Test writing to the Flags reg, focusing on the arithmetic flags...
  li(r2, -16 + 0), // -16 to keep IRQs unmasked
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
  li(r2, -16 + 1),
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
  li(r2, -16 + 2),
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
  li(r2, -16 + 4),
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
  li(r2, -16 + 8),
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
  li(r2, -16 + 15),
  mov(r1, r2),
  mf2(),
  m2f(),
  xor(r1, r2), rli(r1, 6), sli(r1, 6), expect_z(), expect_r16(r1, 0),
#endif

#if 01
  // Test flags preservation in flag-preserving instructions:
  // + push/pop, lx/sx, lx2/sx2, ls5r/ss5r
  // + di/ei
  // + li, lurpc, mov, last
  // + zxt/sxt, cpl
  // + `add sp/pc, r, simm7`, `addu sp, imm9`
  // + j<cond>, j, jal
  // + mrs/msr
  // + swi/reti
  // + mf2/m2f
  addi(sp, sp, -2 * 2),

  // N.B. loading -16 to the flags register to keep IRQs unmasked.
  li(r2, -16), mf2(), push(r0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), pushi(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), pop(r0), pop(r0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), lb(r0, sp, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), sb(r0, sp, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), lw(r0, sp, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), sw(r0, sp, 0), m2f(), andi(r2, 15), expect_z(),
#if !MINI
  li(r1, 0),
  li(r2, -16), mf2(), lb2(r0, r1, sp), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), sb2(r0, r1, sp), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), lw2(r0, r1, sp), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), sw2(r0, r1, sp), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), ls5r(r4), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), ss5r(r4), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), di(), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), ei(), m2f(), andi(r2, 15), expect_z(),
#endif
  li(r2, -16), mf2(), li(r0, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), lurpc(r0, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), mov(r0, r1), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), addi(r5, pc, 4), mf2(), last(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), zxt(r2), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), sxt(r2), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), cpl(r2), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), addi(sp, sp, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), mov(r0, sp), addi(sp, r0, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), addi(r5, pc, 4), mf2(), addi(pc, r5, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), addi(r5, pc, 4), mf2(), addi(pc, r5, 1), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), addu(sp, 0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jc(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jz(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), js(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jo(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jleu(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jl(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jle(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jnc(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jnz(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jns(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jno(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jgu(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jge(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jg(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), j(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), mf2(), jal(0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), li(r0, 4), mf2(), mrs(r1, r0), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), li(r0, 4), mf2(), msr(r0, r1), m2f(), andi(r2, 15), expect_z(),
  li(r2, -16), addi(r5, pc, 6 + /*ints enabled*/1), mf2(), mov(r2, r5), swi(7),
    mov(r0, r2), m2f(), andi(r0, 15), expect_z(), andi(r2, 15), expect_z(),

  // N.B. loading -1 to the flags register to keep IRQs unmasked.
  li(r2, -1), mf2(), push(r0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), pushi(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), pop(r0), pop(r0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), lb(r0, sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), sb(r0, sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), lw(r0, sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), sw(r0, sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
#if !MINI
  li(r1, 0),
  li(r2, -1), mf2(), lb2(r0, r1, sp), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), sb2(r0, r1, sp), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), lw2(r0, r1, sp), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), sw2(r0, r1, sp), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), ls5r(r4), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), ss5r(r4), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), di(), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), ei(), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
#endif
  li(r2, -1), mf2(), li(r0, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), lurpc(r0, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), mov(r0, r1), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), addi(r5, pc, 4), mf2(), last(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), zxt(r2), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), sxt(r2), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), cpl(r2), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), addi(sp, sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), mov(r0, sp), addi(sp, r0, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), addi(r5, pc, 4), mf2(), addi(pc, r5, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), addi(r5, pc, 4), mf2(), addi(pc, r5, 1), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), addu(sp, 0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jc(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jz(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), js(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jo(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jleu(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jl(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jle(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jnc(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jnz(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jns(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jno(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jgu(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jge(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jg(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), j(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), mf2(), jal(0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), li(r0, 4), mf2(), mrs(r1, r0), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), li(r0, 4), mf2(), msr(r0, r1), m2f(), andi(r2, 15), addi(r2, r2, -15), expect_z(),
  li(r2, -1), addi(r5, pc, 6 + /*ints enabled*/1), mf2(), mov(r2, r5), swi(7),
    mov(r0, r2), m2f(), andi(r0, 15), addi(r0, r0, -15), expect_z(), andi(r2, 15), addi(r2, r2, -15), expect_z(),

  addi(sp, sp, 2 * 2),
#endif

#if 01
  // Test resultant flags in flag-modifying instructions:
  // + and/or/xor
  // + rr/rl/sr/sl/asr
  // + neg
  // + adcz
  // + addu
  // + sac
  // + add/sub/adc/sbb
  // + addm/subm
  // = cmp (there're separate tests)
  // = (d)incm/(d)dec (there're separate tests)
  // = add22adc33, cadd24, cadd24adc3z, csub34 (there're separate tests)
  li(r0, 1),
  li(r1, 0),
  li(r3, -1),
  pushi(-1),
  pushi(1),
  pushi(0),

  and(r0, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  and(r0, r3), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  and(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  and(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  and(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  and(r3, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  andi(r0, 1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  andi(r1, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  andi(r1, 1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  or(r0, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  or(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  or(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  or(r3, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  or(r3, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  or(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  ori(r0, 1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  ori(r0, 0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  ori(r1, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  ori(r3, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  ori(r3, 1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  xor(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  xor(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  xor(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  xor(r3, r0), xor(r3, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  xor(r0, r3), xor(r0, r3), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  xori(r0, 0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  xori(r1, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  xori(r3, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  xori(r0, 1), xori(r0, 1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  xori(r3, 1), xori(r3, 1), expect_no(), expect_nc(), expect_s(), expect_nz(),

  rr(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  rr(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  rr(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rr(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rr(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rl(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  rl(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  rl(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rl(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rl(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sr(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  sr(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  sr(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sr(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sr(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sl(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  sl(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  sl(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sl(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sl(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asr(r0, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  asr(r3, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  asr(r1, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asr(r1, r0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asr(r1, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asr(r3, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  asr(r3, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  sl(r0, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  sr(r0, r3), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  sli(r1, 2), expect_no(), expect_nc(), expect_ns(), expect_z(),
  sli(r0, 15), expect_no(), expect_nc(), expect_s(), expect_nz(),
  sri(r0, 15), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  sri(r1, 1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asri(r1, 1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  asri(r3, 1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  asri(r3, 15), expect_no(), expect_nc(), expect_s(), expect_nz(),
  rli(r1, 1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  rli(r0, -1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  rli(r0, 1), expect_no(), expect_nc(), expect_ns(), expect_nz(),

  li16(r4, 0x7FFF),
  addi(r5, r4, 1),
  neg(r0), expect_no(), expect_c(), expect_s(), expect_nz(), neg(r0),
  neg(r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  neg(r3), expect_no(), expect_c(), expect_ns(), expect_nz(), neg(r3),
  neg(r4), expect_no(), expect_c(), expect_s(), expect_nz(),
  neg(r4), expect_no(), expect_c(), expect_ns(), expect_nz(),
  neg(r5), expect_o(), expect_c(), expect_s(), expect_nz(),

  clc(),
  adcz(r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  adcz(r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  adcz(r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  stc(), adcz(r0), expect_no(), expect_nc(), expect_ns(), expect_nz(), addi(r0, r0, -1),
  stc(), adcz(r1), expect_no(), expect_nc(), expect_ns(), expect_nz(), addi(r1, r1, -1),
  stc(), adcz(r3), expect_no(), expect_c(), expect_ns(), expect_z(), addi(r3, r3, -1),

  expect_r16(r0, 1),
  expect_r16(r1, 0),
  expect_r16(r3, -1),
  expect_r16(r4, 0x7FFF),
  expect_r16(r5, 0x8000),

  li(r2, 0), addu(r2, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), addu(r2, 1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0x7F), addu(r2, 0x1FF), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, 0x80), addu(r2, 0x1FF), expect_no(), expect_c(), expect_ns(), expect_z(),
  li16(r2, 0x8000), addu(r2, 0x100), expect_o(), expect_c(), expect_ns(), expect_z(),

  li(r2, 0), sac(r2, r2, 1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 1), sac(r2, r2, 15), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, 1), li16(r5, 0x7FFF), sac(r2, r5, 1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, 2), li16(r5, 0x7FFF), sac(r2, r5, 1), expect_no(), expect_c(), expect_ns(), expect_z(),
  li16(r2, 0x8000), li16(r5, 0x4000), sac(r2, r5, 1), expect_o(), expect_c(), expect_ns(), expect_z(),

  li(r2, 0), add(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), add(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), add(r2, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), add(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), add(r2, r0), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), add(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), add(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), add(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), add(r2, r3), expect_o(), expect_c(), expect_ns(), expect_nz(),
  li(r2, -1), add(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), add(r2, r0), expect_no(), expect_c(), expect_ns(), expect_z(),
  li(r2, -1), add(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),

  li(r2, 0), addm(r2, sp, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), addm(r2, sp, 2), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), addm(r2, sp, 4), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), addm(r2, sp, 0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), addm(r2, sp, 2), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), addm(r2, sp, 4), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), addm(r2, sp, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), addm(r2, sp, 2), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), addm(r2, sp, 4), expect_o(), expect_c(), expect_ns(), expect_nz(),
  li(r2, -1), addm(r2, sp, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), addm(r2, sp, 2), expect_no(), expect_c(), expect_ns(), expect_z(),
  li(r2, -1), addm(r2, sp, 4), expect_no(), expect_c(), expect_s(), expect_nz(),

  li(r2, 0), clc(), adc(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), clc(), adc(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), clc(), adc(r2, r3), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), clc(), adc(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), clc(), adc(r2, r0), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), clc(), adc(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), clc(), adc(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), clc(), adc(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), clc(), adc(r2, r3), expect_o(), expect_c(), expect_ns(), expect_nz(),
  li(r2, -1), clc(), adc(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), clc(), adc(r2, r0), expect_no(), expect_c(), expect_ns(), expect_z(),
  li(r2, -1), clc(), adc(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),

  li(r2, 0), stc(), adc(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), stc(), adc(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), stc(), adc(r2, r3), expect_no(), expect_c(), expect_ns(), expect_z(),
  li16(r2, 0x7FFF), stc(), adc(r2, r1), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), stc(), adc(r2, r0), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), stc(), adc(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), stc(), adc(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), stc(), adc(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), stc(), adc(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, -1), stc(), adc(r2, r1), expect_no(), expect_c(), expect_ns(), expect_z(),
  li(r2, -1), stc(), adc(r2, r0), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li(r2, -1), stc(), adc(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),

  li(r2, 0), addi(r2, r2, +0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), addi(r2, r2, +1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li(r2, 0), addi(r2, r2, -1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), addi(r2, r2, +0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), addi(r2, r2, +1), expect_o(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x7FFF), addi(r2, r2, -1), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), addi(r2, r2, +0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), addi(r2, r2, +1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), addi(r2, r2, -1), expect_o(), expect_c(), expect_ns(), expect_nz(),
  li(r2, -1), addi(r2, r2, +0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), addi(r2, r2, +1), expect_no(), expect_c(), expect_ns(), expect_z(),
  li(r2, -1), addi(r2, r2, -1), expect_no(), expect_c(), expect_s(), expect_nz(),

  li(r2, 0), sub(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), sub(r2, r0), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, 0), sub(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), sub(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), sub(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), sub(r2, r3), expect_o(), expect_c(), expect_s(), expect_nz(),
  li16(r2, 0x8000), sub(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), sub(r2, r0), expect_o(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), sub(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, -1), sub(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), sub(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), sub(r2, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),

  li(r2, 0), subm(r2, sp, 0), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), subm(r2, sp, 2), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, 0), subm(r2, sp, 4), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), subm(r2, sp, 0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), subm(r2, sp, 2), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), subm(r2, sp, 4), expect_o(), expect_c(), expect_s(), expect_nz(),
  li16(r2, 0x8000), subm(r2, sp, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), subm(r2, sp, 2), expect_o(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), subm(r2, sp, 4), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, -1), subm(r2, sp, 0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), subm(r2, sp, 2), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), subm(r2, sp, 4), expect_no(), expect_nc(), expect_ns(), expect_z(),

  li(r2, 0), clc(), sbb(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_z(),
  li(r2, 0), clc(), sbb(r2, r0), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, 0), clc(), sbb(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), clc(), sbb(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), clc(), sbb(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), clc(), sbb(r2, r3), expect_o(), expect_c(), expect_s(), expect_nz(),
  li16(r2, 0x8000), clc(), sbb(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li16(r2, 0x8000), clc(), sbb(r2, r0), expect_o(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), clc(), sbb(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, -1), clc(), sbb(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), clc(), sbb(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), clc(), sbb(r2, r3), expect_no(), expect_nc(), expect_ns(), expect_z(),

  li(r2, 0), stc(), sbb(r2, r1), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, 0), stc(), sbb(r2, r0), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, 0), stc(), sbb(r2, r3), expect_no(), expect_c(), expect_ns(), expect_z(),
  li16(r2, 0x7FFF), stc(), sbb(r2, r1), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), stc(), sbb(r2, r0), expect_no(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x7FFF), stc(), sbb(r2, r3), expect_no(), expect_c(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), stc(), sbb(r2, r1), expect_o(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), stc(), sbb(r2, r0), expect_o(), expect_nc(), expect_ns(), expect_nz(),
  li16(r2, 0x8000), stc(), sbb(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),
  li(r2, -1), stc(), sbb(r2, r1), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), stc(), sbb(r2, r0), expect_no(), expect_nc(), expect_s(), expect_nz(),
  li(r2, -1), stc(), sbb(r2, r3), expect_no(), expect_c(), expect_s(), expect_nz(),

  addi(sp, sp, 3 * 2),
  expect_r16(r0, 1),
  expect_r16(r1, 0),
  expect_r16(r3, -1),
#endif

#if 01
  // Test 16-bit x 16-bit = 16-bit multiplication...
  li16(r3, 200/*00C8*/),
  li16(r4, 300/*012C*/),
  lurpc(r5, 0),
  addi(pc, r5, 14 + 1), // call mul sub; 60000/*EA60*/
  expect_r16(r2, 0xEA60),
  #if !SPEEDUP_MULDIV
  j(9), // skip over mul sub
  #else
  j(7), // skip over mul sub
  #endif

  // multiplication subroutine: r2 = r3 * r4; destroys r0, r3
  li(r2, 0),
  li(r0, 16),

  #if !SPEEDUP_MULDIV
  add(r2, r2),
  add(r3, r3),
  jnc(1),
  add(r2, r4),
  addi(r0, r0, -1),
  jnz(-6),
  #else
  add22adc33(),
  cadd24(),
  addi(r0, r0, -1),
  jnz(-4),
  #endif

  addi(pc, r5, 0),
#endif

#if 01
  // Test 16-bit x 16-bit = 32-bit multiplication...
  li16(r3, 65432/*FF98*/),
  li16(r4, 54321/*D431*/),
  lurpc(r5, 0),
  addi(pc, r5, 24 + 1), // call mul sub; 3554331672/*D3DACC18*/
  expect_r16(r2, 0xCC18),
  expect_r16(r3, 0xD3DA),
  #if !SPEEDUP_MULDIV
  j(10), // skip over mul sub
  #else
  j(7), // skip over mul sub
  #endif

  // multiplication subroutine: r3:r2 = r3 * r4; destroys r0
  li(r2, 0),
  li(r0, 16),

  #if !SPEEDUP_MULDIV
  add(r2, r2),
  adc(r3, r3),
  jnc(2),
  add(r2, r4),
  adcz(r3),
  addi(r0, r0, -1),
  jnz(-7),
  #else
  add22adc33(),
  cadd24adc3z(),
  addi(r0, r0, -1),
  jnz(-4),
  #endif

  addi(pc, r5, 0),
#endif

#if 01
  // Test `16-bit / 16-bit = 16-bit quotient : 16-bit remainder` division...
  li16(r2, 60100/*EAC4*/),
  li16(r4, 300/*012C*/),
  jal(11), // call div sub; 200/*00C8*/, 100/*64*/
  expect_r16(r2, 0x00C8),
  expect_r16(r3, 0x0064),
  #if !SPEEDUP_MULDIV
  j(11), // skip over div sub
  #else
  j(9), // skip over div sub
  #endif

  // division subroutine: r2 = r2 / r4; r3 = r2 % r4; destroys r0
  li(r3, 0),
  li(r0, 16),

  #if !SPEEDUP_MULDIV
  add(r2, r2),
  adc(r3, r3),
  cmp(r3, r4),
  jc(2),
  sub(r3, r4),
  addi(r2, r2, 1),
  addi(r0, r0, -1),
  jnz(-8),
  #else
  add22adc33(),
  csub34(),
  adcz(r2),
  addi(r0, r0, -1),
  jnz(-5),
  cpl(r2),
  #endif

  addi(pc, r5, 0),
#endif

#if 01
  // Test binary to BCD conversion...
  li16(r3, 65091/*FE43*/),
  jal(15), // call bin2bcd sub; 20625/*5091*/, 6/*6*/
  expect_r16(r0, 0x5091),
  expect_r16(r3, 0x0006),
  j(25), // skip over bin2bcd sub

  // binary to BCD conversion subroutine: r3=65091 => r3=0x0006:r0=0x5091; destroys r1, r4
  const16(1000), const16(100), const16(10), const16(0), // subtrahends
  li(r1, -26), // will form address of second subtrahend (1000) in lwp below
  li16(r4, 10000), // r4 = first subtrahend

  // main loop {
  li(r0, -1), // current digit

    // digit loop {
    addi(r0, r0, 1),
    csub34(),
    jnc(-3),
    // }

  push(r0), // save/push digit
  lwp(r4, r1), // r4 = next subtrahend
  addi(r1, r1, 2),
  or(r4, r4),
  jnz(-9),       // repeat if non-zero subtrahend
  // }

  // least significant digit is in r3, other 4 are on stack
  mov(r0, r3),

  li(r4, 4),

  // loop to collect all digits from stack {
  pop(r3),
  rli(r0, -4),
  xor(r0, r3),

  addi(r4, r4, -1),
  jnz(-5),
  // }

  xor(r0, r3),

  addi(pc, r5, 0),
#endif

#if 01
  // Let's write to some 16KB blocks and read the data back...
  li(r0, 0),
  li16(r1, 16384), // ofs within sel1/sel5

  // First, access the blocks through data selector 5.
  li16(r5, 1), msr(r5, r0), // sel1 to point to ROM, selecting block 0
  li(r5, 5), // r5 selects sel5

  li(r2, 2), // starting block
  li(r3, 9), // 16KB block count
    msr(r5, r2), // sel5 to point to block in r2
    sw(r2, r1, 0),
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-5),
  li(r3, 8), // 16KB block count
    msr(r5, r2), // sel5 to point to block in r2
#if !MINI
    sw2(r2, r1, r0),
#else
    sw(r2, r1, 0),
#endif
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-5),

  li(r2, 2), // starting block
  li(r3, 9), // 16KB block count
    msr(r5, r2), // sel5 to point to block in r2
    lw(r4, r1, 0), cmp(r4, r2), expect_e(),
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-7),
  li(r3, 8), // 16KB block count
    msr(r5, r2), // sel5 to point to block in r2
#if !MINI
    lw2(r4, r1, r0), cmp(r4, r2), expect_e(),
#else
    lw(r4, r1, 0), cmp(r4, r2), expect_e(),
#endif
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-7),

  // Next, access the blocks through program/code selector 1.
  msr(r5, r0), // sel5 to point to ROM, selecting block 0
  li(r5, 1), // r5 selects sel1

  li(r2, 3), // starting block
  li(r3, 17), // 16KB block count
    msr(r5, r2), // sel1 to point to block in r2
    addi(r4, r1, -2), // -2 to compensate for different pc's in sub & swp
    sub(r4, pc),
    swp(r2, r4), // at this point, pc + r4 == r1
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-7),

  li(r2, 3), // starting block
  li(r3, 17), // 16KB block count
    msr(r5, r2), // sel1 to point to block in r2
    addi(r4, r1, -2), // -2 to compensate for different pc's in sub & lwp
    sub(r4, pc),
    lwp(r4, r4), // at this point, pc + r4 == r1
    cmp(r4, r2), expect_e(),
    addi(r2, r2, 15), // blocks are 15 apart
    addi(r3, r3, -1),
    jnz(-9),

  msr(r5, r0), // sel1 to point to ROM, selecting block 0
#endif

#if 01
  // Test the SWI that copies words between arbitrary locations.
  // Copy a few words to block 2.
  li16(r0, /*dst_ofs*/0x1090),
  li(r1, /*dst_block*/2),
  addi(r2, /*src_ofs*/pc, 8),
  li(r3, /*src_block*/0),
  li(r4, /*word_count*/9),
  swi(5),
  j(9), // skip data
  const16(0x1111),
  const16(0x2222),
  const16(0x3333),
  const16(0x4444),
  const16(0x5555),
  const16(0x6666),
  const16(0x7777),
  const16(0x8888),
  const16(0x9999),
  // Copy them back to stack and verify.
  addi(sp, sp, -9 * 2),
  mov(r2, /*src_ofs*/r0),
  mov(r3, /*src_block*/r1),
  mov(r0, /*dst_ofs*/sp),
  li(r1, /*dst_block*/1),
  swi(5),
  pop(r0), expect_r16(r0, 0x1111),
  pop(r0), expect_r16(r0, 0x2222),
  pop(r0), expect_r16(r0, 0x3333),
  pop(r0), expect_r16(r0, 0x4444),
  pop(r0), expect_r16(r0, 0x5555),
  pop(r0), expect_r16(r0, 0x6666),
  pop(r0), expect_r16(r0, 0x7777),
  pop(r0), expect_r16(r0, 0x8888),
  pop(r0), expect_r16(r0, 0x9999),
#endif

#if 01
  // Test the SWI that transfers control to arbitrary location.
  // block 0
  // push target address in block 0
  addi(r0, pc, 62), // ofs: r0 = .block0continue
  pushi(0),        // block: 0
  push(r0),
  // copy to block 3
  li16(r0, /*dst_ofs*/0xC030),
  li(r1, /*dst_block*/3),
  addi(r2, /*src_ofs*/pc, 40),
  li(r3, /*src_block*/0),
  li(r4, /*word_count*/5),
  swi(5),
  // push target address in block 3
  push(r1),
  push(r0),

  // copy to block 2
  li16(r0, /*dst_ofs*/0x8020),
  li(r1, /*dst_block*/2),
  addi(r2, /*src_ofs*/pc, 12),
  li(r3, /*src_block*/0),
  li(r4, /*word_count*/5),
  swi(5),
  // target address in block 2 is already in r0,r1

  // block 0
  // ofs, block set in r0,r1 above
  li16(r2, 0x0011),
  swi(4),

  // block 2
  pop(r0), // ofs
  pop(r1), // block
  li16(r3, 0x1122),
  swi(4),

  // block 3
  pop(r0), // ofs
  pop(r1), // block
  li16(r4, 0x2200),
  swi(4),

//.block0continue:
  // block 0
  expect_r16(r2, 0x0011),
  expect_r16(r3, 0x1122),
  expect_r16(r4, 0x2200),
#endif

  j(-1)

#endif // #endif of #ifndef JUST_OPS

#ifndef JUST_OPS

const ushort mem[] =
{
#define LINE(a) a
#define JUST_OPS
#include THIS_FILE
#undef JUST_OPS
#undef LINE
};

const ushort linenums[] =
{
#define LINE(a) __LINE__
#define JUST_OPS
#include THIS_FILE
#undef JUST_OPS
#undef LINE
};

FILE* startup(int argc, char* argv[], char** outname, int* bigendian)
{
  FILE* f;

  if (argc < 2 || argc > 3)
  {
lusage:
    fprintf(stderr,
            "Usage:\n"
            "  " THIS_FILE_SANS_EXT " [options] <output_file>\n"
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
  uint size = sizeof(mem) / sizeof(mem[0]), idx, l = 1;

  for (idx = 0; idx < size; idx++)
    writebytes(f, outname, bigendian, 2, mem[idx]);
  if (fclose(f))
  {
    fprintf(stderr, "Can't write to \"%s\"", outname);
    exit(EXIT_FAILURE);
  }

  if ((f = fopen(THIS_FILE, "r")) == NULL)
  {
    fprintf(stderr, "Can't open " THIS_FILE "\n");
    exit(EXIT_FAILURE);
  }

  for (idx = 0; idx < size; idx++)
  {
    int ch;
    printf(THIS_FILE ":%-5u  %04X  %04X  ", linenums[idx], idx * 2, mem[idx]);
    while (l < linenums[idx])
    {
      while ((ch = fgetc(f)) != '\n');
      l++;
    }
    if (l == linenums[idx])
    {
      while ((ch = fgetc(f)) != '\n')
        putchar(ch);
      l++;
    }
    putchar('\n');
  }
  fclose(f);

  return 0;
}

#endif // #endif of #ifndef JUST_OPS
