#include <stdio.h>

#include "csnip_renum.h"

static void renum_show(void)
{
    printf("ex_enum value: %u\n", (unsigned int)__renum_ex_enum_from_str("ENT3"));
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    renum_show();
    return 0;
}
