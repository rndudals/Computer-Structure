#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

// 명령어 타입을 판별하고 출력하는 함수
void classifyInstruction(uint32_t instruction) {
    uint32_t opcode = instruction >> 26; // 상위 6비트 추출
    printf("\t[Instruction Decode] ");
    if (opcode == 0) {
        printf("Type: R, ");
    }
    else if (opcode == 0x2 || opcode == 0x3) {
        printf("Type: J, ");
    }
    else {
        printf("Type: I, ");
    }
}

int main(int argc, char* argv[]) {
    printf("Hello World\n");

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    unsigned char buffer[4];
    size_t bytesRead;
    int cycle = 1;
    int pc = 0;
    char data[100][33]; // 이진 문자열을 저장하는 배열
    int row = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        uint32_t instruction = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

        for (int i = 0; i < 32; i++) {
            data[row][i] = (instruction & (1 << (31 - i))) ? '1' : '0';
        }
        data[row][32] = '\0';

        printf("32190192> Cycle: %d\n", cycle++);
        printf("\t[Instruction Fetch] %08x  (PC=0x%08x)\n", instruction, pc);
        printf("\tdata[%d]: %s\n", row, data[row]);
        classifyInstruction(instruction); // 명령어 타입 분류 및 출력
        row++;
        pc += 4;
        printf("\n");
    }

    fclose(fp);
    return 0;
}
