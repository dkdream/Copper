/**********************************************************************
This file is part of Copper.

Copper is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Copper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Copper.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/
/***************************
 **
 ** Project: Copper
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "copper.h"

/* */
#include <stdlib.h>
#include <ctype.h>

/* */

/* should be in ctype.h *
extern int isblank(int c);
*/

extern void cu_SyntaxError(FILE* error,
                           Copper cu_input,
                           const char* filename)
{
    unsigned  lineNumber = cu_input->reach.line_number + 1;
    unsigned  charOffset = cu_input->reach.text_inx;
    unsigned       limit = cu_input->data.limit;

    fprintf(stderr, "%s:%d: %s", filename, lineNumber, "syntax error");

    if (charOffset >= limit) {
        fprintf(error, "\n");
        exit(1);
    }

    char   *buffer = cu_input->data.buffer;
    unsigned start = charOffset;
    unsigned inx;

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

