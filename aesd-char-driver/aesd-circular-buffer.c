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
#include<stdio.h>
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
    /**
    * TODO: implement per description
    */
   //Should use the in_offs and out_offs indices to determine the 
    size_t val = buffer->entry[buffer->in_offs].size;
    //printf("Value %d \n",buffer->in_offs);
    size_t total_buf_size = 0;
    for(int i= 0; i<10; i++)
    {
        total_buf_size += buffer->entry[i].size;
       //printf("%s\n",buffer->entry[i].buffptr);
    }
        
    //printf("total size: %ld\n", total_buf_size);
    if(char_offset >= total_buf_size)
        return NULL;
    if(char_offset < val)
    {
        *entry_offset_byte_rtn = char_offset;
        return &(buffer->entry[buffer->in_offs]);
    }
    else
    {
        int i = buffer->in_offs+1;
        //int flag = 0;
        while((val + buffer->entry[i].size) <= char_offset)
        {
            val += buffer->entry[i].size;
            i = (i+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
            //flag = 1;
        }
        //printf("Inside loop: entry val: %ld\n",val);
        *entry_offset_byte_rtn = char_offset - val;

        /*if(char_offset == total_buf_size-1)
        {
            *entry_offset_byte_rtn = 7;
            return &(buffer->entry[9]);
        }*/
        
        //if(flag)
            //return &(buffer->entry[i-1]);
        //else
            return &(buffer->entry[i]);
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
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
   /*we will check if the buffer is full by incrementing a counter and once the buffer is full, we will increment
   both the out_offs and in_offs thus overwriting the existing data*/
   static int count = 0;
   if(!buffer->full)
   {
       buffer->entry[buffer->in_offs] = *add_entry; 
       buffer->in_offs = ((buffer->in_offs)+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
       count++;
       if(count == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        buffer->full = true;
   }
   else
   {
       buffer->entry[buffer->in_offs] = *add_entry; 
       buffer->in_offs = ((buffer->in_offs)+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
       buffer->out_offs = ((buffer->out_offs)+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
   }
   //printf("In: %d Out: %d\n",buffer->in_offs, buffer->out_offs);

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
