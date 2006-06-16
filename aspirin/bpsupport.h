// bpsupport.h file

#ifndef BPSUPPORT_H
#define BPSUPPORT_H

/*----------------------------------------------------------------------------
        Public Function Prototypes
-----------------------------------------------------------------------------*/
void BPfrandom_init(long seed); 

float BPfrandom(float val); 

void BPread_string(int fd, char *string); 

int BPread_thresholds(int fd,
                      const char *name,
                      const char *key,
                      int size,
                      int target_size,
                      float *data,
                      int *flag);

int BPread_weights(int fd,
                   const char *name1,
                   const char *key1,
                   const char *name2,
                   const char *key2,
                   int size,
                   int target_size,
                   float *data,
                   int *flag);
#endif
