#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 각 명령어에 대한 상수 정의
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

// 파일의 최댓값 정의
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
int row_index; // 현재 행 인덱스
int row_cnt[MAX_ROW_LENGTH];

// 파싱해주는 함수
void parse(char* argv[], FILE* fp) {
    char line[MAX_COL_LENGTH]; // 파일에서 읽어온 한 줄을 저장할 배열
    char* token = NULL; // strtok 함수로 파싱된 토큰을 저장할 포인터

    // 파일에서 한 줄씩 읽어와서 파싱합니다.
    while (fgets(line, sizeof(line), fp) != NULL) {
        row_cnt[row_index] = 0; // 각 행의 열 카운트를 초기화합니다.
        // 각 줄을 파싱하여 토큰을 출력합니다.
        token = strtok(line, " \n"); // 스페이스바와 개행 문자로 토큰을 분리합니다.
        while (token != NULL) {
            arr[row_index][row_cnt[row_index]++] = strdup(token); // strdup를 사용하여 토큰을 복사하여 저장합니다.
            token = strtok(NULL, " \n");
        }
        row_index++;
        if (row_index > MAX_ROW_LENGTH) {
            printf("exception! 파일의 세로길이는 %d을 넘을 수 없습니다.\n", MAX_ROW_LENGTH);
            exit(0);
        }
    }
}

// 16진수를 10진수로 바꿔주는 함수
int hex_string_to_int(char* hex_string) {
    // 문자열이 "0x"로 시작하는지 확인하고, 시작한다면 인덱스를 조정합니다.
    int index = 0;
    if (hex_string[0] == '0' && hex_string[1] == 'x') {
        index = 2;
    }
    else {
        // 16진수 예외 처리
        printf("exception! %s는 16진수가 아닙니다.\n", hex_string);
        exit(0);
    }
    // 16진수로 표현된 문자열을 10진수로 변환합니다.
    return (int)strtol(hex_string + index, NULL, 16);
}

// base pointer index 위치를 잡아주는 함수
int base_pointer(int i, int j) {
    int base_pointer = 0;
    if (arr[i][j][0] == 'r') base_pointer += V0_BASE_POINTER;
    if (arr[i][j][0] == 's') base_pointer += S0_BASE_POINTER;
    if (arr[i][j][0] == 't') base_pointer += T0_BASE_POINTER;
    if (arr[i][j][0] == 'z') base_pointer += ZERO_BASE_POINTER;
    return base_pointer;
}

// offset index 위치를 잡아주는 함수
int move_offset(int i, int j) {
    return arr[i][j][1] - '0';
}

// LW instruction 처리 함수
void LW(int i) {
    // 길이 예외 처리
    if (LW_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! LW_OPERAND_SIZE는 %d입니다.\n", LW_OPERAND_SIZE);
        exit(0);
    }

    // 레지스터 검증
    if (base_pointer(i, 1) != S0_BASE_POINTER) {
        printf("exception! LW의 레지스터는 s만 사용 가능합니다.\n");
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);

    Register[dst_offset] = hex_string_to_int(arr[i][2]);
    printf("32190192> Loaded %d to %s\n", Register[dst_offset], arr[i][1]);
}

// ADD instruction 처리 함수
void ADD(int i) {
    // 길이 예외 처리
    if (ADD_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! ADD_OPERAND_SIZE는 %d입니다.\n", ADD_OPERAND_SIZE);
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] + Register[src2_offset];


    printf("32190192> Added %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// SUB instruction 처리 함수
void SUB(int i) {
    // 길이 예외 처리
    if (SUB_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! SUB_OPERAND_SIZE는 %d입니다.\n", SUB_OPERAND_SIZE);
        exit(0);
    }
    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] - Register[src2_offset];

    printf("32190192> Subtracted %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// MUL instruction 처리 함수
void MUL(int i) {
    // 길이 예외 처리
    if (MUL_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! MUL_OPERAND_SIZE는 %d입니다.\n", MUL_OPERAND_SIZE);
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] * Register[src2_offset];

    printf("32190192> Multiplied %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// DIV instruction 처리 함수
void DIV(int i) {
    // 길이 예외 처리
    if (DIV_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! DIV_OPERAND_SIZE는 %d입니다.\n", DIV_OPERAND_SIZE);
        exit(0);
    }

    if (base_pointer(i, 3) == ZERO_BASE_POINTER) {
        printf("exception! 0으로 나눌수 없습니다.\n");
        exit(0);
    }

    int dst_offset = base_pointer(i, 1) + move_offset(i, 1);
    int src1_offset = base_pointer(i, 2) + move_offset(i, 2);
    int src2_offset = base_pointer(i, 3) + move_offset(i, 3);

    Register[dst_offset] = Register[src1_offset] / Register[src2_offset];

    printf("32190192> Divided %s(%d) to %s(%d) and changed %s to %d\n"
        , arr[i][2], Register[src1_offset], arr[i][3], Register[src2_offset], arr[i][1], Register[dst_offset]);
}

// JMP instruction 처리 함수
int JMP(int i, int j, int check) {
    int line = hex_string_to_int(arr[i][j]);
    if (check) {
        printf("32190192> jumped to line %d\n", line);
    }
    return line - 2;
}

// BEQ instruction 처리 함수
int BEQ(int i) {
    // 길이 예외 처리
    if (BEQ_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! BEQ_OPERAND_SIZE는 %d입니다.\n", BEQ_OPERAND_SIZE);
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

// BNE instruction 처리 함수
int BNE(int i) {
    // 길이 예외 처리
    if (BNE_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! BNE_OPERAND_SIZE는 %d입니다.\n", BNE_OPERAND_SIZE);
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

// SLT instruction 처리 함수
void SLT(int i) {
    // 길이 예외 처리
    if (SLT_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! SLT_OPERAND_SIZE는 %d입니다.\n", SLT_OPERAND_SIZE);
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

// NOP instruction 처리 함수
void NOP(int i) {
    // 길이 예외 처리
    if (NOP_OPERAND_SIZE != row_cnt[i] - 1) {
        printf("exception! NOP_OPERAND_SIZE는 %d입니다.\n", NOP_OPERAND_SIZE);
        exit(0);
    }
    printf("32190192> No operation\n");
}

// 동적으로 할당된 메모리를 해제해주는 함수 
void memory_free() {
    for (int i = 0; i < row_index; i++) {
        for (int j = 0; j < row_cnt[i]; j++) {
            free(arr[i][j]);
        }
    }
}

int main(int argc, char* argv[]) {
    // 명령행에서 파일 이름을 인자로 전달하지 않았을 경우 오류 메시지를 출력하고 종료합니다.
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // 파일 포인터를 선언하고 명령행에서 전달된 파일을 읽기 모드로 엽니다.
    FILE* fp = fopen(argv[1], "r");

    // 파일이 제대로 열렸는지 확인합니다.
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    // 2차원 string 배열로 파싱
    parse(argv, fp);

    // 파일을 닫습니다.
    fclose(fp);

    // 명령 수행
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
            // 유효하지 않은 명령어 처리
            printf("Invalid instruction\n");
        }
    }

    // v0 register 값 출력
    printf("32190192> Print out %d\n", Register[V0_BASE_POINTER]);

    // 동적으로 할당된 메모리를 해제
    memory_free();

    return 0;
}
