/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines. Remove this comment and provide
 * a summary of your allocator's design here.
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

s_block_ptr first = NULL;

/* Your final implementation should comment out this macro. */
//#define MM_USE_STUBS

void* mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    if (first == NULL){
        first = sbrk(0);
        if (sbrk(size + BLOCK_SIZE) == (void*)-1) {
            first = NULL;
            return NULL;
        }
        first->prev = NULL;
        first->next = NULL;
        first->size = size;
        first->is_free = 0;                                //It is allocated now.
        first->ptr = &(first->data[0]);
        memset(first->ptr, 0, first->size);
        return first->ptr;
    }

    s_block_ptr current = first;
    s_block_ptr last = current;
    while (current){
        if (current->is_free && current->size >= size){
            current->is_free = 0;
            if (current->size - size >= BLOCK_SIZE){
                last = current->ptr + size;
                last->next = current->next;
                if (current->next != NULL)
                    current->next->prev = last;
                current->next = last;
                last->prev = current;
                last->size = current->size - size - BLOCK_SIZE;
                last->is_free = 1;
                last->ptr = &(last->data[0]);
            }
            current->size = size;
            memset(current->ptr, 0, current->size);
            return current->ptr;
        }
        last = current;
        current = current->next;
    }

    if (last->ptr + last->size > sbrk(0) - size - BLOCK_SIZE){
        if (sbrk(size + BLOCK_SIZE - (sbrk(0) - last->ptr - last->size)) == (void*)-1)
            return NULL;
    }

    current = last->ptr + last->size;
    last->next = current;
    current->prev = last;
    current->next = NULL;
    current->is_free = 0;
    current->size = size;
    current->ptr = &(current->data[0]);
    memset(current->ptr, 0, current->size);
    return current->ptr;

}

void* mm_realloc(void* ptr, size_t size)
{
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    if (ptr == NULL)
        return mm_malloc(size);
    s_block_ptr current = find_head(ptr);
    void* target = mm_malloc(size);
    if (current != NULL && target != NULL){
        if (current->is_free) {
            mm_free(target);
            return NULL;
        }
        memcpy(target, ptr, (current->size > size)?size:current->size);
        mm_free(ptr);
        return target;
    }
    return NULL;
}

void mm_free(void* ptr)
{
    if (ptr == NULL)
        return;
    s_block_ptr target = find_head(ptr);
    if (target == NULL)
        return;
    if (target->is_free)
        return;
    target->is_free = 1;
    if (target->prev != NULL && target->prev->is_free){
        target->prev->next = target->next;
        if (target->next != NULL)
            target->next->prev = target->prev;
        target->prev->size = target->ptr - target->prev->ptr + target->size;
        if (target->next != NULL && target->next->is_free) {
            target->prev->next = target->next->next;
            if (target->next->next != NULL)
                target->next->next->prev = target->prev;
            target->prev->size = target->next->ptr - target->prev->ptr + target->next->size;
        }
        return;
    } else if (target->next != NULL && target->next->is_free) {
        target->is_free = 1;
        target->size = target->next->ptr - target->ptr + target->next->size;
        target->next = target->next->next;
        if (target->next != NULL)
            target->next->prev = target;
        return;
    }
}

s_block_ptr find_head(void *ptr) {
    s_block_ptr iter = NULL;
    if (first == NULL)
        return NULL;
    if (ptr < first->ptr || ptr > sbrk(0))
        return NULL;
    iter = (s_block_ptr) (ptr - (void*)BLOCK_SIZE);
    if (iter->ptr != &(iter->data[0]))
        return NULL;
    return iter;
}
