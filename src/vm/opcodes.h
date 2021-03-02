//
// Created by gpf on 2020/12/17.
//

#ifndef LUUA2_OPCODES_H
#define LUUA2_OPCODES_H

#include "utils/typeAlias.h"
#include <string>

#define SIZE_C		9
#define SIZE_B		9
#define SIZE_Bx		(SIZE_C + SIZE_B)
#define SIZE_A		8
#define SIZE_Ax		(SIZE_C + SIZE_B + SIZE_A)

#define SIZE_OP		6

#define POS_OP		0
#define POS_A		(POS_OP + SIZE_OP)
#define POS_C		(POS_A + SIZE_A)
#define POS_B		(POS_C + SIZE_C)
#define POS_Bx		POS_C
#define POS_Ax		POS_A

//#define MAXARG_Bx        ((1<<SIZE_Bx)-1)
//#define MAXARG_sBx        (MAXARG_Bx>>1)         /* 'sBx' is signed */

/*
** Macros to operate RK indices
*/

/* this bit 1 means constant (0 means register) */
#define BITRK		(1 << (SIZE_B - 1))

/* test whether value is a constant */
#define ISK(x)		((x) & BITRK)

/* gets the index of the constant */
#define INDEXK(r)	((int)(r) & ~BITRK)

#if !defined(MAXINDEXRK)  /* (for debugging only) */
#define MAXINDEXRK	(BITRK - 1)
#endif

/* code a constant index as a RK value */
#define RKMASK(x)	((x) | BITRK)


/*
** invalid register that fits in 8 bits
*/
#define NO_REG		MAXARG_A


/* creates a mask with 'n' 1 bits at position 'p' */
#define MASK1(n,p)	((~((~(Instruction)0)<<(n)))<<(p))

/* creates a mask with 'n' 0 bits at position 'p' */
#define MASK0(n,p)	(~MASK1(n,p))

#define setarg(i,v,pos,size)	((i) = (((i)&MASK0(size,pos)) | \
                ((((Instruction) v)<<pos)&MASK1(size,pos))))


class ByteCode{
public:
    enum OpCode{
        /*----------------------------------------------------------------------
name		args	description
------------------------------------------------------------------------*/
        OP_MOVE,/*	A B	R(A) := R(B)					*/
        OP_LOADK,/*	A Bx	R(A) := Kst(Bx)					*/
        OP_LOADKX,/*	A 	R(A) := Kst(extra arg)				*/
        OP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/
        OP_LOADNIL,/*	A B	R(A), R(A+1), ..., R(A+B) := nil		*/
        OP_GETUPVAL,/*	A B	R(A) := UpValue[B]				*/

        OP_GETTABUP,/*	A B C	R(A) := UpValue[B][RK(C)]			*/
        OP_GETTABLE,/*	A B C	R(A) := R(B)[RK(C)]				*/

        OP_SETTABUP,/*	A B C	UpValue[A][RK(B)] := RK(C)			*/
        OP_SETUPVAL,/*	A B	UpValue[B] := R(A)				*/
        OP_SETTABLE,/*	A B C	R(A)[RK(B)] := RK(C)				*/

        OP_NEWTABLE,/*	A B C	R(A) := {} (size = B,C)				*/

        OP_SELF,/*	A B C	R(A+1) := R(B); R(A) := R(B)[RK(C)]		*/

        OP_ADD,/*	A B C	R(A) := RK(B) + RK(C)				*/
        OP_SUB,/*	A B C	R(A) := RK(B) - RK(C)				*/
        OP_MUL,/*	A B C	R(A) := RK(B) * RK(C)				*/
        OP_MOD,/*	A B C	R(A) := RK(B) % RK(C)				*/
        OP_POW,/*	A B C	R(A) := RK(B) ^ RK(C)				*/
        OP_DIV,/*	A B C	R(A) := RK(B) / RK(C)				*/
        OP_IDIV,/*	A B C	R(A) := RK(B) // RK(C)				*/
        OP_BAND,/*	A B C	R(A) := RK(B) & RK(C)				*/
        OP_BOR,/*	A B C	R(A) := RK(B) | RK(C)				*/
        OP_BXOR,/*	A B C	R(A) := RK(B) ~ RK(C)				*/
        OP_SHL,/*	A B C	R(A) := RK(B) << RK(C)				*/
        OP_SHR,/*	A B C	R(A) := RK(B) >> RK(C)				*/
        OP_UNM,/*	A B	R(A) := -R(B)					*/
        OP_BNOT,/*	A B	R(A) := ~R(B)					*/
        OP_NOT,/*	A B	R(A) := not R(B)				*/
        OP_LEN,/*	A B	R(A) := length of R(B)				*/

        OP_CONCAT,/*	A B C	R(A) := R(B).. ... ..R(C)			*/

        OP_JMP,/*	A sBx	pc+=sBx; if (A) close all upvalues >= R(A - 1)	*/
        OP_EQ,/*	A B C	if ((RK(B) == RK(C)) ~= A) then pc++		*/
        OP_LT,/*	A B C	if ((RK(B) <  RK(C)) ~= A) then pc++		*/
        OP_LE,/*	A B C	if ((RK(B) <= RK(C)) ~= A) then pc++		*/

        OP_TEST,/*	A C	if not (R(A) <=> C) then pc++			*/
        OP_TESTSET,/*	A B C	if (R(B) <=> C) then R(A) := R(B) else pc++	*/

        OP_CALL,/*	A B C	R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) */
        OP_TAILCALL,/*	A B C	return R(A)(R(A+1), ... ,R(A+B-1))		*/
        OP_RETURN,/*	A B	return R(A), ... ,R(A+B-2)	(see note)	*/

        OP_FORLOOP,/*	A sBx	R(A)+=R(A+2);
			if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }*/
        OP_FORPREP,/*	A sBx	R(A)-=R(A+2); pc+=sBx				*/

        OP_TFORCALL,/*	A C	R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));	*/
        OP_TFORLOOP,/*	A sBx	if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }*/

        OP_SETLIST,/*	A B C	R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B	*/

        OP_CLOSURE,/*	A Bx	R(A) := closure(KPROTO[Bx])			*/

        OP_VARARG,/*	A B	R(A), R(A+1), ..., R(A+B-2) = vararg		*/

        OP_EXTRAARG/*	Ax	extra (larger) argument for previous opcode	*/
    };

    enum OpArgMask{
        OpArgN,  /* argument is not used */
        OpArgU,  /* argument is used */
        OpArgR,  /* argument is a register or a jump offset */
        OpArgK   /* argument is a constant or register/constant */
    };

    enum OpMode {iABC, iABx, iAsBx, iAx};

    //constvalues
    static const int kNumOpcode = OP_EXTRAARG + 1;
    static const int MAXARG_BX = (1<<18) -1;
    static const int MAXARG_sBx = MAXARG_BX >>1;

    //functions 一些inline函数用来得到操作数，可以用宏来实现
    static inline int getArgA(Instruction i)     {return (i>>6 & 0xFF);}
    static inline int getArgC(Instruction i)     {return (i>>14 & 0x1FF);}
    static inline int getArgB(Instruction i)     {return (i>>23 & 0x1FF);}

    static inline void setArgA(Instruction& i, int a)     {setarg(i, a, POS_A, SIZE_A);}
    static inline void setArgB(Instruction& i, int b)     {setarg(i, b, POS_B, SIZE_B);}
    static inline void setArgC(Instruction& i, int c)     {setarg(i,c, POS_C, SIZE_C);}
    static inline void setArgBx(Instruction& i, int bx)     {setarg(i,bx, POS_Bx, SIZE_Bx);}
    static inline void setArgsBx(Instruction& i, int b)     {setArgBx(i, static_cast<unsigned int>(b+ MAXARG_sBx));}
    // ABx
    static inline int getArgBx(Instruction i)     {return (i>>14);}
    static inline int getArgsBx(Instruction i)    {return (i>>14) - MAXARG_sBx;}
    //ax
    static inline int getAx(Instruction i)     {return (i>>6);}
    //get opmode
    /*
    static inline int getOpMode(Instruction i)      {return opmodes[getOpCode(i)] & 0x3;}//后两位是mode
    static inline int getBMode(Instruction i)       {return (opmodes[getOpCode(i)]>>4) & 3;}
    static inline int getCMode(Instruction i)       {return (opmodes[getOpCode(i)]>>2) & 3;}
    static inline int testAMode(Instruction i)       {return (opmodes[getOpCode(i)]) & (1<<6);}
    static inline int testTMode(Instruction i)       {return (opmodes[getOpCode(i)]) & (1<<7);}
    static inline const char* getName(Instruction i) {return opnames[getOpCode(i)];}
     */

    //get opmode with op
    static inline int getOpCode(Instruction i)      {return (i&0x3F);}
    static inline void setOpCode(Instruction& i, int op) {
        i = (i&MASK0(SIZE_OP, POS_OP)) | (static_cast<Instruction>(op)<<POS_OP & MASK1(SIZE_OP, POS_OP));}
    static inline int getOpMode(int o)      {return opmodes[o] & 0x3;}//后两位是mode
    static inline int getBMode(int o)       {return (opmodes[o]>>4) & 3;}
    static inline int getCMode(int o)       {return (opmodes[o]>>2) & 3;}
    static inline int testAMode(int o)       {return (opmodes[o]) & (1<<6);}
    static inline int testTMode(int o)       {return (opmodes[o]) & (1<<7);}
    static inline const char* getName(int o) {return opnames[o];}

    //create code
    static inline Instruction createABC(OpCode op, int a,int b,int c){
        return static_cast<Instruction>(op) |
                (static_cast<Instruction>(a)<<POS_A) |
                (static_cast<Instruction>(b)<<POS_B) |
                (static_cast<Instruction>(c)<<POS_C);

    }
    static Instruction createABx(OpCode op, int a, unsigned int bx){
        return static_cast<Instruction>(op) |
               (static_cast<Instruction>(a)<<POS_A) |
               (static_cast<Instruction>(bx)<<POS_Bx);
    }
    static Instruction createAx(OpCode op, int a,int ax){
        return static_cast<Instruction>(op) |
               (static_cast<Instruction>(a)<<POS_A) |
               (static_cast<Instruction>(ax)<<POS_Ax);
    }

    //tostring指令
    static std::string dump(Instruction i);


    //指令与名称
    static LuaByte opmodes[kNumOpcode];
    static const char* opnames[kNumOpcode+1];
private:

};
#define LFIELDS_PER_FLUSH	50


/*
 * Lua每条指令由32位组成
 * 其中后6位是opcode (0~5)
 * 其他24位为操纵码，一共有三种操纵码的类型
 * 1、iABC 有3个寄存器参数 A[6~13], B[14~22],C[23~31]
 * 2、iABx 有两个操纵码，A[6~13], Bx[14~31] 其中bx是立即数
 * 3、iAsBx有两个操作码，A[6~13],sbx[14~31]
 * 4、iAx有一个操作码 ,Ax[6~31]
 *
 * 其中A一定是目标索引寄存器，即a的值为寄存器索引
 * 其他的操作数分为4类：
 * 1、OpArgN: 不表示任何信息，也就是不会被使用
 * 2、OpArgR: 表示寄存器索引，在iasbx模式下表示跳转偏移
 * 3、OpArgK: 表示常量表索引或者寄存器索引，根据操纵数的最高位来判断，如果是1表示常量表的索引
 * 4、OpArgU: 其他类型
 *
 *
 */


#endif //LUUA2_OPCODES_H
