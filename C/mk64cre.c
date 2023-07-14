#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int32_t readInt(uint8_t* data, int index) {
    return (data[index] << 24) | (data[index + 1] << 16) | (data[index + 2] << 8) | data[index + 3];
}

int16_t readShort(uint8_t* data, int index) {
    return (data[index] << 8) | data[index + 1];
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Mario Kart 64 RAM Course Extractor\n");
        printf("Usage: mk64cre <input.bin> <output.obj>\n");
        return 1;
    }

    FILE* inFile;
    errno_t err = fopen_s(&inFile, argv[1], "rb");
    if (err != 0) {
        printf("Error opening input file\n");
        return 1;
    }

    fseek(inFile, 0, SEEK_END);
    long fileSize = ftell(inFile);
    rewind(inFile);
    uint8_t* data = malloc(fileSize + 16);
    fread(data, 1, fileSize, inFile);
    fclose(inFile);
    memset(data + fileSize, 0xFF, 16);  // pad the file in case the vertex data table reaches end of file

    printf("Input file is %ld KB\n", fileSize / 1024);
    int vertTable = readInt(data, 0x00150268);
    printf("Vertex       data starts at 0x%08X according to pointer at 0x00150268\n", vertTable);
    int displayListTable = readInt(data, 0x00150274);
    printf("Display List data starts at 0x%08X according to pointer at 0x00150274\n", displayListTable);

    FILE* outFile;
    err = fopen_s(&outFile, argv[2], "w");
    if (err != 0) {
        printf("Error opening output file\n");
        return 1;
    }

    int vertCount = 0;
    int triCount = 0;
    int p;
    for (p = vertTable; data[p+6] == 0 && data[p+7] >> 4 == 0; p += 16) {   // detect table end by checking known 12 bits
        int16_t x = readShort(data, p);
        int16_t y = readShort(data, p + 2);
        int16_t z = readShort(data, p + 4);
        fprintf(outFile, "v %hd %hd %hd\n", x, y, z);
        vertCount++;
    }
    printf("Vertex Table ended at 0x%08X\n", p);

    int vertOffset = 0;
    for (p = displayListTable; p < vertTable; p += 8) {
        switch (data[p]) {
        case 0x04:  // Load Vertices
            vertOffset = ((data[p+5] << 16) | (data[p+6] << 8) | data[p+7]) >> 4;
            break;
        case 0xB1:  // Draw Two Triangles
            fprintf(outFile, "f %d %d %d\n", (data[p+1] >> 1) + vertOffset + 1, (data[p+2] >> 1) + vertOffset + 1, (data[p+3] >> 1) + vertOffset + 1);
            triCount++;
        case 0xBF:  // Draw One Triangle
            fprintf(outFile, "f %d %d %d\n", (data[p+5] >> 1) + vertOffset + 1, (data[p+6] >> 1) + vertOffset + 1, (data[p+7] >> 1) + vertOffset + 1);
            triCount++;
            break;
        }
    }
    fclose(outFile);
    free(data);
    printf("\nSaved %d vertices and %d triangles to %s\n", vertCount, triCount, argv[2]);
    return 0;
}
