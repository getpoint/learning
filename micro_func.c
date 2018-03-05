/* GENERIC_MAX(type) will generate the func type_max, GENERIC_MAX(float) generate float max.
 * Use GENERIC_MAX can easily generate func to compare num in same type, amazing!
 */

#include <stdio.h>
#include <string.h>

#pragma message("generic max")

#define GENERIC_MAX(type) \
type type##_max(type x, type y) \
{ \
    return x > y ? x : y; \
}

// C99 support variable arguments in micro func. */
// ## before __VA_ARGS__ can ignore the "," if there is only on argument.
#define debug(fmt, ...) \
    do { \
        printf(fmt, ##__VA_ARGS__); \
    } while (0)

// Used same as debug.
#define debug_beforeC99(fmt, args ...) \
    do { \
        printf(fmt, ##args); \
    } while (0)

GENERIC_MAX(int);

int main(void)
{
    int x = 1, y =2;
    int max = 0;

    max = int_max(x, y);
    /* C99 support __func__ */
    debug("[%s_%d]The bigger one in x(%d) and y(%d) is %d\n", __func__, __LINE__, x, y, max);
    return 0;
}
