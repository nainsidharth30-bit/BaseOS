#ifndef PROCESS_H
#define PROCESS_H
#include<stdint.h>
#include<stddef.h>
#define MAX_PROCESSES 32



struct process_block{
   size_t PID ;
} ;

extern struct process_block process_array[MAX_PROCESSES];

void set_inital_stage_for_extensions(); 

#endif 