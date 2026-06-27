#include "ppm.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

IMG_READER(ppm)
{
    if (!fp || !width || !height || !channels) return NULL;

    char magic[3];
    unsigned int max_val;

    /*
     * PPM format metadata: Magic number(P6 or P3), width, height, max value
     * per pixel
     */
    if (fscanf(fp, "%2s", magic) != 1) return NULL;
    int ch;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (ch == '#') while ((ch = fgetc(fp)) != '\n' && ch != EOF);
        else if (!isspace(ch))
        {
            ungetc(ch, fp);
            break;
        }
    }
    if (fscanf(fp, "%u %u %u", width, height, &max_val) != 3) return NULL;
    fgetc(fp);
    *channels = 3; /* PPM only support RGB */

    unsigned char *img = (unsigned char *)malloc(*width * *height * 3);
    if (!img) return NULL;

    if (strcmp(magic, "P3") == 0) goto P3;
    else if (strcmp(magic, "P6") == 0) goto P6;
    else { free(img); return NULL; }

P6:
    fread(img, 1, *width * *height * 3, fp);
    goto exit;
P3:
    for (int i = 0; i < *width * *height * 3; ++i)
    {
        int val;
        if (fscanf(fp, "%d", &val) != 1)
        {
            free(img);
            return NULL;
        }
        img[i] = (unsigned char)val;
    }
    goto exit;
exit:
    return img;
}