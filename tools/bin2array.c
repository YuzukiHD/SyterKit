#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_LINE 16

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

    FILE *inputFile, *outputFile;
    char *inputFileName = argv[1]; // 输入的二进制文件名
    char *outputFileName = argv[2];// 输出的C数组文件名
    int fileSize;

    // 打开输入的二进制文件
    inputFile = fopen(inputFileName, "rb");
    if (inputFile == NULL) {
        printf("无法打开输入文件\n");
        return 1;
    }

    // 提取输出文件的名称作为C数组的名称
    char *arrayName = strrchr(outputFileName, '/');
    if (arrayName == NULL) {
        arrayName = strtok(outputFileName, ".");
    } else {
        arrayName++;// 跳过文件名分隔符
    }

    // 获取文件大小
    fseek(inputFile, 0, SEEK_END);
    fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    // 创建输出的C数组文件
    outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        printf("无法创建输出文件\n");
        fclose(inputFile);
        return 1;
    }

    // 写入C数组声明到输出文件
    fprintf(outputFile, "const unsigned char __attribute__((section(\".%s\"))) %s[%d] = {\n\t", arrayName, arrayName, fileSize);

    // 逐字节读取二进制文件并写入C数组文件
    for (int i = 0; i < fileSize; i++) {
        unsigned char byte;
        if (fread(&byte, 1, 1, inputFile) != 1) {
            printf("读取文件时发生错误\n");
            fclose(inputFile);
            fclose(outputFile);
            return 1;
        }
        fprintf(outputFile, "0x%02X", byte);// 以十六进制格式写入字节值
        if (i < fileSize - 1) {
            fprintf(outputFile, ", ");// 写入逗号分隔符
        }
        if ((i + 1) % BYTES_PER_LINE == 0) {
            fprintf(outputFile, "\n\t");// 每16个字节换行
        }
    }

    fprintf(outputFile, "\n};");// 结尾加上分号

    // 写入C数组声明到输出文件
    fprintf(outputFile, "\n\nunsigned long long %s_length = %d;\n\t", arrayName, fileSize);

    // 关闭文件
    fclose(inputFile);
    fclose(outputFile);

    printf("转换完成\n");

    return 0;
}