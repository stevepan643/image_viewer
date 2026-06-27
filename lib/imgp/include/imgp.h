/*
 * Image Viewer - Image Parser (imgp)
 *
 * Unified image decoding and dispatching layer.
 * 
 * CREATED              2026/06/27
 * VERSION              1.0.0
 * LICENSE              Apache 2.0
 * Copyright (c) 2026 Steve Pan. All rights reserved.
 */

#ifndef IMGP_H
#define IMGP_H

#include <stdio.h>

unsigned char * imgp_load(
        FILE *fp,
        unsigned int *width,
        unsigned int *height,
        unsigned int *channels);

#endif  /* IMGP_H */