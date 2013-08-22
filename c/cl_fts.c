#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <fts.h>

int main()
{
    /* 1st parameter of `fts_open' is a NULL terminated array of strings */
    char * const path_argv[] = { ".", NULL };
    FTSENT *entry = NULL;
    /* `FTS_LOGICAL' will follow the symbolic links and return the real file */
    FTS *pfts = fts_open(path_argv, FTS_LOGICAL, NULL);

    assert(pfts);
    /* loop to read entries from this hierarchy */
    while (entry = fts_read(pfts)) {
        /* struture of `FTSENT' is specified in mannual */
        if (S_ISREG(entry->fts_statp->st_mode))
            printf("%s %s %d\n", entry->fts_path, entry->fts_accpath, entry->fts_level);
    }
    fts_close(pfts);
    return 0;
}
