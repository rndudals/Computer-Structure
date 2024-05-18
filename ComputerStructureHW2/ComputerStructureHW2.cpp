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


uint32_t R_opcode;
uint32_t R_rs;
uint32_t R_rt;
uint32_t R_rd;
uint32_t R_shamt;
uint32_t R_funct;

uint32_t J_opcode;
uint32_t J_address;

uint32_t I_opcode;
uint32_t I_rs;
uint32_t I_rt;
int16_t I_immediate;



char* defineInstruction(int n) {


}



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
    R_opcode = (instruction >> 26) & 0x3F; // opcode 추출 (26-31비트, 총 6비트)
    R_rs = (instruction >> 21) & 0x1F;     // rs 추출 (21-25비트, 총 5비트)
    R_rt = (instruction >> 16) & 0x1F;     // rt 추출 (16-20비트, 총 5비트)
    R_rd = (instruction >> 11) & 0x1F;     // rd 추출 (11-15비트, 총 5비트)
    R_shamt = (instruction >> 6) & 0x1F;   // shamt 추출 (6-10비트, 총 5비트)
    R_funct = instruction & 0x3F;          // funct 추출 (0-5비트, 총 6비트)
    switch (R_funct) {
    case 37:
    case 33:
        printf("%s %s %s %s\n", "addu", defineRegisterName(R_rd), defineRegisterName(R_rs), defineRegisterName(R_rt)); break;
    case 24:
        printf("%s %s %s\n", "mult", defineRegisterName(R_rs), defineRegisterName(R_rt)); break;
    case 18:
        printf("%s %s\n", "mflo", defineRegisterName(R_rd)); break;
    case 8:
        printf("%s %s\n", "jr", defineRegisterName(R_rs)); break;
    default: break;
    }

    printf("\nOpcode: %02d\n", R_opcode);
    printf("RS: %02d\n", R_rs);
    printf("RT: %02d\n", R_rt);
    printf("RD: %02d\n", R_rd);
    printf("Shamt: %02d\n", R_shamt);
    printf("Funct: %02d\n", R_funct);
}

void parseJType(uint32_t instruction) {
    J_opcode = (instruction >> 26) & 0x3F; // opcode 추출 (26-31비트, 총 6비트)
    J_address = instruction & 0x03FFFFFF; // 주소 추출 (0-25비트, 총 26비트)
    printf("%s %08x\n", "jal : 뭔가 이상하다 ", J_address);
    printf("\nOpcode: %02X\n", J_opcode);
    printf("Address: %08X\n", J_address);
}

void parseIType(uint32_t instruction) {
    I_opcode = (instruction >> 26) & 0x3F; // opcode 추출 (26-31비트, 총 6비트)
    I_rs = (instruction >> 21) & 0x1F;     // rs 추출 (21-25비트, 총 5비트)
    I_rt = (instruction >> 16) & 0x1F;     // rt 추출 (16-20비트, 총 5비트)
    I_immediate = (int16_t)(instruction & 0xFFFF); // Immediate 추출 (0-15비트, 총 16비트, 부호 확장 고려)
    switch (I_opcode) {
    case 9:
        printf("%s %s %s %d\n", "addiu", defineRegisterName(I_rt), defineRegisterName(I_rs), I_immediate); break;
    case 43:
        printf("%s %s %d(%s)\n", "sw", defineRegisterName(I_rt), I_immediate, defineRegisterName(I_rs)); break;
    case 35:
        printf("%s %s %d(%s)\n", "lw", defineRegisterName(I_rt), I_immediate, defineRegisterName(I_rs)); break;
    case 10:
        printf("%s %s %s %d\n", "slti", defineRegisterName(I_rt), defineRegisterName(I_rs), I_immediate); break;
    case 5:
        if (I_rt == 0) {
            printf("%s %s %d\n", "bnez", defineRegisterName(I_rs), I_immediate);
        }
        else {
            printf("%s %s %s %d\n", "bne", defineRegisterName(I_rs), defineRegisterName(I_rt), I_immediate);
        }
        break;
    case 4:
        if (I_rt == 0) {
            printf("%s %d\n", "b", I_immediate);
        }
        else {
            printf("%s %s %d\n", "beqz", defineRegisterName(I_rs), I_immediate);
        }
        break;
    default: break;
    }
    printf("\nOpcode: %02X\n", I_opcode);
    printf("RS: %02d\n", I_rs);
    printf("RT: %02d\n", I_rt);
    printf("Immediate: %04X (Signed: %d)\n", (uint16_t)I_immediate, I_immediate);
}

// 명령어 타입을 판별하고 출력하는 함수
char classifyInstruction(uint32_t instruction) {
    char ret;
    uint32_t opcode = instruction >> 26; // 상위 6비트 추출

    if (opcode == 0) {
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

void instructionDecode() {

    char type = classifyInstruction(binaryToUint32(cur_instruction)); // 명령어 타입 분류
    printf("\t[Instruction Decode] Type: %c, ", type);





    if (type == 'R') {
        parseRType(binaryToUint32(cur_instruction));
    }
    else if (type == 'J') {
        parseJType(binaryToUint32(cur_instruction));
    }
    else { // type == 'I'
        parseIType(binaryToUint32(cur_instruction));
    }
    printf("[cur_instruction] : %s", cur_instruction);
}

void run() {
    while (1) {

        // 1. Instruction Fetch
        int next_row = instructionFetch();
        if (next_row > data_row) break;

        // 2. Instruction Decode
        instructionDecode();


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
