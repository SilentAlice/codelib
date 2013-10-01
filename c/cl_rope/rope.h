#ifndef ROPE_H_
#define ROPE_H_

struct rope {
    struct rope *left;
    struct rope *right;
    union {
        char *pstr;
        char  leaf;
    };
};

#endif /* ROPE_H_ */
