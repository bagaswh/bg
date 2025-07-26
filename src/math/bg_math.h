#ifndef MATH_H
#define MATH_H

#include "common.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

//////////////
// Random
//

struct fastrand {
    unsigned seed;
};

static struct fastrand
bg_fast_rand_init()
{
    struct timespec ts = { 0, 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (struct fastrand) { .seed = ts.tv_nsec };
}

static u64
bg_fast_rand(struct fastrand *r)
{
    r->seed = 214013 * r->seed + 2531011;
    return (r->seed >> 16) & 0x7fff;
}

static u64
bg_fast_rand_n(struct fastrand *r, int n)
{
    return bg_fast_rand(r) % n;
}

static void
reverse_string(char *s)
{
    for (int i = 0; i < (strlen(s) / 2); i++) {
        bg_swap(char, s[i], s[strlen(s) - 1 - i]);
    }
}

static void
itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse_string(s);
}

#endif // MATH_H