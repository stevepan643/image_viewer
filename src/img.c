#include "img.h"
#include "img_png.h"
#include "img_bmp.h"
#include "img_ppm.h"

#include <stdio.h>

#define IMG_UNK     0
#define IMG_PPM     1
#define IMG_BMP     2
#define IMG_PNG     4

unsigned int get_image_type(FILE *fp)
{
    if (!fp) return IMG_UNK;

    long offset = ftell(fp);
    rewind(fp);

    unsigned char buf[8] = {0};
    size_t read_len = fread(buf, 1, 8, fp);

    fseek(fp, offset, SEEK_SET);

    if (read_len < 2) return IMG_UNK;

    if (buf[0] == 0x42 && buf[1] == 0x4D) return IMG_BMP;
    if (buf[0] == 'P' && (buf[1] == '3' || buf[1] == '6')) return IMG_PPM;
    if (buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47 &&
        buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A)
        return IMG_PNG;

    return IMG_UNK;
}

unsigned char * read_image(
        FILE *fp,
        unsigned int *width,
        unsigned int *height,
        unsigned int *channels)
{
    unsigned int type = get_image_type(fp);
    unsigned char *img;
    if (type == IMG_PPM) img = read_image_ppm(fp, width, height, channels);
    else if (type == IMG_BMP) img = read_image_bmp(fp, width, height, channels);
    else if (type == IMG_PNG) img = read_image_png(fp, width, height, channels);
    else
    {
        printf("Unkown image type\n");
        return NULL;
    }
    if (img == NULL) return NULL;

    return img;
}


