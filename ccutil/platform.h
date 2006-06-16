// Place holder
#define DLLSYM
#ifdef __MSW32__
#define SIGNED
#else
#define __UNIX__
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#define SIGNED signed
#endif
