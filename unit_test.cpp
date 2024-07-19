#include <iostream>
#include <bitset>
#include "MemoryConstants.h"
#include "PhysicalMemory.h"
uint64_t get_page(uint64_t virtual_address)
{
    return virtual_address >> OFFSET_WIDTH;
}
uint64_t concatenate_binary(uint64_t cur_path,uint64_t to_add)
{
    cur_path <<= OFFSET_WIDTH;
    cur_path |= to_add;
    return cur_path;
}
uint64_t translate_address(uint64_t virtualAddress)
{
    uint64_t cur_address = 0;
    uint64_t page = get_page(virtualAddress);
    std::cout << "page numbers is " <<page << std::endl;
    for (word_t level = 0; level < 2; level++)
    {
        // the offset in the current page


        word_t next_address;
        // getting the next page
        std::cout << ((virtualAddress >> ((2 - level) * OFFSET_WIDTH)) & ((1ll << OFFSET_WIDTH) - 1))<< std::endl;
        // deal with missing page
        if (next_address == 0)
        {
        }
        cur_address = next_address;
    }

    return cur_address * PAGE_SIZE + virtualAddress % PAGE_SIZE;
}
int main() {
    uint64_t cur_path = 0b1000;
    uint64_t to_add = 0b1000;
    int offset_width = 4;  // Number of bits in to_add

    uint64_t result = concatenate_binary(cur_path, to_add);

    std::cout << "Concatenated result: " << std::bitset<64>(result) << std::endl;
    translate_address(1302);
    return 0;
}//
// Created by omer1 on 18/07/2024.
//
