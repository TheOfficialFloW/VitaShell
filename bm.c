#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

const char * boyer_moore(const char *haystack, const char *needle) {
    size_t plen = strlen(needle), slen = strlen(haystack);

    if (plen > slen) {
        return NULL;
    }

    int skip[UCHAR_MAX+1];
    int i, j, k, * next;

    /* calc skip table („bad rule“) */
    for (i = 0; i <= UCHAR_MAX; i++) {
         skip[i] = plen;
    }

    for (i = 0; i < plen; i++) {
         skip[tolower((unsigned char)needle[i])] = plen - i - 1;
    }


    /* calc next table („good rule“) */
    next = (int*)malloc((plen+1) * sizeof(int));

    for (j = 0; j <= plen; j++) {
        for (i = plen - 1; i >= 1; i--) {
            for (k = 1; k <= j; k++) {
                if (i - k < 0) {
                    break;
                }
                if (tolower((unsigned char)needle[plen - k]) != tolower((unsigned char)needle[i - k])) {
                    goto nexttry;
                }
            }
            goto matched;
nexttry:
            ;
        }
matched:
        next[j] = plen - i;
    }

    plen--;
    i = plen; /* position of last p letter in s */

    while (i < slen) {
        j = 0; /* matched letter count */
        while (j <= plen) {
            if (tolower((unsigned char)haystack[i - j]) == tolower((unsigned char)needle[plen - j])) {
                j++;
            } else {
                i += skip[tolower((unsigned char)haystack[i - j])] > next[j] ? skip[tolower((unsigned char)haystack[i - j])] - j : next[j];
                goto newi;
            }
        }
        free(next);
        return haystack + i - plen;
newi:
        ;
    }
    free(next);
    return NULL;
}