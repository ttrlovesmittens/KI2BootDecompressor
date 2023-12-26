#include <iostream>

#include "CompressionHelpers.h"
#include "Readers.h"

std::vector<char unsigned> CreateTable()
{
    std::vector<char unsigned> table;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0x1F; j >= 0; j--)
        {
            table.push_back(static_cast<char unsigned>(j));
        }
    }
    return table;
}

void SeekNullByte(ByteReader* byte_stream, BitReader* bit_stream)
{
    int signed read_byte;
    do
    {
        read_byte = static_cast<char signed>(byte_stream->ReadByte());  // NOLINT(bugprone-signed-char-misuse)
        bool bit_status = bit_stream->committed_buffer & 0x01;
        if (read_byte <= 0)
        {
            if (read_byte == 0)
            {
                return;
            }
            read_byte &= 0x7f;
            read_byte <<= 0x08;
            char unsigned next_byte = byte_stream->ReadByte();
            read_byte += next_byte;
        }
        bit_stream->current_window_distance += 1;
        bit_stream->committed_buffer >>= 1;
        if (bit_stream->current_window_distance > 0)
        {
            bit_stream->seekg(bit_stream->current_pointer);
            bit_stream->read(reinterpret_cast<char*>(&bit_stream->committed_buffer), sizeof &bit_stream->committed_buffer);
            bit_stream->seekg(bit_stream->current_pointer);
            bit_stream->current_pointer += 0x07;
            bit_stream->committed_buffer >>= bit_stream->current_window_distance;
            bit_stream->current_window_distance -= 0x38;
        }
        if (bit_status == false)
        {
            byte_stream->current_pointer += read_byte;
        }
    } while (read_byte != 0 && !byte_stream->eof() && !bit_stream->eof());
}

ByteTable::ByteTable()
{
    table.clear();
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0x1F; j >= 0; j--)
        {
            table.push_back(static_cast<char unsigned>(j));
        }
    }
}

void ByteTable::ModifyTable(unsigned char byte_to_search, bool read_second_table)
{
    if (byte_to_search > 0x1F)
    {
        std::cout << "ModifyTable was called but the byte used to search was greater than 0x1F! Debug this!";
        return;
    }
    int tracker = 0 - 0x1F;
    int unsigned table_position = 0;
    for (int unsigned i = read_second_table ? 0x20 : 0x00 ; i < table.size(); i++)
    {
        table_position = i;
        unsigned char table_value = table[i];
        if (table_value == byte_to_search)
        {
            break;
        }
        tracker += 1;
    }
    while (tracker < 0)
    {
        unsigned char next_table_value = 0;
        if (table_position + 1 < table.size())
        {
            next_table_value = table[table_position + 1];
            
        }
        else
        {
            std::cout << "ModifyTable was called but the next table value should have been INVALID! Debug this!";
        }
        tracker += 1;
        table_position += 1;
        if (table_position - 1 > table.size())
        {
            std::cout << "ModifyTable was called but the previous table value may have overflowed! Debug this!";
            
        }
        else
        {
            table[table_position - 1] = next_table_value;
        }
    }
    table[table_position] = byte_to_search;
}

unsigned char ByteTable::operator[](size_t index) const
{
    if (index < table.size())
    {
        return table[index];
    }
    return table[0];
}

unsigned char& ByteTable::operator[](size_t index)
{
    if (index < table.size())
    {
        return table[index];
    }
    return table[0];
}
