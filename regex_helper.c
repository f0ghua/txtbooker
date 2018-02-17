#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <gc.h>
#include <regexp.h>
#include "msgprintf.h"
#include "regex_helper.h"

char match_patterns[NSUBEXP][REG_MAX_MTLEN];

int regex_match_ERE(const char *str, const char *pattern)
{
    int r, i;
    regexp *prog;

	for (i = 0; i < NSUBEXP; i++) {
		match_patterns[i][0] = '\0';
	}

    if ((prog = regcomp(pattern)) == NULL) {
        return -1;
    }

    if ((r = regexec(prog, str)) == 0) {
        return -1;
    }

    for (i = 0; i < NSUBEXP; i++) {
        char *start_index = prog->startp[i];
        char *end_index = prog->endp[i];
        int len = end_index - start_index;
        len = (len > REG_MAX_MTLEN)?REG_MAX_MTLEN:len;
        if (start_index != NULL) {
            strncpy(match_patterns[i], start_index, len);
            match_patterns[i][len]='\0';
        }
    }

    return 0;
}
