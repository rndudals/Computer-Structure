#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ENABLE_FORWARD 1
#define ENABLE_BRANCH_PREDICTION 1

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
    case 7: return "a3";   case 8: return "t0"; case 9: return "t1"; case 10: return "t2"; case 11: return "t3"; case 12: return "t4"; case 13: return "t5";
    case 14: return "t6";  case 15: return "t7"; case 16: return "s0"; case 17: return "s1"; case 18: return "s2"; case 19: return "s3"; case 20: return "s4";
    case 21: return "s5";  case 22: return "s6"; case 23: return "s7";  case 24: return "t8"; case 25: return "t9"; case 26: return "k0"; case 27: return "k1";
    case 28: return "gp";  case 29: return "sp"; case 30: return "s8";  case 31: return "ra"; default: return "unknown"; // �� �� ���� �������� ��ȣ
    }
}

// ������ ���ڿ��� 16���� �ҹ��� ���ڿ��� ��ȯ�Ͽ� ��ȯ�ϴ� �Լ�
char* binaryToHexLower(const char* binaryString) {
    // ��ȯ�� 16���� ���� ������ ���� ���� ���ڿ� �Ҵ�
    static char hexString[9]; // 8�ڸ� 16���� + �� ���� ����
    unsigned long number = strtol(binaryString, NULL, 2);

    // 16���� �ҹ��� ���ڿ� ����
    sprintf(hexString, "%08lx", number); // �ҹ��ڷ� ����ϱ� ���� %lx ���

    return hexString;
}

// ������ ���ڿ��� uint32_t�� ��ȯ�ϴ� �Լ�
uint32_t binaryToUint32(const char* binaryString) {
    // ������ ���ڿ��� uint32_t�� ��ȯ
    return (uint32_t)strtol(binaryString, NULL, 2);
}

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

// ���� ��ȣ�� �����ϴ� �Լ�
// �� �Լ��� �Ľ̵� ��ɾ ���� CPU�� ���� ��ȣ�� �����մϴ�.
void setControlSignals(int regDst, int regWrite, int aluSrc, int pcSrc, int memRead, int memWrite, int memToReg, int aluOp) {
    RegDst = regDst;         // �������� ��� ����
    RegWrite = regWrite;     // �������� ���� Ȱ��ȭ
    ALUSrc = aluSrc;         // ALU �Է� ����
    PCSrc = pcSrc;           // ���α׷� ī���� �ҽ� ����
    MemRead = memRead;       // �޸� �б� Ȱ��ȭ
    MemWrite = memWrite;     // �޸� ���� Ȱ��ȭ
    MemtoReg = memToReg;     // �޸�-�������� ����
    ALUOp = aluOp;           // ALU ���� ���� ����
}

// RType ��ɾ �Ľ��ϰ� ���� ���� ���� ��ȣ�� �����ϴ� �Լ��Դϴ�.
void parseRType(uint32_t instruction) {
    R_type_cnt++;  // �Ľ̵� R-type ��ɾ��� ���� ������ŵ�ϴ�.

    // ��ɾ�� �� �ʵ带 �����մϴ�.
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    rd = (instruction >> 11) & 0x1F;
    shamt = (instruction >> 6) & 0x1F;
    funct = instruction & 0x3F;

    // funct �ʵ忡 ���� �ٸ� ������ �����մϴ�.
    switch (funct) {
    case 37: // move
    case 33: // addu
        // �ش� ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %s %s\n", "addu", defineRegisterName(rd), defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rd: %d (%d), rs: %d (%d), rt: %d (%d)\n", opcode, rd, Register[rd], rs, Register[rs], rt, Register[rt]);
        // ���� ��ȣ�� �����մϴ�.
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 2);
        break;
    case 24: // mult
        // mult ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %s\n", "mult", defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rs: %d (%d), rt: %d (%d)\n", opcode, rs, Register[rs], rt, Register[rt]);
        setControlSignals(0, 1, 0, 0, 0, 0, 0, 3);
        break;
    case 18: // mflo
        // mflo ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s\n", "mflo", defineRegisterName(rd));
        printf("\t\topcode: %d, rd: %d (%d)\n", opcode, rd, Register[rd]);
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 4);
        break;
    case 8: // jr
        // jr ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s\n", "jr", defineRegisterName(rs));
        printf("\t\topcode: %d, rs: %d (%d)\n", opcode, rs, Register[rs]);
        setControlSignals(0, 0, 0, 1, 0, 0, 0, 0);
        break;
    default:
        // �� �� ���� �Լ� �ڵ� ó��
        printf("\t\topcode: %d, Unknown funct %02d\n", opcode, funct);
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// JType ��ɾ �Ľ��ϴ� �Լ��Դϴ�.
void parseJType(uint32_t instruction) {
    J_type_cnt++;  // �Ľ̵� J-type ��ɾ��� ���� ������ŵ�ϴ�.

    // ��ɾ�� opcode�� ���� �ּҸ� �����մϴ�.
    opcode = (instruction >> 26) & 0x3F;
    J_address = instruction & 0x03FFFFFF;
    // ��ɾ� ������ ����մϴ�.
    printf("Inst: jal: %d\n", J_address);
    printf("\t\topcode: %d, address: %d\n", opcode, J_address);

    // ���� ��ȣ�� �����մϴ�.
    setControlSignals(0, 1, 0, 1, 0, 0, 0, 0);
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// IType ��ɾ �Ľ��ϴ� �Լ��Դϴ�.
void parseIType(uint32_t instruction) {
    I_type_cnt++;  // �Ľ̵� I-type ��ɾ��� ���� ������ŵ�ϴ�.

    // ��ɾ�� opcode, rs, rt, immediate�� �����մϴ�.
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    immediate = (int16_t)(instruction & 0xFFFF);

    // opcode�� ���� �ٸ� ������ �����մϴ�.
    switch (opcode) {
    case 9: // addiu
        // addiu ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %s %d\n", "addiu", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 2);
        break;
    case 43: // sw
        // sw ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %d(%s)\n", "sw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 0, 1, 0, 0, 1, 0, 2);
        break;
    case 35: // lw
        // lw ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %d(%s)\n", "lw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 1, 0, 1, 2);
        break;
    case 10: // slti
        // slti ��ɾ� ������ ����մϴ�.
        printf("Inst: %s %s %s %d\n", "slti", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 7);
        break;
    case 5: // bnez or bne
        // bne ��ɾ� ������ ����մϴ�.
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
        // b beqz ��ɾ� ������ ����մϴ�.
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
        // �� �� ���� opcode ó��
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// ��ɾ� Ÿ���� �Ǻ��ϰ� ��ȯ�ϴ� �Լ��Դϴ�.
// �� �Լ��� ��ɾ��� opcode�� �м��Ͽ� R, I, J Ÿ�� �� �ϳ��� �����ϰ� �ش� Ÿ�� ���ڸ� ��ȯ�մϴ�.
char classifyInstruction(uint32_t instruction) {
    char ret = '?';  // ��ȯ�� ��ɾ� Ÿ���� ������ ����
    uint32_t opcode = instruction >> 26; // ��ɾ�� ���� 6��Ʈ�� �����Ͽ� opcode�� ����ϴ�.
    int R_funct = instruction & 0x3F;    // R-type ��ɾ��� ���, ���� 6��Ʈ�� funct �ʵ�� ����մϴ�.

    if (opcode == 0) {
        int isNop = instruction & 0x3F;  // NOP ��ɾ�� funct �ʵ尡 0�̰� opcode�� 0�� Ư�� ����Դϴ�.
        if (isNop == 0) {
            return 'N';  // NOP ��ɾ��� ��� 'N'�� ��ȯ�մϴ�.
        }

        ret = 'R';  // opcode�� 0�̸� R-type ��ɾ��Դϴ�.
    }
    else if (opcode == 0x2 || opcode == 0x3) {
        ret = 'J';  // opcode�� 0x2 �Ǵ� 0x3�� ��� J-type ��ɾ��Դϴ�.
    }
    else {
        ret = 'I';  // �� ���� ���� I-type ��ɾ�� �з��մϴ�.
    }
    return ret;  // ������ ��ɾ� Ÿ���� ��ȯ�մϴ�.
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
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0) && (EX_MEM.writeReg == ID_EX.rs)) {
            ID_EX.readData1 = EX_MEM.ALUResult;
        }
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0) && (EX_MEM.writeReg == ID_EX.rt)) {
            ID_EX.readData2 = EX_MEM.ALUResult;
        }

        // MEM �ܰ迡�� ������
        if (MEM_WB.RegWrite && (MEM_WB.writeReg != 0) && (MEM_WB.writeReg == ID_EX.rs)) {
            ID_EX.readData1 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
        }
        if (MEM_WB.RegWrite && (MEM_WB.writeReg != 0) && (MEM_WB.writeReg == ID_EX.rt)) {
            ID_EX.readData2 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
        }
    }
}

// ���� ó�� �Լ� (������ ����)
int hazardDetection() {
    // Load-Use ������ ���� ����
    if (ID_EX.MemRead && ((ID_EX.rt == IF_ID.rs) || (ID_EX.rt == IF_ID.rt))) {
        // ���� ����: IF/ID ���������� �������͸� �����ϰ�, ID/EX ���������� �������͸� �ʱ�ȭ
        ID_EX.valid = 0;
        return 1;
    }
    return 0;
}

// ��ɾ� ���� �ܰ� ó�� �Լ�
int instructionFetch() {
    printf("32190192> Cycle: %d\n", ++cycle); // ����Ŭ �� ���� �� ���
    printf("\t[Instruction Fetch] 0x%s  (PC=0x%08x)\n", binaryToHexLower(data[pc / 4]), pc); // ���� PC�� ��ɾ 16������ ���

    IF_ID.pc = pc;
    IF_ID.instruction = binaryToUint32(data[pc / 4]);
    IF_ID.rs = (IF_ID.instruction >> 21) & 0x1F;
    IF_ID.rt = (IF_ID.instruction >> 16) & 0x1F;
    IF_ID.rd = (IF_ID.instruction >> 11) & 0x1F; // ������ �κ�
    IF_ID.immediate = (int16_t)(IF_ID.instruction & 0xFFFF); // ������ �κ�
    IF_ID.valid = 1;
    pc = pc + 4; // PC�� ���� ��ɾ�� �̵�
    return pc / 4; // ���� ��ɾ��� �ε��� ��ȯ
}

int instructionDecode() {
    if (!IF_ID.valid) return 1;

    ID_EX.pc = IF_ID.pc;
    ID_EX.instruction = IF_ID.instruction;
    ID_EX.valid = 1;

    uint32_t instruction = IF_ID.instruction;
    char type = classifyInstruction(instruction);
    if (type == 'N') {
        printf("\t[Instruction Decode] NOP!!!\n");
        return 1;
    }
    else if (type == '?') {  // �߸��� ��ɾ� Ÿ���� �ĺ�
        printf("\t[Instruction Decode] Unknown instruction type\n");
        return 1;
    }
    printf("\t[Instruction Decode] Type: %c, ", type);

    ID_EX.rs = (instruction >> 21) & 0x1F;
    ID_EX.rt = (instruction >> 16) & 0x1F;
    ID_EX.rd = (instruction >> 11) & 0x1F;
    ID_EX.immediate = (int16_t)(instruction & 0xFFFF);

    if (type == 'R') {
        parseRType(instruction);
    }
    else if (type == 'J') {
        parseJType(instruction);
    }
    else { // type == 'I'
        parseIType(instruction);
    }

    ID_EX.readData1 = Register[ID_EX.rs];
    ID_EX.readData2 = Register[ID_EX.rt];
    ID_EX.RegDst = RegDst;
    ID_EX.RegWrite = RegWrite;
    ID_EX.ALUSrc = ALUSrc;
    ID_EX.PCSrc = PCSrc;
    ID_EX.MemRead = MemRead;
    ID_EX.MemWrite = MemWrite;
    ID_EX.MemtoReg = MemtoReg;
    ID_EX.ALUOp = ALUOp;

    forwarding(); // ������ ó��

    return 0;
}

void execute() {
    if (!ID_EX.valid) return;

    EX_MEM.pc = ID_EX.pc;
    EX_MEM.instruction = ID_EX.instruction;
    EX_MEM.valid = 1;

    EX_MEM.MemRead = ID_EX.MemRead;
    EX_MEM.MemWrite = ID_EX.MemWrite;
    EX_MEM.MemtoReg = ID_EX.MemtoReg;
    EX_MEM.RegWrite = ID_EX.RegWrite;

    printf("\t[Execute]");
    int readData1 = ID_EX.readData1;
    int readData2 = ID_EX.readData2;

    if (ALUSrc == 1) {
        readData2 = ID_EX.immediate;
    }

    switch (ID_EX.ALUOp) {
    case 0: // jr(8), jal(3) -> and 
        printf(" Pass");
        break;
    case 2: // move(37) addu(33), addiu(9), sw(43), lw(35)  -> add
        EX_MEM.ALUResult = readData1 + readData2;
        printf(" ALU = %d", EX_MEM.ALUResult);
        break;
    case 3: // mult(24)
    {
        uint64_t product = (int64_t)readData1 * (int64_t)readData2;
        LO = (uint32_t)(product & 0xFFFFFFFF);
        HI = (uint32_t)(product >> 32);
        printf(" mult result: LO=%lu, HI=%lu", LO, HI);
    }
    break;
    case 4: // mflo(18)
        EX_MEM.ALUResult = LO;
        printf(" mflo result: %u", EX_MEM.ALUResult);
        break;
    case 6: // bnez(5), bne(5), beqz(4), b(4) -> sub
        if (opcode == 5) {
            EX_MEM.ALUResult = readData1 - readData2;
            printf(" ALU = %d", EX_MEM.ALUResult);
            if (EX_MEM.ALUResult == 0) { // 0�̸� �� ���� �����Ƿ� �б� x
                PCSrc = 0;
            }
            break;
        }
        else {
            EX_MEM.ALUResult = readData1 - readData2;
            printf(" ALU = %d", EX_MEM.ALUResult);
            if (EX_MEM.ALUResult != 0) { // 0�� �ƴϸ� �� ���� �ٸ��Ƿ� �б� x
                PCSrc = 0;
            }
            break;
        }

    case 7: // slti(35)
        EX_MEM.ALUResult = (readData1 < readData2) ? 1 : 0; // �� ����� ���� 1 �Ǵ� 0 ����
        printf(" ALU = %d", EX_MEM.ALUResult);
        break;
    default:
        printf(" Pass");
        break;
    }

    EX_MEM.writeData = ID_EX.readData2;
    if (ID_EX.RegDst == 1) {
        EX_MEM.writeReg = ID_EX.rd;
    }
    else {
        EX_MEM.writeReg = ID_EX.rt;
    }

    printf("\n");
}

// �޸� ���� �ܰ� ó�� �Լ�
void memoryAccess() {
    if (!EX_MEM.valid) return;

    MEM_WB.pc = EX_MEM.pc;
    MEM_WB.instruction = EX_MEM.instruction;
    MEM_WB.valid = 1;

    MEM_WB.ALUResult = EX_MEM.ALUResult;
    MEM_WB.writeReg = EX_MEM.writeReg;
    MEM_WB.RegWrite = EX_MEM.RegWrite;
    MEM_WB.MemtoReg = EX_MEM.MemtoReg;

    if (EX_MEM.ALUResult >= 0 && EX_MEM.ALUResult < sizeof(memory) / sizeof(memory[0])) {
        if (EX_MEM.MemRead == 1) { // �޸� �б� Ȱ��ȭ�� ���
            MEM_WB.readData = memory[EX_MEM.ALUResult]; // �޸𸮿��� rt �������ͷ� ������ �ε�
            printf("\t[Memory Access] Load, Address: 0x%08x, Value: %d\n", EX_MEM.ALUResult, MEM_WB.readData);
        }
        else if (EX_MEM.MemWrite == 1) { // �޸� ���� Ȱ��ȭ�� ���
            memory[EX_MEM.ALUResult] = EX_MEM.writeData; // rt �������Ϳ��� �޸𸮷� ������ ����
            printf("\t[Memory Access] Store, Address: 0x%08x, Value: %u\n", EX_MEM.ALUResult, memory[EX_MEM.ALUResult]);
        }
        else {
            printf("\t[Memory Access] Pass\n"); // �޸� �۾� ����
        }
    }
    else {
        printf("\t[Memory Access] Invalid memory access\n");
    }
}

// �������Ϳ� ��� ���� �ǵ����� ���� �ܰ踦 �����ϴ� �Լ�
void writeBack() {
    if (!MEM_WB.valid) return;

    printf("\t[Write Back]");

    // RegWrite ��ȣ�� Ȱ��ȭ�Ǿ� �ְ�, ����� �޸𸮿��� ���� ���� �� ����
    if (MEM_WB.RegWrite == 1 && MEM_WB.MemtoReg != 1) { // �������� ���Ⱑ ���ǰ� �޸𸮷κ����� ���� �ƴ� ���
        if (opcode == 3) { // opcode�� 3 (jal ��ɾ�)�� ��, RA �������� (31)�� PC �� ����
            Register[31] = pc;
            return; // �Լ� ����
        }
        if (opcode == 0 && ALUOp == 3) { // mult�� ��
            return;
        }
        Register[MEM_WB.writeReg] = MEM_WB.ALUResult; // writeReg �������Ϳ� ALU ��� ����
        printf(" Target: %s, Value: %d /", defineRegisterName(MEM_WB.writeReg), MEM_WB.ALUResult);
    }

    // MemtoReg ��ȣ�� Ȱ��ȭ�� ���, �޸𸮷κ��� ���� ���� �������Ϳ� ����
    if (MEM_WB.MemtoReg == 1) { // �޸𸮿��� ���� ����� �������Ϳ� ������ �ϴ� ���
        Register[MEM_WB.writeReg] = MEM_WB.readData; // �޸𸮿��� ���� ���� writeReg �������Ϳ� ����
        printf(" target: %s, Value: %d /", defineRegisterName(MEM_WB.writeReg), MEM_WB.readData);
    }
}

// ���α׷� ī����(PC)�� ������Ʈ�Ͽ� ������ ������ ó���ϴ� �Լ�
void possibleJump() {
    if (PCSrc == 1) { // PCSrc ��ȣ�� 1�̸� ���� ����
        // opcode�� ���� �ٸ� ���� ���� ����
        switch (opcode) {
        case 4: // beqz(4), b(4): ���Ǻ� �б� (zero ����) �Ǵ� �������� �б�
            pc = pc + 4 * immediate; // ���� ��ġ���� ��� �ּҸ�ŭ ����
            break;

        case 5: // bnez(5), bne(5): ���Ǻ� �б� (not zero ����)
            pc = pc + 4 * immediate; // ���� ��ġ���� ��� �ּҸ�ŭ ����
            break;

        case 0: // jr: �������Ϳ� ����� �ּҷ� ����
            pc = Register[31]; // ra ��������(31��)�� ����� �ּҷ� ����

            break;

        case 3: // jal: �����ϰ� ��ũ
            pc = 4 * J_address; // ���� �ּ� ��ġ�� ���� (�ּҴ� ��ɾ�� ����)
            printf("pc�� ra�� update / ");
            break;

        default: // �� �� ���� ���� ����
            break;
        }
        printf(" newPC: 0x%08x\n", pc); // ���ο� PC �� ���
    }
    else {
        printf(" newPC: 0x%08x\n", pc); // ���� ���� ��� ���� PC ���
    }
}

void init() {
    Register[29] = 16777216; // sp = 0x1000000�� ����
    Register[31] = 0xffffffff; // sp = 0xffffffff�� ����
}

// ���������� �������� ��� �Լ�
void printPipelineRegisters() {
    printf("\t[Pipeline Registers]\n");
    printf("\t\tIF/ID: PC = 0x%08x, Instruction = 0x%08x, Valid = %d\n", IF_ID.pc, IF_ID.instruction, IF_ID.valid);
    printf("\t\tID/EX: PC = 0x%08x, Instruction = 0x%08x, Valid = %d, ALUOp = %d, RegDst = %d, ALUSrc = %d, MemRead = %d, MemWrite = %d, MemtoReg = %d, RegWrite = %d\n",
        ID_EX.pc, ID_EX.instruction, ID_EX.valid, ID_EX.ALUOp, ID_EX.RegDst, ID_EX.ALUSrc, ID_EX.MemRead, ID_EX.MemWrite, ID_EX.MemtoReg, ID_EX.RegWrite);
    printf("\t\tEX/MEM: PC = 0x%08x, Instruction = 0x%08x, Valid = %d, ALUResult = %d, MemRead = %d, MemWrite = %d, MemtoReg = %d, RegWrite = %d\n",
        EX_MEM.pc, EX_MEM.instruction, EX_MEM.valid, EX_MEM.ALUResult, EX_MEM.MemRead, EX_MEM.MemWrite, EX_MEM.MemtoReg, EX_MEM.RegWrite);
    printf("\t\tMEM/WB: PC = 0x%08x, Instruction = 0x%08x, Valid = %d, ALUResult = %d, ReadData = %d, MemtoReg = %d, RegWrite = %d\n",
        MEM_WB.pc, MEM_WB.instruction, MEM_WB.valid, MEM_WB.ALUResult, MEM_WB.readData, MEM_WB.MemtoReg, MEM_WB.RegWrite);
}

// �ùķ��̼��� �����ϴ� �� �Լ�
void run() {
    init(); // �ʱ�ȭ �Լ� ȣ��� �������� �� �޸� �ʱ�ȭ
    int loopCounter = 0;  // ���� ���� ������ ���� ī����

    while (1) { // ���� ������ ��ɾ� ����
        loopCounter++;
        if (loopCounter > 1000000) {  // 1000,000 ����Ŭ�� �Ѿ�� ���� ����
            printf("Infinite loop detected, stopping the simulation\n");
            break;
        }

        printf("\n");
        printf("\n");

        // 1. Instruction Fetch: ��ɾ� ���� �ܰ�
        int next_row = instructionFetch(); // ���� PC�� ��ɾ ����
        if (next_row > data_row) break; // ������ ���� �ʰ��ϸ� ���� ����

        // ���� ���� �� ó��
        if (hazardDetection()) {
            printf("\t[Hazard Detection] Bubble Inserted\n");
            EX_MEM.valid = 0;
            ID_EX.valid = 0;
            continue;
        }

        // 2. Instruction Decode: ��ɾ� ���ڵ� �ܰ�
        if (instructionDecode()) { // ��ɾ� ���ڵ� ����, NOP ���� ó�� Ȯ��
            continue; // NOP�̸� ������ ���� �ݺ����� �Ѿ
        }

        // 3. Execute: ���� �ܰ�
        execute(); // ALU ���� �� �б� ���� ����

        // 4. Memory Access: �޸� ���� �ܰ�
        memoryAccess(); // �޸� �б� �Ǵ� ���� �۾� ����

        // 5. Write Back: ��� ���� �ܰ�
        writeBack(); // ���� ����� �������Ϳ� ����

        // 6. Possible Jump: ���� ó��
        possibleJump(); // ���ǿ� ���� PC ������Ʈ

        // ���α׷� ���� ���� üũ
        if (pc == 0xffffffff) { break; } // PC�� Ư�� ��(���� ����)�� �����ϸ� ���� ����

        printf("\n");
        printf("\n");

        // ���������� �������� ������Ʈ
        MEM_WB.pc = EX_MEM.pc;
        MEM_WB.instruction = EX_MEM.instruction;
        MEM_WB.valid = EX_MEM.valid;
        MEM_WB.ALUResult = EX_MEM.ALUResult;
        MEM_WB.writeReg = EX_MEM.writeReg;
        MEM_WB.RegWrite = EX_MEM.RegWrite;
        MEM_WB.MemtoReg = EX_MEM.MemtoReg;
        MEM_WB.readData = EX_MEM.writeData;

        EX_MEM.pc = ID_EX.pc;
        EX_MEM.instruction = ID_EX.instruction;
        EX_MEM.valid = ID_EX.valid;
        EX_MEM.MemRead = ID_EX.MemRead;
        EX_MEM.MemWrite = ID_EX.MemWrite;
        EX_MEM.MemtoReg = ID_EX.MemtoReg;
        EX_MEM.RegWrite = ID_EX.RegWrite;
        EX_MEM.ALUResult = ID_EX.ALUResult;
        EX_MEM.writeData = ID_EX.readData2;
        EX_MEM.writeReg = (ID_EX.RegDst == 1) ? ID_EX.rd : ID_EX.rt;

        ID_EX.pc = IF_ID.pc;
        ID_EX.instruction = IF_ID.instruction;
        ID_EX.valid = IF_ID.valid;
        ID_EX.rs = IF_ID.rs;
        ID_EX.rt = IF_ID.rt;
        ID_EX.rd = IF_ID.rd;
        ID_EX.readData1 = Register[IF_ID.rs];
        ID_EX.readData2 = Register[IF_ID.rt];
        ID_EX.immediate = IF_ID.immediate;
        ID_EX.RegDst = RegDst;
        ID_EX.RegWrite = RegWrite;
        ID_EX.ALUSrc = ALUSrc;
        ID_EX.PCSrc = PCSrc;
        ID_EX.MemRead = MemRead;
        ID_EX.MemWrite = MemWrite;
        ID_EX.MemtoReg = MemtoReg;
        ID_EX.ALUOp = ALUOp;

        IF_ID.valid = 0; // IF_ID�� �ʱ�ȭ�Ͽ� ���� ��ɾ �޵��� ����

        // ���������� �������� ���� ���
        printPipelineRegisters();
    }
}

// ���� �� ��� �Լ�
void answer() {
    printf("32190192> Final Result\n");
    printf("\tCycles: %d, R-type instructions: %d, I-type instructions: %d, J-type instructions: %d\n", cycle, R_type_cnt, I_type_cnt, J_type_cnt);
    printf("\tReturn value (v0) : %d\n", Register[2]);
}

// ���� �Լ�: ���α׷��� ������
int main(int argc, char* argv[]) {
    // ����� �μ� Ȯ��
    if (argc != 2) { // ����ڰ� ���� �̸��� �Է����� �ʾ��� ���
        printf("Usage: %s <filename>\n", argv[0]); // ���� �ȳ�
        return -1; // ������ ����
    }

    // ���� ���� �õ�
    FILE* fp = fopen(argv[1], "rb"); // �Էµ� ���ϸ����� ������ ���̳ʸ� �б� ���� ����
    if (fp == NULL) { // ���� ���� ����
        perror("Error opening file"); // ���� �޽��� ���
        exit(0); // ���α׷� ����
    }

    // ������ �Ľ�
    parseData(fp); // ���Ͽ��� �����͸� �о� �Ľ�

    // �ùķ��̼� ����
    run(); // ��ɾ� ����

    // ��� ���
    answer(); // ���� ��� ���

    // ���� �ݱ�
    fclose(fp); // ���� ���� �����͸� ����

    return 0; // ���� ����
}
