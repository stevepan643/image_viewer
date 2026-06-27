#ifndef IMGP_INTERNAL_H
#define IMGP_INTERNAL_H

#include <stdio.h>

#define IMG_READER(f)           \
unsigned char * read_image_##f( \
        FILE *fp,               \
        unsigned int *width,    \
        unsigned int *height,   \
        unsigned int *channels)

#endif  /* IMGP_INTERNAL_H */