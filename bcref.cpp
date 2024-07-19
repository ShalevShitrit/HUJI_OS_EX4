#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/********** Constants **********/

#define FRAME_INIT_VALUE 0
#define ROOT_FRAME 0

/********** Typedefs **********/

typedef struct TraversalResult {
    word_t max_frame;
    word_t cyclic_max_leaf;
    word_t cyclic_max_frame;
    word_t cyclic_max_dist;
    word_t zeros_frame;

} TraversalResult;

/********** Internal Functions **********/

uint64_t concatenate_binary (uint64_t num1, uint64_t num2)
{
  return (num1 << OFFSET_WIDTH) + num2;
}

word_t cyclical_distance (word_t page_swapped_in, word_t p)
{
  int dif = page_swapped_in - p;
  int difference = dif > 0 ? dif : (-1 * dif);
  return (NUM_PAGES - difference) < difference ? (word_t) (NUM_PAGES - difference) : difference;
}



uint64_t get_p_addr (word_t frame, word_t offset)
{
  return (frame * PAGE_SIZE) + offset;
}


void reset_frame (word_t frame)
{
  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      PMwrite (get_p_addr (frame, i), FRAME_INIT_VALUE);
    }
}


void split_address_to_array (uint64_t virtualAddress, word_t *addresses)
{
  for (int i = 0; i <= TABLES_DEPTH; ++i)
    {
      addresses[i] = (word_t) virtualAddress >> (OFFSET_WIDTH * (TABLES_DEPTH - i));
      addresses[i] = addresses[i] & ((word_t) (1ll << OFFSET_WIDTH) - 1);
    }
}


uint64_t get_page (uint64_t address)
{
  return address >> OFFSET_WIDTH;
}

void leaf_operation (TraversalResult *result, word_t frame, uint64_t page, uint64_t path)
{
  result->max_frame = frame > result->max_frame ? frame : result->max_frame;
  word_t cyclic_dist = cyclical_distance ((word_t) page, (word_t) path);
  if (cyclic_dist > result->cyclic_max_dist)
    {
      result->cyclic_max_dist = cyclic_dist;
      result->cyclic_max_leaf = (word_t) path;
      result->cyclic_max_frame = frame;
    }
}


void tree_traversal (uint64_t parent,
                     uint64_t page,
                     TraversalResult *result,
                     word_t frame = ROOT_FRAME,
                     int depth = -1,
                     uint64_t path = 0)
{
  depth++; // represents the current tree depth
  if (depth == TABLES_DEPTH) // if the current frame is actually a leaf
    {
      leaf_operation (result, frame, page, path);
      return;
    }
  int zero_counter = 0;
  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      word_t child;
      PMread (get_p_addr (frame, i), &child);
      if (child == 0)
        {
          zero_counter++;
          continue;
        }
      tree_traversal (parent,
                      page,
                      result,
                      child,
                      depth,
                      concatenate_binary (path, i));
    }
  if (zero_counter == PAGE_SIZE
      && frame != (word_t)parent
      && frame < result->zeros_frame)
    {
      result->zeros_frame = frame;
    }
  result->max_frame = frame > result->max_frame ? frame : result->max_frame;
}

void unlink_frame (word_t frame_to_unlink, word_t curr_frame = ROOT_FRAME, int depth = -1)
{
  depth++;
  if (depth == TABLES_DEPTH) return;

  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      word_t child;
      PMread (get_p_addr (curr_frame, i), &child);

      if (child == 0) continue;
      if (child == frame_to_unlink)
        {
          PMwrite (get_p_addr (curr_frame, i), FRAME_INIT_VALUE);
          return;
        }
      unlink_frame (frame_to_unlink, child, depth);
    }
}

word_t find_available_frame (uint64_t parent, uint64_t page)
{
  TraversalResult traversal_result = {0, 0, 0, 0,
                                      NUM_FRAMES};
  tree_traversal (parent, page, &traversal_result);
  if (traversal_result.max_frame < NUM_FRAMES - 1)
    {
      return traversal_result.max_frame + 1;
    }
  if (traversal_result.zeros_frame != NUM_FRAMES)
    {
      unlink_frame (traversal_result.zeros_frame);
      return traversal_result.zeros_frame;
    }
  PMevict (traversal_result.cyclic_max_frame, traversal_result.cyclic_max_leaf);
  unlink_frame (traversal_result.cyclic_max_frame);
  return traversal_result.cyclic_max_frame;
}

word_t access_frame (uint64_t virtualAddress)
{
  word_t addresses[TABLES_DEPTH + 1];
  split_address_to_array (virtualAddress, addresses);
  uint64_t page = get_page (virtualAddress);
  word_t child = ROOT_FRAME;
  word_t parent = ROOT_FRAME;
  for (int i = 0; i < TABLES_DEPTH; ++i)
    {
      PMread (get_p_addr (parent, addresses[i]), &child);
      if (child == 0)
        {
          // find next frame
          child = find_available_frame (parent, page);
          if (i < TABLES_DEPTH-1) reset_frame (child);
          PMwrite (get_p_addr (parent, addresses[i]), child);
        }
      parent = child;
    }
  PMrestore (child, page);
  return (word_t) get_p_addr (child, addresses[TABLES_DEPTH]);
}



void VMinitialize ()
{
  reset_frame (ROOT_FRAME);
}

int VMread (uint64_t virtualAddress, word_t *value)
{
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) return 0;
  word_t frame_addr = access_frame (virtualAddress);
  PMread (frame_addr, value);
  return 1;
}

int VMwrite (uint64_t virtualAddress, word_t value)
{
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) return 0;
  word_t frame_addr = access_frame (virtualAddress);
  PMwrite (frame_addr, value);
  return 1;
}
