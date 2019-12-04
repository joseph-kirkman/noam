#ifndef NOAM_UTILITY_H
#define NOAM_UTILITY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NOAM_DEBUG

/* noam_release_func: used in all noam data structures to release memory */
typedef void(*noam_release_func)(void*);

/* noam_atob: converts a C string representation of a boolean to an int */
int noam_atob(const char* str);

#endif //NOAM_UTILITY_H
