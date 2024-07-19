//
// Created by omer1 on 16/07/2024.
//
#include "VirtualMemory.h"
#include <sys/types.h>
#include <cmath>
#include <algorithm>
#include "PhysicalMemory.h"

// ***************************** declarations  *****************************//
typedef struct dfsRes
{
    word_t max_frame;
    word_t max_cyclic_ditance;
    word_t max_cyclic_frame;
    word_t max_cyclic_page;
    word_t empty_frame;
} dfsRes;

bool is_valid_address(uint64_t virtualAddress);
void clearFrame(uint64_t base_address);
void leaf_act(dfsRes& results, word_t frame, uint64_t page, uint64_t path);
word_t cyclic_distance(uint64_t p, uint64_t swapped_in);
uint64_t get_page(uint64_t virtual_address);
word_t handle_page_fault();
uint64_t concatenate_binary(uint64_t cur_path, uint64_t to_add);
void dfs(uint64_t parent, uint64_t target_page, dfsRes& results, word_t frame,int level, uint64_t path);
void delete_from_parent (word_t frame_to_unlink, word_t curr_frame = 0, int depth = -1);


// ***************************** function implementations   *****************************//
void leaf_act(dfsRes& results, word_t frame, uint64_t target_page, uint64_t path)
{
    results.max_frame = frame > results.max_frame ? frame : results.max_frame;
    word_t cyclic_distane = cyclic_distance(path, target_page);
    if (cyclic_distane < results.max_cyclic_ditance)
    {
        results.max_cyclic_ditance = cyclic_distane;
        results.max_cyclic_frame = frame;
        results.max_cyclic_page = static_cast<word_t>(path);
    }
}

uint64_t concatenate_binary(uint64_t cur_path, uint64_t to_add)
{
    cur_path <<= OFFSET_WIDTH;
    cur_path |= to_add;
    return cur_path;
}

word_t cyclic_distance(uint64_t p, uint64_t swapped_in)
{
    int diff = swapped_in - p;
    diff = (swapped_in - p < 0) ? diff * -1 : diff;
    return (NUM_PAGES - diff < diff) ? NUM_PAGES - diff : diff;
}

bool is_valid_address(uint64_t virtualAddress)
{
    if ((virtualAddress > VIRTUAL_MEMORY_SIZE) || (TABLES_DEPTH > NUM_FRAMES))
    {
        return false;
    }
    return true;
}

void clearFrame(uint64_t base_address)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        PMwrite(base_address * PAGE_SIZE + i, 0);
    }
}

uint64_t get_page(uint64_t virtual_address)
{
    return virtual_address >> OFFSET_WIDTH;
}
void dfs(uint64_t parent, uint64_t target_page, dfsRes& results, word_t frame,int level, uint64_t path)
{
    if(level == TABLES_DEPTH)
    {
        leaf_act(results,frame,target_page,path);
        return;
    }
    int empty_childs = 0;
    for(int i = 0 ;i < PAGE_SIZE;i++)
    {
        word_t child;
        PMread(frame * PAGE_SIZE + i,&child);
        if(child == 0)
        {
            empty_childs ++;
            continue;
        }
        dfs(parent,target_page,results,child,level + 1,concatenate_binary(path,i));
    }
    if(empty_childs == PAGE_SIZE && frame != parent && frame < results.empty_frame)
    {
        results.empty_frame = frame;
    }
    if(frame > results.max_frame)
    {
        results.max_frame = frame;
    }
}
word_t handle_page_fault(uint64_t current_address,uint64_t target_page)
{
    dfsRes results= {0,0,0,0,NUM_FRAMES};
    dfs(current_address,target_page,results,0,0,0);
    if(results.empty_frame != NUM_FRAMES )
    {
        delete_from_parent(results.empty_frame);
        return results.empty_frame ;
    }
    if(results.max_frame < NUM_FRAMES - 1)
    {
        return results.max_frame + 1;
    }
    PMevict(results.max_cyclic_frame,results.max_cyclic_page);
    delete_from_parent(results.max_cyclic_frame);
    return results.max_cyclic_frame;

}

void delete_from_parent (word_t frame_to_unlink, word_t curr_frame , int depth )
{
    depth++;
    if (depth == TABLES_DEPTH) return;

    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        word_t child;
        PMread (curr_frame * PAGE_SIZE +  i, &child);

        if (child == 0) continue;
        if (child == frame_to_unlink)
        {
            PMwrite (curr_frame * PAGE_SIZE +  i, 0);
            return;
        }
        delete_from_parent (frame_to_unlink, child, depth);
    }
}

//********************************* EX FUNCTIONS *********************************//
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
    uint64_t page = get_page(virtualAddress);
    for (word_t level = 0; level < TABLES_DEPTH; level++)
    {
        // the offset in the current page

        uint64_t trans_level =
            (virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH)) & ((1ll << OFFSET_WIDTH) - 1);
        word_t next_address;
        // getting the next page
        PMread(cur_address * PAGE_SIZE + trans_level, &next_address);
        // deal with missing page
        if (next_address == 0)
        {
            next_address = handle_page_fault(cur_address,page);
        }
        cur_address = next_address;
    }

    return cur_address * PAGE_SIZE + virtualAddress % PAGE_SIZE;
}


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
