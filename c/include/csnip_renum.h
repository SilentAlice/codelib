#ifndef __CSNIP_RENUM_H__
#define __CSNIP_RENUM_H__

#include <string.h>

#define RENUM_GLUEX(x, y) x##y
#define RENUM_GLUE(x, y) RENUM_GLUEX(x, y)

#define RENUM_EXPAND(a, b, c) a, b, c,

#define RENUM_MAP0(name, prefix, m, ...)
#define RENUM_MAP1(name, prefix, m, a, b, c, ...) m##1(prefix, a, b, c, RENUM_MAP0(name, prefix, m, __VA_ARGS__))
#define RENUM_MAP2(name, prefix, m, a, b, c, ...) m(prefix, a, b, c, RENUM_MAP1(name, prefix, m, __VA_ARGS__))
#define RENUM_MAP3(name, prefix, m, a, b, c, ...) m(prefix, a, b, c, RENUM_MAP2(name, prefix, m, __VA_ARGS__))
#define RENUM_MAP4(name, prefix, m, a, b, c, ...) m(prefix, a, b, c, RENUM_MAP3(name, prefix, m, __VA_ARGS__))
#define RENUM_MAP5(name, prefix, m, a, b, c, ...) m(prefix, a, b, c, RENUM_MAP4(name, prefix, m, __VA_ARGS__))
#define RENUM_MAP6(name, prefix, m, a, b, c, ...) m(prefix, a, b, c, RENUM_MAP5(name, prefix, m, __VA_ARGS__))
#define RENUM_MAPx(n, ...) RENUM_MAP##n(__VA_ARGS__)

/* Definition of enum structure */
#define RENUM_JOIN_PREFIX1(prefix, a, b, c, tail) RENUM_GLUE(prefix, a), tail
#define RENUM_JOIN_PREFIX(prefix, a, b, c, tail) RENUM_GLUE(prefix, a), tail

#define RENUM_DEF(n, name, prefix, table)                                      \
    enum name {                                                                \
        RENUM_MAPx(n, name, prefix, RENUM_JOIN_PREFIX, table(RENUM_EXPAND))    \
    };

/* Query enum with given string */
#define RENUM_LAST1(prefix, a, b, c, branch) (prefix##a)
#define RENUM_LAST(prefix, a, b, c, tail) tail

#define RENUM_END(n, name, prefix, table)                                      \
    RENUM_MAPx(n, name, prefix, RENUM_LAST, table(RENUM_EXPAND))

#define RENUM_COND1(prefix, a, b, c, branch) (prefix##a)
#define RENUM_COND(prefix, a, b, c, tail)                                      \
    ((strcmp(str, b) == 0) ? prefix##a : tail)

#define RENUM_CMP(n, name, prefix, table)                                      \
    RENUM_MAPx(n, name, prefix, RENUM_COND, table(RENUM_EXPAND))

#ifdef RENUM_IMPL
#define RENUM_FROM_STR(n, name, prefix, table)                                 \
    enum name __renum_##name##_from_str(const char *str) {                     \
        return ((str == NULL) ? RENUM_END(n, name, prefix, table)              \
                              : RENUM_CMP(n, name, prefix, table));            \
    }
#else
#define RENUM_FROM_STR(n, name, prefix, table)                                 \
    enum name __renum_##name##_from_str(const char *str);
#endif

/* Definition of enumeration */
#define RENUM(...)                                                             \
    RENUM_DEF(__VA_ARGS__)                                                     \
    RENUM_FROM_STR(__VA_ARGS__)

#define EX_ENUM_TABLE(ENTRY)                                                   \
    ENTRY(ENT0, "ENT0", 0)                                                     \
    ENTRY(ENT1, "ENT1", 1)                                                     \
    ENTRY(ENT2, "ENT2", 2)                                                     \
    ENTRY(ENT3, "ENT3", 3)                                                     \
    ENTRY(ENT4, "ENT4", 4)                                                     \
    ENTRY(ENTMAX, "EX_ENTMAX", 5)

RENUM(6, ex_enum, EX_, EX_ENUM_TABLE)

#endif /* __CSNIP_RENUM_H__ */
