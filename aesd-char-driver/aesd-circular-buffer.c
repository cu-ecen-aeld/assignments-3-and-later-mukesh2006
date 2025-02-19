/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"


/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    uint8_t finish_pos, traverse_ix;
    
    if (buffer->in_offs > buffer->out_offs)
    {
        finish_pos = buffer->in_offs; 
    }
    else
    {
        finish_pos = buffer->in_offs + AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1; 
    }

    traverse_ix = buffer->out_offs;

    
    PDEBUG("\n LOG 1: aesd_circular_buffer_find_entry_offset_for_fpos:  buffer full: %d  buffer->in_offs: %d,  buffer->in_offs: %d, finish_pos: %d, traverse_ix:%d finish_pos: %d\n", buffer->full, buffer->in_offs, buffer->in_offs, finish_pos, traverse_ix, finish_pos);
    
    while (traverse_ix <= finish_pos)
    {
            PDEBUG("\n LOG 2: aesd_circular_buffer_find_entry_offset_for_fpos:  in while traverse_ix: %d finish_pos: %d \n", traverse_ix, finish_pos);
        if (buffer->entry[traverse_ix % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED].size > char_offset)
        {
            /* Char must be in this string entry */
            if (entry_offset_byte_rtn != NULL)
            {
                *entry_offset_byte_rtn = char_offset;
            }
             // b. Return the content (or partial content) related to the most recent 10 write commands, in the order they were received, on any read attempt.
            return &buffer->entry[traverse_ix % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
        }
        else
        {
            char_offset -= buffer->entry[traverse_ix % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED].size;
            traverse_ix++;
        }
    }
  
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char * aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char *ret_ptr = NULL;
    if ((buffer == NULL) || (add_entry == NULL))
    {
        return ret_ptr;
    }

    if (buffer->full)
    {
        ret_ptr = buffer->entry[buffer->in_offs].buffptr;
    }

    struct aesd_buffer_entry *tempEntry=&buffer->entry[buffer->in_offs];
    tempEntry->buffptr = add_entry->buffptr;
    tempEntry->size = add_entry->size;

    // correct the entry for the in_offs 
    buffer->in_offs=((buffer->in_offs+1)< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)?buffer->in_offs+1:0;
 
    PDEBUG("\n LOG 1: aesd_circular_buffer_add_entry:  buffer->in_offs: %d, buffer->out_offs: %d, buffer->full: %d  \n", buffer->in_offs, buffer->out_offs, buffer->full);
   

    // check if this entry will make the buffer full
    if (true==buffer->full)
    {
        PDEBUG("\n LOG 2: aesd_circular_buffer_add_entry:  buffer full: %d  \n", buffer->in_offs);        
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if (buffer->out_offs == buffer->in_offs)
    {
        // bufferEntry=&buffer[buffer->out_offs]; //todo: free the memory or let the user take care of it
        // buffer->out_offs=(buffer->out_offs+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        buffer->full = true;
        PDEBUG("\n LOG 3: aesd_circular_buffer_add_entry:  buffer full: %d  \n", buffer->full);
    }

    return ret_ptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
