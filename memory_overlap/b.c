#include <stdio.h>

typedef struct {
    int a;
    int b;
    int c;
} b_s;

static b_s g_ctx;

int b_store_data(int a, int b, int c)
{
    g_ctx.a = a;
    g_ctx.b = b;
    g_ctx.c = c;
    return 0;
}

int b_read_data()
{
    printf("b_read_data: a(%d) b(%d) c(%d)\n", g_ctx.a, g_ctx.b, g_ctx.c);
    return 0;
}
