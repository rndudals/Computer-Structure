#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int cycle = 0;
int data_row;
char data[100][33]; // 이진 문자열을 저장하는 배열
int pc = 0;
char* cur_instruction; // 현재 instruction
int Register[32];



uint32_t opcode;
uint32_t rs;
uint32_t rt;
uint32_t rd;
uint32_t shamt;
uint32_t funct;

uint32_t J_address;

int16_t immediate;

int RegDst;     // RegDst 신호에 대한 변수
int RegWrite;   // RegWrite 신호에 대한 변수
int ALUSrc;     // ALUSrc 신호에 대한 변수
int PCSrc;      // PCSrc 신호에 대한 변수
int MemRead;    // MemRead 신호에 대한 변수
int MemWrite;   // MemWrite 신호에 대한 변수
int MemtoReg;   // MemtoReg 신호에 대한 변수
int ALUOp;      // ALUOp 신호에 대한 변수

int ALUResult;  // ALU 연산 결과





char* defineRegisterName(int n) {
    switch (n) {
    case 0: return "zero";
    case 1: return "at";
    case 2: return "v0";
    case 3: return "v1";
    case 4: return "a0";
    case 5: return "a1";
    case 6: return "a2";
    case 7: return "a3";
    case 8: return "t0";
    case 9: return "t1";
    case 10: return "t2";
    case 11: return "t3";
    case 12: return "t4";
    case 13: return "t5";
    case 14: return "t6";
    case 15: return "t7";
    case 16: return "s0";
    case 17: return "s1";
    case 18: return "s2";
    case 19: return "s3";
    case 20: return "s4";
    case 21: return "s5";
    case 22: return "s6";
    case 23: return "s7";
    case 24: return "t8";
    case 25: return "t9";
    case 26: return "k0";
    case 27: return "k1";
    case 28: return "gp";
    case 29: return "sp";
    case 30: return "s8";
    case 31: return "ra";
    default: return "unknown"; // 알 수 없는 레지스터 번호
    }
}

// 이진수 문자열을 16진수 소문자 문자열로 변환하여 반환하는 함수
char* binaryToHexLower(const char* binaryString) {
    // 변환된 16진수 값을 저장할 고정 길이 문자열 할당
    static char hexString[9]; // 8자리 16진수 + 널 종료 문자
    unsigned long number = strtol(binaryString, NULL, 2);

    // 16진수 소문자 문자열 생성
    sprintf(hexString, "%08lx", number); // 소문자로 출력하기 위해 %lx 사용

    return hexString;
}

// 이진수 문자열을 uint32_t로 변환하는 함수
uint32_t binaryToUint32(const char* binaryString) {
    // 이진수 문자열을 uint32_t로 변환
    return (uint32_t)strtol(binaryString, NULL, 2);
}

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}


void parseRType(uint32_t instruction) {
    opcode = (instruction >> 26) & 0x3F; // opcode 추출
    rs = (instruction >> 21) & 0x1F;     // rs 추출
    rt = (instruction >> 16) & 0x1F;     // rt 추출
    rd = (instruction >> 11) & 0x1F;     // rd 추출
    shamt = (instruction >> 6) & 0x1F;   // shamt 추출
    funct = instruction & 0x3F;          // funct 추출


    switch (funct) {
    case 37: // move
    case 33: // addu
        printf("Inst: %s %s %s %s\n", "addu", defineRegisterName(rd), defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rd: %d (%x), rs: %d (%x), rt: %d (%x)\n", opcode, rd, rd, rs, rs, rt, rt);
        RegDst = 1; RegWrite = 1; ALUSrc = 0; PCSrc = 0; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 2;
        break;
    case 24: // mult
        printf("Inst: %s %s %s\n", "mult", defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rs: %d (%x), rt: %d (%x)\n", opcode, rs, rs, rt, rt);
        RegDst = 0; RegWrite = 1; ALUSrc = 0; PCSrc = 0; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 3;
        break;
    case 18: // mflo
        printf("Inst: %s %s\n", "mflo", defineRegisterName(rd));
        printf("\t\topcode: %d, rd: %d (%x)\n", opcode, rd, rd);
        RegDst = 1; RegWrite = 1; ALUSrc = 0; PCSrc = 0; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 4;
        break;
    case 8: // jr
        printf("Inst: %s %s\n", "jr", defineRegisterName(rs));
        printf("\t\topcode: %d, rs: %d (%x)\n", opcode, rs, rs);
        RegDst = 0; RegWrite = 0; ALUSrc = 0; PCSrc = 1; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 0;
        break;
    default:
        RegDst = RegWrite = ALUSrc = PCSrc = MemRead = MemWrite = MemtoReg = ALUOp = 0;
        printf("\t\topcode: %d, Unknown funct %02d\n", opcode, funct);
        break;
    }

    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}


void parseJType(uint32_t instruction) {
    opcode = (instruction >> 26) & 0x3F; // opcode 추출
    J_address = instruction & 0x03FFFFFF; // 주소 추출
    printf("Inst: jal: 0x%08x\n", J_address);
    printf("\t\topcode: %d, address: %08x\n", opcode, J_address);

    int RegDst = 0; int RegWrite = 1; int ALUSrc = 0; int PCSrc = 1; int MemRead = 0; int MemWrite = 0; int MemtoReg = 0; int ALUOp = 0;

    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}


void parseIType(uint32_t instruction) {
    opcode = (instruction >> 26) & 0x3F; // opcode 추출 (26-31비트, 총 6비트)
    rs = (instruction >> 21) & 0x1F;     // rs 추출 (21-25비트, 총 5비트)
    rt = (instruction >> 16) & 0x1F;     // rt 추출 (16-20비트, 총 5비트)
    immediate = (int16_t)(instruction & 0xFFFF); // Immediate 추출 (0-15비트, 총 16비트, 부호 확장 고려)
    switch (opcode) {
    case 9:
        printf("Inst: %s %s %s %d\n", "addiu", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 1; ALUSrc = 1; PCSrc = 0; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 2;
        break;
    case 43: // sw
        printf("Inst: %s %s %d(%s)\n", "sw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 0; ALUSrc = 1; PCSrc = 0; MemRead = 0; MemWrite = 1; MemtoReg = 0; ALUOp = 2;
        break;
    case 35: // lw
        printf("Inst: %s %s %d(%s)\n", "lw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 1; ALUSrc = 1; PCSrc = 0; MemRead = 1; MemWrite = 0; MemtoReg = 1; ALUOp = 2;
        break;
    case 10: // slti
        printf("Inst: %s %s %s %d\n", "slti", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 1; ALUSrc = 1; PCSrc = 0; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 7; // SLTI specific operation code
        break;
    case 5: // bnez or bne
        if (rt == 0) {
            printf("Inst: %s %s %d\n", "bnez", defineRegisterName(rs), immediate);
        }
        else {
            printf("Inst: %s %s %s %d\n", "bne", defineRegisterName(rs), defineRegisterName(rt), immediate);
        }
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 0; ALUSrc = 0; PCSrc = 1; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 6; // Branch specific operation
        break;
    case 4: // b or beqz
        if (rt == 0) {
            printf("Inst: %s %d\n", "b", immediate);
        }
        else {
            printf("Inst: %s %s %d\n", "beqz", defineRegisterName(rs), immediate);
        }
        printf("\t\topcode: %d, rt: %d (%x), rs: %d (%x), imm: %d\n", opcode, rt, rt, rs, rs, immediate);
        RegDst = 0; RegWrite = 0; ALUSrc = 0; PCSrc = 1; MemRead = 0; MemWrite = 0; MemtoReg = 0; ALUOp = 6; // Branch specific operation
        break;
    default:
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// 명령어 타입을 판별하고 출력하는 함수
char classifyInstruction(uint32_t instruction) {
    char ret;
    uint32_t opcode = instruction >> 26; // 상위 6비트 추출
    int R_funct = instruction & 0x3F;          // funct 추출
    if (opcode == 0) {

        int isNop = instruction & 0x3F;          // NOP 예외처리
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

int instructionFetch() {
    printf("32190192> Cycle: %d\n", ++cycle);
    printf("\t[Instruction Fetch] 0x%s  (PC=0x%08x)\n", binaryToHexLower(data[pc / 4]), pc);

    // 현재 instruction 저장 
    cur_instruction = data[pc / 4];
    pc = pc + 4;
    return pc / 4;
}

int instructionDecode() {

    char type = classifyInstruction(binaryToUint32(cur_instruction)); // 명령어 타입 분류
    if (type == 'N') {
        printf("\t[Instruction Decode] NOP!!!");
        return 1;
    }
    else {
        printf("\t[Instruction Decode] Type: %c, ", type);
    }


    if (type == 'R') {
        parseRType(binaryToUint32(cur_instruction));
    }
    else if (type == 'J') {
        parseJType(binaryToUint32(cur_instruction));
    }
    else { // type == 'I'
        parseIType(binaryToUint32(cur_instruction));
    }

    return 0;
}

void execute() {
    int readData1 = Register[rs];
    int readDaat2 = Register[rt];

    printf("\t[Execute]\n");
    if (ALUSrc == 1) {

    }

    switch (ALUOp) {
    case 0: // jr(8), jal(3) -> and 

        break;
    case 2: // addu(33), addiu(9), sw(43), lw(35)  -> add
        switch (opcode)
        {
        case 33: // addu(33)
            ALUResult = Register[rs] + Register[rt];

            break;
        case 9: // addiu(9)
            ALUResult = Register[rs] + immediate;

            break;
        case 43: // sw(43)
            ALUResult = Register[rs] + immediate;

            break;
        case 35: // lw(35)
            ALUResult = Register[rs] + immediate;

            break;
        default:
            break;
        }
        break;
    case 3: // mult(24)

        break;
    case 4: // mflo(18)

        break;
    case 6: // bnez(5), bne(5), beqz(4), b(4) -> sub

        break;
    case 7: // slti(35)


        break;

    default: break;
    }
}

void memoryAccess() {
    if (MemRead == 1) { // load일 때만 1
        printf("\t[Memory Access] Load, Address: , Value: \n");
    }
    else if (MemWrite == 1) { // store일 때만 1
        printf("\t[Memory Access] Store, Address: 0x%08x, Value: %d\n", ALUResult, ALUResult);
    }
    else {
        printf("\t[Memory Access] Pass\n");
    }
}

void writeBack() {
    printf("\t[Write Back]");
    if (MemtoReg == 1) { // load일 때만 1
        printf(" target: %d, Value: 0x%08x", Register[rt], Register[rt]);
    }

    if (RegWrite == 1) { //addu mult mflo lw addiu slti : 1
        printf(" TODO");
        switch (opcode)
        {
        case 9:
            Register[rt] = ALUResult;
            printf(" Target: %s, Value: %d / ", defineRegisterName(rt), ALUResult);
            break;

        default:
            break;
        }
    }
}

void possibleJump() {
    if (PCSrc == 1) {
        // PC
        printf(" newPC: 0x%08x\n", pc);
    }
    else {
        printf(" newPC: 0x%08x\n", pc);
    }
}

void init() {
    Register[29] = 16777216; // sp = 0x1000000로 시작
}


void run() {
    init();
    while (1) {
        printf("\n");
        printf("\n");
        // 1. Instruction Fetch
        int next_row = instructionFetch();
        if (next_row > data_row) break;

        // 2. Instruction Decode
        if (instructionDecode()) { continue; }

        // 3. Excute
        execute();

        // 4. Memory Access
        memoryAccess();

        // 5. Write Back  
        writeBack();

        possibleJump();
        printf("[cur_instruction] : %s", cur_instruction);
        printf("\n");
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    parseData(fp);

    run();

    fclose(fp);
    return 0;
}
