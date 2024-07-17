//
// Created by omer1 on 16/07/2024.
//
#include "VirtualMemory.h"
#include <sys/types.h>
#include <cmath>
#include <algorithm>
#include "PhysicalMemory.h"
uint64_t find_unused_frame(uint64_t virtual_page, uint64_t protected_frame);
void
dfs(uint64_t parent_address, word_t& max_frame, uint64_t& max_distance_frame, uint64_t& max_distance
    , int level,
    uint64_t virtual_page, uint64_t protected_frame, uint64_t& empty_frame, bool& found_empty);
word_t
handle_page_fault(int level, uint64_t& cur_address, uint64_t& fault_address,
                  uint64_t protected_frame, uint64_t virtual_address);

void clearFrame(uint64_t base_address)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        PMwrite(base_address * PAGE_SIZE + i, 0);
    }
}

bool is_valid_address(uint64_t virtualAddress)
{
    if ((virtualAddress > VIRTUAL_MEMORY_SIZE) || (TABLES_DEPTH > NUM_FRAMES))
    {
        return false;
    }
    return true;
}

uint64_t find_unused_frame(uint64_t virtual_page, uint64_t protected_frame)
{
    word_t max_frame = 0;
    uint64_t max_distance_frame = 0;
    uint64_t max_distance = 0;
    uint64_t empty_frame = 0;
    bool found_empty = false;

    dfs(0, max_frame, max_distance_frame, max_distance, 0, virtual_page, protected_frame, empty_frame, found_empty);

    if (found_empty)
    {
        PMwrite(empty_frame, 0);
        return empty_frame;
    }

    if (max_frame + 1 < NUM_FRAMES)
    {
        return max_frame + 1;
    }

    PMevict(max_distance_frame, virtual_page);
    return max_distance_frame;
}

word_t
handle_page_fault(int level, uint64_t& fault_address,
                  uint64_t protected_frame, uint64_t virtual_address)
{
    uint64_t new_frame = find_unused_frame(virtual_address / PAGE_SIZE, protected_frame);

    if (level < TABLES_DEPTH - 1)
    {
        clearFrame(new_frame);
    }
    else
    {
        PMrestore(new_frame, virtual_address / PAGE_SIZE);
    }

    PMwrite(fault_address, new_frame);
    return new_frame;
}

uint64_t cyclic_distance(uint64_t page1, uint64_t page2)
{
    return std::min((int)NUM_PAGES - std::abs((int)page1 - (int)page2), std::abs((int)page1 - (int)page2));
}

// go over the childs of the current frame
// for each frame check if it's not empty, if it empty and not the protected frame, return it and zero the
// base location that points on it
// else check the cyclic distance from the page we want  min{NUM P AGES − |page swapped in − p|, |page swapped in − p|}
// if it greater than the current max, update the max weight and the maxw distance frame to the current frame.
// check the values of the frames seen, is greater than the curren max frame if it does update the max frame
// if the current frame is table in the tree, make it all zeros, else if it's leaf restore the a
// for each one read the frame it assigned to
// if the frame is not 0 - dfs on this child
void
dfs(uint64_t parent_address, word_t& max_frame, uint64_t& max_distance_frame, uint64_t& max_distance, int level,
    uint64_t virtual_page, uint64_t protected_frame, uint64_t& empty_frame, bool& found_empty)
{
    if (level == TABLES_DEPTH)
    {
        return;
    }

    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        word_t next_address;
        PMread(parent_address * PAGE_SIZE + i, &next_address);
        if (next_address != 0)
        {
            uint64_t cur_distance = cyclic_distance(next_address, virtual_page);
            if (cur_distance > max_distance)
            {
                max_distance_frame = next_address;
                max_distance = cur_distance;
            }
            if (next_address > max_frame)
            {
                max_frame = next_address;
            }
            dfs(next_address, max_frame, max_distance_frame, max_distance, level + 1, virtual_page, protected_frame,
                empty_frame, found_empty);
        }
        else if (!found_empty && parent_address != protected_frame)
        {
            empty_frame = parent_address * PAGE_SIZE + i;
            found_empty = true;
        }
    }
}

/*
 * Initialize the virtual memory
 */
void VMinitialize()
{
    clearFrame(0);
}

uint64_t translate_address(uint64_t virtualAddress)
{
    uint64_t cur_address = 0;
    for (word_t level = 0; level < TABLES_DEPTH; level++)
    {
        // the offset in the current page
        uint64_t trans_level =
            (virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH))
            & (PAGE_SIZE - 1);
        word_t next_address;
        // getting the next page
        PMread(cur_address * PAGE_SIZE + trans_level, &next_address);
        // deal with missing page
        if (next_address == 0)
        {
            // the frame where we found page fault
            uint64_t base_address = cur_address * PAGE_SIZE + trans_level;
            // trans level is the offset of inside the page where the fault happened
            next_address = handle_page_fault(level, base_address, cur_address,
                                             virtualAddress);
        }
        cur_address = next_address;
    }

    return cur_address * PAGE_SIZE + virtualAddress % PAGE_SIZE;
}

/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value)
{
    if (!is_valid_address(virtualAddress))
    {
        return 0;
    }
    uint64_t address = translate_address(virtualAddress);
    PMread(address, value);
    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (!is_valid_address(virtualAddress))
    {
        return 0;
    }
    uint64_t address = translate_address(virtualAddress);
    // printRam();
    PMwrite(address, value);
    return 1;
}
