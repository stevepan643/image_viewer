#ifndef IMG_VWR_BIT_STREAM_H
#define IMG_VWR_BIT_STREAM_H

typedef struct BitStream BitStream;

// BitStream *bs_init(const unsigned char *data, unsigned int size);
BitStream *bs_init();
void       bs_close(BitStream *bs);     /* Free all chunk data */

int        bs_add_chunk(BitStream *bs, const unsigned char *data, unsigned int size);

#define BS_SEEK_SET 0 // Seek from the beginning of the bitstream
#define BS_SEEK_CUR 1 // Seek relative to the current read position
#define BS_SEEK_END 2 // Seek from the very end of the bitstream
int bs_seek(BitStream *bs, int bit_pos, unsigned int start);

unsigned int bs_read(BitStream *bs, unsigned int n);

unsigned int bs_peek(BitStream *bs, unsigned int n);

#endif
