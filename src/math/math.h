#ifndef MATH_H
#define MATH_H

#include <stdlib.h>
#include <string.h>

void
swap_char(char *a, char *b)
{
    char temp = *b;
    *b = *a;
    *a = temp;
}

void
reverse_string(char *s)
{
    for (int i = 0; i < (strlen(s) / 2); i++) {
        swap_char(s[i], s[strlen(s) - 1 - i]);
    }
}

void
itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do {                       /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0); /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse_string(s);
}

#endif // MATH_H