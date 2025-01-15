/*
 * aesd-circular-buffer.h
 *
 * Created on: March 1st, 2020
 * Author: Dan Walkes
 *
 * Description:
 * This header file defines the structure and operations of a circular buffer.
 * It provides the ability to store and retrieve a fixed number of write operations
 * in a buffer, suitable for scenarios like logging or buffer-based data management.
 */

#ifndef AESD_CIRCULAR_BUFFER_H
#define AESD_CIRCULAR_BUFFER_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stddef.h>  // size_t
#include <stdint.h>  // uintx_t
#include <stdbool.h>
#include <stdio.h>
#endif

// Maximum number of write operations supported by the AESD character device
#define AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED 10

// Enable debug output
// To enable debugging, define AESD_DEBUG in the build configuration
#define AESD_DEBUG 1  // Uncomment this line to enable debug output

// Debugging macro
#undef PDEBUG  // Undefine in case it's defined elsewhere
#ifdef AESD_DEBUG
    #ifdef __KERNEL__
        // Kernel space debugging output
        #define PDEBUG(fmt, args...) printk(KERN_DEBUG "aesdchar: " fmt, ## args)
    #else
        // User space debugging output
        #define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
    #endif
#else
    // No debugging output
    #define PDEBUG(fmt, args...) /* No-op */
#endif

/**
 * Structure representing a single buffer entry.
 * Each entry holds a pointer to the buffer data and its size.
 */
struct aesd_buffer_entry {
    const char *buffptr;  ///< Pointer to the buffer content
    size_t size;          ///< Size of the buffer content
};

/**
 * Structure representing the AESD circular buffer.
 * The buffer can hold a fixed number of write operations and maintains
 * the current read/write positions.
 */
struct aesd_circular_buffer {
    struct aesd_buffer_entry entry[AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];  ///< Array of buffer entries
    uint8_t in_offs;  ///< Index for the next write operation
    uint8_t out_offs; ///< Index for the next read operation
    bool full;        ///< Flag indicating whether the buffer is full
};

/**
 * Finds the buffer entry and byte offset corresponding to a given file position.
 * @param buffer The circular buffer
 * @param char_offset The file offset to search for
 * @param entry_offset_byte_rtn The byte offset within the found entry
 * @return A pointer to the buffer entry at the specified position
 */
extern struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(
    struct aesd_circular_buffer *buffer,
    size_t char_offset,
    size_t *entry_offset_byte_rtn
);

/**
 * Adds a new entry to the circular buffer.
 * @param buffer The circular buffer
 * @param add_entry The entry to add
 * @return A pointer to the newly added entry, or NULL if the buffer is full
 */
extern const char *aesd_circular_buffer_add_entry(
    struct aesd_circular_buffer *buffer,
    const struct aesd_buffer_entry *add_entry
);

/**
 * Initializes the circular buffer.
 * Resets the read/write positions and marks the buffer as empty.
 * @param buffer The circular buffer to initialize
 */
extern void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer);

/**
 * Macro for iterating over the entries of the circular buffer.
 * This is useful when freeing allocated memory for buffer entries.
 *
 * @param entryptr Pointer to a struct aesd_buffer_entry, will be set to the current entry
 * @param buffer The circular buffer
 * @param index A uint8_t index used to track the current position in the loop
 */
#define AESD_CIRCULAR_BUFFER_FOREACH(entryptr, buffer, index) \
    for (index = 0, entryptr = &(buffer)->entry[index]; \
         index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; \
         index++, entryptr = &(buffer)->entry[index])

#endif /* AESD_CIRCULAR_BUFFER_H */
