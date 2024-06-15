#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ENABLE_FORWARD 1
#define ENABLE_BRANCH_PREDICTION 1

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
    case 7: return "a3";   case 8: return "t0"; case 9: return "t1"; case 10: return "t2"; case 11: return "t3"; case 12: return "t4"; case 13: return "t5";
    case 14: return "t6";  case 15: return "t7"; case 16: return "s0"; case 17: return "s1"; case 18: return "s2"; case 19: return "s3"; case 20: return "s4";
    case 21: return "s5";  case 22: return "s6"; case 23: return "s7";  case 24: return "t8"; case 25: return "t9"; case 26: return "k0"; case 27: return "k1";
    case 28: return "gp";  case 29: return "sp"; case 30: return "s8";  case 31: return "ra"; default: return "unknown"; // 알 수 없는 레지스터 번호
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

// 제어 신호를 설정하는 함수
// 이 함수는 파싱된 명령어에 따라 CPU의 제어 신호를 설정합니다.
void setControlSignals(int regDst, int regWrite, int aluSrc, int pcSrc, int memRead, int memWrite, int memToReg, int aluOp) {
    RegDst = regDst;         // 레지스터 대상 결정
    RegWrite = regWrite;     // 레지스터 쓰기 활성화
    ALUSrc = aluSrc;         // ALU 입력 선택
    PCSrc = pcSrc;           // 프로그램 카운터 소스 선택
    MemRead = memRead;       // 메모리 읽기 활성화
    MemWrite = memWrite;     // 메모리 쓰기 활성화
    MemtoReg = memToReg;     // 메모리-레지스터 선택
    ALUOp = aluOp;           // ALU 연산 유형 선택
}

// RType 명령어를 파싱하고 실행 관련 제어 신호를 설정하는 함수입니다.
void parseRType(uint32_t instruction) {
    R_type_cnt++;  // 파싱된 R-type 명령어의 수를 증가시킵니다.

    // 명령어에서 각 필드를 추출합니다.
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    rd = (instruction >> 11) & 0x1F;
    shamt = (instruction >> 6) & 0x1F;
    funct = instruction & 0x3F;

    // funct 필드에 따라 다른 연산을 수행합니다.
    switch (funct) {
    case 37: // move
    case 33: // addu
        // 해당 명령어 정보를 출력합니다.
        printf("Inst: %s %s %s %s\n", "addu", defineRegisterName(rd), defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rd: %d (%d), rs: %d (%d), rt: %d (%d)\n", opcode, rd, Register[rd], rs, Register[rs], rt, Register[rt]);
        // 제어 신호를 설정합니다.
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 2);
        break;
    case 24: // mult
        // mult 명령어 정보를 출력합니다.
        printf("Inst: %s %s %s\n", "mult", defineRegisterName(rs), defineRegisterName(rt));
        printf("\t\topcode: %d, rs: %d (%d), rt: %d (%d)\n", opcode, rs, Register[rs], rt, Register[rt]);
        setControlSignals(0, 1, 0, 0, 0, 0, 0, 3);
        break;
    case 18: // mflo
        // mflo 명령어 정보를 출력합니다.
        printf("Inst: %s %s\n", "mflo", defineRegisterName(rd));
        printf("\t\topcode: %d, rd: %d (%d)\n", opcode, rd, Register[rd]);
        setControlSignals(1, 1, 0, 0, 0, 0, 0, 4);
        break;
    case 8: // jr
        // jr 명령어 정보를 출력합니다.
        printf("Inst: %s %s\n", "jr", defineRegisterName(rs));
        printf("\t\topcode: %d, rs: %d (%d)\n", opcode, rs, Register[rs]);
        setControlSignals(0, 0, 0, 1, 0, 0, 0, 0);
        break;
    default:
        // 알 수 없는 함수 코드 처리
        printf("\t\topcode: %d, Unknown funct %02d\n", opcode, funct);
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// JType 명령어를 파싱하는 함수입니다.
void parseJType(uint32_t instruction) {
    J_type_cnt++;  // 파싱된 J-type 명령어의 수를 증가시킵니다.

    // 명령어에서 opcode와 점프 주소를 추출합니다.
    opcode = (instruction >> 26) & 0x3F;
    J_address = instruction & 0x03FFFFFF;
    // 명령어 정보를 출력합니다.
    printf("Inst: jal: %d\n", J_address);
    printf("\t\topcode: %d, address: %d\n", opcode, J_address);

    // 제어 신호를 설정합니다.
    setControlSignals(0, 1, 0, 1, 0, 0, 0, 0);
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// IType 명령어를 파싱하는 함수입니다.
void parseIType(uint32_t instruction) {
    I_type_cnt++;  // 파싱된 I-type 명령어의 수를 증가시킵니다.

    // 명령어에서 opcode, rs, rt, immediate를 추출합니다.
    opcode = (instruction >> 26) & 0x3F;
    rs = (instruction >> 21) & 0x1F;
    rt = (instruction >> 16) & 0x1F;
    immediate = (int16_t)(instruction & 0xFFFF);

    // opcode에 따라 다른 연산을 수행합니다.
    switch (opcode) {
    case 9: // addiu
        // addiu 명령어 정보를 출력합니다.
        printf("Inst: %s %s %s %d\n", "addiu", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 2);
        break;
    case 43: // sw
        // sw 명령어 정보를 출력합니다.
        printf("Inst: %s %s %d(%s)\n", "sw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 0, 1, 0, 0, 1, 0, 2);
        break;
    case 35: // lw
        // lw 명령어 정보를 출력합니다.
        printf("Inst: %s %s %d(%s)\n", "lw", defineRegisterName(rt), immediate, defineRegisterName(rs));
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 1, 0, 1, 2);
        break;
    case 10: // slti
        // slti 명령어 정보를 출력합니다.
        printf("Inst: %s %s %s %d\n", "slti", defineRegisterName(rt), defineRegisterName(rs), immediate);
        printf("\t\topcode: %d, rt: %d (%d), rs: %d (%d), imm: %d\n", opcode, rt, Register[rt], rs, Register[rs], immediate);
        setControlSignals(0, 1, 1, 0, 0, 0, 0, 7);
        break;
    case 5: // bnez or bne
        // bne 명령어 정보를 출력합니다.
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
        // b beqz 명령어 정보를 출력합니다.
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
        // 알 수 없는 opcode 처리
        setControlSignals(0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    printf("\t\tRegDst: %d, RegWrite: %d, ALUSrc: %d, PCSrc: %d, MemRead: %d, MemWrite: %d, MemtoReg: %d, ALUOp: %d\n",
        RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, ALUOp);
}

// 명령어 타입을 판별하고 반환하는 함수입니다.
// 이 함수는 명령어의 opcode를 분석하여 R, I, J 타입 중 하나를 결정하고 해당 타입 문자를 반환합니다.
char classifyInstruction(uint32_t instruction) {
    char ret = '?';  // 반환할 명령어 타입을 저장할 변수
    uint32_t opcode = instruction >> 26; // 명령어에서 상위 6비트를 추출하여 opcode를 얻습니다.
    int R_funct = instruction & 0x3F;    // R-type 명령어의 경우, 하위 6비트를 funct 필드로 사용합니다.

    if (opcode == 0) {
        int isNop = instruction & 0x3F;  // NOP 명령어는 funct 필드가 0이고 opcode도 0인 특수 경우입니다.
        if (isNop == 0) {
            return 'N';  // NOP 명령어일 경우 'N'을 반환합니다.
        }

        ret = 'R';  // opcode가 0이면 R-type 명령어입니다.
    }
    else if (opcode == 0x2 || opcode == 0x3) {
        ret = 'J';  // opcode가 0x2 또는 0x3인 경우 J-type 명령어입니다.
    }
    else {
        ret = 'I';  // 그 외의 경우는 I-type 명령어로 분류합니다.
    }
    return ret;  // 결정된 명령어 타입을 반환합니다.
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
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0) && (EX_MEM.writeReg == ID_EX.rs)) {
            ID_EX.readData1 = EX_MEM.ALUResult;
        }
        if (EX_MEM.RegWrite && (EX_MEM.writeReg != 0) && (EX_MEM.writeReg == ID_EX.rt)) {
            ID_EX.readData2 = EX_MEM.ALUResult;
        }

        // MEM 단계에서 포워딩
        if (MEM_WB.RegWrite && (MEM_WB.writeReg != 0) && (MEM_WB.writeReg == ID_EX.rs)) {
            ID_EX.readData1 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
        }
        if (MEM_WB.RegWrite && (MEM_WB.writeReg != 0) && (MEM_WB.writeReg == ID_EX.rt)) {
            ID_EX.readData2 = MEM_WB.MemtoReg ? MEM_WB.readData : MEM_WB.ALUResult;
        }
    }
}

// 버블 처리 함수 (데이터 위험)
int hazardDetection() {
    // Load-Use 데이터 위험 감지
    if (ID_EX.MemRead && ((ID_EX.rt == IF_ID.rs) || (ID_EX.rt == IF_ID.rt))) {
        // 버블 삽입: IF/ID 파이프라인 레지스터를 정지하고, ID/EX 파이프라인 레지스터를 초기화
        ID_EX.valid = 0;
        return 1;
    }
    return 0;
}

// 명령어 인출 단계 처리 함수
int instructionFetch() {
    printf("32190192> Cycle: %d\n", ++cycle); // 사이클 수 증가 및 출력
    printf("\t[Instruction Fetch] 0x%s  (PC=0x%08x)\n", binaryToHexLower(data[pc / 4]), pc); // 현재 PC의 명령어를 16진수로 출력

    IF_ID.pc = pc;
    IF_ID.instruction = binaryToUint32(data[pc / 4]);
    IF_ID.rs = (IF_ID.instruction >> 21) & 0x1F;
    IF_ID.rt = (IF_ID.instruction >> 16) & 0x1F;
    IF_ID.rd = (IF_ID.instruction >> 11) & 0x1F; // 수정된 부분
    IF_ID.immediate = (int16_t)(IF_ID.instruction & 0xFFFF); // 수정된 부분
    IF_ID.valid = 1;
    pc = pc + 4; // PC를 다음 명령어로 이동
    return pc / 4; // 다음 명령어의 인덱스 반환
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
    else if (type == '?') {  // 잘못된 명령어 타입을 식별
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

    forwarding(); // 포워딩 처리

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
            if (EX_MEM.ALUResult == 0) { // 0이면 두 값이 같으므로 분기 x
                PCSrc = 0;
            }
            break;
        }
        else {
            EX_MEM.ALUResult = readData1 - readData2;
            printf(" ALU = %d", EX_MEM.ALUResult);
            if (EX_MEM.ALUResult != 0) { // 0이 아니면 두 값이 다르므로 분기 x
                PCSrc = 0;
            }
            break;
        }

    case 7: // slti(35)
        EX_MEM.ALUResult = (readData1 < readData2) ? 1 : 0; // 비교 결과에 따라 1 또는 0 설정
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

// 메모리 접근 단계 처리 함수
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
        if (EX_MEM.MemRead == 1) { // 메모리 읽기 활성화인 경우
            MEM_WB.readData = memory[EX_MEM.ALUResult]; // 메모리에서 rt 레지스터로 데이터 로드
            printf("\t[Memory Access] Load, Address: 0x%08x, Value: %d\n", EX_MEM.ALUResult, MEM_WB.readData);
        }
        else if (EX_MEM.MemWrite == 1) { // 메모리 쓰기 활성화인 경우
            memory[EX_MEM.ALUResult] = EX_MEM.writeData; // rt 레지스터에서 메모리로 데이터 저장
            printf("\t[Memory Access] Store, Address: 0x%08x, Value: %u\n", EX_MEM.ALUResult, memory[EX_MEM.ALUResult]);
        }
        else {
            printf("\t[Memory Access] Pass\n"); // 메모리 작업 없음
        }
    }
    else {
        printf("\t[Memory Access] Invalid memory access\n");
    }
}

// 레지스터에 결과 값을 되돌리는 쓰기 단계를 수행하는 함수
void writeBack() {
    if (!MEM_WB.valid) return;

    printf("\t[Write Back]");

    // RegWrite 신호가 활성화되어 있고, 결과가 메모리에서 오지 않을 때 실행
    if (MEM_WB.RegWrite == 1 && MEM_WB.MemtoReg != 1) { // 레지스터 쓰기가 허용되고 메모리로부터의 값이 아닌 경우
        if (opcode == 3) { // opcode가 3 (jal 명령어)일 때, RA 레지스터 (31)에 PC 값 저장
            Register[31] = pc;
            return; // 함수 종료
        }
        if (opcode == 0 && ALUOp == 3) { // mult일 때
            return;
        }
        Register[MEM_WB.writeReg] = MEM_WB.ALUResult; // writeReg 레지스터에 ALU 결과 저장
        printf(" Target: %s, Value: %d /", defineRegisterName(MEM_WB.writeReg), MEM_WB.ALUResult);
    }

    // MemtoReg 신호가 활성화된 경우, 메모리로부터 읽은 값을 레지스터에 쓰기
    if (MEM_WB.MemtoReg == 1) { // 메모리에서 읽은 결과가 레지스터에 쓰여야 하는 경우
        Register[MEM_WB.writeReg] = MEM_WB.readData; // 메모리에서 읽은 값을 writeReg 레지스터에 저장
        printf(" target: %s, Value: %d /", defineRegisterName(MEM_WB.writeReg), MEM_WB.readData);
    }
}

// 프로그램 카운터(PC)를 업데이트하여 가능한 점프를 처리하는 함수
void possibleJump() {
    if (PCSrc == 1) { // PCSrc 신호가 1이면 점프 실행
        // opcode에 따라 다른 점프 동작 수행
        switch (opcode) {
        case 4: // beqz(4), b(4): 조건부 분기 (zero 조건) 또는 무조건적 분기
            pc = pc + 4 * immediate; // 현재 위치에서 상대 주소만큼 점프
            break;

        case 5: // bnez(5), bne(5): 조건부 분기 (not zero 조건)
            pc = pc + 4 * immediate; // 현재 위치에서 상대 주소만큼 점프
            break;

        case 0: // jr: 레지스터에 저장된 주소로 점프
            pc = Register[31]; // ra 레지스터(31번)에 저장된 주소로 점프

            break;

        case 3: // jal: 점프하고 링크
            pc = 4 * J_address; // 절대 주소 위치로 점프 (주소는 명령어에서 추출)
            printf("pc를 ra로 update / ");
            break;

        default: // 그 외 경우는 점프 없음
            break;
        }
        printf(" newPC: 0x%08x\n", pc); // 새로운 PC 값 출력
    }
    else {
        printf(" newPC: 0x%08x\n", pc); // 변경 없는 경우 현재 PC 출력
    }
}

void init() {
    Register[29] = 16777216; // sp = 0x1000000로 시작
    Register[31] = 0xffffffff; // sp = 0xffffffff로 시작
}

// 파이프라인 레지스터 출력 함수
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

// 시뮬레이션을 실행하는 주 함수
void run() {
    init(); // 초기화 함수 호출로 레지스터 및 메모리 초기화
    int loopCounter = 0;  // 무한 루프 감지를 위한 카운터

    while (1) { // 무한 루프로 명령어 실행
        loopCounter++;
        if (loopCounter > 1000000) {  // 1000,000 사이클이 넘어가면 루프 감지
            printf("Infinite loop detected, stopping the simulation\n");
            break;
        }

        printf("\n");
        printf("\n");

        // 1. Instruction Fetch: 명령어 인출 단계
        int next_row = instructionFetch(); // 현재 PC의 명령어를 인출
        if (next_row > data_row) break; // 데이터 행을 초과하면 루프 종료

        // 버블 감지 및 처리
        if (hazardDetection()) {
            printf("\t[Hazard Detection] Bubble Inserted\n");
            EX_MEM.valid = 0;
            ID_EX.valid = 0;
            continue;
        }

        // 2. Instruction Decode: 명령어 디코드 단계
        if (instructionDecode()) { // 명령어 디코드 실행, NOP 등의 처리 확인
            continue; // NOP이면 루프의 다음 반복으로 넘어감
        }

        // 3. Execute: 실행 단계
        execute(); // ALU 연산 및 분기 결정 수행

        // 4. Memory Access: 메모리 접근 단계
        memoryAccess(); // 메모리 읽기 또는 쓰기 작업 수행

        // 5. Write Back: 결과 쓰기 단계
        writeBack(); // 연산 결과를 레지스터에 저장

        // 6. Possible Jump: 점프 처리
        possibleJump(); // 조건에 따라 PC 업데이트

        // 프로그램 종료 조건 체크
        if (pc == 0xffffffff) { break; } // PC가 특정 값(종료 조건)에 도달하면 루프 종료

        printf("\n");
        printf("\n");

        // 파이프라인 레지스터 업데이트
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

        IF_ID.valid = 0; // IF_ID를 초기화하여 다음 명령어를 받도록 설정

        // 파이프라인 레지스터 상태 출력
        printPipelineRegisters();
    }
}

// 최종 답 출력 함수
void answer() {
    printf("32190192> Final Result\n");
    printf("\tCycles: %d, R-type instructions: %d, I-type instructions: %d, J-type instructions: %d\n", cycle, R_type_cnt, I_type_cnt, J_type_cnt);
    printf("\tReturn value (v0) : %d\n", Register[2]);
}

// 메인 함수: 프로그램의 진입점
int main(int argc, char* argv[]) {
    // 명령행 인수 확인
    if (argc != 2) { // 사용자가 파일 이름을 입력하지 않았을 경우
        printf("Usage: %s <filename>\n", argv[0]); // 사용법 안내
        return -1; // 비정상 종료
    }

    // 파일 열기 시도
    FILE* fp = fopen(argv[1], "rb"); // 입력된 파일명으로 파일을 바이너리 읽기 모드로 열기
    if (fp == NULL) { // 파일 열기 실패
        perror("Error opening file"); // 에러 메시지 출력
        exit(0); // 프로그램 종료
    }

    // 데이터 파싱
    parseData(fp); // 파일에서 데이터를 읽어 파싱

    // 시뮬레이션 실행
    run(); // 명령어 실행

    // 결과 출력
    answer(); // 최종 결과 출력

    // 파일 닫기
    fclose(fp); // 열린 파일 포인터를 닫음

    return 0; // 정상 종료
}
