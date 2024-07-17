#include "VirtualMemory.h"
#include "PhysicalMemory.h"

// Clears a frame by setting all its entries to 0
void clearFrame(uint64_t base_address) {
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        PMwrite(base_address * PAGE_SIZE + i, 0);
    }
}

// Checks if the given virtual address is valid
bool is_valid_address(uint64_t virtualAddress) {
    // Check if the address is within the valid range and if the table depth is within the number of frames
    if ((virtualAddress >= VIRTUAL_MEMORY_SIZE) || (TABLES_DEPTH > NUM_FRAMES)) {
        return false;
    }
    return true;
}

// DFS to find an empty frame, the maximum frame, and a frame to evict if necessary
void dfs(uint64_t parent_address, uint64_t base_address, int &max_frame, word_t &empty_frame, word_t &frame_to_evict) {
    for (uint64_t i = 0; i < PAGE_SIZE; i++) { // Iterate over each entry in the frame
        word_t value;
        PMread(base_address + i, &value); // Read the value at the current entry
        if (value != 0) { // If the entry is not empty
            max_frame = std::max(max_frame, static_cast<int>(value)); // Update the maximum frame number
            dfs(base_address, value * PAGE_SIZE, max_frame, empty_frame, frame_to_evict); // Recursively call DFS on the next frame
        } else if (empty_frame == -1) { // If an empty frame has not been found yet
            empty_frame = base_address / PAGE_SIZE; // Mark this frame as empty
        }
    }

    // Check if the current frame is empty
    bool is_empty = true;
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        word_t value;
        PMread(base_address + i, &value); // Read the value at the current entry
        if (value != 0) { // If any entry is not empty
            is_empty = false; // Mark the frame as not empty
            break;
        }
    }

    if (is_empty && empty_frame == -1) { // If the frame is empty and no empty frame has been found yet
        empty_frame = base_address / PAGE_SIZE; // Mark this frame as empty
    }

    // Update the parent to point to zero if an empty frame is found
    if (empty_frame != -1 && parent_address != base_address) {
        uint64_t parent_index = base_address / PAGE_SIZE % PAGE_SIZE;
        PMwrite(parent_address + parent_index, 0); // Write 0 to the parent entry
    }

    // If no empty frame is found, choose a frame to evict
    if (empty_frame == -1 && frame_to_evict == -1) {
        frame_to_evict = base_address / PAGE_SIZE; // Mark this frame as the one to evict
    }
}

// Finds a free frame by calling DFS
uint64_t find_frame() {
    int max_frame = 0;
    word_t empty_frame = -1;
    word_t frame_to_evict = -1;

    dfs(0, 0, max_frame, empty_frame, frame_to_evict); // Perform DFS starting from the root

    if (empty_frame != -1) {
        return empty_frame; // Return the empty frame if found
    }

    if (frame_to_evict != -1) {
        clearFrame(frame_to_evict); // Clear the frame to evict
        return frame_to_evict; // Return the frame to evict
    }

    return max_frame + 1; // Return the next frame if no empty frame or frame to evict was found
}

// Translates a virtual address to a physical address
uint64_t translate_address(uint64_t virtualAddress) {
    word_t cur_address = 0; // Start from the root frame

    for (word_t level = 0; level < TABLES_DEPTH; level++) { // Traverse the page table hierarchy
        uint64_t trans_level = (virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH)) & (PAGE_SIZE - 1); // Calculate the index for the current level
        word_t next_address;
        PMread(cur_address * PAGE_SIZE + trans_level, &next_address); // Read the address of the next frame

        if (next_address == 0) { // If the next frame is not allocated
            uint64_t new_frame = find_frame(); // Find a free frame
            PMwrite(cur_address * PAGE_SIZE + trans_level, new_frame); // Write the new frame to the current entry
            clearFrame(new_frame); // Clear the new frame
            next_address = new_frame; // Set the next address to the new frame
        }

        cur_address = next_address; // Move to the next frame
    }

    return cur_address * PAGE_SIZE + (virtualAddress % PAGE_SIZE); // Return the physical address
}

/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value) {
    if (!is_valid_address(virtualAddress)) { // Check if the address is valid
        return 0;
    }
    uint64_t address = translate_address(virtualAddress); // Translate the virtual address to a physical address
    PMread(address, value); // Read the value from the physical address
    return 1;
}

/* writes a word to the given virtual address
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
    if (!is_valid_address(virtualAddress)) { // Check if the address is valid
        return 0;
    }
    uint64_t address = translate_address(virtualAddress); // Translate the virtual address to a physical address
    PMwrite(address, value); // Write the value to the physical address
    return 1;
}
