#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// defenitions
#define INITIAL_FRAME_VALUE 0
#define START_FRAME 0
typedef struct {
    word_t max_frame_taken;
    word_t max_cyclic_page;
    word_t max_cyclic_frame;
    word_t max_cyclic_dist;
    word_t empty_frame;
} dfsRes;
bool is_address_legal(uint64_t virtual_address);
uint64_t combine_binary(uint64_t cur_path, uint64_t to_add);
word_t calculate_cyclic_distance(word_t in_page, word_t ref_page);
void clear_frame(word_t frame);
void process_leaf(dfsRes* result, word_t frame, uint64_t page, uint64_t path);
void dfs(uint64_t parent, uint64_t page, dfsRes* result, word_t frame , int level , uint64_t path );
void remove_frame(word_t target_frame, word_t current_frame, int level );
word_t locate_available_frame(uint64_t parent, uint64_t page);
word_t resolve_frame(uint64_t virtualAddress);


bool is_address_legal(uint64_t virtual_address)
{
    if(virtual_address >= VIRTUAL_MEMORY_SIZE || TABLES_DEPTH == NUM_FRAMES)
    {
        return false;
    }
    return true;
}
// Helper Functions
uint64_t combine_binary(uint64_t cur_path, uint64_t to_add)
{
    return (cur_path << OFFSET_WIDTH) + to_add;
}

word_t calculate_cyclic_distance(word_t in_page, word_t ref_page)
{
    int diff = in_page - ref_page;
    int abs_diff = diff > 0 ? diff : -diff;
    if (NUM_PAGES - abs_diff < abs_diff)
    {
        return (word_t)(NUM_PAGES - abs_diff);
    }
    return  abs_diff;
}

void clear_frame(word_t frame)
{
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frame * PAGE_SIZE + i, INITIAL_FRAME_VALUE);
    }
}

void process_leaf(dfsRes* result, word_t frame, uint64_t page, uint64_t path)
{
    if (frame > result->max_frame_taken)
        result->max_frame_taken = frame;

    word_t cyclic_dist = calculate_cyclic_distance((word_t)page, (word_t)path);
    if (cyclic_dist > result->max_cyclic_dist)
    {
        result->max_cyclic_dist = cyclic_dist;
        result->max_cyclic_page = (word_t)path;
        result->max_cyclic_frame = frame;
    }
}

void dfs(uint64_t parent, uint64_t page, dfsRes* result, word_t frame , int level , uint64_t path )
{
    if (level == TABLES_DEPTH)
    {
        process_leaf(result, frame, page, path);
        return;
    }

    int empty_childs = 0;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        word_t child;
        PMread(frame * PAGE_SIZE + i, &child);
        if (child == 0)
        {
            empty_childs++;
            continue;
        }
        dfs(parent, page, result, child, level + 1, combine_binary(path, i));
    }

    if (empty_childs == PAGE_SIZE && frame != (word_t)parent && frame < result->empty_frame)
    {
        result->empty_frame = frame;
    }

    if (frame > result->max_frame_taken)
        result->max_frame_taken = frame;
}

void remove_frame(word_t target_frame, word_t current_frame, int level )
{
    if (level == TABLES_DEPTH) return;

    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        word_t child;
        PMread(current_frame * PAGE_SIZE + i, &child);

        if (child == 0) continue;
        if (child == target_frame)
        {
            PMwrite(current_frame * PAGE_SIZE + i, INITIAL_FRAME_VALUE);
            return;
        }
        remove_frame(target_frame, child, level + 1);
    }
}

word_t locate_available_frame(uint64_t parent, uint64_t page)
{
    dfsRes result = {0, 0, 0, 0, NUM_FRAMES};
    dfs(parent, page, &result,0,0,0);

    if (result.empty_frame != NUM_FRAMES)
    {
        remove_frame(result.empty_frame,START_FRAME,0);
        return result.empty_frame;
    }
    if (result.max_frame_taken < NUM_FRAMES - 1)
    {
        return result.max_frame_taken + 1;
    }
    PMevict(result.max_cyclic_frame, result.max_cyclic_page);
    remove_frame(result.max_cyclic_frame,START_FRAME,0);
    return result.max_cyclic_frame;
}

word_t resolve_frame(uint64_t virtualAddress)
{
    word_t next_address = 0;
    word_t current_address = 0;
    uint64_t page = virtualAddress >> OFFSET_WIDTH;

    for (int level = 0; level < TABLES_DEPTH; level++)
    {
        uint64_t offset = (virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH)) & ((1ll << OFFSET_WIDTH) - 1);
        PMread(current_address * PAGE_SIZE + offset, &next_address);
        if (next_address == 0)
        {
            next_address = locate_available_frame(current_address, page);
            if (level < TABLES_DEPTH - 1) clear_frame(next_address);
            PMwrite(current_address * PAGE_SIZE + offset, next_address);
        }
        current_address = next_address;
    }
    PMrestore(next_address, page);
    return current_address * PAGE_SIZE + virtualAddress % PAGE_SIZE;
}

void VMinitialize()
{
    clear_frame(START_FRAME);
}

int VMread(uint64_t virtualAddress, word_t* value)
{
    if(!is_address_legal(virtualAddress))
    {
        return 0;
    }
    word_t frame_addr = resolve_frame(virtualAddress);
    PMread(frame_addr, value);
    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value)
{
    if(!is_address_legal(virtualAddress))
    {
        return 0;
    }
    word_t frame_addr = resolve_frame(virtualAddress);
    PMwrite(frame_addr, value);
    return 1;
}
