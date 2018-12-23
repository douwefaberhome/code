#ifndef _CONVENIENT_H_
#define _CONVENIENT_H_

typedef unsigned long long u64;
typedef long long          s64;
typedef unsigned int       u32;
typedef int                s32;
typedef unsigned short     u16;
typedef short              s16;
typedef unsigned char      u8;
typedef char               s8;

typedef struct con_chunk
{
   con_chunk* next;
   u32 capacity;
   u8* buffer;
   u8* new_object_pointer;
} con_chunk;

typedef struct
{
   u32 chunk_default_size;
   con_chunk* head;
   con_chunk* current;
} con_arena;

void        con_arena_setup(con_arena* heap, u32 chunk_default_size = 0);
void        con_arena_teardown(con_arena* heap);
#define     con_arena_push(heap, type) (type *)con_arena_push_(heap, sizeof (type))
#define     con_arena_push_array(heap, count, type) (type *)xyheap_push_(heap, (count) * sizeof (type) )
void*       con_arena_push_(con_arena* heap, u32 object_size);
void        con_arena_reset(con_arena* heap);

typedef struct
{
   char* data;
   u32 size;
   u32 capacity;
} con_string;

// we will use the term nts for a c style string or null terminated string in arguments
// we will use the term string for con_string arguments

con_string con_string_from_nts(char* nts_string);
char*      con_string_to_nts(con_string string);

con_string string_skip_white_space(con_string string);
s32        string_match(con_string string_a, con_string string_b);

con_string string_part(con_string string, u32 start, u32 size);

#endif   // _CONVENIENT_H_


////////////////////////////////
///// Implementation starts here

#ifdef CONVENIENT_IMPLEMENTATION

#include <string.h>

static const int XY_INITIALHEAPCHUNKSIZE = 1024 * 1024;  // 1 mb

void con_arena_setup(con_arena* heap, u32 size)
{
   if(heap == NULL)
      return;
   // Only create chunks of 1 MB and larger
   heap->chunk_default_size = (size > XY_INITIALHEAPCHUNKSIZE) ? size : XY_INITIALHEAPCHUNKSIZE;

#if _DEBUG
   // In debug we can use smaller sizes for testing the xyHEAP for memory leaks etc.
   heap->chunk_default_size = (size != 0) ? size : XY_INITIALHEAPCHUNKSIZE;
#endif

   heap->current = heap->head = NULL;
}

void con_arena_teardown(con_arena* heap)
{
   if(heap == NULL)
      return;

   con_chunk* p = heap->head;
   while(p->next != 0)
   {
      con_chunk* pmem = p->next;
      free(p->buffer);
      p = pmem;
   }
}

void* con_arena_push_(con_arena* heap, u32 object_size)
{
   // TODO(df): we keep skipping to next chunks without looking at the previously allocated chunks.
   //          there could be enough capacity there for storing this object.

   if(!heap || (object_size < 1))
      return NULL;

   if(!heap->head)
   {  // Create the first chunk
      heap->head = heap->current = (con_chunk*)calloc(1, sizeof (con_chunk));
      if(heap->head == NULL)
      {
         free(heap);
         return NULL;
      }
      u32 block_size = (heap->chunk_default_size > object_size) ? heap->chunk_default_size : object_size;
      heap->head->buffer   = (unsigned char*)calloc(block_size, sizeof (char));
      heap->head->capacity = block_size;
      heap->head->new_object_pointer = heap->head->buffer;
   }
   else
   {
      // Maybe we already allocated a chunk and the heap was reset
      bool need_new_chunk = true;
      do
      {
         s32 chunk_capacity_remaining = heap->current->capacity - (heap->current->new_object_pointer - heap->current->buffer);
         assert(chunk_capacity_remaining > -1);

         if(object_size < heap->current->capacity)
         {
            need_new_chunk = false; // This chunk is big enough
            break;
         }
         heap->current = heap->current->next;
      }while(heap->current->next != NULL);

      if(need_new_chunk)
      {
         heap->current->next = (con_chunk*)calloc(1, sizeof (con_chunk));
         heap->current = heap->current->next;

         u32 block_size = (heap->chunk_default_size > object_size) ? heap->chunk_default_size : object_size;
         heap->current->buffer = (unsigned char*)calloc(block_size, sizeof (char));
         heap->current->capacity = block_size;
         heap->current->new_object_pointer = heap->current->buffer;
      }
   }

   void* object_pointer = heap->current->new_object_pointer;
   heap->current->new_object_pointer += object_size;
   return object_pointer;
}

void con_arena_reset(con_arena* heap)
{
   con_chunk* p = heap->head;
   if(!p)
   {
      return;  // Nothing to do. No chunks allocated
   }

   // First reset the newItemPointer in every chunk
   do
   {
      con_chunk* pmem = p->next;
      p->new_object_pointer = p->buffer;
      p = pmem;
   }while(p->next != NULL);

   heap->current = heap->head;
}


/*
 * String implementation
 */


// static (local) functions 
static bool is_white_space(char c)
{
   return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' || c == 0xEF || c == 0xBB || c == 0xBF);
}


// public interface

con_string con_string_from_nts(char* nts_string)
{
   con_string result;
   result.data = nts_string;
   result.capacity = result.size = strlen(nts_string);
   return result;
}

char* con_string_to_nts(con_string string)
{
   // Terminate the string with null
   string.data[string.size] = '\0';
   return string.data;
}

con_string con_string_skip_white_space(con_string string)
{
   for(;;)
   {
      if(is_white_space(*string.data))
      {
         ++ string.data;
         // size and capacity of the string we are going to return have to be decreased here
         -- string.capacity;
         -- string.size;
      }
      else
         break;
   }
   return string;
}

s32 con_string_match(con_string string_a, con_string string_b)
{
   if(string_a.size != string_b.size)
      return 1;

   for(u32 i = 0; i < string_a.size; ++ i)
   {
      if(string_a.data[i] != string_b.data[i])
         return 2;
   }
   return 0;
}

con_string con_string_part(con_string string, u32 start, u32 size)
{
   string.data += start;
   string.size -= start - size;
   return string;
}


#endif   // XY_DEFINE

