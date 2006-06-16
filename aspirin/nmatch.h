#ifndef NMATCH_H
#define NMATCH_H

#include "aspirin_bp.h"

/*----------------------------------------------------------------------------
        Public Function Prototypes
-----------------------------------------------------------------------------*/
void nmatch_propagate_forward(); 

void nmatch_set_input(float *vector); 

float *nmatch_get_output(); 

char *nmatch_error_string(); 

TOC_PTR read_header(int fd); 

int load_black_box(int fd, const char *name, const char *key, TOC_PTR toc); 

int nmatch_load_network(char *filename); 

void nmatch_network_forward(); 

int nmatch_init_network(); 

/*
#ifdef __cplusplus
extern "C"
{
#endif

extern int nmatch_init_network();
extern int nmatch_load_network(char *filename);
extern char *nmatch_error_string();
extern void nmatch_propagate_forward();
extern float *nmatch_get_output();
extern void nmatch_set_input(float *input);

#ifdef __cplusplus
};
#endif
*/
#endif
