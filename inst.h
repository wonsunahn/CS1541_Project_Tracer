#ifndef INST_H
#define INST_H

enum opcode {
	ti_NOP = 0,
	ti_RTYPE,
	ti_ITYPE,
	ti_LOAD,
	ti_STORE,
	ti_BRANCH,
	ti_JTYPE,
	ti_SPECIAL,
	ti_JRTYPE
};

typedef struct {
	unsigned char type;		// see above
	unsigned char sReg_a;		// 1st operand
	unsigned char sReg_b;		// 2nd operand
	unsigned char dReg;		// dest. operand
	unsigned int PC;		// program counter
	unsigned int Addr;		// mem. address
} instruction;

typedef struct {
	instruction inst;		// static instruction fetched to create this dynamic instruction
	unsigned int seq;		// dynamic sequence number (important for in-order commit)
} dynamic_inst;

#endif /* #define INST_H */
