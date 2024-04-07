#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// �� ��ɾ ���� ��� ����
#define ADD_OPERAND_SIZE 3
#define SUB_OPERAND_SIZE 3
#define MUL_OPERAND_SIZE 3
#define DIV_OPERAND_SIZE 3
#define LW_OPERAND_SIZE 2
#define NOP_OPERAND_SIZE 0
#define JMP_OPERAND_SIZE 1
#define BEQ_OPERAND_SIZE 3
#define BNE_OPERAND_SIZE 3
#define SLT_OPERAND_SIZE 3

// ������ �ִ� ����
#define MAX_COL_LENGTH 100
#define MAX_ROW_LENGTH 1000

// Register
// t0 ~ t9 = Register[0] ~ Register[9] 
// s0 ~ s7 = Register[10] ~ Register[17]
// v0 = Register[18]
// zero = Register[19] 
#define T0_BASE_POINTER 0
#define S0_BASE_POINTER 10
#define V0_BASE_POINTER 18
#define ZERO_BASE_POINTER 19
int Register[20];


char* arr[MAX_ROW_LENGTH][MAX_COL_LENGTH];
int row_index; // ���� �� �ε���
int row_cnt[MAX_ROW_LENGTH];

// �Ľ����ִ� �Լ�
void parse(char* argv[], FILE* fp) {
    char line[MAX_COL_LENGTH]; // ���Ͽ��� �о�� �� ���� ������ �迭
    char* token = NULL; // strtok �Լ��� �Ľ̵� ��ū�� ������ ������

    // ���Ͽ��� �� �پ� �о�ͼ� �Ľ��մϴ�.
    while (fgets(line, sizeof(line), fp) != NULL) {
        row_cnt[row_index] = 0; // �� ���� �� ī��Ʈ�� �ʱ�ȭ�մϴ�.
        // �� ���� �Ľ��Ͽ� ��ū�� ����մϴ�.
        token = strtok(line, " \n"); // �����̽��ٿ� ���� ���ڷ� ��ū�� �и��մϴ�.
        while (token != NULL) {
            arr[row_index][row_cnt[row_index]++] = strdup(token); // strdup�� ����Ͽ� ��ū�� �����Ͽ� �����մϴ�.
            token = strtok(NULL, " \n");
        }
        row_index++;
        if (row_index > MAX_ROW_LENGTH) {
            printf("exception! ������ ���α��̴� %d�� ���� �� �����ϴ�.\n", MAX_ROW_LENGTH);
            exit(0);
        }
    }
}

// 16������ 10������ �ٲ��ִ� �Լ�
int hex_string_to_int(char* hex_string) {
    // ���ڿ��� "0x"�� �����ϴ��� Ȯ���ϰ�, �����Ѵٸ� �ε����� �����մϴ�.
    int index = 0;
    if (hex_string[0] == '0' && hex_string[1] == 'x') {
        index = 2;
    }
    else {
        // 16���� ���� ó��
        printf("exception! %s�� 16������ �ƴմϴ�.\n", hex_string);
        exit(0);
    }
    // 16������ ǥ���� ���ڿ��� 10������ ��ȯ�մϴ�.
    return (int)strtol(hex_string + index, NULL, 16);
}

// base pointer index ��ġ�� ����ִ� �Լ�
int base_pointer(int i, int j) {
    int base_pointer = 0;
    if (arr[i][j][0] == 'r') base_pointer += V0_BASE_POINTER;
    if (arr[i][j][0] == 's') base_pointer += S0_BASE_POINTER;
    if (arr[i][j][0] == 't') base_pointer += T0_BASE_POINTER;
    if (arr[i][j][0] == 'z') base_pointer += ZERO_BASE_POINTER;
    return base_pointer;
}

// offset index ��ġ�� ����ִ� �Լ�
int move_offset(int i, int j) {
    return arr[i][j][1] - '0';
}

// LW instruction ó�� �Լ�
void LW(int i) {
    // ���� ���� ó��
    if (LW_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! LW_OPERAND_SIZE�� %d�Դϴ�.\n", LW_OPERAND_SIZE);
        exit(0);
    }

    // �������� ����
    if (base_pointer(i, 1) != S0_BASE_POINTER) {
        printf("exception! LW�� �������ʹ� s�� ��� �����մϴ�.\n");
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);

    Register[dst_offset] = hex_string_to_int(arr[i][2]);
    printf("32190192> Loaded %d to %s\n", Register[dst_offset], arr[i][1]);
}

// ADD instruction ó�� �Լ�
void ADD(int i) {
    // ���� ���� ó��
    if (ADD_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! ADD_OPERAND_SIZE�� %d�Դϴ�.\n", ADD_OPERAND_SIZE);
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] + Register[src2_offset];


    printf("32190192> Added %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// SUB instruction ó�� �Լ�
void SUB(int i) {
    // ���� ���� ó��
    if (SUB_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! SUB_OPERAND_SIZE�� %d�Դϴ�.\n", SUB_OPERAND_SIZE);
        exit(0);
    }
    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] - Register[src2_offset];

    printf("32190192> Subtracted %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// MUL instruction ó�� �Լ�
void MUL(int i) {
    // ���� ���� ó��
    if (MUL_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! MUL_OPERAND_SIZE�� %d�Դϴ�.\n", MUL_OPERAND_SIZE);
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] * Register[src2_offset];

    printf("32190192> Multiplied %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// DIV instruction ó�� �Լ�
void DIV(int i) {
    // ���� ���� ó��
    if (DIV_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! DIV_OPERAND_SIZE�� %d�Դϴ�.\n", DIV_OPERAND_SIZE);
        exit(0);
    }

    if (base_pointer(i, 3) == ZERO_BASE_POINTER) {
        printf("exception! 0���� ������ �����ϴ�.\n");
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] / Register[src2_offset];

    printf("32190192> Divided %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// JMP instruction ó�� �Լ�
int JMP(int i, int j, int check) {
    int line = hex_string_to_int(arr[i][j]);
    if (check) {
        printf("32190192> jumped to line %d\n", line);
    }
    return line - 2;
}

// BEQ instruction ó�� �Լ�
int BEQ(int i) {
    // ���� ���� ó��
    if (BEQ_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! BEQ_OPERAND_SIZE�� %d�Դϴ�.\n", BEQ_OPERAND_SIZE);
        exit(0);
    }
    int src1_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src2_offset = base_pointer(i, 2) + move_offset(i, 2);

    int line = hex_string_to_int(arr[i][3]);

    if (Register[src1_offset] == Register[src2_offset]) {
        printf("32190192> Checked if %s(%d) is equal to %s(%d) and jumped to line %d\n",
            arr[i][1], Register[src1_offset], arr[i][2], Register[src2_offset], line);
        return 1;
    }
    else {
        printf("32190192> Checked if %s(%d) is not equal to %s(%d) and didn't jumped to line %d\n",
            arr[i][1], Register[src1_offset], arr[i][2], Register[src2_offset], line);
        return 0;
    }
}

// BNE instruction ó�� �Լ�
int BNE(int i) {
    // ���� ���� ó��
    if (BNE_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! BNE_OPERAND_SIZE�� %d�Դϴ�.\n", BNE_OPERAND_SIZE);
        exit(0);
    }
    int src1_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src2_offset = base_pointer(i, 2) + move_offset(i, 2);

    int line = hex_string_to_int(arr[i][3]);

    if (Register[src1_offset] != Register[src2_offset]) {
        printf("32190192> Checked if %s(%d) is not equal to %s(%d) and jumped to line %d\n",
            arr[i][1], Register[src1_offset], arr[i][2], Register[src2_offset], line);
        return 1;
    }
    else {
        printf("32190192> Checked if %s(%d) is equal to %s(%d) and didn't jumped to line %d\n",
            arr[i][1], Register[src1_offset], arr[i][2], Register[src2_offset], line);
        return 0;
    }
}

// SLT instruction ó�� �Լ�
void SLT(int i) {
    // ���� ���� ó��
    if (SLT_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! SLT_OPERAND_SIZE�� %d�Դϴ�.\n", SLT_OPERAND_SIZE);
        exit(0);
    }
    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    if (Register[src1_offset] < Register[src2_offset]) {
        Register[dst_offset] = 1;
        printf("32190192> Checked if %s(%d) is less than %s(%d) and set 1 to %s\n",
            arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1]);

    }
    else {
        Register[dst_offset] = 0;
        printf("32190192> Checked if %s(%d) is not less than %s(%d) and set 0 to %s\n",
            arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1]);
    }
}

// NOP instruction ó�� �Լ�
void NOP(int i) {
    // ���� ���� ó��
    if (NOP_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! NOP_OPERAND_SIZE�� %d�Դϴ�.\n", NOP_OPERAND_SIZE);
        exit(0);
    }
    printf("32190192> No operation\n");
}

// �������� �Ҵ�� �޸𸮸� �������ִ� �Լ� 
void memory_free() {
    for (int i = 0; i < row_index; i++) {
        for (int j = 0; j < row_cnt[i]; j++) {
            free(arr[i][j]);
        }
    }
}

int main(int argc, char* argv[]) {
    // ����࿡�� ���� �̸��� ���ڷ� �������� �ʾ��� ��� ���� �޽����� ����ϰ� �����մϴ�.
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // ���� �����͸� �����ϰ� ����࿡�� ���޵� ������ �б� ���� ���ϴ�.
    FILE* fp = fopen(argv[1], "r");

    // ������ ����� ���ȴ��� Ȯ���մϴ�.
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    // 2���� string �迭�� �Ľ�
    parse(argv, fp);

    // ������ �ݽ��ϴ�.
    fclose(fp);

    // ��� ����
    for (int i = 0; i < row_index; i++) {
        printf("(line %d)", i + 1);

        for (int j = 0; j < row_cnt[i]; j++) {
            printf("  %s", arr[i][j]);
        }
        printf("\n");

        if (strcmp(arr[i][0], "ADD") == 0) {
            ADD(i);
        }
        else if (strcmp(arr[i][0], "SUB") == 0) {
            SUB(i);
        }
        else if (strcmp(arr[i][0], "MUL") == 0) {
            MUL(i);
        }
        else if (strcmp(arr[i][0], "DIV") == 0) {
            DIV(i);
        }
        else if (strcmp(arr[i][0], "LW") == 0) {
            LW(i);
        }
        else if (strcmp(arr[i][0], "NOP") == 0) {
            NOP(i);
        }
        else if (strcmp(arr[i][0], "JMP") == 0) {
            i = JMP(i, 1, 1);
        }
        else if (strcmp(arr[i][0], "BEQ") == 0) {

            int check = BEQ(i);
            if (check) i = JMP(i, 3, 0);

        }
        else if (strcmp(arr[i][0], "BNE") == 0) {

            int check = BNE(i);
            if (check) i = JMP(i, 3, 0);

        }
        else if (strcmp(arr[i][0], "SLT") == 0) {
            SLT(i);
        }
        else {
            // ��ȿ���� ���� ��ɾ� ó��
            printf("Invalid instruction\n");
        }
    }

    // v0 register �� ���
    printf("32190192> Print out %d\n", Register[V0_BASE_POINTER]);

    // �������� �Ҵ�� �޸𸮸� ����
    memory_free();

    return 0;
}
