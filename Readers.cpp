#include "Readers.h"

#include <iostream>

ByteReader::ByteReader(const std::string& filename)
{
    open(filename, std::ios::in | std::ios::binary);
    current_pointer = 0;
}

void ByteReader::ReadOnDemand(void* pointer_to_variable, char unsigned bytes_to_read, int unsigned read_address)
{
    if (read_address != 0)
    {
        seekg(read_address);
    }
    else
    {
        seekg(current_pointer);
    }
    read(static_cast<char*>(pointer_to_variable), bytes_to_read);
    seekg(current_pointer);
}

unsigned char ByteReader::ReadByte()
{
    char unsigned result;
    seekg(current_pointer);
    read(reinterpret_cast<char*>(&result), sizeof result);
    current_pointer += 1;
    return result;
}

void ByteReader::RequestBufferSeek(unsigned seek_address)
{
    seekg(seek_address);
    current_pointer = seek_address;
}

BitReader::BitReader(const std::string& filename)
{
    open(filename, std::ios::in | std::ios::binary);
    temporary_buffer_ = 0;
    committed_buffer = 0;
    current_pointer = 0;
    current_window_distance = 0;
    last_requested_num_of_bits = 0;
}

int BitReader::ReadUnaligned(char unsigned num_bits_to_read)
{
    // std::cout << "Attempting Unaligned Read!" << std::endl;
    last_requested_num_of_bits = num_bits_to_read;
    seekg(current_pointer);
    read(reinterpret_cast<char*>(&temporary_buffer_), sizeof temporary_buffer_);
    seekg(current_pointer);
    current_window_distance = 0 - current_window_distance;
    long long shifted_temporary_buffer = temporary_buffer_ << current_window_distance;
    committed_buffer |= shifted_temporary_buffer;
    current_window_distance = static_cast<char signed>(last_requested_num_of_bits - current_window_distance);
    long long signed bit_mask = 1;
    bit_mask <<= last_requested_num_of_bits;
    bit_mask -= 1;
    int result = static_cast<int>(committed_buffer & bit_mask);
    if (current_window_distance > 0)
    {
        current_pointer += 0x07;
        committed_buffer = temporary_buffer_ >> current_window_distance;
        current_window_distance -= 0x38;
    }
    else
    {
        committed_buffer >>= last_requested_num_of_bits;
    }
    // std::cout << "Read Result is: " << std::hex << result << std::endl;
    // std::cout << "Data Buffer is: " << std::hex << committed_buffer << std::endl;
    // std::cout << "Buffer Pointer is: " << std::hex << current_pointer << std::endl;
    // std::cout << "Window Distance is: " << std::hex << static_cast<int unsigned>(current_window_distance) << std::endl;
    return result;
}

void BitReader::InitializeBuffer(unsigned start_address)
{
    current_pointer = start_address;
    seekg(current_pointer);
    read(reinterpret_cast<char*>(&committed_buffer), sizeof committed_buffer);
    seekg(current_pointer);
    current_pointer += 0x07;
    current_window_distance = 0 - 0x38;
}

void BitReader::ResetBuffer()
{
    current_window_distance += last_requested_num_of_bits;
    committed_buffer >>= last_requested_num_of_bits;
    if (current_window_distance > 0)
    {
        seekg(current_pointer);
        read(reinterpret_cast<char*>(&committed_buffer), sizeof committed_buffer);
        seekg(current_pointer);
        current_pointer += 0x07;
        committed_buffer = temporary_buffer_ >> current_window_distance;
        current_window_distance -= 0x38;
    }
}

unsigned BitReader::PrepareBlockAddress()
{
    int unsigned result = 0 - current_window_distance;
    result >>= 0x03;
    result = current_pointer - result;
    return result;
}

void BitReader::RequestBufferSeek(int unsigned seek_address)
{
    seekg(seek_address);
    read(reinterpret_cast<char*>(&committed_buffer), sizeof committed_buffer);
    seekg(seek_address);
    current_window_distance = 0 - 0x38;
    current_pointer = seek_address + 0x07;
}
