#include "bit_stream.h"

#include <stdlib.h>

typedef struct
{
    const unsigned char *data;
    unsigned int size;
} BitStreamChunk;

struct BitStream
{
    BitStreamChunk         *chunks;
    unsigned int            chunk_count;
    unsigned int            chunk_capacity;
    unsigned int            current_chunk;
    unsigned int            byte_off;
    unsigned int            cache;
    int                     bits_available;
};

BitStream *bs_init()
{
    BitStream *bs = malloc(sizeof(BitStream));
    if (!bs) return NULL;
    bs->chunks = NULL;
    bs->chunk_count = 0;
    bs->chunk_capacity = 0;

    bs->current_chunk = 0;
    bs->byte_off = 0;
    bs->cache = 0;
    bs->bits_available = 0;
    return bs;
}
void bs_close(BitStream *bs)
{
    free(bs->chunks);
    free(bs);
}

int bs_add_chunk(BitStream *bs, const unsigned char *data, unsigned int size)
{
    if (!bs || !data || size == 0) return 0;

    // Grow capacity if needed (doubling strategy)
    if (bs->chunk_count >= bs->chunk_capacity) {
        unsigned int new_capacity = bs->chunk_capacity == 0 ? 4 : bs->chunk_capacity * 2;
        BitStreamChunk *new_chunks = realloc(bs->chunks, new_capacity * sizeof(BitStreamChunk));
        if (!new_chunks) return 0; // Allocation failed

        bs->chunks = new_chunks;
        bs->chunk_capacity = new_capacity;
    }

    // Append the new chunk info
    bs->chunks[bs->chunk_count].data = data;
    bs->chunks[bs->chunk_count].size = size;
    bs->chunk_count++;

    return 1;
}

static unsigned int bs_total_bits(const BitStream *bs)
{
    unsigned int total_bytes = 0;
    for (unsigned int i = 0; i < bs->chunk_count; i++) {
        total_bytes += bs->chunks[i].size;
    }
    return total_bytes * 8;
}

// Helper function to get the current absolute bit position
static unsigned int bs_tell_bits(const BitStream *bs)
{
    if (bs->chunk_count == 0) return 0;

    unsigned int accumulated_bytes = 0;
    for (unsigned int i = 0; i < bs->current_chunk; i++) {
        accumulated_bytes += bs->chunks[i].size;
    }

    // Total bits processed up to the current byte offset
    unsigned int total_bits = (accumulated_bytes + bs->byte_off) * 8;

    // Subtract the unread bits still sitting in the cache
    return total_bits - bs->bits_available;
}

int bs_seek(BitStream *bs, int bit_pos, unsigned int start)
{
    if (!bs || bs->chunk_count == 0) return 0;

    int absolute_bit_pos = 0;
    int total_bits = (int)bs_total_bits(bs);

    // 1. Resolve origin to an absolute bit position
    switch (start)
    {
        case BS_SEEK_SET:
            absolute_bit_pos = bit_pos;
            break;

        case BS_SEEK_CUR:
            absolute_bit_pos = (int)bs_tell_bits(bs) + bit_pos;
            break;

        case BS_SEEK_END:
            absolute_bit_pos = total_bits + bit_pos;
            break;

        default:
            return 0; // Invalid origin
    }

    // 2. Bounds check
    if (absolute_bit_pos < 0 || absolute_bit_pos > total_bits) {
        return 0; // Out of bounds
    }

    // 3. Map absolute bit position to target chunks and bytes
    unsigned int target_byte = (unsigned int)absolute_bit_pos / 8;
    int target_bit_in_byte = (unsigned int)absolute_bit_pos % 8;
    
    unsigned int chunk_idx = 0;
    unsigned int accumulated_bytes = 0;

    while (chunk_idx < bs->chunk_count) 
    {
        if (target_byte < accumulated_bytes + bs->chunks[chunk_idx].size) {
            break;
        }
        accumulated_bytes += bs->chunks[chunk_idx].size;
        chunk_idx++;
    }

    // Edge case: seeking exactly to the end of the final chunk
    if (chunk_idx >= bs->chunk_count && target_byte == accumulated_bytes) {
        bs->current_chunk = bs->chunk_count - 1;
        bs->byte_off = bs->chunks[bs->current_chunk].size;
        bs->cache = 0;
        bs->bits_available = 0;
        return 1;
    }

    if (chunk_idx >= bs->chunk_count) return 0;

    bs->current_chunk = chunk_idx;
    bs->byte_off = target_byte - accumulated_bytes;
    
    // Refresh cache with the byte containing the targeted bit
    bs->cache = bs->chunks[bs->current_chunk].data[bs->byte_off++];
    bs->bits_available = 8 - target_bit_in_byte;

    // Advance the cache state over the already-consumed bits in this byte
    bs->cache >>= target_bit_in_byte;

    return 1;
}

unsigned int bs_read(BitStream *bs, unsigned int n)
{
    if (!bs || bs->chunk_count == 0 || n > 32 || n == 0) return 0; 

    unsigned int result = 0;
    unsigned int bits_collected = 0;

    while (bits_collected < n)
    {
        if (bs->bits_available == 0)
        {
            // Roll over to the next chunk if we finish the current one
            while (bs->byte_off >= bs->chunks[bs->current_chunk].size) 
            {
                bs->current_chunk++;
                bs->byte_off = 0;
                if (bs->current_chunk >= bs->chunk_count) return 0;
            }
            bs->cache = bs->chunks[bs->current_chunk].data[bs->byte_off++] & 0xFF;
            bs->bits_available = 8;
        }
        unsigned int bit = bs->cache & 1;
        bs->cache >>= 1;
        bs->bits_available --;

        result |= (bit << bits_collected);
        bits_collected++;
    }

    return result;
}

unsigned int bs_peek(BitStream *bs, unsigned int n)
{
    if (!bs || bs->chunk_count == 0 || n > 32 || n == 0) return 0; 

    unsigned int current_chunk  = bs->current_chunk;
    unsigned int byte_off       = bs->byte_off;
    unsigned int cache          = bs->cache;
    int          bits_available = bs->bits_available;

    unsigned int result = 0;
    unsigned int bits_collected = 0;

    while (bits_collected < n)
    {
        if (bits_available == 0)
        {
            // Roll over to the next chunk if we finish the current one
            while (byte_off >= bs->chunks[current_chunk].size) 
            {
                current_chunk++;
                byte_off = 0;
                if (current_chunk >= bs->chunk_count) return 0;
            }
            cache = bs->chunks[current_chunk].data[byte_off++] & 0xFF;
            bits_available = 8;
        }
        unsigned int bit = cache & 1;
        cache >>= 1;
        bits_available --;

        result |= (bit << bits_collected);
        bits_collected++;
    }

    return result;
}
