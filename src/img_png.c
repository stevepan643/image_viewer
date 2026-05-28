#include "img_png.h"
#include "bit_stream.h"

#include <stdlib.h>
#include <string.h>

unsigned int be32tohe(unsigned int n)
{
    return ((n << 24) & 0xff000000) |
           ((n << 8)  & 0x00ff0000) |
           ((n >> 8)  & 0x0000ff00) |
           ((n >> 24) & 0x000000ff);
}

unsigned int reserve_bits(unsigned int val, int len)
{
    unsigned int res = 0;
    for (int i = 0; i < len; i++)
    {
        res = (res << 1) | (val & 1);
        val >>= 1;
    }
    return res;
}

/* Structures of PNG format. */
#pragma pack(push, 1)

typedef struct
{
    unsigned int    length;
    unsigned char   type[4];
} PNGChunkHeader;

typedef struct
{
    unsigned int    width;
    unsigned int    height;
    unsigned char   bit_depth;
    unsigned char   color_type;
    unsigned char   compression_mth;
    unsigned char   filter_mth;
    unsigned char   interlace_mth;
} PNGIDHRChunk;

#pragma pack(pop)

typedef struct
{
    PNGChunkHeader  header;
    unsigned char  *data;
    unsigned int    crc;
} PNGChunk;

unsigned int PNGCRCTable[256];

void png_crc_init()
{
    unsigned int crc32 = 1;
    for (unsigned int i = 0; i < 256; ++i)
    {
        crc32 = (unsigned int)i;
        for (int j = 0; j < 8; ++j)
        {
            if (crc32 & 1)  crc32 = 0xEDB88320L ^ (crc32 >> 1);
            else            crc32 = crc32 >> 1;
        }
        PNGCRCTable[i] = crc32;
    }
}

int png_crc_check(PNGChunk *chunk)
{
    if (PNGCRCTable[255] == 0) png_crc_init();

    unsigned int c = 0xFFFFFFFFu;

    for (size_t i = 0; i < 4; ++i)
    {
        c ^= chunk->header.type[i];
        c = (c >> 8) ^ PNGCRCTable[c & 0xFF];
    }
    if (chunk->header.length > 0 && chunk->data != NULL)
    {
        for (size_t i = 0; i < chunk->header.length; ++i)
        {
            c ^= chunk->data[i];
            c = (c >> 8) ^ PNGCRCTable[c & 0xFF];
        }
    }

    c ^= 0xFFFFFFFFu;
    return c == chunk->crc;
}

void png_clean_chunk(PNGChunk *chunk);
PNGChunk *png_next_chunk(FILE *fp)
{

    PNGChunk *chunk = malloc(sizeof(PNGChunk));
    if (chunk == NULL)
    {
        printf("Malloc failed %lu\n", sizeof(PNGChunk));
        return NULL;
    }

    if (fread(&chunk->header, sizeof(PNGChunkHeader), 1, fp) != 1)
    {
        if (feof(fp)) return NULL;
    }
    chunk->header.length = be32tohe(chunk->header.length);

    if (chunk->header.length != 0) {
        chunk->data = malloc(chunk->header.length);
        if (chunk->data == NULL)
        {
            printf("Malloc failed %u\n", chunk->header.length);
            free(chunk);
            return NULL;
        }

        if (fread(chunk->data, chunk->header.length, 1, fp) != 1)
        {
            printf("Read data failed\n");
            free(chunk->data);
            free(chunk);
            return NULL;
        }
    }
    else
    {
        chunk->data = NULL;
    }

    fread(&chunk->crc, 4, 1, fp);
    chunk->crc = be32tohe(chunk->crc);

    if(!png_crc_check(chunk)) {
        printf("CRC32 didn't pass\n");
        png_clean_chunk(chunk);
        return NULL;
    }
    printf("Type: %.4s, size: %u\n", chunk->header.type, chunk->header.length);
    printf("CRC32 passed\n");
    return chunk;
}

void png_clean_chunk_without_data(PNGChunk *chunk)
{
    free(chunk);
}
void png_clean_chunk(PNGChunk *chunk)
{
    if (chunk->header.length != 0) free(chunk->data);
    free(chunk);
}

typedef struct
{
    unsigned char bfinal;
    unsigned char btype;
} PNGInflateBlockHeader;

typedef struct
{
    unsigned int hlit;
    unsigned int hdist;
    unsigned int hclen;
} PNGHuffmanTableSpec;

typedef struct
{
    unsigned short symbol[1024];
    unsigned char  len[1024];
} PNGHuffmanTable;

PNGInflateBlockHeader get_block_header(BitStream *bs)
{
    unsigned int bfinal = bs_read(bs, 1);
    unsigned int btype = bs_read(bs, 2);

    return (PNGInflateBlockHeader){bfinal, btype};
}

PNGHuffmanTableSpec get_huffman_table_spec(BitStream *bs)
{
    unsigned int hlit = bs_read(bs, 5);
    unsigned int hdist = bs_read(bs, 5);
    unsigned int hclen = bs_read(bs, 5);

    return (PNGHuffmanTableSpec){hlit, hdist, hclen};
}

void build_canonical_huffman_table(unsigned char *lengths, int num_symbols, PNGHuffmanTable *table)
{
    int bl_count[16];
}

void build_dynamic_table(
        BitStream *bs,
        PNGHuffmanTableSpec spec,
        PNGHuffmanTable *lit_len_table,
        PNGHuffmanTable *dist_table)
{
    unsigned char order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    unsigned char code_len_tree_lengths[19] = {0};
    for (int i = 0; i < spec.hclen + 4; ++i) code_len_tree_lengths[order[i]] = bs_read(bs, 3);

    int bl_count[16] = {0};
    for (int i = 0; i < 19; ++i) bl_count[code_len_tree_lengths[i]]++;

    int next_code[16] = {0};
    int code = 0;
    bl_count[0] = 0;
    for (int bits = 1; bits <= 15; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    unsigned int symbol_codes[19];
    for (int i = 0; i < 19; i++)
    {
        int len = code_len_tree_lengths[i];
        if (len > 0)
        {
            symbol_codes[i] = next_code[len];
            next_code[len]++;
        }
    }

    for (int i = 0; i < 19; i++)
    {
        int len = code_len_tree_lengths[i];
        if (len > 0)
            printf("[%2d] len: %d %0*b\n", i, len, len, symbol_codes[i]);
    }

    for (int i = 0; i < 19; i++)
    {
        if (code_len_tree_lengths[i] > 0)
        {
            printf("symbol: %2d, len: %d, code: %0*b\n", i, code_len_tree_lengths[i], code_len_tree_lengths[i], symbol_codes[i]);
        }
    }

    PNGHuffmanTable tmp = {0};
    for (int i = 0; i < 19; i++)
    {
        int len = code_len_tree_lengths[i];
        if (len > 0)
        {
            int code = symbol_codes[i];
            int reserved = reserve_bits(code, len);
            int num_fill = 1 << (7 - len);
            for (int j = 0; j < num_fill; j++)
            {
                int index = (j << len) | reserved;
                tmp.symbol[index] = i;
                tmp.len[index] = len;

                // printf("Symbol: %2d, Len: %d, Index: %7b\n", i, len, index);
            }
        }
    }
    int total_symbols = (spec.hlit + 257) + (spec.hdist + 1);
    int lit_len_table_lengths[286];
    int dist_table_lengths[32];
    int cur = 0;
    int last_len = 0;

    while (cur < total_symbols)
    {
        int peeked_bits = bs_peek(bs, 7);
        int len = tmp.len[peeked_bits];
        int symbol = tmp.symbol[peeked_bits];
        bs_read(bs, len);

        if (symbol <= 15)
        {
            if (cur < (spec.hlit + 257)) lit_len_table_lengths[cur] = symbol;
            else dist_table_lengths[cur - (spec.hlit + 257)] = symbol;
            cur++;
        }
        else if (symbol == 16)
        {
            int repeat = bs_read(bs, 2) + 3;
            for (int i = 0; i < repeat; i++)
            {
                if (cur < (spec.hlit + 257)) lit_len_table_lengths[cur] = last_len;
                else dist_table_lengths[cur - (spec.hlit + 257)] = last_len;
                cur++;
            }
        }
        else if (symbol == 17)
        {
            int zeros = bs_read(bs, 3) + 3;
            for (int i = 0; i < zeros; i++)
            {
                if (cur < (spec.hlit + 257)) lit_len_table_lengths[cur] = 0;
                else dist_table_lengths[cur - (spec.hlit + 257)] = 0;
                cur++;
            }
            last_len = 0;
        }
        else if (symbol == 18)
        {
            int zeros = bs_read(bs, 7) + 11;
            for (int i = 0; i < zeros; i++)
            {
                if (cur < (spec.hlit + 257)) lit_len_table_lengths[cur] = 0;
                else dist_table_lengths[cur - (spec.hlit + 257)] = 0;
                cur++;
            }
            last_len = 0;
        }
        printf("symbol: %d, len: %d, %0*b\n", symbol, len, len, peeked_bits);
    }

    
}

/* BTYPE = 0x00 */
void get_huffman_table_def(PNGHuffmanTable *lit_len_table, PNGHuffmanTable *dist_table)
{
    /* TODO */
}

void inflate_block(
        BitStream *bs,
        BitStream *out,
        PNGHuffmanTable *lit_len_table,
        PNGHuffmanTable *dist_table)
{}

#define PNG_IHDR    "IHDR"
#define PNG_IDAT    "IDAT"
#define PNG_IEND    "IEND"

#include <stdio.h>

// Helper function to print an unsigned short as a binary string with a specific bit length
void print_binary(unsigned short code, unsigned char len)
{
    // Print bits from Left to Right (Most Significant Bit first for the Huffman code itself)
    for (int i = len - 1; i >= 0; i--)
    {
        printf("%d", (code >> i) & 1);
    }
}

void print_huffman_table(const char *table_name, const PNGHuffmanTable *table, int total_symbols)
{
    printf("\n=== Huffman Table: %s ===\n", table_name);
    printf("%-10s | %-12s | %-16s\n", "Symbol", "Bit Length", "Huffman Code");
    printf("---------------------------------------------\n");

    int active_symbols = 0;
    for (int i = 0; i < total_symbols; i++)
    {
        // Only print symbols that actually exist in the Huffman tree
        if (table->len[i] > 0)
        {
            // Print symbol (as decimal and hex if it's a printable character/special marker)
            if (i >= 32 && i <= 126) {
                printf("0x%03X ('%c') | %-12d | ", i, (char)i, table->len[i]);
            } else {
                printf("0x%03X       | %-12d | ", i, table->len[i]);
            }

            // Print the variable-length binary code
            print_binary(table->symbol[i], table->len[i]);
            printf("\n");

            active_symbols++;
        }
    }

    if (active_symbols == 0) {
        printf("(Table is empty / no active symbols)\n");
    }
    printf("=============================================\n");
}

/* Read image (PNG format)
 *
 * width - the pointer to save image width
 * height - the pointer to save image height
 * channels - the pointer to save image channel count, e.g. 3 (RGB) or 4 (RGBA)
 *
 * return - image [RBGRBG ...]
 */
IMG_READER(png)
{
    fseek(fp, 8, SEEK_SET);
    PNGChunk *ck;
    BitStream *bs = bs_init();

    while ((ck = png_next_chunk(fp)))
    {
        if (memcmp(ck->header.type, PNG_IHDR, 4) == 0)
        {
            PNGIDHRChunk *ihdr = (PNGIDHRChunk *)ck->data;
            ihdr->width = be32tohe(ihdr->width);
            ihdr->height = be32tohe(ihdr->height);
            printf("%dx%d\n", ihdr->width, ihdr->height);
        }
        else if (memcmp(ck->header.type, PNG_IDAT, 4) == 0)
        {
            bs_add_chunk(bs, ck->data, ck->header.length);
            png_clean_chunk_without_data(ck);
            continue;
        }
        else if (memcmp(ck->header.type, PNG_IEND, 4) == 0);    /* nothing to do yet */
        png_clean_chunk(ck);
    }

    unsigned int cmf = bs_read(bs, 8);
    unsigned int flg = bs_read(bs, 8);
    printf("CMF:0x%X FLG:0x%X\n", cmf, flg);

    BitStream *out = bs_init();

    while (1)
    {
        PNGInflateBlockHeader bh = get_block_header(bs);
        if (bh.bfinal == 1) break;
        if (bh.btype == 0b00); /* TODO: copy data */
        PNGHuffmanTable lit_len = {0};
        PNGHuffmanTable dist = {0};
        PNGHuffmanTableSpec spec = get_huffman_table_spec(bs);
        if (bh.btype == 0b01) get_huffman_table_def(&lit_len, &dist);
        if (bh.btype == 0b10)
        {
            build_dynamic_table(bs, spec, &lit_len, &dist);
        }
        // To print the Literal/Length Table (Max 286 symbols in DEFLATE)
        print_huffman_table("Literal/Length Tree", &lit_len, spec.hlit + 257);

        // To print the Distance Table (Max 32 symbols in DEFLATE)
        print_huffman_table("Distance Tree", &dist, spec.hdist + 1);
        break;
        inflate_block(bs, out, &lit_len, &dist);
    }
    bs_close(bs);

    /* TODO: filter */
    return NULL;
}

