#include <iostream>
#include <vector>

#include "Readers.h"
#include "CompressionHelpers.h"



int main(int argc, char* argv[])
{
    BitReader bit_stream("bootrom.bin");
    ByteReader byte_stream("bootrom.bin");

    std::ofstream output_stream;
    
    if (bit_stream.fail() || byte_stream.fail())
    {
        std::cout << "An error occured during BootRom reading." << std::endl;
        return -1;
    }
    
    if (bit_stream.is_open() && bit_stream.good() && byte_stream.is_open() && byte_stream.good())
    {
        std::cout << "File opened successfully." << std::endl;
    }

    output_stream.open("decompressed_output.bin", std::ios::out | std::ios::binary);

    if (output_stream.fail())
    {
        std::cout << "An error occured reserving the output bin." << std::endl;
        return -1;
    }

    if (output_stream.is_open() && output_stream.good())
    {
        std::cout << "Output Stream created successfully." << std::endl;
    }
    
    bit_stream.InitializeBuffer(0x1040);

    DataHeader current_header;
    // this section roughly approximates function 9fc00d7c
    current_header.key = static_cast<short unsigned>(bit_stream.ReadUnaligned(0x10));

    if (current_header.key != 0x7262)
    {
        std::cout << "Warning: Current compressed data seems invalid! Magic key is not 0x7262!" << std::endl;
        std::cout << "This ROM will not load correctly in game!" << std::endl;
    }
    
    current_header.num_of_data_chunks = static_cast<char unsigned>(bit_stream.ReadUnaligned(0x8));

    for (int num_of_chunks = current_header.num_of_data_chunks; num_of_chunks > 0; num_of_chunks--)
    {
        CompressedDataChunk current_chunk;

        // THIS IS A LOOP POINT SET UP BY THE COUNTER. oof gotta rework some of these data structures.
        current_chunk.key = static_cast<char unsigned>(bit_stream.ReadUnaligned(0x8));
        if (current_chunk.key != 0x0)
        {
            std::cout << "Warning: Current compressed chunk seems invalid! Compressed chunk magic key is not 0!" << std::endl;
            std::cout << "This ROM will not load correctly in game!" << std::endl;
        }
        current_chunk.uncompressed_size = static_cast<int unsigned>(bit_stream.ReadUnaligned(0x20));
        
        // the total amount of bytes to unpack, unpacked in 32bit double words.
        int unsigned num_bytes_to_unpack = current_chunk.uncompressed_size;

        // this is cheating a little bit, but i'd rather cheat than do something less readable. this should happen after the output address read, but that means the pointer is off by 0x07 because of the window adjustment.
        current_chunk.first_block_end_address = bit_stream.current_pointer;
        current_chunk.decompression_output_address = static_cast<int unsigned>(bit_stream.ReadUnaligned(0x20));
        current_chunk.decompression_output_address = current_chunk.decompression_output_address & 0x00FFFFFF;
        current_chunk.first_block_num_of_bits = 0 - 0x38;

        int unsigned current_output_address = current_chunk.decompression_output_address;

        ByteTable byte_table;
    
        std::vector<int unsigned> block_addresses;
    
        {
            int unsigned address_to_write = 0;
            for (int i = 0; i < 0x18; i++)
            {
                bit_stream.last_requested_num_of_bits = 0 - bit_stream.current_window_distance;
                bit_stream.last_requested_num_of_bits &= 0x0007;
                bit_stream.ResetBuffer();
                const auto bytes_to_skip = static_cast<short unsigned>(bit_stream.ReadUnaligned(0x10));
                address_to_write = bit_stream.PrepareBlockAddress();
                block_addresses.push_back(address_to_write);
                address_to_write += bytes_to_skip;
                bit_stream.RequestBufferSeek(address_to_write);
            }
        }

        // this is t0 in the decompression routine.
        int unsigned dword_to_write = 0;

        // i'm still not sure the purpose of the routine using the registers, so i'll fill this info in later...

        // this should be t4.
        int unsigned bits_0_to_5 = 0;
        // this should be s3.
        int unsigned bits_6_to_10 = 0;
        // this should be t7.
        int unsigned bits_11_to_15 = 0;
        // this should be t6
        int unsigned bits_16_to_20 = 0;
        // this should be t5.
        int unsigned bits_21_to_25 = 0;
        // this should be t1.
        int unsigned bits_26_to_31 = 0;
        
        // landmark 9fc00790
        
        while (num_bytes_to_unpack > 0)
        {
            dword_to_write = 0;
            bits_0_to_5 = 0;
            bits_6_to_10 = 0;
            bits_11_to_15 = 0;
            bits_16_to_20 = 0;
            bits_21_to_25 = 0;
            bits_26_to_31 = 0;
            byte_stream.current_pointer = block_addresses[0];
            SeekNullByte(&byte_stream, &bit_stream);
            char unsigned byte_to_read = 0;
            byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
            bits_26_to_31 |= byte_to_read;
            // this should be s4.
            short unsigned control_command = 0;
            byte_stream.ReadOnDemand(&control_command, sizeof control_command, 0xFB0 + bits_26_to_31 * 2);
            
            if (bits_26_to_31 == 0)
            {
                // 80c
                byte_stream.current_pointer = block_addresses[0x01];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_0_to_5 |= byte_to_read;
                int unsigned address_to_read = 0xEB0 + bits_0_to_5 * 2;
                byte_stream.ReadOnDemand(&control_command, sizeof control_command, address_to_read);
                if (control_command == 0)
                {
                    auto bytes_to_read = static_cast<int unsigned>(bit_stream.ReadUnaligned(0x14));
                    bytes_to_read <<= 0x06;
                    dword_to_write |= bytes_to_read;
                    control_command = 0;
                }
                
            }
            else if (bits_26_to_31 == 1)
            {
                // 85c
                byte_stream.current_pointer = block_addresses[0x02];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_16_to_20 |= byte_to_read;
                int unsigned address_to_read = 0xF30 + bits_16_to_20 * 2;
                byte_stream.ReadOnDemand(&control_command, sizeof control_command, address_to_read);
                if (control_command == 0)
                {
                    byte_stream.current_pointer = block_addresses[0x15];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_21_to_25 |= byte_to_read;
                    byte_stream.current_pointer = block_addresses[0x09];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    int unsigned shifted_byte_to_read = byte_to_read << 0x08;
                    bits_0_to_5 |= shifted_byte_to_read;
                    byte_stream.current_pointer = block_addresses[0x0A];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_0_to_5 |= byte_to_read;
                }
            }
            else if (control_command == 0xFFFF)
            {
                // 8cc
                byte_stream.current_pointer = block_addresses[0x16];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                int unsigned address_to_read = 0xF70 + byte_to_read * 2;
                byte_stream.ReadOnDemand(&control_command, sizeof control_command, address_to_read);
                bits_21_to_25 |= byte_to_read;
                if (control_command == 0)
                {
                    byte_stream.current_pointer = block_addresses[0x15];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_16_to_20 |= byte_to_read;
                    byte_stream.current_pointer = block_addresses[0x9];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    int unsigned shifted_byte_to_read = 0;
                    shifted_byte_to_read = byte_to_read << 0x08;
                    bits_0_to_5 |= shifted_byte_to_read;
                    byte_stream.current_pointer = block_addresses[0xA];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_0_to_5 |= byte_to_read;
                }
                if (control_command == 0xC00)
                {
                    byte_stream.current_pointer = block_addresses[0x17];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_16_to_20 |= byte_to_read;
                    if (bits_16_to_20 >= 0x04)
                    {
                        byte_stream.current_pointer = block_addresses[0x09];
                        SeekNullByte(&byte_stream, &bit_stream);
                        byte_to_read = 0;
                        byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                        int unsigned shifted_byte_to_read = byte_to_read << 0x08;
                        bits_0_to_5 |= shifted_byte_to_read;
                        byte_stream.current_pointer = block_addresses[0x0A];
                        SeekNullByte(&byte_stream, &bit_stream);
                        byte_to_read = 0;
                        byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                        control_command = 0;
                        bits_0_to_5 |= byte_to_read;
                    }
                }
                if (control_command == 0xFFFF)
                {
                    dword_to_write |= bit_stream.ReadUnaligned(0x15);
                    control_command = 0;
                }
                
            }
            else if (control_command == 0)
            {
                // 9ac
                dword_to_write |= bit_stream.ReadUnaligned(0x1A);
                control_command = 0;
            }
            else if (control_command == 0x4000)
            {
                // 9c0
                dword_to_write = 0x03E00008;
                bits_26_to_31 = 0;
            }
            else if (control_command == 0x2000)
            {
                // 9d0
                dword_to_write = 0;
                bits_26_to_31 = 0;
            }

            // landmark 9fc009dc
            
            int flag_check = control_command & 0x0004;
            if (flag_check != 0)
            {
                // 9e8
                byte_stream.current_pointer = block_addresses[0x4];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                char unsigned table_byte = byte_table[byte_to_read];
                bits_16_to_20 |= table_byte;
                byte_table.ModifyTable(table_byte);
            }
            else
            {
                // a10
                flag_check = control_command & 0x0008;
                if (flag_check != 0)
                {
                    byte_stream.current_pointer = block_addresses[0x08];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_16_to_20 = byte_to_read;
                }
            }

            // landmark 9fc00a2c
            
            flag_check = control_command & 0x0001;
            if (flag_check != 0)
            {
                // a38
                flag_check = control_command & 0x0200;
                if (flag_check != 0)
                {
                    byte_stream.current_pointer = block_addresses[0x07];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    char unsigned table_byte = byte_table[byte_to_read + 0x20];
                    bits_21_to_25 |= table_byte;
                    byte_table.ModifyTable(table_byte, true);
                }
                else
                {
                    byte_stream.current_pointer = block_addresses[0x03];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    char unsigned table_byte = byte_table[byte_to_read];
                    bits_21_to_25 |= table_byte;
                    byte_table.ModifyTable(table_byte);
                }
            }
            else
            {
                // a94
                flag_check = control_command & 0x0002;
                if (flag_check != 0)
                {
                    byte_stream.current_pointer = block_addresses[0x08];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_21_to_25 |= byte_to_read;
                }
            }
            
            // landmark 9fc00ab0
            
            flag_check = control_command & 0x0010;
            if (flag_check != 0)
            {
                byte_stream.current_pointer = block_addresses[0x05];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                char unsigned table_byte = byte_table[byte_to_read];
                bits_11_to_15 |= table_byte;
                byte_table.ModifyTable(table_byte);
            }
            else
            {
                flag_check = control_command & 0x0020;
                if (flag_check != 0)
                {
                    byte_stream.current_pointer = block_addresses[0x08];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_11_to_15 |= byte_to_read;
                }
            }
            
            // landmark 9fc00b00

            flag_check = control_command & 0x0040;
            if (flag_check != 0)
            {
                byte_stream.current_pointer = block_addresses[0x06];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_6_to_10 = byte_to_read;
            }
            else
            {
                flag_check = control_command & 0x0080;
                if (flag_check != 0)
                {
                    byte_stream.current_pointer = block_addresses[0x08];
                    SeekNullByte(&byte_stream, &bit_stream);
                    byte_to_read = 0;
                    byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                    bits_6_to_10 = byte_to_read;
                }
            }

            // landmark 9fc00b34

            flag_check = control_command & 0x0100;
            if (flag_check != 0)
            {
                byte_stream.current_pointer = block_addresses[0x0B];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                int unsigned shifted_byte_to_read = byte_to_read << 0x14;
                dword_to_write |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x0C];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                shifted_byte_to_read = byte_to_read << 0x0E;
                dword_to_write |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x0D];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                shifted_byte_to_read = byte_to_read << 0x07;
                dword_to_write |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x0E];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                dword_to_write |= byte_to_read;
            }
            
            // landmark 9fc00b8c
            
            flag_check = control_command & 0x0c00;
            if (flag_check == 0x400)
            {
                // b9c
                byte_stream.current_pointer = block_addresses[0x0F];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                int unsigned shifted_byte_to_read = byte_to_read << 0x08;
                bits_0_to_5 |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x10];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_0_to_5 |= byte_to_read;
            }

            // landmark 9fc00bc0
            
            flag_check = control_command & 0x0c00;
            if (flag_check == 0x800)
            {
                byte_stream.current_pointer = block_addresses[0x11];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                int unsigned shifted_byte_to_read = byte_to_read << 0x08;
                bits_0_to_5 |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x12];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_0_to_5 |= byte_to_read;
            }

            // landmark 9fc00bf4

            flag_check = control_command & 0x0c00;
            if (flag_check == 0xc00)
            {
                // c04
                byte_stream.current_pointer = block_addresses[0x13];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                int unsigned shifted_byte_to_read = byte_to_read << 0x08;
                bits_0_to_5 |= shifted_byte_to_read;
                byte_stream.current_pointer = block_addresses[0x14];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_0_to_5 |= byte_to_read;
            }

            // landmark 9fc00c28

            flag_check = control_command & 0x1000;
            if (flag_check != 0)
            {
                byte_stream.current_pointer = block_addresses[0x08];
                SeekNullByte(&byte_stream, &bit_stream);
                byte_to_read = 0;
                byte_stream.ReadOnDemand(&byte_to_read, sizeof byte_to_read);
                bits_0_to_5 = byte_to_read;
            }

            // landmark 9fc00c44

            int unsigned shifted_bits_26_to_31 = bits_26_to_31 << 0x1A;
            dword_to_write |= shifted_bits_26_to_31;
            int unsigned shifted_bits_21_to_25 = bits_21_to_25 << 0x15;
            dword_to_write |= shifted_bits_21_to_25;
            int unsigned shifted_bits_16_to_20 = bits_16_to_20 << 0x10;
            dword_to_write |= shifted_bits_16_to_20;
            int unsigned shifted_bits_11_to_15 = bits_11_to_15 << 0x0B;
            dword_to_write |= shifted_bits_11_to_15;
            int unsigned shifted_bits_6_to_10 = bits_6_to_10 << 0x06;
            dword_to_write |= shifted_bits_6_to_10;
            dword_to_write |= bits_0_to_5;

            int unsigned address_check = current_output_address & 0x001f;
            if (address_check == 0)
            {
                // normally a dump to the screen to show decompressing progress would go here
                // don't need it so it's empty!
            }
            // write to a file or stream here!
            // std::cout << "DWORD UNPACK AT " << std::hex << current_output_address << " : " << std::hex << dword_to_write << std::endl;
            output_stream.seekp(current_output_address);
            output_stream.write(reinterpret_cast<char*>(&dword_to_write), sizeof dword_to_write);
            current_output_address += 0x4;
            num_bytes_to_unpack -= 0x4;
        }
    }
    bit_stream.close();
    byte_stream.close();
    output_stream.close();
    return 0;
}
