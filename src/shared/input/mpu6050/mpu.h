#pragma once
#include <stddef.h>
#include <math.h>

struct s_mympu {
	float ypr[3];
	float gyro[3];
};

extern struct s_mympu mympu;

int mympu_open(unsigned int rate);
int mympu_update(void);
