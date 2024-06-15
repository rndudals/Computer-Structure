#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ENABLE_FORWARD 1
#define ENABLE_BRANCH_PREDICTION 1
#define ALWAYS_TAKEN 1

int cycle = 0; // ���α׷� ���� �� ����Ŭ ���� ����
int data_row; // MIPS ��ɾ� ������ �迭�� �� ��
char data[100][33]; // ���� ���ڿ��� ǥ���� MIPS ��ɾ �����ϴ� �迭
int pc = 0; // ���α׷� ī����, ���� ���� ���� ��ɾ��� ��ġ�� ����Ŵ
char* cur_instruction; // ���� ���� ���� ��ɾ��� ���� ���ڿ�

uint32_t Register[32]; // MIPS ��Ű��ó�� 32�� ��������

uint32_t opcode; // ��ɾ��� ���� �ڵ� �ʵ�
uint32_t rs; // ��ɾ��� ù ��° �ҽ� ��������
uint32_t rt; // ��ɾ��� �� ��° �ҽ� �������� �Ǵ� ��� ��������
uint32_t rd; // ��ɾ��� ��� �������� (R-type ��ɾ�)
uint32_t shamt; // ��ɾ��� ����Ʈ �� (shift amount)
uint32_t funct; // R-type ��ɾ��� ��� �ڵ�

uint32_t J_address; // J-type ��ɾ��� ���� �ּ�
uint32_t memory[16777217]; // MIPS �ùķ������� ���� �޸� ����
int16_t immediate; // I-type ��ɾ��� ��� ��

int RegDst; // ������ �������� ���� ��ȣ (0: rt, 1: rd)
int RegWrite; // �������� ���� Ȱ��ȭ ��ȣ
int ALUSrc; // ALU �ҽ� ���� ��ȣ (0: ��������, 1: ��� ��)
int PCSrc; // PC �ҽ� ���� ��ȣ (0: �⺻, 1: �б�/����)
int MemRead; // �޸� �б� Ȱ��ȭ ��ȣ
int MemWrite; // �޸� ���� Ȱ��ȭ ��ȣ
int MemtoReg; // �޸�-�������� ���� ��ȣ (0: ALU ���, 1: �޸� �б� ���)
int ALUOp; // ALU ���� ���� ��ȣ (���� ���� �ڵ�)

int ALUResult; // ALU ���� ���

int readData1; // ù ��° �������Ϳ��� ���� ������
int readData2; // �� ��° �������Ϳ��� ���� ������

int R_type_cnt; // ����� R-type ��ɾ��� ����
int J_type_cnt; // ����� J-type ��ɾ��� ����
int I_type_cnt; // ����� I-type ��ɾ��� ����

// ���� ������ ������ ����� �����ϱ� ���� ��������
uint64_t HI = 0; // ���� ����� ���� 32��Ʈ ����
uint64_t LO = 0; // ���� ����� ���� 32��Ʈ ����

// ���������� �������� ����ü ����
typedef struct {
    uint32_t pc;
    uint32_t instruction;
    int valid;
    uint32_t rs, rt, rd;
    int16_t immediate;
} IF_ID_Reg;

typedef struct {
    uint32_t pc;
    uint32_t instruction;
    int valid;
    uint32_t rs, rt, rd;
    uint32_t ALUResult, readData1, readData2;
    int16_t immediate;
    int RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp;
} ID_EX_Reg;

typedef struct {
    uint32_t pc;
    uint32_t instruction;
    int valid;
    uint32_t ALUResult, writeData;
    uint32_t writeReg;
    int MemRead, MemWrite, MemtoReg, RegWrite;
} EX_MEM_Reg;

typedef struct {
    uint32_t pc;
    uint32_t instruction;
    int valid;
    uint32_t readData;
    uint32_t ALUResult;
    uint32_t writeReg;
    int MemtoReg, RegWrite;
} MEM_WB_Reg;

// ���������� �������� �ʱ�ȭ
IF_ID_Reg IF_ID;
ID_EX_Reg ID_EX;
EX_MEM_Reg EX_MEM;
MEM_WB_Reg MEM_WB;

char* defineRegisterName(int n) {
    switch (n) {
    case 0: return "zero"; case 1: return "at"; case 2: return "v0"; case 3: return "v1"; case 4: return "a0"; case 5: return "a1"; case 6: return "a2";
    case 7: return "a3"; case 8: return "t0"; case 9: return "t1"; case 10: return "t2"; case 11: return "t3"; case 12: return "t4"; case 13: return "t5";
    case 14: return "t6"; case 15: return "t7"; case 16: return "s0"; case 17: return "s1"; case 18: return "s2"; case 19: return "s3"; case 20: return "s4";
    case 21: return "s5"; case 22: return "s6"; case 23: return "s7"; case 24: return "t8"; case 25: return "t9"; case 26: return "k0"; case 27: return "k1";
    case 28: return "gp"; case 29: return "sp"; case 30: return "s8"; case 31: return "ra"; default: return "unknown"; // �� �� ���� �������� ��ȣ
    }
}

// ������ ���ڿ��� 16���� �ҹ��� ���ڿ��� ��ȯ�Ͽ� ��ȯ�ϴ� �Լ�
char* binaryToHexLower(const char* binaryString) {
    static char hexString[9]; // 8�ڸ� 16���� + �� ���� ����
    unsigned long number = strtol(binaryString, NULL, 2);
    sprintf(hexString, "%08lx", number); // �ҹ��ڷ� ����ϱ� ���� %lx ���
    return hexString;
}

// ������ ���ڿ��� uint32_t�� ��ȯ�ϴ� �Լ�
uint32_t binaryToUint32(const char* binaryString) {
    return (uint32_t)strtol(binaryString, NULL, 2);
}

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

// ���� ��ȣ�� �����ϴ� �Լ�
void setControlSignals(int regDst, int regWrite, int aluSrc, int pcSrc, int memRead, int memWrite, int memToReg, int aluOp) {
    RegDst = regDst;
    RegWrite = regWrite;
    ALUSrc = aluSrc;
    PCSrc = pcSrc;
    MemRead = memRead;
    MemWrite = memWrite;
    MemtoReg = memToReg;
    ALUOp = aluOp;
}

// RType ��ɾ �Ľ��ϰ� ���� ���� ���� ��ȣ�� �����ϴ� �Լ��Դϴ�.
void parseRType(uint32_t instruction) {
    R_type_cnt++;
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    rd = (instruction >> 11) & 0x1F;
    shamt = (instruction >> 6) & 0x1F;
    funct = instruction & 0x3F;

    switch (funct) {
    case 37: // move
    case 33: // addu
        printf("Inst: %s %s %s %s\n", "addu", defineRegisterName(rd), defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rd: %d (%d), rs: %d (%d), rt: %d (%d)\n", opcode, rd, Register[rd], rs, Register[rs], rt, Register[rt]);
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 2);
        break;
    case 24: // mult
        printf("Inst: %s %s %s\n", "mult", defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rs: %d (%d), rt: %d (%d)\n", opcode, rs, Register[rs], rt, Register[rt]);
        setControlSignals(0, 1, 0, 0, 0, 0, 0, 3);
        break;
    case 18: // mflo
        printf("Inst: %s %s\n", "mflo", defineRegisterName(rd));
        printf("\t\topcode: %d, rd: %d (%d)\n", opcode, rd, Register[rd]);
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 4);
        break;
    case 8: // jr
        printf("Inst: %s %s\n", "jr", defineRegisterName(rs));
        printf("\t\topcode: %d, rs: %d (%d)\n", opcode, rs, Register[rs]);
        setControlSignals(0, 0, 0, 1, 0, 0, 0, 0);
        break;
    default:
        printf("\t\topcode: %d, Unknown funct %02d\n", opcode, funct);
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// JType ��ɾ �Ľ��ϴ� �Լ��Դϴ�.
void parseJType(uint32_t instruction) {
    J_type_cnt++;
    opcode = (instruction >> 26) & 0x3F;
    J_address = instruction & 0x03FFFFFF;
    printf("Inst: jal: %d\n", J_address);
    printf("\t\topcode: %d, address: %d\n", opcode, J_address);
    setControlSignals(0, 1, 0, 1, 0, 0, 0, 0);
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// IType ��ɾ �Ľ��ϴ� �Լ��Դϴ�.
void parseIType(uint32_t instruction) {
    I_type_cnt++;
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    immediate = (int16_t)(instruction & 0xFFFF);

    switch (opcode) {
    case 9: // addiu
        printf("Inst: %s %s %s %d\n", "addiu", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 2);
        break;
    case 43: // sw
        printf("Inst: %s %s %d(%s)\n", "sw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 0, 1, 0, 0, 1, 0, 2);
        break;
    case 35: // lw
        printf("Inst: %s %s %d(%s)\n", "lw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 1, 0, 1, 2);
        break;
    case 10: // slti
        printf("Inst: %s %s %s %d\n", "slti", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 7);
        break;
    case 5: // bnez or bne
        if (rt == 0) {
            printf("Inst: %s %s %d\n", "bnez", defineRegisterName(rs), immediate);
            printf("\t\topcode: %d, rs: %d (%d), imm: %d\n", opcode, rs, Register[rs], immediate);
        }
        else {
            printf("Inst: %s %s %s %d\n", "bne", defineRegisterName(rs), defineRegisterName(rt), immediate);
            printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        }
        setControlSignals(0, 0, 0, 1, 0, 0, 0, 6);
        break;
    case 4: // b or beqz
        if (rt == 0) {
            printf("Inst: %s %d\n", "b", immediate);
        }
        else {
            printf("Inst: %s %s %d\n", "beqz", defineRegisterName(rs), immediate);
        }
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 0, 0, 1, 0, 0, 0, 6);
        break;
    default:
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// ��ɾ� Ÿ���� �Ǻ��ϰ� ��ȯ�ϴ� �Լ��Դϴ�.
char classifyInstruction(uint32_t instruction) {
    char ret = '?';
    uint32_t opcode = instruction >> 26;
    int R_funct = instruction & 0x3F;

    if (opcode == 0) {
        int isNop = instruction & 0x3F;
        if (isNop == 0) {
            return 'N';
        }
        ret = 'R';
    }
    else if (opcode == 0x2 || opcode == 0x3) {
        ret = 'J';
    }
    else {
        ret = 'I';
    }
    return ret;
}

void parseData(FILE* fp) {
    unsigned char buffer[4];
    size_t bytesRead;

    int row = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        uint32_t instruction = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

        for (int i = 0; i < 32; i++) {
            data[row][i] = (instruction & (1 << (31 - i))) ? '1' : '0';
        }
        data[row][32] = '\0';

        row++;
    }
    data_row = row;
}

// ������ ������ �����ϴ� �Լ�
void forwarding() {
    if (ENABLE_FORWARD) {
        // EX �ܰ迡�� ������
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0)) {
            if (EX_MEM.writeReg == ID_EX.rs) {
                ID_EX.readData1 = EX_MEM.ALUResult;
            }
            if (EX_MEM.writeReg == ID_EX.rt) {
                ID_EX.readData2 = EX_MEM.ALUResult;
            }
        }

        // MEM �ܰ迡�� ������
        if (MEM_WB.RegWrite && (MEM_WB.writeReg != 0)) {
            if (MEM_WB.writeReg == ID_EX.rs) {
                ID_EX.readData1 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
            }
            if (MEM_WB.writeReg == ID_EX.rt) {
                ID_EX.readData2 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
            }
        }
    }
}

// ���� ó�� �Լ� (������ ����)
int hazardDetection() {
    // Load-Use ������ ���� ����
    if (ID_EX.MemRead && ((ID_EX.rt == IF_ID.rs) || (ID_EX.rt == IF_ID.rt))) {
        //
