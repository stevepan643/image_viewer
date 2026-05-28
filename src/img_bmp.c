#include "img_bmp.h"

#include <stdlib.h>

/* Structures of BMP format. */
#pragma pack(push, 1)

typedef struct
{
    unsigned short  type;       /* magic identifier */
    unsigned int    size;
    unsigned short  reserved1;
    unsigned short  reserved2;
    unsigned int    offset;     /* offset to start of pixel data */
} BMPFileHeader;

typedef struct
{
    unsigned int    size;
    signed int      width;
    signed int      height;
    unsigned short  planes;
    unsigned short  bit_per_pixel;
    unsigned int    compression;
    unsigned int    image_size;
    signed int      x_resolution;
    signed int      y_resolution;
    unsigned int    color_used;
    unsigned int    color_important;
} BMPInfoHeader;

typedef struct
{
    unsigned char   blue;
    unsigned char   green;
    unsigned char   red;
    unsigned char   reserved;
} BMPPaletteColor;

#pragma pack(pop)

/* Read image (BMP format)
 *
 * width - the pointer to save image width
 * height - the pointer to save image height
 * channels - the pointer to save image channel count, e.g. 3 (RGB) or 4 (RGBA)
 *
 * return - image [RBGRBG ...]
 */
IMG_READER(bmp)
{
    if (!fp || !width || !height || !channels) return NULL;

    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    if (fread(&file_header, sizeof(BMPFileHeader), 1, fp) != 1) return NULL;
    if (file_header.type != 0x4D42) return NULL;
    if (fread(&info_header, sizeof(BMPInfoHeader), 1, fp) != 1) return NULL;

    if (info_header.compression != 0 && info_header.compression != 3) return NULL;

    *width = info_header.width;
    *height = info_header.height;
    *channels = (info_header.bit_per_pixel >= 32) ? 4 : 3;
    int is_top_down = (info_header.height < 0);

    BMPPaletteColor *palette = NULL;
    if (info_header.bit_per_pixel <= 8)
    {
        unsigned int num_colors = info_header.color_used ? info_header.color_used : (1 << info_header.bit_per_pixel);
        palette = (BMPPaletteColor *)malloc(num_colors * sizeof(BMPPaletteColor));
        if (!palette) return NULL;
        fseek(fp, sizeof(BMPFileHeader) + info_header.size, SEEK_SET);
        fread(palette, sizeof(BMPPaletteColor), num_colors, fp);
    }

    fseek(fp, file_header.offset, SEEK_SET);

    int file_row_size = ((*width * info_header.bit_per_pixel + 31) / 32) * 4;
    int out_row_size = *width * *channels;

    unsigned char *file_row_buf = (unsigned char *)malloc(file_row_size);
    unsigned char *out_pixel_data = (unsigned char *)malloc(*width * *height * *channels);

    if (!file_row_buf || !out_pixel_data)
    {
        free(palette); free(file_row_buf); free(out_pixel_data);
        return NULL;
    }

    for (int y = 0; y < *height; ++y)
    {
        if (fread(file_row_buf, 1, file_row_size, fp) != (size_t)file_row_size)
        {
            free(palette); free(file_row_buf); free(out_pixel_data);
            return NULL;
        }

        int target_y = is_top_down ? y : (*height - 1 - y);
        unsigned char *out_pixel = out_pixel_data + (target_y * out_row_size);

        for (int x = 0; x < *width; ++x)
        {
            unsigned char r = 0, g = 0, b = 0, a = 255;

            if (info_header.bit_per_pixel == 24)
            {
                b = file_row_buf[x * 3 + 0];
                g = file_row_buf[x * 3 + 1];
                r = file_row_buf[x * 3 + 2];
            }
            else if (info_header.bit_per_pixel == 32)
            {
                b = file_row_buf[x * 4 + 0];
                g = file_row_buf[x * 4 + 1];
                r = file_row_buf[x * 4 + 2];
                a = file_row_buf[x * 4 + 3];
            }
            else if (info_header.bit_per_pixel == 8 && palette)
            {
                unsigned char index = file_row_buf[x];
                b = palette[index].blue;
                g = palette[index].green;
                r = palette[index].red;
            }

            out_pixel[0] = r;
            out_pixel[1] = g;
            out_pixel[2] = b;

            if (*channels == 4) out_pixel[3] = a;

            out_pixel += *channels;
        }
    }

    free(palette);
    free(file_row_buf);
    return out_pixel_data;
}


