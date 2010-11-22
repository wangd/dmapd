#include <stdio.h>
#include <stdlib.h>

char *usage = "dmapd [options]\n\n"
"directory\n"
"	directory containing media files to serve\n\n"
;

/* ============================ print_usage () ============================== */
void print_usage(const int exitcode, const char *error, const char *more)
{
    fprintf(stderr, "%s\n", usage);
    if (error)
        fprintf(stderr, "%s: %s.\n\n", error, more);
    exit(exitcode);
}
