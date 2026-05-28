#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "bit_stream.h"

// Helper to print test status
#define RUN_TEST(test_func) do { \
    printf("Running %s...", #test_func); \
    test_func(); \
    printf(" PASSED\n"); \
} while (0)

/* * Test 1: Basic reading within a single dynamically added chunk 
 */
void test_basic_read(void) 
{
    BitStream *bs = bs_init();
    assert(bs != NULL);

    // 0xA5 = 10100101
    unsigned char data[] = { 0xA5 }; 
    int success = bs_add_chunk(bs, data, sizeof(data));
    assert(success == 1);

    // Read 4 bits: expected 1010 (binary) = 10 (decimal)
    unsigned int val1 = bs_read(bs, 4);
    assert(val1 == 10);

    // Read remaining 4 bits: expected 0101 (binary) = 5 (decimal)
    unsigned int val2 = bs_read(bs, 4);
    assert(val2 == 5);

    bs_close(bs);
}

/* * Test 2: Reading values that span across two different chunks
 */
void test_cross_chunk_read(void) 
{
    BitStream *bs = bs_init();

    // Chunk 1: 0xFF (11111111)
    unsigned char chunk1[] = { 0xFF };
    // Chunk 2: 0x00 (00000000)
    unsigned char chunk2[] = { 0x00 };

    bs_add_chunk(bs, chunk1, sizeof(chunk1));
    bs_add_chunk(bs, chunk2, sizeof(chunk2));

    // Read 4 bits from chunk 1 (leaving 4 bits of '1's remaining)
    bs_read(bs, 4); 

    // Read 8 bits spanning across chunk 1 and chunk 2
    // It should pick up the remaining 4 bits of '1' (1111) and the first 4 bits of '0' (0000)
    // Expected: 11110000 (binary) = 240 (decimal)
    unsigned int cross_val = bs_read(bs, 8);
    assert(cross_val == 240);

    bs_close(bs);
}

/* * Test 3: Seeking to specific bit positions across multiple chunks
 */
void test_seek_boundaries(void) 
{
    BitStream *bs = bs_init();

    unsigned char chunk1[] = { 0xAA }; // 10101010 (8 bits)
    unsigned char chunk2[] = { 0x55 }; // 01010101 (8 bits)

    bs_add_chunk(bs, chunk1, sizeof(chunk1));
    bs_add_chunk(bs, chunk2, sizeof(chunk2));

    // Seek to bit index 10 (which is the 3rd bit of chunk 2)
    // chunk 2 bits:  0  1  0  1  0  1  0  1
    // bit indices:   8  9 10 11 12 13 14 15
    // Starting from index 10, reading 4 bits should yield: 0101 (binary) = 5 (decimal)
    int seek_success = bs_seek(bs, 10);
    assert(seek_success == 1);

    unsigned int val = bs_read(bs, 4);
    assert(val == 5);

    // Seek past total size limit (16 bits total, try seeking to bit 16)
    int seek_fail = bs_seek(bs, 16);
    assert(seek_fail == 0);

    bs_close(bs);
}

/* * Test 4: Dynamic array expansion safety (adding many chunks)
 */
void test_many_chunks(void)
{
    BitStream *bs = bs_init();
    
    unsigned char data = 0xFF;
    // Push 20 chunks to trigger internal array reallocations multiple times
    for(int i = 0; i < 20; i++) {
        int res = bs_add_chunk(bs, &data, 1);
        assert(res == 1);
    }

    // Read 16 bits (spanning across chunk 0 and chunk 1)
    unsigned int val = bs_read(bs, 16);
    assert(val == 0xFFFF);

    bs_close(bs);
}

/* * Test 5: Underflow safety (reading more bits than exist)
 */
void test_stream_underflow(void)
{
    BitStream *bs = bs_init();
    unsigned char data[] = { 0xFF }; // 8 bits available
    bs_add_chunk(bs, data, sizeof(data));

    // Requesting 12 bits when only 8 exist
    // Should gracefully return the remaining 8 bits left in the cache (0xFF)
    unsigned int val = bs_read(bs, 12);
    assert(val == 0xFF);

    // Subsequent reads should return 0 as stream is exhausted
    unsigned int empty_val = bs_read(bs, 4);
    assert(empty_val == 0);

    bs_close(bs);
}

int main(void) 
{
    printf("=== STARTING BIT STREAM TESTS ===\n");
    
    RUN_TEST(test_basic_read);
    RUN_TEST(test_cross_chunk_read);
    RUN_TEST(test_seek_boundaries);
    RUN_TEST(test_many_chunks);
    RUN_TEST(test_stream_underflow);

    printf("=== ALL TESTS PASSED SUCCESSFULLY ===\n");
    return 0;
}
