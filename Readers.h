#pragma once
#include <fstream>

class ByteReader : public std::ifstream
{
public:

    // default constructor
    ByteReader(const std::string& filename);

    // this fills a variable on demand while omitting bookkeeping on the pointer.
    // this does not have an equivalent function, just for my own sanity.
    void ReadOnDemand(void* pointer_to_variable, char unsigned bytes_to_read = 0, int unsigned read_address = 0);
    
    // this grabs a byte and returns it while bookkeeping the pointer.
    // this does not have an equivalent function, just for my own sanity.
    char unsigned ReadByte();

    // this seeks without grabbing a byte.
    void RequestBufferSeek(int unsigned seek_address);
    
    // the current pointer that has been committed.
    // this emulates the v0 register in most functions.
    int unsigned current_pointer;
};

class BitReader : public std::ifstream
{
public:
    // default constructor
    BitReader(const std::string& filename);

    // this reads a certain amount of bits from the bit stream.
    // num_bits_to_read usually corresponds to the a0 register.
    // this emulates the 9fc00dc4 function and its variants.
    int ReadUnaligned(char unsigned num_bits_to_read);

    // this sets up the buffer based on a starting address.
    // this is cheating, normally you'd point to the data header, but this was easier to implement.
    // this emulates the 9fc00da0 function.
    void InitializeBuffer(int unsigned start_address);

    // this resets the buffer and adjusts the window if needed.
    // this emulates the 9fc00e08 function.
    void ResetBuffer();

    // translates the current data into a block address to fill in the decompression block table.
    // this emulates the 9fc00e44 function.
    int unsigned PrepareBlockAddress();

    // this seeks the buffer forward without grabbing any bits.
    // this emulates the 9fc00e30 function.
    void RequestBufferSeek(int unsigned seek_address);
    
    // the buffer that is actually committed to after successful reads.
    // this emulates the s0 register in most functions.
    long long unsigned committed_buffer;

    // the current pointer that has been committed.
    // this emulates the s1 register in most functions.
    int unsigned current_pointer;

    // the current distance left from the end of the window backwards.
    // this emulates the s2 register in most functions.
    char signed current_window_distance;

    // the last used number of bits to read.
    // this emulates the a0 register in most functions.
    char unsigned last_requested_num_of_bits;

private:
    
    // a temporary buffer to read into before committing.
    // this emulates v1 register in most functions.
    long long unsigned temporary_buffer_;
    
};
