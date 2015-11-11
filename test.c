/* A global tear_down function to
   call gcov-flush for all
   tests under the /test tree  */
#include <np.h>
#include <stdio.h>

static int tear_down(void)
{
   puts("tear_down called");
    __gcov_flush(); return 0;
}
