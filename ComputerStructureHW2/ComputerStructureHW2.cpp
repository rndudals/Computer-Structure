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

char* defineInstruction(int n) {


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

// 명령어 타입을 판별하고 출력하는 함수
char classifyInstruction(uint32_t instruction) {
    char ret;
    uint32_t opcode = instruction >> 26; // 상위 6비트 추출
    printf("\t[Instruction Decode] ");
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

void parseRType(uint32_t instruction) {
    uint32_t opcode = (instruction >> 26) & 0x3F; // opcode 추출 (26-31비트, 총 6비트)
    uint32_t rs = (instruction >> 21) & 0x1F;     // rs 추출 (21-25비트, 총 5비트)
    uint32_t rt = (instruction >> 16) & 0x1F;     // rt 추출 (16-20비트, 총 5비트)
    uint32_t rd = (instruction >> 11) & 0x1F;     // rd 추출 (11-15비트, 총 5비트)
    uint32_t shamt = (instruction >> 6) & 0x1F;   // shamt 추출 (6-10비트, 총 5비트)
    uint32_t funct = instruction & 0x3F;          // funct 추출 (0-5비트, 총 6비트)

    printf("\nOpcode: %02d\n", opcode);
    printf("RS: %02d\n", rs);
    printf("RT: %02d\n", rt);
    printf("RD: %02d\n", rd);
    printf("Shamt: %02d\n", shamt);
    printf("Funct: %02d\n", funct);
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
    printf("Type: %c, ", type);

    printf("%s", cur_instruction);


    if (type = 'R') {
        parseRType(binaryToUint32(cur_instruction));
    }
    else if (type = 'J') {

    }
    else { // type = 'I'

    }
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
