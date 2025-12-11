#include <math.h>
#include <stdlib.h>
#include <string.h>

int lexerDigitExponent(char *text, int length) {
    int  number;
    char digits[length + 1];
    char exponent[length + 1];
    memset(digits, 0, length + 1);
    memset(exponent, 0, length + 1);

    for (int i = 0; i < length; i++) {
        if (text[i] >= '0' && text[i] <= '9') {
            digits[i] = text[i];
        } else if (text[i] == 'e' || text[i] == 'E') {
            int index = 0;
            for (int j = i + 1; j < length; j++) {
                if (text[j] >= '0' && text[j] <= '9') { exponent[index++] = text[j]; }
            }
            break;
        }
    }
    int expo = atoi(exponent);
    number   = atoi(digits);
    for (int i = 0; i < expo; i++) { number = number * 10; }
    return number;
}
