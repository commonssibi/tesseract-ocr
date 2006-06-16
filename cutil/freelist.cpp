/**************************************************************************

    $Log$
    Revision 1.1  2006/06/16 22:17:07  lvincent
    Initial checkin of Tesseract 1.0

    Revision 1.1.1.1  2004/02/20 19:38:55  slumos
    Import original HP distribution

* Revision 5.1  89/07/27  11:46:57  11:46:57  ray ()
* Added ratings acces methods.
* This version ready for independent development.
*

**************************************************************************/
#include "freelist.h"
#include "danerror.h"
#include "callcpp.h"

#include <memory.h>

static int mem_alloc_counter = 0;

/**********************************************************************
 * memalloc_p
 *
 * Memory allocator with protection.
 **********************************************************************/
int *memalloc_p(int size) { 
  mem_alloc_counter++;
  if (!size)
    DoError (0, "Allocation of 0 bytes");
  return ((int *) c_alloc_mem_p (size));
}


/**********************************************************************
 * memalloc
 *
 * Memory allocator with protection.
 **********************************************************************/
int *memalloc(int size) { 
  mem_alloc_counter++;
  return ((int *) c_alloc_mem (size));
}


/**********************************************************************
 * memrealloc
 *
 * Memory allocator with protection.
 **********************************************************************/
int *memrealloc(void *ptr, int size, int oldsize) { 
  int shiftsize;
  int *newbuf;

  shiftsize = size > oldsize ? oldsize : size;
  newbuf = (int *) c_alloc_mem (size);
  memcpy(newbuf, ptr, shiftsize); 
  c_free_mem(ptr); 
  return newbuf;
}


/**********************************************************************
 * memfree
 *
 * Memory allocator with protection.
 **********************************************************************/
void memfree(void *element) { 
  if (element) {
    c_free_mem(element); 
    mem_alloc_counter--;
  }
  else {
    cprintf ("%d MEM_ALLOC's used\n", mem_alloc_counter);
    DoError (0, "Memfree of NULL pointer");
  }
}


/**********************************************************************
 * mem_tidy
 *
 * Do nothing.
 **********************************************************************/
void mem_tidy(int level) { 
  c_check_mem ("Old tidy", level);
}
