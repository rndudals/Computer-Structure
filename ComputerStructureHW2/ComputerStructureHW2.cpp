#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printBinary(unsigned int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

int main(int argc, char* argv[]) {
    printf("Hello World\n");
    // 명령행에서 파일 이름을 인자로 전달하지 않았을 경우 오류 메시지를 출력하고 종료합니다.
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // 파일 포인터를 선언하고 명령행에서 전달된 파일을 읽기 모드로 엽니다.
    FILE* fp = fopen(argv[1], "rb");

    // 파일이 제대로 열렸는지 확인합니다.
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    // 파일을 읽고 각 바이트를 16진수 형식으로 출력합니다.
    unsigned char buffer[4];  // 4 바이트씩 읽기 위한 버퍼
    size_t bytesRead;
    int count = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            printf("%02X", buffer[i]);
        }
        count++;

        // 2개의 4바이트 블록을 읽고 나면 줄 바꿈을 추가합니다.
        if (count % 1 == 0) {
            printf("\n");
        }
    }


    // 파일을 닫습니다.
    fclose(fp);




    return 0;
}
