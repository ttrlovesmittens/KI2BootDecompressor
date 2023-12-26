#pragma once
#include <vector>

class ByteReader;
class BitReader;

// this seeks until a 00 null byte is found.
// this emulates the 9fc00e54 function.
void SeekNullByte(ByteReader* byte_stream, BitReader* bit_stream);

struct ByteTable
{
    // this creates a table that is then modified later.
    // what it's used for i'm not too sure, but it's created dynamically at runtime.
    // this emulates the 9fc00d18 function.
    ByteTable();

    // this searches for a specified byte in the table, then modifies it once that byte is found.
    // this emulates the 9fc00d3c function.
    void ModifyTable(char unsigned byte_to_search, bool read_second_table = false);
    
    std::vector<char unsigned> table;
    
    char unsigned operator[](size_t index) const;
    char unsigned& operator[](size_t index);
};

struct CompressedDataChunk
{
    // i'm not sure what this does yet. it's set to 0x00 in both KI1 and KI2.
    // if this is not recognized as 0, the real game fails to load.
    char unsigned key = 0;

    // the total size of the data after compression.
    int unsigned uncompressed_size = 0;

    // first block length in bits, saved as an unsigned byte in unsigned int width.
    // this is backwards from the end.
    // the bootrom usually fills this in dynamically: it's usually always 0xc8
    int unsigned first_block_num_of_bits = 0;

    // the address to the end of the first block.
    int unsigned first_block_end_address = 0;

    // the address in memory to output decompressed data to. this is useless, but will ALWAYS be 0x88000000 or more due to how memory mapping and caching works on MIPS chips.
    int unsigned decompression_output_address = 0;
};

struct DataHeader
{
    // some sort of magic key. it's set to 0x7262 in both KI1 and KI2.
    // if this is not recognized, the real game fails to load.
    short unsigned key = 0;
    
    // the amount of chunks compressed in the rom. this is usually 0x03.
    // this is to account for the game loading data in different areas with large gaps.
    char unsigned num_of_data_chunks = 0;
};
