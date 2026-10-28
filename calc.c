// Daiguin MILLOGO  260ADB180
// https://github.com/DaiguinMillogo/Gradedlab_C
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic), conversation history: <lien de cette discussion>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // for mkdir(), which creates the output folder

#define FIRSTNAME "daiguin"
#define LASTNAME "millogo"
#define STUDENT_ID "260ADB180"

char *text;
long pos;

void skip_spaces() {
    while (text[pos] == ' ' || text[pos] == '\t') {
        pos++;
    }
}
// skip_spaces: skips the spaces and tabs.

long long read_number() {
    long long n = 0;
    while (text[pos] >= '0' && text[pos] <= '9') {
        n = n * 10 + (text[pos] - '0');
        pos++;
    }
    return n;
}
// read_number: reads a number written with digits, like 25.

long long compute_line() {
    skip_spaces();
    long long result = read_number();
    skip_spaces();

    while (text[pos] == '+' || text[pos] == '-') {
        char op = text[pos];
        pos++;
        skip_spaces();
        long long n = read_number();
        if (op == '+') {
            result = result + n;
        } else {
            result = result - n;
        }
        skip_spaces();
    }
    return result;
}
// compute_line: calculates one line like "2 + 5 - 3", from left to right.

int main(int argc, char **argv) {
    char *outdir = NULL;
    char *input = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outdir = argv[i + 1];
            i++;
        } else {
            input = argv[i];
        }
    }
    if (outdir == NULL || input == NULL) {
        fprintf(stderr, "usage: calc -o OUTDIR input.txt\n");
        return 1;
    }

    // AI (Claude): prompt "give me a simple beginner-style code for grade 4".
    // Reading the whole file in one block (fseek/ftell + malloc) is Claude's idea.
    FILE *f = fopen(input, "rb");
    if (f == NULL) {
        fprintf(stderr, "cannot open %s\n", input);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) {
        fprintf(stderr, "cannot read %s\n", input);
        fclose(f);
        return 1;
    }

    text = malloc(size + 1);
    if (text == NULL) {
        fclose(f);
        return 1;
    }
    size_t count = fread(text, 1, size, f);
    text[count] = '\0';
    fclose(f);

    // Gets the file name without the folder and without ".txt".
    // Example: "labs/task1.txt" becomes "task1".
    char *name = strrchr(input, '/');
    if (name == NULL) name = input; else name = name + 1;
    int len = strlen(name);
    if (len > 4 && strcmp(name + len - 4, ".txt") == 0) {
        len = len - 4;
    }

    mkdir(outdir, 0777);

    // AI (Claude): prompt "merge my code with yours".
    // I use snprintf instead of sprintf because it never writes more than
    // sizeof path characters, so a very long name cannot overflow the array.
    // 4096 is the usual maximum length of a file path on Linux, so any
    // real path fits.
    char path[4096];
    snprintf(path, sizeof path, "%s/%.*s_%s_%s_%s.txt",
             outdir, len, name, FIRSTNAME, LASTNAME, STUDENT_ID);

    FILE *out = fopen(path, "w");
    if (out == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        free(text);
        return 1;
    }

    pos = 0;
    while (text[pos] != '\0') {
        skip_spaces();

        if (text[pos] == '\n') {
            pos++;
            continue;
        }
        if (text[pos] == '#') {
            while (text[pos] != '\n' && text[pos] != '\0') pos++;
            continue;
        }
        if (text[pos] == '\0') break;

        fprintf(out, "%lld\n", compute_line());

        while (text[pos] != '\n' && text[pos] != '\0') pos++;
        if (text[pos] == '\n') pos++;
    }

    fclose(out);
    free(text);
    return 0;
}