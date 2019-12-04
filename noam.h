#ifndef NOAM_H
#define NOAM_H

#include "noam_parser.h"

#define NOAM_TITLE "noam"
#define NOAM_VERSION "1.0"
#define NOAM_FULL_TITLE NOAM_TITLE " " NOAM_VERSION


#define NOAM_EXIT(cond, message)                   \
if((cond)) {                                       \
    fprintf(stderr, NOAM_TITLE ": " message "\n"); \
    exit(-1);                                      \
}                                                  \

#endif //NOAM_H