//
// Created by omer1 on 16/07/2024.
//
#include "VirtualMemory.h"

#include <sys/types.h>

#include "PhysicalMemory.h"
uint64_t
handle_page_fault (int level, uint64_t &cur_address, uint64_t &target_frame,
                   uint64_t protected_frame);

void clearFrame (uint64_t base_address)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
  {
    PMwrite (base_address * PAGE_SIZE + i, 0);
  }
}

bool is_valid_address (uint64_t virtualAddress)
{
  if ((virtualAddress > VIRTUAL_MEMORY_SIZE) || (TABLES_DEPTH > NUM_FRAMES))
  {
    return false;
  }
  return true;
}

uint64_t
handle_page_fault (int level, uint64_t &cur_address, uint64_t &target_frame,
                   uint64_t protected_frame)
{
  uint64_t max_distnce = 0;
  uint64_t cur_distance = 0;

}

void
dfs (uint64_t parent_address, uint64_t base_address, word_t &max_frame, uint64_t max_distance, int cur_distance, int level,
     bool &found_empty)
{
  if (level == TABLES_DEPTH)
  {
    if (cur_distance > max_distance)
    {
      max_distance = cur_distance;
      max_frame = parent_address;
    }
    return;
  }

  
}

/*
 * Initialize the virtual memory
 */
void VMinitialize ()
{
  clearFrame (0);
}

uint64_t translate_address (uint64_t virtualAddress)
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
    PMread (cur_address * PAGE_SIZE + trans_level, &next_address);
    // deal with missing page
    if (next_address == 0)
    {
      // the frame where we found page fault
      uint64_t base_address = cur_address * PAGE_SIZE + trans_level;
      // trans level is the offset of inside the page where the fault happened
      uint64_t next_address = handle_page_fault (level, cur_address, base_address,
                                                 cur_address);
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
int VMread (uint64_t virtualAddress, word_t *value)
{
  if (!is_valid_address (virtualAddress))
  {
    return 0;
  }
  uint64_t address = translate_address (virtualAddress);
  PMread (address, value);
  return 1;
}
