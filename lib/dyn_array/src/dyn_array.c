#include "../include/dyn_array.h"

// Flag values
// SHRUNK to indicate shrink_to_fit was called and size needs to be rehandled
// SORTED to track if the objects have been sorted by us (sorted is set by sort and unset by insert/push)
// these are just ideas
//typedef enum {NONE = 0x00, SHRUNK = 0x01, SORTED = 0x02, ALL = 0xFF} DYN_FLAGS;

struct dyn_array {
    // DYN_FLAGS flags;
    size_t capacity;
    size_t size;
    size_t data_size;
    void *array;
    void (*destructor)(void *);
};

// Supports 64bit+ size_t!
// Semi-arbitrary cap on contents. We'll run out of memory before this happens anyway.
// Allowing it to be externally set
#ifndef DYN_MAX_CAPACITY
    #define DYN_MAX_CAPACITY (((size_t)1) << ((sizeof(size_t) << 3) - 8))
#endif

// casts pointer and does arithmatic to get index of element
#define DYN_ARRAY_POSITION(dyn_array_ptr, idx) (((uint8_t*)dyn_array_ptr->array) + ((idx) * dyn_array_ptr->data_size))
// Gets the size (in bytes) of n dyn_array elements
#define DYN_SIZE_N_ELEMS(dyn_array_ptr, n) (dyn_array_ptr->data_size * (n))



// Modes of operation for dyn_shift
typedef enum {CREATE_GAP = 0x01, FILL_GAP = 0x02, FILL_GAP_DESTRUCT = 0x06} DYN_SHIFT_MODE;

// The core of any insert/remove operation, check the impl for details
bool dyn_shift(dyn_array_t *const dyn_array, const size_t position, const size_t count, const DYN_SHIFT_MODE mode, void *const data_location);


dyn_array_t *dyn_array_create(const size_t capacity, const size_t data_type_size, void (*destruct_func)(void *)) {
    if (data_type_size && capacity <= DYN_MAX_CAPACITY) {
        dyn_array_t *dyn_array =  (dyn_array_t *) malloc(sizeof(dyn_array_t));
        if (dyn_array) {
            // would have inf loop if requested size was between DYN_MAX_CAPACITY
            // and SIZE_MAX
            size_t actual_capacity = 16;
            while (capacity > actual_capacity) {actual_capacity <<= 1;}

            dyn_array->capacity = actual_capacity;
            dyn_array->size = 0;
            dyn_array->data_size = data_type_size;
            dyn_array->destructor = destruct_func;

            dyn_array->array = (uint8_t *) malloc(data_type_size * actual_capacity);
            if (dyn_array->array) {
                // other malloc worked, yay!
                // we're done?
                return dyn_array;
            }
            free(dyn_array);
        }
    }
    return NULL;
}

// Creates a dynamic array from a standard array
dyn_array_t *dyn_array_import(const void *const data, const size_t count, const size_t data_type_size, void (*destruct_func)(void *)) {
    // Oh boy I'm going to be lazy with this
    dyn_array_t *dyn_array = NULL;
    // literally could not give us an overlapping pointer unless they guessed it
    // I'd just do a memcpy here instead of dyn_shift, but the dyn_shift branch for this is
    // short. DYN_SHIFT CANNOT fail if create worked properly, but we'll cleanup if it did anyway
    if (data && (dyn_array = dyn_array_create(count, data_type_size, destruct_func))) {
        if (count && !dyn_shift(dyn_array, 0, count, CREATE_GAP, (void *const) data)) {
            dyn_array_destroy(dyn_array);
        }
    }
    return dyn_array;
}

// Consting the pointer to discourage monkeying with it.
// If they're smart they'll just use front() if they want to play with it
// Or just cast off the const and make the compiler allow it
// or memcpy the pointer's value to a non-const pointer (my favorite trick)
// Oh C...
const void *dyn_array_export(const dyn_array_t *const dyn_array) {
    return dyn_array_front(dyn_array);
}

void dyn_array_destroy(dyn_array_t *dyn_array) {
    if (dyn_array) {
        dyn_array_clear(dyn_array);
        free(dyn_array->array);
        free(dyn_array);
    }
}




void *dyn_array_front(const dyn_array_t *const dyn_array) {
    if (dyn_array && dyn_array->size) {
        // If array is null, well, this is ok, because it's null
        // but if array is broken, well, we can't help that
        // nor can we detect that, so I guess it's not an error
        return dyn_array->array;
    }
    return NULL;
}

bool dyn_array_push_front(dyn_array_t *const dyn_array, const void *const object) {
    //dyn_shift(dyn_array_t* dyn_array, size_t position, size_t count, DYN_SHIFT_MODE mode)
    // make sure to check pointer FIRST, shift does stuff to the structure that we don't want to have to undo
    return object && dyn_shift(dyn_array, 0, 1, CREATE_GAP, (void *const)object);
}

bool dyn_array_pop_front(dyn_array_t *const dyn_array) {
    // can this really ever fail? (other than NULL)
    // ... no. But we'll make it bool anyway. Allows for creative use.
    return dyn_shift(dyn_array, 0, 1, FILL_GAP_DESTRUCT, NULL);
}

bool dyn_array_extract_front(dyn_array_t *const dyn_array, void *const object) {
    // making them allocate room, makes us play nice with other things
    // as opposed to forcing them to do it our way like we know best
    // FILL_GAP can't have an error unless front doesn't exist,
    // which we're suppressing (for now at least)

    return object && dyn_array && dyn_array->size && dyn_shift(dyn_array, 0, 1, FILL_GAP, object);
}




void *dyn_array_back(const dyn_array_t *const dyn_array) {
    if (dyn_array && dyn_array->size) {
        return DYN_ARRAY_POSITION(dyn_array, dyn_array->size - 1);
    }
    return NULL;
}

bool dyn_array_push_back(dyn_array_t *const dyn_array, const void *const object) {
    return object && dyn_array && dyn_shift(dyn_array, dyn_array->size, 1, CREATE_GAP, (void *const)object);
}

bool dyn_array_pop_back(dyn_array_t *const dyn_array) {
    // MUST assert the size for the pop_back
    // if size is zero, it will rollover (rollunder?)
    // and then the size and count check will roll it back to zero during the check
    // and that will cause it to pass that check
    // and then things will probably break.
    // (a memmove with src = dest, I believe)
    // (that might not cause a disaster, but the size decrement will rollover the size variable)

    // NOTE: This has been fixed, but keeping it anyway because no one like rollover
    return dyn_array && dyn_array->size &&
           dyn_shift(dyn_array, dyn_array->size - 1, 1, FILL_GAP_DESTRUCT, NULL);
}

bool dyn_array_extract_back(dyn_array_t *const dyn_array, void *const object) {
    // FILL_GAP can't have an error unless front doesn't exist,
    // which we're suppressing (for now at least)
    return object && dyn_array && dyn_array->size &&
           dyn_shift(dyn_array, dyn_array->size - 1, 1, FILL_GAP, object);
}




void *dyn_array_at(const dyn_array_t *const dyn_array, const size_t index) {
    if (dyn_array && index < dyn_array->size) {
        return DYN_ARRAY_POSITION(dyn_array, index);
    }
    return NULL;
}

bool dyn_array_insert(dyn_array_t *const dyn_array, const size_t index, const void *const object) {
    // putting object at INDEX
    // so we shift a gap at INDEX
    return object && dyn_shift(dyn_array, index, 1, CREATE_GAP, (void *const)object);
}

bool dyn_array_erase(dyn_array_t *const dyn_array, const size_t index) {
    return dyn_shift(dyn_array, index, 1, FILL_GAP_DESTRUCT, NULL);
}

bool dyn_array_extract(dyn_array_t *const dyn_array, const size_t index, void *const object) {
    return dyn_array && object && dyn_array->size > index &&
           dyn_shift(dyn_array, index, 1, FILL_GAP, object);
}




void dyn_array_clear(dyn_array_t *const dyn_array) {
    if (dyn_array && dyn_array->size) {
        dyn_shift(dyn_array, 0, dyn_array->size, FILL_GAP_DESTRUCT, NULL);
    }
}

bool dyn_array_empty(const dyn_array_t *const dyn_array) {
    return dyn_array_size(dyn_array) == 0;
}

size_t dyn_array_size(const dyn_array_t *const dyn_array) {
    if (dyn_array) {
        return dyn_array->size;
    }
    return 0; // hmmmmm...
}

size_t dyn_array_capacity(const dyn_array_t *const dyn_array) {
    if (dyn_array) {
        return dyn_array->capacity;
    }
    return 0; // hmmmmm...
}

size_t dyn_array_data_size(const dyn_array_t *const dyn_array) {
    if (dyn_array) {
        return dyn_array->data_size;
    }
    return 0; // hmmmmm...
}



bool dyn_array_sort(dyn_array_t *const dyn_array, int (*compare)(const void *, const void *)) {
    // hah, turns out there's a quicksort in cstdlib.
    // and it works exactly like we want it to
    if (dyn_array && dyn_array->size && compare) {
        qsort(dyn_array->array, dyn_array->size, dyn_array->data_size, compare);
        return true;
    }
    return false;
}


bool dyn_array_insert_sorted(dyn_array_t *const dyn_array, const void *const object,
                             int (*compare)(const void *, const void *)) {
    if (dyn_array && compare && object) {
        size_t ordered_position = 0;
        if (dyn_array->size) {
            while (ordered_position < dyn_array->size &&
                    compare(object, DYN_ARRAY_POSITION(dyn_array, ordered_position)) > 0) {
                ++ordered_position;
            }
        }
        return dyn_shift(dyn_array, ordered_position, 1, CREATE_GAP, (void *const) object);
    }
    return false;
}


bool dyn_array_for_each(dyn_array_t *const dyn_array, void (*func)(void *const)) {
    if (dyn_array && dyn_array->array && func) {
        // So I just noticed we never check the data array ever
        // Which is both unsafe and potentially undefined behavior
        // Although we're the only ones that touch the pointer and we always validate it.
        // So it's questionable. We'll check it here.
        uint8_t *data_walker = (uint8_t *)dyn_array->array;
        for (size_t idx = 0; idx < dyn_array->size; ++idx, data_walker += dyn_array->data_size) {
            func((void *const) data_walker);
        }
        return true;
    }
    return false;
}


/*
    // No return value. It either goes or it doesn't. shrink_to_fit is more of a request
    void dyn_array_shrink_to_fit(dyn_array_t *const dyn_array) {
    if (dyn_array) {
        void *new_address = realloc(dyn_array->data, DYN_ARRAY_SIZE_N_ELEMS(dyn_array, dyn_array->size));
        if (new_address) {
            dyn_array->data = new_address;
            SET_FLAG(dyn_array,SHRUNK);
        }
    }
    }
*/


//
///
// BELOW HERE BE DRAGONS
///
//




// Checks to see if the object can handle an increase in size (and optionally increases capacity)
bool dyn_request_size_increase(dyn_array_t *const dyn_array, const size_t increment);

bool dyn_shift(dyn_array_t *const dyn_array, const size_t position, const size_t count, const  DYN_SHIFT_MODE mode, void *const data_location) {
    // Shifts contents. Mode flag controls what happens and how (duh?)
    // So, if you erase idx 2, you're filling the gap at position two
    // [A][X][B][C][D][E]
    //       <---- 1
    // [A][B][C][D][E][?]

    // inserting between idx 2 and 3 (between B and C) means you're moving everything from 3 down to make room
    // (can't use traditional insert lingo (new space is following idx) because push_front can't say position -1)
    // [A][B][C][D][E][?]
    //       \--F
    //        -----> 1
    // [A][B][?][C][D][E]
    // (and the insert, not done by this function)
    // [A][B][F][C][D][E]

    // the count parameter controls how many gaps are created/filled, and grows to the LEFT.
    // Zero will result in an error (until we have error codes(?))

    // !!! WARNING !!!
    // This function will handle capacity increases and will change the size to reflect the changes made
    // it WILL NOT destruct objects, because we don't know when/if that should happen without adding another flag
    // (better to do that here and have a flag?)
    // This function will check ranges and sizes, it should be safe to pass parameters to this without checking them first.
    // This enables us to have one function that handles error checking and will be more maintainable, hopefully

    // VERSION 2.0
    // We now take in a data_location pointer. When increasing capacity, we fill the gap from that data source
    // When extracting objects, we place them at that location
    // Deconstructing does not use or check that pointer
    // if the pointer was bad, the data structure wasn't changed (unless it was deconstruction)
    // can't const const the pointer because then we can't write to it on extract


    if (dyn_array && count) {
        // dyn good, count ok
        if (mode == CREATE_GAP && data_location) {
            // may or may not need to increase capacity.
            // We'll ask the capacity function if we can do it.
            // If we can, do it. If not... Too bad for the user.
            if (position <= dyn_array->size && dyn_request_size_increase(dyn_array, count)) {
                if (position != dyn_array->size) { // wasn't a gap at the end, we need to move data
                    memmove(DYN_ARRAY_POSITION(dyn_array, position + count),
                            DYN_ARRAY_POSITION(dyn_array, position),
                            DYN_SIZE_N_ELEMS(dyn_array, dyn_array->size - position));
                }
                memcpy(DYN_ARRAY_POSITION(dyn_array, position),
                       data_location,
                       dyn_array->data_size * count);
                dyn_array->size += count;
                return true;
            }
        } else if (mode & 0x02) { // mode = FILL_GAP_DESTRUCT || FILL_GAP
            // shrinking in size
            // nice and simple (?)

            // verify size and range
            if ((position + count) <= dyn_array->size) {
                if (mode == FILL_GAP_DESTRUCT) {
                    if (dyn_array->destructor) { // destruct AND HAVE DESTRUCTOR
                        uint8_t *arr_pos = DYN_ARRAY_POSITION(dyn_array, position);
                        for (size_t total = count; total; --total, arr_pos += dyn_array->data_size) {
                            dyn_array->destructor(arr_pos);
                        }
                    }
                } else {
                    if (data_location) {
                        memcpy(data_location,
                               DYN_ARRAY_POSITION(dyn_array, position),
                               dyn_array->data_size * count);
                    } else {
                        return false;
                    }
                }
                // pointer arithmatic on void pointers is illegal nowadays :C
                // GCC allows it for compatability, other provide it for GCC compatability. Way to implement a standard.
                // It should be cast to some sort of byte pointer, which is a pain. Hooray for macros
                if (position + count < dyn_array->size) {
                    // there's a actual gap, not just a hole to make at the end
                    memmove(DYN_ARRAY_POSITION(dyn_array, position),
                            DYN_ARRAY_POSITION(dyn_array, position + count),
                            DYN_SIZE_N_ELEMS(dyn_array, dyn_array->size - (position + count)));
                }
                // decrease the size and return
                dyn_array->size -= count;
                return true;
            }
        }
    }
    return false;
}

bool dyn_request_size_increase(dyn_array_t *const dyn_array, const size_t increment) {
    // check to see if the size can be increased by the increment
    // and increase capacity if need be
    // average case will be perfectly fine, single increment
    if (dyn_array) {
        //if (!ADDITION_MAY_OVERFLOW(dyn_array->size, increment)) {
        // increment is ok, but is the capacity?
        if (dyn_array->capacity >= (dyn_array->size + increment)) {
            // capacity is ok!
            return true;
        }
        // have to reallocate, is that even possible?
        size_t needed_size = dyn_array->size + increment;

        // INSERT SHRINK_TO_FIT CORRECTION HERE

        if (needed_size <= DYN_MAX_CAPACITY) {
            size_t new_capacity = dyn_array->capacity << 1;
            while (new_capacity < needed_size) {new_capacity <<= 1;}

            // we can theoretically hold this, check if we can allocate that
            //if (!MULTIPLY_MAY_OVERFLOW(new_capacity, dyn_array->data_size)) {
            // we won't overflow, so we can at least REQUEST this change
            void *new_array = realloc(dyn_array->array, new_capacity * dyn_array->data_size);
            if (new_array) {
                // success! Wasn't that easy?
                dyn_array->array = new_array;
                dyn_array->capacity = new_capacity;
                return true;
            }
        }
    }
    return false;
}
