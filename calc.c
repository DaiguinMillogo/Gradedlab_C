// Daiguin MILLOGO  260ADB180
// https://github.com/DaiguinMillogo/Gradedlab_C
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic), conversation history: <https://claude.ai/chat/81484bf6-655d-4776-a2c9-cd31e26add14>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // for mkdir(), which creates the output folder

#define FIRSTNAME "daiguin"
#define LASTNAME "millogo"
#define STUDENT_ID "260ADB180"

char *text;
long pos;
long error_pos = 0;   // 0 means no error so far

void skip_spaces() {
    while (text[pos] == ' ' || text[pos] == '\t') {
        pos++;
    }
}
// skip_spaces: skips the spaces and tabs.

long long compute_expr();   // declared early because compute_primary calls it

long long read_number() {
    long long n = 0;
    // AI (Claude): prompt "help me with grade 5 errors".
    // Every grade 5 error happens here: we expect a digit and find something
    // else, so the error is reported at that character (pos + 1, because
    // positions start at 1).
    if (text[pos] < '0' || text[pos] > '9') {
        error_pos = pos + 1;
        return 0;
    }
    while (text[pos] >= '0' && text[pos] <= '9') {
        n = n * 10 + (text[pos] - '0');
        pos++;
    }
    return n;
}
// read_number: reads a number written with digits, like 25.


// AI (Claude): prompt "help me with grade 7, parentheses".
// I followed the grammar from the assignment (primary := NUMBER | '(' expr ')')
// and merged Claude's function with my own compute_expr and compute_term.
long long compute_primary() {
    skip_spaces();
    if (text[pos] != '(') {
        return read_number();
    }
    pos++;
    long long value = compute_expr();
    if (error_pos != 0) return 0;
    skip_spaces();
    // the ')' must be here, otherwise the line ended (or something else
    // came) and the error is reported at that position
    if (text[pos] != ')') {
        error_pos = pos + 1;
        return 0;
    }
    pos++;
    return value;
}
// compute_primary: reads a number, or an expression between parentheses.


// Design (grade 8): ** has the highest priority and is right-associative,
// so after the base we call compute_power again for the exponent:
// "2 ** 3 ** 2" is 2 ** (3 ** 2).
//
// AI (Claude): prompt "help me with grade 8, **".
// The idea of calling compute_power again for the exponent, and the fast
// power loop (squaring), are Claude's.
long long compute_power() {
    long long base = compute_primary();
    if (error_pos != 0) return 0;
    skip_spaces();

    if (text[pos] != '*' || text[pos + 1] != '*') {
        return base;
    }
    pos += 2;   // skip the "**"
    skip_spaces();
    // saved before reading the exponent: a negative exponent is reported
    // where the exponent starts
    long start = pos;
    long long exp = compute_power();
    if (error_pos != 0) return 0;
    if (exp < 0) {
        error_pos = start + 1;
        return 0;
    }

    // fast power: instead of multiplying exp times (too slow for a huge
    // exponent), we square the base and halve the exponent each turn
    long long result = 1;
    while (exp > 0) {
        if (exp % 2 == 1) result = result * base;
                if (exp > 1) base = base * base;   // no useless squaring at the end (could overflow)
        exp = exp / 2;
    }
    return result;
}
// compute_power: calculates a ** b, from right to left.



// Design (grade 6): * and / must be calculated before + and -, so this
// function handles a whole chain like "2 * 3 / 4" with a loop, from left to
// right. For a division, we check that the operand is not 0 before dividing.
//
// AI (Claude): prompt "help me with grade 6, * and / with priority".
// The idea of one function per priority level is Claude's.
// Grade 7: read_number is replaced by compute_primary, so that a
// parenthesis can be used as an operand.
// Grade 8: compute_primary is replaced by compute_power, so that ** has a
// higher priority than * and /.



long long compute_term() {
    skip_spaces();
    long long result = compute_power(); 
    if (error_pos != 0) return 0;
    skip_spaces();

    while (text[pos] == '*' || text[pos] == '/') {
        char op = text[pos];
        pos++;
        skip_spaces();
        // we save the position now, because compute_power moves pos and the
        // division by zero must be reported where the right operand starts
        long start = pos;
        long long n = compute_power();
        if (error_pos != 0) return 0;
        if (op == '*') {
            result = result * n;
        } else {
            if (n == 0) {
                error_pos = start + 1;   // +1 because positions start at 1
                return 0;
            }
            result = result / n;   // C truncates toward zero, as the task asks
        }
        skip_spaces();
    }
    return result;
}
// compute_term: calculates a part made of * and /, like "2 * 5 / 3".


// AI (Claude): prompt "help me with grade 7, parentheses".
// Claude suggested splitting my compute_line in two: compute_expr (this one,
// which the parentheses can call again) and compute_line (below).
long long compute_expr() {
    skip_spaces();
    // we call compute_term and not read_number, so that * and / are always
    // calculated before + and - (this is what gives them the priority)
    long long result = compute_term();
    if (error_pos != 0) return 0;
    skip_spaces();

    while (text[pos] == '+' || text[pos] == '-') {
        char op = text[pos];
        pos++;
        skip_spaces();
        long long n = compute_term();
        // after an error we stop at once, so the error stays the first one
        if (error_pos != 0) return 0;
        if (op == '+') {
            result = result + n;
        } else {
            result = result - n;
        }
        skip_spaces();
    }
    return result;
}
// compute_expr: calculates + and - between terms, like "3 + 2 * 5 - 4".

long long compute_line() {
    long long result = compute_expr();
    if (error_pos != 0) return 0;

    // if something is left before the end of the line (like the & in
    // "2 & 5", or an extra ')'), the error is at that character
    if (text[pos] != '\n' && text[pos] != '\0') {
        error_pos = pos + 1;
        return 0;
    }
    return result;
}
// this new procedure compute_line: calculates a whole line and checks that nothing is left.

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

        long long value = compute_line();
        if (error_pos != 0) break;
        fprintf(out, "%lld\n", value);

        while (text[pos] != '\n' && text[pos] != '\0') pos++;
        if (text[pos] == '\n') pos++;
    }
    fclose(out);

    // with an error, the output must contain only the error (no partial results)
    if (error_pos != 0) {
        out = fopen(path, "w");
        if (out == NULL) {
            free(text);
            return 1;
        }
        fprintf(out, "ERROR:%ld\n", error_pos);
        fclose(out);
    }

    free(text);
    return 0;
}