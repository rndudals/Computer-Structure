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
    // ����࿡�� ���� �̸��� ���ڷ� �������� �ʾ��� ��� ���� �޽����� ����ϰ� �����մϴ�.
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // ���� �����͸� �����ϰ� ����࿡�� ���޵� ������ �б� ���� ���ϴ�.
    FILE* fp = fopen(argv[1], "rb");

    // ������ ����� ���ȴ��� Ȯ���մϴ�.
    if (fp == NULL) {
        perror("Error opening file");
        exit(0);
    }

    // ������ �а� �� ����Ʈ�� 16���� �������� ����մϴ�.
    unsigned char buffer[4];  // 4 ����Ʈ�� �б� ���� ����
    size_t bytesRead;
    int count = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            printf("%02X", buffer[i]);
        }
        count++;

        // 2���� 4����Ʈ ����� �а� ���� �� �ٲ��� �߰��մϴ�.
        if (count % 1 == 0) {
            printf("\n");
        }
    }


    // ������ �ݽ��ϴ�.
    fclose(fp);




    return 0;
}
