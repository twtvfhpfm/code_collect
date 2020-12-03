#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static char module1_name[8];
static int module1_value;
static int value = 2;

int module1_init()
{
    strcpy(module1_name, "mod1");
    module1_value = 1;

    return 0;
}

int module1_set_name(char* new_name)
{
    strcpy(module1_name, new_name);
    return 0;
}

int module1_get_value()
{
    value = module1_value;
    return module1_value;
}