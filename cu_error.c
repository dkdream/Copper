/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "copper.h"

/* */
#include <stdlib.h>
#include <ctype.h>

/* */

extern void cu_SyntaxError(FILE* error,
                           Copper cu_input,
                           const char* filename,
                           MoreData fetch_more)
{
    inline bool more() {
        return fetch_more(cu_input);
    }

    unsigned  lineNumber = cu_input->reach.line_number + 1;
    unsigned  charOffset = cu_input->reach.text_inx;
    unsigned       limit = cu_input->data.limit;

    fprintf(stderr, "%s:%d: %s", filename, lineNumber, "syntax error");

    if (charOffset > limit) {
        if (more()) {
            limit = cu_input->data.limit;
        }
    }

    if (charOffset >= limit) {
        fprintf(error, "\n");
        exit(1);
    }

    char *buffer = cu_input->data.buffer;
    int start    = charOffset;
    int inx;

    if ('\n' == buffer[start]) {
        --start;
    }

    for ( ; 0 < start ; --start) {
        int chr = buffer[start];
        if ('\n' == chr) {
            ++start;
            break;
        }
        if ('\r' == chr) {
            ++start;
            break;
        }
    }

    fprintf(error, " in text (%d,%d)\n", charOffset,limit);

    for (inx = start ; inx < limit ; ++inx) {
        int chr =  buffer[inx];
        if ('\n' == chr) break;
        if ('\r' == chr) break;
        fputc(chr, stderr);
    }

    fprintf(error, "\n");

    for (inx = start ; inx < charOffset; ++inx) {
        int chr =  buffer[inx];
        if (isblank(chr)) {
            fputc(chr, stderr);
        } else {
            fputc(' ', stderr);
        }
    }

    fprintf(error, "^\n");

    exit(1);
}

/*****************
 ** end of file **
 *****************/

