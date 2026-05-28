#ifndef IMG_VWR_IMG_H
#define IMG_VWR_IMG_H

#include <stdio.h>

#define IMG_READER(f)           \
unsigned char * read_image_##f( \
        FILE *fp,               \
        unsigned int *width,    \
        unsigned int *height,   \
        unsigned int *channels)

unsigned char * read_image(
        FILE *fp,
        unsigned int *width,
        unsigned int *height,
        unsigned int *channels);

#endif /* IMG_VWR_IMG_H */
