#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int cycle = 0;
int data_row;
char data[100][33]; // ���� ���ڿ��� �����ϴ� �迭
int pc = 0;
char* cur_instruction; // ���� instruction
int Register[32];

char* defineInstruction(int n) {


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

// ��ɾ� Ÿ���� �Ǻ��ϰ� ����ϴ� �Լ�
char classifyInstruction(uint32_t instruction) {
    char ret;
    uint32_t opcode = instruction >> 26; // ���� 6��Ʈ ����
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
    uint32_t opcode = (instruction >> 26) & 0x3F; // opcode ���� (26-31��Ʈ, �� 6��Ʈ)
    uint32_t rs = (instruction >> 21) & 0x1F;     // rs ���� (21-25��Ʈ, �� 5��Ʈ)
    uint32_t rt = (instruction >> 16) & 0x1F;     // rt ���� (16-20��Ʈ, �� 5��Ʈ)
    uint32_t rd = (instruction >> 11) & 0x1F;     // rd ���� (11-15��Ʈ, �� 5��Ʈ)
    uint32_t shamt = (instruction >> 6) & 0x1F;   // shamt ���� (6-10��Ʈ, �� 5��Ʈ)
    uint32_t funct = instruction & 0x3F;          // funct ���� (0-5��Ʈ, �� 6��Ʈ)

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

    // ���� instruction ���� 
    cur_instruction = data[pc / 4];
    pc = pc + 4;
    return pc / 4;
}

void instructionDecode() {

    char type = classifyInstruction(binaryToUint32(cur_instruction)); // ��ɾ� Ÿ�� �з�
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
