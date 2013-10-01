#include "rope.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_rope(struct rope *r, const char *str)
{
    r->pstr = strdup(str);
}

void tear_rope(struct rope *r)
{
    free(r->pstr);
}

void join_rope(struct rope *r, struct rope *r1, struct rope *r2)
{
    r->left = r1;
    r->right = r2;
    r->leaf = 0;
}

void print_rope(struct rope *r)
{
    if (!r)
        return;

    if (r->leaf) {
        printf("%s", r->pstr);
    } else {
        print_rope(r->left);
        print_rope(r->right);
    }
}

int main(int argc, char *argv[])
{
    struct rope r1, r2, r3;
    set_rope(&r1, "hello");
    set_rope(&r2, "world");
    join_rope(&r3, &r1, &r2);
    print_rope(&r3);
    tear_rope(&r1);
    tear_rope(&r2);
    return 0;
}
