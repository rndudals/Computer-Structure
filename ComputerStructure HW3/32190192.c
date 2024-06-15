#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ENABLE_FORWARD 1
#define ENABLE_BRANCH_PREDICTION 1
#define ALWAYS_TAKEN 1

int cycle = 0; // 프로그램 실행 중 사이클 수를 추적
int data_row; // MIPS 명령어 데이터 배열의 행 수
char data[100][33]; // 이진 문자열로 표현된 MIPS 명령어를 저장하는 배열
int pc = 0; // 프로그램 카운터, 현재 실행 중인 명령어의 위치를 가리킴
char* cur_instruction; // 현재 실행 중인 명령어의 이진 문자열

uint32_t Register[32]; // MIPS 아키텍처의 32개 레지스터

uint32_t opcode; // 명령어의 연산 코드 필드
uint32_t rs; // 명령어의 첫 번째 소스 레지스터
uint32_t rt; // 명령어의 두 번째 소스 레지스터 또는 대상 레지스터
uint32_t rd; // 명령어의 대상 레지스터 (R-type 명령어)
uint32_t shamt; // 명령어의 쉬프트 양 (shift amount)
uint32_t funct; // R-type 명령어의 기능 코드

uint32_t J_address; // J-type 명령어의 점프 주소
uint32_t memory[16777217]; // MIPS 시뮬레이터의 가상 메모리 공간
int16_t immediate; // I-type 명령어의 즉시 값

int RegDst; // 목적지 레지스터 결정 신호 (0: rt, 1: rd)
int RegWrite; // 레지스터 쓰기 활성화 신호
int ALUSrc; // ALU 소스 결정 신호 (0: 레지스터, 1: 즉시 값)
int PCSrc; // PC 소스 결정 신호 (0: 기본, 1: 분기/점프)
int MemRead; // 메모리 읽기 활성화 신호
int MemWrite; // 메모리 쓰기 활성화 신호
int MemtoReg; // 메모리-레지스터 결정 신호 (0: ALU 결과, 1: 메모리 읽기 결과)
int ALUOp; // ALU 연산 결정 신호 (연산 유형 코드)

int ALUResult; // ALU 연산 결과

int readData1; // 첫 번째 레지스터에서 읽은 데이터
int readData2; // 두 번째 레지스터에서 읽은 데이터

int R_type_cnt; // 실행된 R-type 명령어의 개수
int J_type_cnt; // 실행된 J-type 명령어의 개수
int I_type_cnt; // 실행된 I-type 명령어의 개수

// 곱셈 연산의 고정밀 결과를 저장하기 위한 레지스터
uint64_t HI = 0; // 곱셈 결과의 상위 32비트 저장
uint64_t LO = 0; // 곱셈 결과의 하위 32비트 저장

// 파이프라인 레지스터 구조체 정의
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

// 파이프라인 레지스터 초기화
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
    case 28: return "gp"; case 29: return "sp"; case 30: return "s8"; case 31: return "ra"; default: return "unknown"; // 알 수 없는 레지스터 번호
    }
}

// 이진수 문자열을 16진수 소문자 문자열로 변환하여 반환하는 함수
char* binaryToHexLower(const char* binaryString) {
    static char hexString[9]; // 8자리 16진수 + 널 종료 문자
    unsigned long number = strtol(binaryString, NULL, 2);
    sprintf(hexString, "%08lx", number); // 소문자로 출력하기 위해 %lx 사용
    return hexString;
}

// 이진수 문자열을 uint32_t로 변환하는 함수
uint32_t binaryToUint32(const char* binaryString) {
    return (uint32_t)strtol(binaryString, NULL, 2);
}

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

// 제어 신호를 설정하는 함수
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

// RType 명령어를 파싱하고 실행 관련 제어 신호를 설정하는 함수입니다.
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

// JType 명령어를 파싱하는 함수입니다.
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

// IType 명령어를 파싱하는 함수입니다.
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

// 명령어 타입을 판별하고 반환하는 함수입니다.
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

// 포워딩 로직을 구현하는 함수
void forwarding() {
    if (ENABLE_FORWARD) {
        // EX 단계에서 포워딩
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0)) {
            if (EX_MEM.writeReg == ID_EX.rs) {
                ID_EX.readData1 = EX_MEM.ALUResult;
            }
            if (EX_MEM.writeReg == ID_EX.rt) {
                ID_EX.readData2 = EX_MEM.ALUResult;
            }
        }

        // MEM 단계에서 포워딩
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

// 버블 처리 함수 (데이터 위험)
int hazardDetection() {
    // Load-Use 데이터 위험 감지
    if (ID_EX.MemRead && ((ID_EX.rt == IF_ID.rs) || (ID_EX.rt == IF_ID.rt))) {
        //
