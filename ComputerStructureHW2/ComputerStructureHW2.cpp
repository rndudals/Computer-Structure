#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int cycle = 0;
int data_row;
char data[100][33]; // ���� ���ڿ��� �����ϴ� �迭
int pc = 0;

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

// ��ɾ� Ÿ���� �Ǻ��ϰ� ����ϴ� �Լ�
void classifyInstruction(uint32_t instruction) {
    uint32_t opcode = instruction >> 26; // ���� 6��Ʈ ����
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
    printf("\tdata[%d]: %s\n", pc / 4, data[pc / 4]);
    classifyInstruction(binaryToUint32(data[pc / 4])); // ��ɾ� Ÿ�� �з� �� ���
    pc = pc + 4;
    return pc / 4;
}

void run() {

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
    while (1) {

        int next_row = instructionFetch();
        if (next_row > data_row) break;


        printf("\n");
        printf("\n");
    }

    run();
    fclose(fp);
    return 0;
}
