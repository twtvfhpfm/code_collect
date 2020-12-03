#include <stdio.h>

static int g_ctx = 0;
int g_overlap = 0;

int a_store_data(int n)
{
    g_ctx = n;
    return 0;
}

int a_read_data()
{
    printf("a_read_data: g_ctx(%d) g_overlap(%d)\n", g_ctx, g_overlap);
    return 0;
}
