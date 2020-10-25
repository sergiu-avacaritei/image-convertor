#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

// Define height and width as 200 in order to view the sketch file properly when
// using the sketch viewer.
#define H 200
#define W 200

// Operations and tool types.
enum { NONE = 0x80, LINE = 0x81, COLOUR = 0x83, TARGETX = 0x84, DY = 0x40 };

// A byte is defined as an unsigned 8bit value.
typedef unsigned char byte;

// -----------------------------------------------------------------------------
// Convert PGM to sketch.

// Change the file extension from ".pgm" to ".sk".
char *convertFilenameToSK(char *args) {
    char *filename = strtok(args, ".");
    strcat(filename, ".sk");
    return filename;
}

// Jump after the "magic number", increasing i by 2.
void readP5(const byte *buffer, int *i) {
    (*i) = (*i) + 2;
}

// Jump after a whitespace and width.
void readWidth(const byte *buffer, int *i) {
    for ((*i) = (*i) + 1; isdigit(buffer[(*i)]); (*i)++);
}

// Jump after a whitespace and height.
void readHeight(const byte *buffer, int *i) {
    for ((*i) = (*i) + 1; isdigit(buffer[(*i)]); (*i)++);
}

// Jump after a whitespace and Maxval.
void readMaxval(const byte *buffer, int *i) {
    for ((*i) = (*i) + 1; isdigit(buffer[(*i)]); (*i)++);
}

// Write DATA commands to build up the colour and then write COLOUR command to
// change it.
void writeColour(byte data, FILE *out) {
    unsigned int bigByte = data;
    bigByte = (bigByte << 8) | data;
    bigByte = (bigByte << 8) | data;
    bigByte = (bigByte << 8) | 0xFF;

    byte op;
    op = 0xC0 | ((bigByte >> 30) & 0x03); fputc(op, out);
    op = 0xC0 | ((bigByte >> 24) & 0x3F); fputc(op, out);
    op = 0xC0 | ((bigByte >> 18) & 0x3F); fputc(op, out);
    op = 0xC0 | ((bigByte >> 12) & 0x3F); fputc(op, out);
    op = 0xC0 | ((bigByte >>  6) & 0x3F); fputc(op, out);
    op = 0xFF; fputc(op, out);
    op = COLOUR; fputc(op, out);
}

// Write DX commands to increase x by runLength.
void writeDX(int runLength, FILE *out) {
    while (runLength >= 0x1F) {
        fputc(0x1F, out);
        runLength = runLength - 0x1F;
    }
    if (runLength > 0x0) {
      fputc(runLength, out);
    }
}

// Write TARGETX command to set x to 0.
void writeTargetX(int byteLength, FILE *out) {
    fputc(TARGETX, out);
}

// Write DY command to increase y by 1.
// Note that this will not draw anything because the tool will always be set to
// NONE when the command is called.
void writeTargetY(FILE *out) {
    fputc(DY + 1, out);
}

// Write TOOL command to change the current tool to NONE.
void writeToolNone(FILE *out) {
    fputc(NONE, out);
}

// Write TOOL command to change the current tool to LINE.
void writeToolLine(FILE *out) {
    fputc(LINE, out);
}

// Write DY command to draw the current state.
void writeDY(FILE *out) {
    fputc(DY, out);
}

// Takes a PGM image file and converts it into a sketch file.
void encode(const char *filename, const byte *buffer, const int length) {
    int startPosition = 0;
    readP5(buffer, &startPosition);
    readWidth(buffer, &startPosition);
    readHeight(buffer, &startPosition);
    readMaxval(buffer, &startPosition);
    byte result[length];
    int currentX = 0;
    FILE *out = fopen(filename, "wb");
    writeToolLine(out);
    for (int i = startPosition + 1, bytes = 0; i < length; i++, bytes++) {
        result[bytes] = buffer[i];
        int runLength = 1;
        while (i + 1 < length && buffer[i] == buffer[i + 1] && runLength + currentX < W) {
            runLength++;
            i++;
        }
        writeColour(result[bytes], out);
        writeDX(runLength, out);
        writeDY(out);
        if (currentX + runLength == W) {
            currentX = 0;
            writeToolNone(out);
            writeTargetX(currentX, out);
            writeTargetY(out);
            writeToolLine(out);
        }
        else currentX += runLength;
    }
    fclose(out);
    printf("File %s has been written.\n", filename);
}

// -----------------------------------------------------------------------------
// Convert sketch to PGM.

// Change the file extension from ".sk" to ".pgm".
char *convertFilenameToPGM(char *args) {
    char *filename = strtok(args, ".");
    strcat(filename, ".pgm");
    return filename;
}

// Build up data for colour.
byte getColour(const byte *buffer, int *i) {
    (*i)++;
    unsigned int data = buffer[(*i)] & 0x03; (*i)++;
    while (buffer[(*i)] != COLOUR) {
        data = (data << 6) | (buffer[(*i)] & 0x3F); (*i)++;
    }
    (*i)++;
    return (data >> 8) & 0xFF;
}

// Build up data for x position.
int getTargetX(const byte *buffer, int *i) {
    int targetX = 0;
    while (buffer[(*i)] != DY) {
        targetX += buffer[(*i)]; (*i)++;
    }
    (*i)--;
    return targetX;
}

// Takes a sketch file and converts it into a PGM image file.
void decode(const char *filename, const byte *buffer, const int length) {
    FILE *out = fopen(filename, "wb");
    fprintf(out,"P5 %d %d 255\n", W, H);
    byte image[H][W];
    int currentX = 0, currentY = 0;
    for (int i = 0; i < length - 1; i++) {
        if (buffer[i] == LINE || (buffer[i] == DY && buffer[i + 1] != NONE)) {
            byte colour =  getColour(buffer, &i);
            int targetX = getTargetX(buffer, &i);
            if (currentX + targetX == W) {
                for (int x = currentX; x < W; x++) {
                    image[currentY][x] = colour;
                }
                currentX = 0;
                currentY++;
            }
            else {
                for (int x = currentX; x < currentX + targetX; x++) {
                    image[currentY][x] = colour;
                }
                currentX += targetX;
            }
        }
    }
    fwrite(image, 1, H * W, out);
    fclose(out);
    printf("File %s has been written.\n", filename);
}

// -----------------------------------------------------------------------------
// User interface and testing.

// A replacement for the library assert function.
void assert(int line, bool b) {
    if (b) return;
    printf("The test on line %d fails.\n", line);
    exit(1);
}

// Check convertFilenameToPGM().
void testConvertFilenameToPGM() {
    char file1[12] = "myimage.sk";
    char file2[10] = "bands.sk";
    assert(__LINE__, strcmp(convertFilenameToPGM(file1), "myimage.pgm") == 0);
    assert(__LINE__, strcmp(convertFilenameToPGM(file2), "bands.sk") != 0);
}

// Check convertFilenameToSK().
void testConvertFilenameToSK() {
  char file1[12] = "myimage.pgm";
  char file2[10] = "bands.pgm";
  assert(__LINE__, strcmp(convertFilenameToSK(file1), "myimage.sk") == 0);
  assert(__LINE__, strcmp(convertFilenameToSK(file2), "bands.pgm") != 0);
}

// Check getColour().
void testGetColour() {
    int i = 0;
    byte buffer1[8] = {LINE, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, COLOUR};
    byte colour = getColour(buffer1, &i);
    assert(__LINE__, colour == 255);
    i = 0;
    byte buffer2[] = {LINE, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, COLOUR};
    colour = getColour(buffer2, &i);
    assert(__LINE__, colour == 0);
}

// Check getTargetX().
void testgetTargetX() {
    int i = 0;
    byte buffer1[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0E, DY};
    int targetX = getTargetX(buffer1, &i);
    assert(__LINE__, targetX == 200);
    i = 0;
    byte buffer2[8] = {0x1F, 0x01, DY};
    targetX = getTargetX(buffer2, &i);
    assert(__LINE__, targetX == 32);
}

// Run tests on functions.
void test() {
    testConvertFilenameToPGM();
    testConvertFilenameToSK();
    testGetColour();
    testgetTargetX();
    printf("All tests pass.\n");
}

// Run the program or, if there are no arguments, test it.
int main(int n, char *args[n]) {
    if (n == 1) {
        test();
    }
    else if (n == 2) {
        FILE *in = fopen(args[1], "rb");
        fseek(in, 0, SEEK_END);
        long length = ftell(in);
        fseek(in, 0, SEEK_SET);
        byte *buffer = malloc(length * sizeof(byte));
        fread(buffer, 1, length, in);
        fclose(in);
        if (strstr(args[1], ".pgm")) {
            encode(convertFilenameToSK(args[1]), buffer, length); free(buffer);
        }
        else if (strstr(args[1], ".sk")) {
            decode(convertFilenameToPGM(args[1]), buffer, length); free(buffer);
        }
        else {
            fprintf(stderr, "The program converts only .sk and .pgm files!\n");
            free(buffer);
            exit(1);
        }
    }
    else {
        fprintf(stderr, "Use e.g.: ./converter myimage.pgm or ");
        fprintf(stderr, "./converter myimage.sk.\n");
        exit(1);
    }
}
