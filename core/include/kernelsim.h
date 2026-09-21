#ifndef KERNELSIM_H
#define KERNELSIM_H

#include "retcode.h"
#include "pcb.h"

#define NUM_CHILDREN 6

pcb_child_t processes[NUM_CHILDREN];
pcb_controller_t controller;


retcode_t
kernelsim_init();

retcode_t
kernelsim_start();

retcode_t
kernelsim_show();

retcode_t
kernelsim_pause();

retcode_t
kernelsim_resume();

#endif