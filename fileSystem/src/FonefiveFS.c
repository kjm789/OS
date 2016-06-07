#include "../include/FonefiveFS_basic.h"

// For iterator stuff, tells you which hunk of pointers we are in
//#define POSITION_TO_ITR_POS(position) ((POSITION_TO_BLOCK_INDEX(position) < 6)? DIRECT : ((POSITION_TO_BLOCK_INDEX(position) < (6+256)) ? INDIRECT : DBL_INDIRECT))
itr_pos_t position_to_itr_pos(uint32_t position) {
    const uint32_t block_index = POSITION_TO_BLOCK_INDEX(position);
    if (block_index < DIRECT_TOTAL) {
        return DIRECT;
    } else if (block_index < (DIRECT_TOTAL + INDIRECT_TOTAL)) {
        return INDIRECT;
    } else if (block_index < (DIRECT_TOTAL + INDIRECT_TOTAL + DBL_INDIRECT_TOTAL)) {
        return DBL_INDIRECT;
    }
    return UNDEFINED;
}

// For iterator stuff, fills out the index tuple
// First index is the data_ptr index
// Second, index in indirect, or the indirect within the dbl_indirect
// Third, direct in the indirect in the double indirect
void get_position_and_index_tuple(const uint32_t position, data_itr_t *const info) {
    if (info) {
        info->position = position_to_itr_pos(position);
        block_ptr_t block_index = POSITION_TO_BLOCK_INDEX(position);
        switch (info->position) {
            case DIRECT:
                // index is just what direct pointer we're at
                info->indices[0] = block_index;
                info->indices[1] = 0;
                info->indices[2] = 0;
                return;
            case INDIRECT:
                // index is just what indirect's direct we're at
                info->indices[0] = 6; // the indirect pointer's index
                info->indices[1] = block_index - DIRECT_TOTAL; // the position in that indirect (...probably)
                info->indices[2] = 0;
                return;
            case DBL_INDIRECT:
                info->indices[0] = 7;
                info->indices[1] = (block_index - DIRECT_TOTAL - INDIRECT_TOTAL) >> 8; // the indirect we're on
                info->indices[2] = (block_index - DIRECT_TOTAL - INDIRECT_TOTAL) & 0xFF; // the direct in the indirect in the dbl_indirect
                return;
            default:
                // What. How. Setting it to numbers that will probably break something somewhere
                info->indices[0] = UINT32_MAX;
                info->indices[1] = UINT32_MAX;
                info->indices[1] = UINT32_MAX;
        }
    }
}

// file system format..given a fname, create a fs and link to given filename 
int fs_format(const char *const fname)
{
	if( !fname || !(fname[0]) ) { //error checking
		fprintf(stderr, "Couldn't open file \"%s\"\n", fname);
		return -1;
	}
		
	block_store_t* bs = block_store_create(); //where all the magic happens
	if(!bs) { //error checking
		fprintf(stderr, "Couldn't create a bs. Block store states: %s\n", block_store_strerror(block_store_errno()));
		return -1;
	}

	block_store_link(bs, fname);
	if( !(block_store_errno() == BS_OK) ) { //error checking
		fprintf(stderr, "Couldn't link block store. Block store states: %s\n", block_store_strerror(block_store_errno()));
		block_store_destroy(bs, BS_NO_FLUSH);
		return -1;
	}
	
	bool success = true;
	for(size_t i = INODE_BLOCK_OFFSET; i < (INODE_BLOCK_OFFSET + INODE_BLOCK_TOTAL); ++i) {
		success &= block_store_request(bs, i);
	}

	if(!success) {// error checking request
		fprintf(stderr, "Couldn't request inode table. Block store states: %s\n", block_store_strerror(block_store_errno()));
		block_store_destroy(bs, BS_FLUSH);
		return -1;
	}

	size_t root_file_block = block_store_allocate(bs);
	if(!root_file_block) {
		fprintf(stderr, "Couldn't request root file. Block store states: %s\n", block_store_strerror(block_store_errno()));
		block_store_destroy(bs, BS_FLUSH);
		return -1;
	}

	inode_t root;
	dir_block_t root_dir;

	memset(&root, 0, sizeof(inode_t));
	memset(&root_dir, 0, sizeof(dir_block_t));

	root.fname[0] = '/';
	root.mdata.size = 0;
	root.mdata.mode = 0;
	root.mdata.c_time = time(NULL);
	root.mdata.a_time = time(NULL);
	root.mdata.m_time = time(NULL);
	root.mdata.parent = 0;
	root.mdata.type = (uint8_t)DIRECTORY;
	root.data_ptrs[0] = root_file_block;

	root_dir.mdata.size = 0;

	if( !(block_store_write(bs, INODE_TO_BLOCK(0), &root, sizeof(inode_t), 0) == sizeof(inode_t) && block_store_write(bs, root_file_block, &root_dir, sizeof(dir_block_t), 0) == sizeof(dir_block_t)) ) {
		fprintf(stderr, "Something didn't write. Block store states: %s\n", block_store_strerror(block_store_errno()));
		block_store_destroy(bs, BS_FLUSH);
		return -1;
	}
	block_store_destroy(bs, BS_FLUSH);
	if(block_store_errno() == BS_OK) {
		return 0;
	}
	fprintf(stderr, "Flush died. Block store states: %s\n", block_store_strerror(block_store_errno()));
	return -1;	
}	


// File system mount....given a fname, we are to mount fs object from existing
F15FS_t *fs_mount(const char *const fname)
{

	F15FS_t* fs= malloc(sizeof(F15FS_t)); //create space for fs object
	if(!fs) { // error check
		fprintf(stderr, "couldn't malloc fs\n");
		return NULL;
	}

	fs->bs = block_store_import(fname); //import bs from filename, storing it in fs object 
	if(!(fs->bs)) { //error check
		fprintf(stderr, "Couldn't open file \"%s\". Block store states: %s\n", fname, block_store_strerror(block_store_errno()));
		free(fs);
		return NULL;
	}
	if(block_store_errno() == BS_OK) //error check
		return fs; //return the imported/mounted fs object
	fprintf(stderr, "Issue with import (link problem)? Block store states: %s\n", block_store_strerror(block_store_errno()));
	block_store_destroy(fs->bs, BS_NO_FLUSH);
	free(fs);
	return NULL; 
}

/// file system unmount...passed in a F15FS object
//returns 0 for success, < 0 on error, object is always destructed, if exists
int fs_unmount(F15FS_t *fs)
{
	if(!fs) //error check 
		return -1;
	block_store_destroy(fs->bs, BS_FLUSH);// destory blockstore of fs and Flush any changes made
	free(fs);// free the memory for fs object
	fs = NULL;
	if(block_store_errno() == BS_OK) { //check for erros
		return 0;
	}
	fprintf(stderr, "BS_DESTORY failed to flush! Block store states: %s", block_store_strerror(block_store_errno())); //throw error produced by block store
	return -1;
}

// Dumps the given inode to the provided data pointer
// Returns false IFF there was a busted pointer or block_store died for some reason
bool load_inode(const F15FS_t *const fs, const inode_ptr_t inode, void *data) {
    return (fs && data && block_store_read(fs->bs, INODE_TO_BLOCK(inode), data, sizeof(inode_t), INODE_INNER_OFFSET(inode)) == sizeof(inode_t) &&
            block_store_errno() == BS_OK);
}

// Writes data to the specified inode
// Returns false IFF there was a busted pointer or block_store died for some reason
bool write_inode(const F15FS_t *const fs, const inode_ptr_t inode, const void *const data) {
    return (fs && data && block_store_write(fs->bs, INODE_TO_BLOCK(inode), data, sizeof(inode_t), INODE_INNER_OFFSET(inode)) == sizeof(inode_t) &&
            block_store_errno() == BS_OK);
}

// loads the specified block from the fs to the given data pointer
// Returns false IFF there was a busted pointer, the block index was bad, or block_store died
bool load_block(const F15FS_t *const fs, const block_ptr_t block, void *data) {
    return (fs && data && BLOCK_IDX_VALID(block) && block_store_read(fs->bs, block, data, sizeof(data_block_t), 0) == sizeof(data_block_t) &&
            block_store_errno() == BS_OK);
}

// Writes the specified block
// Returns false IFF there was a busted pointer, the block index was bad, or block_store died
bool write_block(const F15FS_t *const fs, const block_ptr_t block, const void *const data) {
    return (fs && data && BLOCK_IDX_VALID(block) && block_store_write(fs->bs, block, data, sizeof(data_block_t), 0) == sizeof(data_block_t) &&
            block_store_errno() == BS_OK);
}

// loads the specified block from the fs to the given data pointer
// Returns false IFF there was a busted pointer, the block index was bad, or block_store died
bool load_partial_block(const F15FS_t *const fs, const block_ptr_t block, void *const data, size_t nbyte, const size_t offset) {
    return (fs && data && BLOCK_IDX_VALID(block) && block_store_read(fs->bs, block, data, nbyte, offset) == nbyte &&
            block_store_errno() == BS_OK);
}

// Writes the specified block
// Returns false IFF there was a busted pointer, the block index was bad, or block_store died
bool write_partial_block(const F15FS_t *const fs, const block_ptr_t block, const void *const data, size_t nbyte, const size_t offset) {
    return (fs && data && BLOCK_IDX_VALID(block) && block_store_write(fs->bs, block, data, nbyte, offset) == nbyte &&
            block_store_errno() == BS_OK);
}

// Given an fname and an inode, scan_directory will:
//  Validate the fname pointer (and result struct)
//  Load and validate that the specified inode is a file that 1: exists and 2: is a directory
//  Load the directory record for the inode and then find the given fname in the record
//  Fill out the result structure
// RESULT STRUCT CONTENTS:
// Success - Parameter checking passed, all needed data could be loaded, dir is a directory
// Valid - Requested file was found
// Inode - Inode of file, if Valid
// Total - Number of files inside directory, if Success
// (Success but not Valid means it wasn't found)
void scan_directory(const F15FS_t *const fs, const char *const fname, const inode_ptr_t inode, search_struct_t *result_info) {
    inode_t search_inode;
    dir_block_t directory_record;
    if (result_info) {
        memset(result_info, 0, sizeof(search_struct_t)); // Just wipe everything
        if (fs && fname && fname[0] && strnlen(fname, FNAME_MAX + 1) <= FNAME_MAX) {
            // Alrighty, load the inode, check that it's real and that it's a directory
            if (load_inode(fs, inode, &search_inode)) {
                // Inode loaded, check type
                if (INODE_IS_TYPE(&search_inode, DIRECTORY)) {
                    // Load data block and check it
                    if (load_block(fs, search_inode.data_ptrs[0], &directory_record)) {
                        // Loop a string check for all entries! Thrilling!
                        result_info->success = true;
                        result_info->total = directory_record.mdata.size;
                        for (unsigned idx = 0; idx < DIR_REC_MAX; ++idx) {
                            if (strncmp(fname, directory_record.entries[idx].fname, FNAME_MAX) == 0) {
                                // MATCH! Woooooooooooooo!
                                result_info->valid = true;
                                result_info->inode = directory_record.entries[idx].inode;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Finds a file's inode given the absolute path. Writes data to given search struct
// RESULT STRUCT CONTENTS:
// Success - Parameters/etc are good
// Valid - File was found, if Success
// Inode - Inode of file, if Valid
// Data - Pointer into provided string at the last token (located file's name, or where traversal failed), if Success
void locate_file(const F15FS_t *const fs, const char *const abs_path, search_struct_t *result_info) {
    // Uhh, the max possible absolute path is... large. Like > 12000 charaters.
    // So it's a little hard to test and manage

    // Given our 256-entry inode table, we can have 255 directories of 47-char name,
    // with 254 slashes between them, and one at the beginning. Assuming we forbid double slashes, that's...
    // 12241 including null terminator????
    // I don't want to put 12k on the stack. And we're going to overallocate because I don't trust my math entirely
    // (but it should(?) be right????) (maybe a +1 for a trailing slash?)
    // Let's call it an even 13000.

    // We're also going to have to mangle the hell out of this string
    // That's just how strtok works, so we'll have to malloc room for a copy so we don't wreck the given string

    // Also, we technically should update a_time for each directory traversed, but I don't want to, so I won't!
    // That adds a lot of complexity. Let's consider a_time only for explicit access and not things we have to do
    // for an operation

    static const char delims[] = { '/', '\0' }; // c-string!
    if (result_info) {
        memset(result_info, 0, sizeof(search_struct_t));
        if (fs && abs_path) {
            // Ok, since we KNOW that MAX_PATH is an overallocation, we can use strnlen to validate it a bit
            const size_t path_length = strnlen(abs_path, FS_PATH_MAX);
            if (!(path_length == 0 || path_length == FS_PATH_MAX)) {
                char *path_copy = (char *)calloc(1, FS_PATH_MAX); // Allocating extra? Probably.
                // But it also means we can reuse it if we want since we know the size
                if (path_copy) {
                    memcpy(path_copy, abs_path, path_length); // We already did size validation, so we might as well memcpy

                    // Ok, we are actually ready to start working. Crazy.
                    // Find the first token. While our token isn't NULL, pass the inode to a directory parser
                    // and update the inode to what it gives back.
                    // Also, do this because if we get NULL immediately, it was a root req b/c we already threw out empty strings
                    result_info->inode = 0;
                    result_info->success = true;
                    result_info->valid = true;
                    result_info->data = (void *)abs_path; // const strip, deal with it.

                    search_struct_t dir_search_results;

                    char *token = strtok(path_copy, delims);
                    while (token) {
                        // Update the data pointer because it's going to be handy
                        result_info->data = (void *)(abs_path + (token - path_copy)); // const strip, deal.

                        // Ok, token is a file/folder. and it SHOULD exist inside the current inode we have flagged for search
                        scan_directory(fs, token, result_info->inode, &dir_search_results);
                        // Ok, if the current inode is a directory AND the token fname exists inside it...
                        if (dir_search_results.success && dir_search_results.valid) {
                            // Cool, cycle token
                            result_info->inode = dir_search_results.inode;
                            token = strtok(NULL, delims);
                            continue;
                        }
                        // broken search path! File not found!
                        // Also, we're still in the loop right here
                        result_info->valid = false;
                        break;
                    }
                    // Ok, at this point we either recursed until we finished the string (success!) or we short-circuted out
                    // Either way, our result struct is valid!
                    free(path_copy);
                    return;
                }
            }
            // WAY BAD STRING, buffer overflow attempt?
            // or it's just empty, in which case, way2go.
        }
    }
}

// Finds a free inode and fills out a search structure
// RESULT STRUCT CONTENTS:
// Success - Operation successful (no load/parameter errors)
// Valid - Free inode found, if Success
// Inode - Free inode, if Valid
void find_free_inode(const F15FS_t *const fs, search_struct_t *search_results) {
    if (search_results) {
        memset(search_results, 0, sizeof(search_struct_t));
        if (fs) {
            // Load blocks, iterate through inodes
            // Screw it, load inodes individually, smaller error surface
            inode_t inode;
            // Oh, hey, we can skip root I suppose
            for (int inode_idx = 1; inode_idx < INODE_TOTAL; ++inode_idx) {
                if (load_inode(fs, inode_idx, &inode)) {
                    if (!inode.fname[0]) {
                        // Inode is free, name is empty
                        search_results->success = true;
                        search_results->valid = true;
                        search_results->inode = inode_idx;
                        return;
                    }
                } else {
                    fprintf(stderr, "Could not load inode!\n");
                    return;
                }
            }
        }
        // Couldn't find a free one. Sad.
        search_results->success = true;
    }
}

// Creates an empty regular file under the specified parent inode
// Returns true if it worked, false if there was an allocation error, parent not a directory, etc
bool create_file(F15FS_t *const fs, const char *const fname, const inode_ptr_t parent_inode) {
    if (fs && fname && fname[0] && strnlen(fname, FNAME_MAX + 1) <= FNAME_MAX) {
        search_struct_t inode_search_info, dir_search_info;
        inode_t *parent = (inode_t *)malloc(sizeof(inode_t)), *child = (inode_t *)calloc(sizeof(inode_t), 1);
        dir_block_t *parent_block = (dir_block_t *)malloc(sizeof(dir_block_t));
        if (parent && child && parent_block) {
            scan_directory(fs, fname, parent_inode, &dir_search_info);
            if (dir_search_info.success && !dir_search_info.valid && dir_search_info.total < DIR_REC_MAX) {
                find_free_inode(fs, &inode_search_info);
                if (inode_search_info.success && inode_search_info.valid) {
                    if (load_inode(fs, parent_inode, parent) && load_block(fs, parent->data_ptrs[0], parent_block)) {
                        // all data loaded and ready to fill in.

                        // This is mostly a copy from create_directory:

                        // Loop to find free entry in directory (we KNOW there is space)
                        for (int i = 0; i < DIR_REC_MAX; ++i) {
                            if (! parent_block->entries[i].fname[0]) {
                                // free entry, set it
                                strncpy(parent_block->entries[i].fname, fname, FNAME_MAX);
                                parent_block->entries[i].inode = inode_search_info.inode;
                                break;
                            }
                        }
                        ++(parent_block->mdata.size);
                        // Directory record for parent ready for update

                        // Why not
                        parent->mdata.m_time = time(NULL);
                        // Parent inode ready for update

                        strncpy(child->fname, fname, FNAME_MAX);
                        child->data_ptrs[0] = 0;
                        child->mdata.c_time = time(NULL);
                        child->mdata.a_time = time(NULL);
                        child->mdata.m_time = time(NULL);
                        child->mdata.parent = parent_inode;
                        child->mdata.type = (uint8_t) REGULAR;
                        // Child inode is ready to go!

                        if (write_inode(fs, parent_inode, parent) &&
                                write_block(fs, parent->data_ptrs[0], parent_block) &&
                                write_inode(fs, inode_search_info.inode, child)) {
                            // Full write successful, flush data to file
                            block_store_flush(fs->bs);
                            free(parent); free(child); free(parent_block);
                            if (block_store_errno() == BS_OK) {
                                // Done!
                                return true;
                            }
                            fprintf(stderr, "Block_store died during flush. FILE IN BAD STATE, block_store says: %s\n", block_store_strerror(block_store_errno()));
                            return false;
                        }
                        // We died writing SOMETHING. That's... bad.
                        // BUT! We haven't actually flushed to disk, so the FILE is ok, but our copy of it is busted.
                        // to bad unmount always flushes, so... we're going to get to a bad state unless the process gets killed right now
                        // Technically we'll raise the alarm on a warning, but we shouldn't trigger a warning because we're doing it right
                        fprintf(stderr, "Block_store partial write, UNMOUNT WILL PUT FILE IN BAD STATE, block_store says: %s\n", block_store_strerror(block_store_errno()));
                        free(parent); free(child); free(parent_block);
                        return false;
                    }
                    fprintf(stderr, "Error loading parent data, block_store says: %s\n", block_store_strerror(block_store_errno()));
                    free(parent); free(child); free(parent_block);
                    return false;
                }
                fprintf(stderr, "Error finding free inode!\n");
                free(parent); free(child); free(parent_block);
                return false;
            }
            fprintf(stderr, "Parent directory not valid (Full? Not a directory?)\n");
            free(parent); free(child); free(parent_block);
            return false;
        }
        fprintf(stderr, "Could not malloc things!\n");
        return false;
    }
    fprintf(stderr, "Parameter error.\n");
    return false;
}

// Actually creates a directory with given name at the specified location
// Returns true if it worked, false if an allocation error (or parent not a directory)
bool create_directory(F15FS_t *const fs, const char *const fname, const inode_ptr_t parent_inode) {
    if (fs && fname && fname[0] && strnlen(fname, FNAME_MAX + 1) <= FNAME_MAX) {
        search_struct_t inode_search_info, dir_search_info;
        // Because I suppose it's a lot to put on the stack
        inode_t *parent = (inode_t *)malloc(sizeof(inode_t)), *child = (inode_t *)calloc(sizeof(inode_t), 1);
        dir_block_t *parent_block = (dir_block_t *)malloc(sizeof(dir_block_t)), *child_block = (dir_block_t *)calloc(sizeof(dir_block_t), 1);

        if (parent && child && parent_block && child_block) {
            scan_directory(fs, fname, parent_inode, &dir_search_info);
            if (dir_search_info.success && !dir_search_info.valid && dir_search_info.total < DIR_REC_MAX) {
                find_free_inode(fs, &inode_search_info);
                if (inode_search_info.success && inode_search_info.valid) {
                    // Got an inode, get a data block
                    const size_t free_block = block_store_allocate(fs->bs);
                    if (free_block && block_store_errno() == BS_OK) {
                        // Got a data block, load data and write it back out
                        // This could be done more safely
                        if (load_inode(fs, parent_inode, parent) && load_block(fs, parent->data_ptrs[0], parent_block)) {
                            // OK. Everything is loaded. Time to fill out data, write it back, and call sync

                            // Loop to find free entry in directory (we KNOW there is space)
                            for (int i = 0; i < DIR_REC_MAX; ++i) {
                                if (! parent_block->entries[i].fname[0]) {
                                    // free entry, set it
                                    strncpy(parent_block->entries[i].fname, fname, FNAME_MAX);
                                    parent_block->entries[i].inode = inode_search_info.inode;
                                    break;
                                }
                            }
                            ++(parent_block->mdata.size);
                            // Directory record for parent ready for update

                            // Why not
                            parent->mdata.m_time = time(NULL);
                            // Parent inode ready for update

                            // Child block is 0'd out, so... it's valid?
                            // All metadata in a directory block is just going to be ignored except for size
                            // Which is 0, so... child block ready to write

                            strncpy(child->fname, fname, FNAME_MAX);
                            child->data_ptrs[0] = (block_ptr_t) free_block;
                            child->mdata.c_time = time(NULL);
                            child->mdata.a_time = time(NULL);
                            child->mdata.m_time = time(NULL);
                            child->mdata.parent = parent_inode;
                            child->mdata.type = (uint8_t) DIRECTORY;
                            // Child inode is ready to go!

                            if (write_inode(fs, parent_inode, parent) &&
                                    write_block(fs, parent->data_ptrs[0], parent_block) &&
                                    write_inode(fs, inode_search_info.inode, child) &&
                                    write_block(fs, free_block, child_block)) {
                                // Full write successful, flush data to file
                                block_store_flush(fs->bs);
                                free(parent); free(parent_block); free(child); free(child_block);
                                if (block_store_errno() == BS_OK) {
                                    // Done!
                                    return true;
                                }
                                fprintf(stderr, "Block_store died during flush. FILE IN BAD STATE, block_store says: %s\n", block_store_strerror(block_store_errno()));
                                return false;
                            }
                            // We died writing SOMETHING. That's... bad.
                            // BUT! We haven't actually flushed to disk, so the FILE is ok, but our copy of it is busted.
                            // to bad unmount always flushes, so... we're going to get to a bad state unless the process gets killed right now
                            // Technically we'll raise the alarm on a warning, but we shouldn't trigger a warning because we're doing it right
                            fprintf(stderr, "Block_store partial write, UNMOUNT WILL PUT FILE IN BAD STATE, block_store says: %s\n", block_store_strerror(block_store_errno()));
                            // We don't really know if we should free the block. This is a DISASTER.
                            free(parent); free(parent_block); free(child); free(child_block);
                            return false;
                        }
                        fprintf(stderr, "Could not load parent data, block_store says: %s\n", block_store_strerror(block_store_errno()));
                        block_store_release(fs->bs, free_block);
                        free(parent); free(parent_block); free(child); free(child_block);
                        return false;
                    }
                    fprintf(stderr, "Block_store may be full, block_store says: %s\n", block_store_strerror(block_store_errno()));
                    block_store_release(fs->bs, free_block);
                    free(parent); free(parent_block); free(child); free(child_block);
                    return false;
                }
                fprintf(stderr, "Could not find free inode\n");
                free(parent); free(parent_block); free(child); free(child_block);
                return false;
            }
            fprintf(stderr, "Parent directory not valid (Full? Not a directory?)\n");
            free(parent); free(parent_block); free(child); free(child_block);
            return false;
        }
        fprintf(stderr, "Malloc failure in create_directory\n");
        free(parent); free(parent_block); free(child); free(child_block);
        return false;
    }
    return false;
}

//
// Creates a new file in the given F15FS object
// param fs the F15FS file
// param fname the file to create
// param ftype the type of file to create
// return 0 on success, < 0 on error
//
int fs_create_file(F15FS_t *const fs, const char *const fname, const ftype_t ftype)
{
	// Validate string, find the actual creation target (the last token), locate the parent,
    //  scan the parent for pre-existing file, begin creation process:
    // Find an empty inode
    // If its a file, find a free block
    // Fill out the inode, fill out the block, (optionally)
    // Write
    char *path_copy = NULL;
    if (fs && fname) {
        const size_t path_length = strnlen(fname, FS_PATH_MAX);
        if (path_length && path_length < FS_PATH_MAX) {
            path_copy = (char *)calloc(1, FS_PATH_MAX); // string we can modify
            memcpy(path_copy, fname, path_length);
            char *requested_name = strrchr(path_copy, '/'); // pointer to the name of the file they wish to create
            // Also, this function is a terrible idea since we know the length and strrchr is
            // most likely a forward traversal
            // if this returns NULL, then there is no slash in the string, so it's another bad string detection
            if (requested_name) {
                *requested_name = '\0';
                ++requested_name;
                // now path_copy is two c-strings. path_copy is the path of the requested creation
                // requested_name is the name of the file they want to make.

                // If path_copy is empty now, it's a file under root. locate treats this as an error
                //  but, we know where root is, so... work around it?
                // Gotta range check requested_name
                const size_t requested_name_length = path_length - (requested_name - path_copy); // CHECK LOGIC
                if (requested_name_length && requested_name_length <= FNAME_MAX) {
                    search_struct_t path_search_info;
                    if (path_copy[0]) {
                        locate_file(fs, path_copy, &path_search_info);
                    } else {
                        // Request was for a file at root!
                        path_search_info.success = true;
                        path_search_info.valid = true;
                        path_search_info.inode = 0;
                        path_search_info.data = NULL; // just in case
                    }

                    // Search struct is filled out in some way now with where we need to create the file
                    if (path_search_info.valid) { // valid implies success
                        // The full directory path exists.
                        // offloaded validating parent info to creation functions
                        //  (file doesn't already exist, parent is a directory, not full, etc)
                        switch (ftype) {
                            case DIRECTORY:
                                // create directory at x
                                if (create_directory(fs, requested_name, path_search_info.inode)) {
                                    free(path_copy);
                                    return 0;
                                }
                                fprintf(stderr, "Failed to create file, block_store may be in bad state :C\n");
                                free(path_copy);
                                return -1;
                            case REGULAR:
                                // create regular file at x
                                if (create_file(fs, requested_name, path_search_info.inode)) {
                                    free(path_copy);
                                    return 0;
                                }
                                fprintf(stderr, "Failed to create file, block_store may be in bad state :C\n");
                                free(path_copy);
                                return -1;
                            default:
                                // ...what?
                                fprintf(stderr, "Invalid file type\n");
                                free(path_copy);
                                return -1;
                        }
                    }
                    fprintf(stderr, "Could not resolve path. Missing directory \"%s\"\n", (char *)path_search_info.data);
                    free(path_copy);
                    return -1;
                }
                fprintf(stderr, "Requested filename length bad!\n");
                free(path_copy);
                return -1;
            }
            fprintf(stderr, "Malformed path!\n");
            free(path_copy);
            return -1;
        }
        fprintf(stderr, "Path length bad!\n");

        return -1;
    }
    fprintf(stderr, "Bad pointer!\n");
    return -1;
}

//
// Returns the contents of a directory
// param fs the F15FS file
// param fname the file to query
// param records the record object to fill
// return 0 on success, < 0 on error
// 
int fs_get_dir(F15FS_t *const fs, const char *const fname, dir_rec_t *const records)
{
	// Functions and error handling is so much prettier and easier when you report no diagnostic info whatsoever!
    if (fs && fname && records && fname[0] && strnlen(fname, FS_PATH_MAX) < FS_PATH_MAX) {
        search_struct_t dir_search;
        locate_file(fs, fname, &dir_search);
        if (dir_search.valid) {
            // further analysis, is it actually a directory...?
            // Also, malloc.
            inode_t *file_inode = (inode_t *)malloc(sizeof(inode_t));
            dir_block_t *dir_record = (dir_block_t *)malloc(sizeof(dir_block_t));
            if (file_inode && dir_record && load_inode(fs, dir_search.inode, file_inode)) {
                if (INODE_IS_TYPE(file_inode, DIRECTORY)) {
                    // Inode is a directory, yay!
                    if (load_block(fs, file_inode->data_ptrs[0], dir_record)) {
                        // Ok, now we have everything to complete the request. Let's update the inode's a_time
                        // but just gloss over if it breaks
                        file_inode->mdata.a_time = time(NULL);
                        // Also, ASSUMING DIRECT CONTROL to skip writing the full inode
                        // This way if ANYTHING could break/corrupt, it's just the a_time entry
                        /*
                            block_store_write(fs->bs, INODE_TO_BLOCK(dir_search.inode),
                                ((uint8_t*)file_inode) + (&(file_inode->mdata.a_time) - file_inode),
                                4, INODE_INNER_OFFSET(dir_search.inode) + (&(file_inode->mdata.a_time) - file_inode));
                        */
                        // Actually, going back on that. It's a good idea, but less modular
                        // Writing data over itself can't cause any corruption except where it differes, which is still
                        // just the a_time slot.
                        write_inode(fs, dir_search.inode, file_inode); // Still ignoring success/failure, though

                        // Ok, time to actually fill out the records. We know how many records there are
                        //  (dir_record.mdata.size), BUT they aren't contiguous, which is annoying
                        //  (once again, making it contiguous is super easy, but that's an exercise for the reader)
                        memset(records, 0, sizeof(dir_rec_t));
                        records->total = dir_record->mdata.size;
                        if (dir_record->mdata.size <= DIR_REC_MAX) {
                            for (unsigned rec_total = 0, rec_idx = 0; rec_total < dir_record->mdata.size; ++rec_total, ++rec_idx) {
                                while (! dir_record->entries[rec_idx].fname[0]) { ++rec_idx; }
                                // That should jump us to the next valid entry

                                // OH NO. It seems I was wrong about having all the data ready.
                                // We have to fill in the ftype field, so we have to load EVERY refrenced inode.
                                // Well then.
                                if (load_inode(fs, dir_record->entries[rec_idx].inode, file_inode)) {
                                    records->contents[rec_total].ftype = file_inode->mdata.type; // this may need to be cast
                                    // I'm still going to read the name from the dir record to make sure it's right
                                    memcpy(records->contents[rec_total].fname, dir_record->entries[rec_idx].fname, FNAME_MAX + 1);
                                    continue;
                                }
                                free(file_inode); free(dir_record);
                                return -1;
                            }
                            free(file_inode); free(dir_record);
                            return 0;
                        }
                        // Ok, one error message, because this should never happen
                        fprintf(stderr, "Directory reports more than allowed entries, how did this happen?\n");
                    }
                }
            }
            free(file_inode); free(dir_record);
        }
    }
    return -1;
}

//
// Writes nbytes from the given buffer to the specified file and offset
// param fs the F15FS file
// param fname the name of the file
// param data the buffer to read from
// param nbyte the number of bytes to write
// param offset the offset in the file to begin writing to
// return ammount written, < 0 on error
//

/*
    itr_pos_t position_to_itr_pos(uint32_t position);
    // For iterator stuff, fills out the index tuple
    void get_position_and_index_tuple(const uint32_t position, data_itr_t *const info);
    // I had an idea. This may end in tears
    typedef enum { DIRECT = 0x01, INDIRECT = 0x02, DBL_INDIRECT = 0x04 } itr_pos_t;
    typedef struct {
        inode_ptr_t inode;
        itr_pos_t position;
        block_ptr_t indices[3];
        uint32_t size;
    } data_itr_t;
    }
*/

// checks a given position to see if it exists within a file, if it doesn't, we will create it
//  - The inode pointer will be up to date when this returns (bad state if we fail to allocate)
//  - And the location of the block this position goes to will be set in the iterator
// Returns true if it worked, false if we broke somehow
bool check_and_allocate_block(F15FS_t *const fs, data_itr_t *const data_itr, inode_t *const file_inode) {
    if (fs && data_itr && load_inode(fs, data_itr->inode, file_inode)) {
        // Loading it again in case I somehow mess it up
        // Block pointers should be zero if they are invalid
        // (IMPORTANT REMINDER: ZERO OUT NEW INDIRECT BLOCKS ON CREATION)
        size_t new_block[3];
        block_ptr_t *indir_block = NULL, *dbl_indir_block = NULL;
        bool success = false;

        if (data_itr->position == DIRECT) {
            // Score, this is easy
            if (file_inode->data_ptrs[data_itr->indices[0]] == 0) {
                // need to make a new block, pointer is 0
                new_block[0] = block_store_allocate(fs->bs);
                if (new_block[0]) {
                    // Cool, we got a fresh block, update the inode
                    file_inode->data_ptrs[data_itr->indices[0]] = new_block[0];
                    if (write_inode(fs, data_itr->inode, file_inode)) {
                        data_itr->location = new_block[0];
                        return true;
                    }
                    // Broke while updating inode. This is bad. return the block to block_store
                    fprintf(stderr, "Broke while updating inode pointers, block_store says: %s\n",
                            block_store_strerror(block_store_errno()));
                    block_store_release(fs->bs, new_block[0]);
                }
                // DISASTER!!!!!
                // Block store's full?
                return false;
            }
            return true;
        } else if (data_itr->position == INDIRECT) {
            // A little complicated
            if (file_inode->data_ptrs[data_itr->indices[0]] == 0) {
                // we need to make a fresh indirect and setup the first direct in it
                // (we need two blocks!)
                new_block[0] = block_store_allocate(fs->bs); // New indir block (need to write contents)
                new_block[1] = block_store_allocate(fs->bs); // New data block (nothing to write)
                if (new_block[0] && new_block[1]) {
                    indir_block = calloc(sizeof(data_block_t), 1);
                    if (indir_block) {
                        file_inode->data_ptrs[data_itr->indices[0]] = new_block[0];
                        indir_block[0] = new_block[1];
                        // Hooray for the comma operator
                        if (success = write_block(fs, new_block[0], indir_block), free(indir_block), success) {
                            // I guess this could just be return write_inode(...);
                            if (write_inode(fs, data_itr->inode, file_inode)) {
                                //done?
                                data_itr->location = new_block[1];
                                return true;
                            }
                            // If this failed, the inode is in a bad state
                            // It might not be safe to release those blocks.
                            // Oh no.
                            fprintf(stderr, "Broke while updating inode pointers, block_store says: %s\n",
                                    block_store_strerror(block_store_errno()));
                            return false;
                        }
                        // Well, we couldn't setup the indirect block, so I guess we have to bail.
                        // At least the inode wasn't changed yet
                    }
                }
                block_store_release(fs->bs, new_block[0]);
                block_store_release(fs->bs, new_block[1]);
                return false;
            } else {
                // If this is the case, then we need to load the indirect and check the direct pointer.
                // We might not actually have to allocate a block.
                indir_block = (block_ptr_t *) malloc(sizeof(data_block_t));
                if (indir_block) {
                    if (load_block(fs, file_inode->data_ptrs[data_itr->indices[0]], indir_block)) {
                        // Ok, we have the indirect block (that already existed) loaded
                        if (!indir_block[data_itr->indices[1]]) {
                            // block does not exist, need to create it
                            new_block[0] = block_store_allocate(fs->bs);
                            if (new_block[0]) {
                                indir_block[data_itr->indices[1]] = new_block[0];
                                if (write_block(fs, file_inode->data_ptrs[data_itr->indices[0]], indir_block)) {
                                    // All good?
                                    free(indir_block);
                                    data_itr->location = new_block[0];
                                    return true;
                                }
                                block_store_release(fs->bs, new_block[0]);
                            }
                        } else {
                            // block exists, no work to do
                            data_itr->location = indir_block[data_itr->indices[1]];
                            free(indir_block);
                            return true;
                        }
                    }
                    free(indir_block);
                }
                return false;
            }
        } else if (data_itr->position == DBL_INDIRECT) {
            // Maaaaaaaan...
            // Also, if I was smarter/not as tired/had more time
            // I'd put this all in functions and have it laid out better
            //   instead of copy/paste

            // So now we have 4 cases
            // 1: Double indirect doesn't exist
            //    Allocate 3 blocks
            // 2: The indirect doesn't exist
            //    Allocate 2 blocks
            // 3: The direct doesn't exist
            //    Allocate 1 block
            // 4: All exist
            if (!file_inode->data_ptrs[data_itr->indices[0]]) {
                // Gotta make EVERYTHING
                // ... copy/paste from indirect again

                new_block[0] = block_store_allocate(fs->bs); // New dbl_indir block (need to write contents)
                new_block[1] = block_store_allocate(fs->bs); // New indir block (need to write contents)
                new_block[2] = block_store_allocate(fs->bs); // New data block (nothing to write)
                if (new_block[0] && new_block[1] && new_block[2]) {
                    indir_block = calloc(sizeof(data_block_t), 1);
                    dbl_indir_block = calloc(sizeof(data_block_t), 1);
                    if (indir_block && dbl_indir_block) {
                        file_inode->data_ptrs[data_itr->indices[0]] = new_block[0];
                        dbl_indir_block[0] = new_block[1];
                        indir_block[0] = new_block[2];
                        // Hooray for the comma operator
                        if (success = write_block(fs, new_block[1], indir_block), free(indir_block), success) {
                            if (success = write_block(fs, new_block[0], dbl_indir_block), free(dbl_indir_block), success) {
                                // I guess this could just be return write_inode(...);
                                if (write_inode(fs, data_itr->inode, file_inode)) {
                                    //done?
                                    data_itr->location = new_block[2];
                                    return true;
                                }
                                // If this failed, the inode is in a bad state
                                // It might not be safe to release those blocks.
                                // Oh no.
                                fprintf(stderr, "Broke while updating inode pointers, block_store says: %s\n",
                                        block_store_strerror(block_store_errno()));
                                return false;
                            }
                        }
                        // Well, we couldn't setup the indirect block, so I guess we have to bail.
                        // At least the inode wasn't changed yet
                    }
                    free(indir_block); free(dbl_indir_block);
                }
                block_store_release(fs->bs, new_block[0]);
                block_store_release(fs->bs, new_block[1]);
                block_store_release(fs->bs, new_block[2]);
                return false;
            } else {
                // Same as indirect, but we need to update the dbl_indirect on a new indirect
                dbl_indir_block = (block_ptr_t *) malloc(sizeof(data_block_t));
                if (dbl_indir_block) {
                    if (load_block(fs, file_inode->data_ptrs[data_itr->indices[0]], dbl_indir_block)) {
                        if (dbl_indir_block[data_itr->indices[1]] == 0) {
                            // COPY/PASTE STARTS HERE
                            // we need to make a fresh indirect and setup the first direct in it
                            // (we need two blocks!)
                            new_block[0] = block_store_allocate(fs->bs); // New indir block (need to write contents)
                            new_block[1] = block_store_allocate(fs->bs); // New data block (nothing to write)
                            if (new_block[0] && new_block[1]) {
                                indir_block = calloc(sizeof(data_block_t), 1);
                                if (indir_block) {
                                    indir_block[0] = new_block[1];
                                    dbl_indir_block[data_itr->indices[1]] = new_block[0];
                                    // Hooray for the comma operator
                                    if (success = write_block(fs, new_block[0], indir_block), free(indir_block), success) {
                                        // DBL_INDIR_UPDATE HERE
                                        if (success = write_block(fs, file_inode->data_ptrs[data_itr->indices[0]], dbl_indir_block),
                                                free(dbl_indir_block), success) {

                                            data_itr->location = new_block[1];
                                            return true;
                                        }
                                        // The double_indirect didn't get the link, so the blocks are orphaned
                                    }
                                    // Well, we couldn't setup the indirect block, so I guess we have to bail.
                                    // At least the inode wasn't changed yet
                                }
                            }
                            block_store_release(fs->bs, new_block[0]);
                            block_store_release(fs->bs, new_block[1]);
                        } else {
                            // If this is the case, then we need to load the indirect and check the direct pointer.
                            // We might not actually have to allocate a block.
                            indir_block = (block_ptr_t *) malloc(sizeof(data_block_t));
                            if (indir_block) {
                                if (load_block(fs, dbl_indir_block[data_itr->indices[1]], indir_block)) {
                                    // Ok, we have the indirect block (that already existed) loaded
                                    if (!indir_block[data_itr->indices[2]]) {
                                        // block does not exist, need to create it
                                        new_block[0] = block_store_allocate(fs->bs);
                                        if (new_block[0]) {
                                            indir_block[data_itr->indices[2]] = new_block[0];
                                            if (success = write_block(fs, dbl_indir_block[data_itr->indices[1]], indir_block),
                                                    free(indir_block), success) {
                                                // All good?
                                                free(dbl_indir_block);
                                                data_itr->location = new_block[0];
                                                return true;
                                            }
                                            block_store_release(fs->bs, new_block[0]);
                                        }
                                    } else {
                                        // block exists, no work to do
                                        data_itr->location = indir_block[data_itr->indices[2]];
                                        free(indir_block);
                                        free(dbl_indir_block);
                                        return true;
                                    }
                                }
                                free(indir_block);
                            }
                        }
                    }
                    free(dbl_indir_block);
                    return false;
                }
            }
        }
    }
    return false;
}

/*
typedef enum { DIRECT = 0x01, INDIRECT = 0x02, DBL_INDIRECT = 0x04, UNDEFINED = 0x00 } itr_pos_t;

typedef struct {
    inode_ptr_t inode;
    itr_pos_t position;
    block_ptr_t indices[3];
    uint32_t size;
    block_ptr_t location;
} data_itr_t;

// Generic search/request struct to handle various requests without making a ton of different structs.
// Use and values depend on what function you use, check the function's info
typedef struct {
    bool success;
    bool valid;
    inode_ptr_t inode;
    block_ptr_t block;
    uint64_t total;
    void *data;
} search_struct_t;
*/

//find the fname given, where assign the offset of the beginning of the file in that particular block to file descriptor
int fs_open_file(F15FS_t *const fs, const char * const fname)
{

	return -1;
}
//TODO FINAL
int fs_close_file(F15FS_t *const fs, const int fd)
{
	return -1;
}

// Write file to the file system using the file descriptor 
ssize_t fs_write_file(F15FS_t *const fs, const char *const fname, const void *data, size_t nbyte, size_t offset)
{
	 // OH MAN. THIS ONE IS THE HARD ONE

    // We need to:
    //  Locate the target, validate type
    //  Validate that offset is, at most, the size (if size, we're one past the end, so we're adding data)
    //  Validate that offset & size won't blow past the file size max (no? Write all available data THEN fail? (yes))
    //  Locate write position, figure out where the next boundary is.
    //  LOOP WHILE DATA/BLOCKS TO WRITE
    //    Detect if next block is a new block
    //      Allocate and link new block if it's new (Break out if out of blocks/filesize max)
    //      Else, just navigate to block
    //    Write data to current block
    //    Update inode's size data
    //  Update inode's m_time
    //  Return total data written

    ssize_t data_written = -1;


    // The last part SHOULD prevent an offset+nbyte rollover
    if (fs && fname && fname[0] && strnlen(fname, FS_PATH_MAX) < FS_PATH_MAX && data) {
        search_struct_t file_data;
        locate_file(fs, fname, &file_data);
        if (file_data.valid) {
            // Well it's something
            inode_t file_inode; // Skip the malloc because they're tedious.
            if (load_inode(fs, file_data.inode, &file_inode)) {
                if (INODE_IS_TYPE(&file_inode, REGULAR)) {

                    // OK, NOW we can validate size & offset
                    if (file_inode.mdata.size >= offset) {
                        // Offset makes sense. We may still be at EOF, though, if ==
                        size_t data_to_write = (offset + nbyte > FILE_SIZE_MAX) ? (FILE_SIZE_MAX - offset) : nbyte;;
                        data_written = 0;
                        data_itr_t data_iterator;
                        data_iterator.inode = file_data.inode;
                        // We know where we're starting out, but that doesn't entirely help

                        // It's really always going to be zero unless this is the first iteration
                        size_t current_offset = offset & 0x3FF;
                        // It's going to be block_size unless it's the first iteration
                        size_t current_total = (data_to_write > (BLOCK_SIZE - current_offset)) ? (BLOCK_SIZE - current_offset) : data_to_write; // must recalc every loop


                        while (data_to_write) {
                            get_position_and_index_tuple(offset, &data_iterator);
                            if (!check_and_allocate_block(fs, &data_iterator, &file_inode)) {
                                // Could not allocate space. We're having a bad time
                                fprintf(stderr, "Could not extend file length any more, stopping write\n");
                                if (!data_written) {
                                    data_written = -1;
                                }
                                data_to_write = 0;
                                continue;
                            }
                            // We have space lined up... somewhere
                            // Iterator has the location variable set to our writing block

                            // Ok, offset is where we need to start
                            // offset+current_total should not exceed block size now... probably
                            // tester will figure it out

                            if (write_partial_block(fs, data_iterator.location, data, current_total, current_offset)) {
                                // Success!
                                data_written += current_total;
                                data_to_write -= current_total;
                                data = INCREMENT_VOID_PTR(data, current_total);
                                offset += current_total;
                                current_offset = 0;
                                current_total = (data_to_write > BLOCK_SIZE) ? BLOCK_SIZE : data_to_write;
                                continue;
                            }
                            fprintf(stderr, "Block_store couldn't write, BS says: %s\n",
                                    block_store_strerror(block_store_errno()));
                            data_to_write = 0;
                            if (!data_written) {
                                data_written = -1;
                            }
                        }
                        // Ok, we have finished writing data. Or it broke. Or we at least wrote something.
                        if (data_written > 0) {
                            // We wrote something, we should update the inode to reflect that
                            file_inode.mdata.size += data_written;
                            if (!write_inode(fs, file_data.inode, &file_inode)) {
                                // Oh no.
                                // Size no longer reflects the allocated size.
                                fprintf(stderr, "Broke updating inode, data has disappeared (even though it's there), BS says: %s\n",
                                        block_store_strerror(block_store_errno()));
                                data_written = -1;
                            }

                            block_store_flush(fs->bs);
                            if (block_store_errno() != BS_OK) {
                                // Oh no.
                                fprintf(stderr, "Broke flushing data, BS in bad state, BS says: %s\n",
                                        block_store_strerror(block_store_errno()));
                                data_written = -1;
                            }

                        }

                    }
                }
            }
        }
    }
    return data_written;
}

// Finds block index of given position
// Sets the iterator's location field if found (0 on some failure)
// Ideally, this would be combined with the "check_and_allocate_block" function,
//   with allocation gated behind a creation flag
// Also, this function can be done with way less IO since we know the exact block and offset
//   we want to check and block_store isn't really a true blokc device
void find_block_index(const F15FS_t *const fs, data_itr_t *const file_info) {
    if (fs && file_info) {
        file_info->location = 0;
        // This is actually probably the wrong macro to use
        // But that's a problem for later
        block_ptr_t indir_block[INDIRECT_TOTAL];
        inode_t inode_data;
        if (load_inode(fs, file_info->inode, &inode_data)) {
            switch (file_info->position) {
                case DIRECT:
                    // Easy, woo!
                    // And, since I specified that uninitialized block pointers shall be zero, we can do a blind copy!
                    file_info->location = inode_data.data_ptrs[file_info->indices[0]];
                    return;
                case INDIRECT:
                    // Only slightly less easy
                    if (inode_data.data_ptrs[file_info->indices[0]] && load_block(fs, inode_data.data_ptrs[file_info->indices[0]], &indir_block)) {
                        file_info->location = indir_block[file_info->indices[1]];
                    }
                    return;
                case DBL_INDIRECT:
                    // Actually, turns out that this function is lovely. Writing is just THAT BAD.
                    if (inode_data.data_ptrs[file_info->indices[0]] && load_block(fs, inode_data.data_ptrs[file_info->indices[0]], &indir_block)
                            && load_block(fs, indir_block[file_info->indices[1]], &indir_block)) {
                        file_info->location = indir_block[file_info->indices[2]];
                    }
                    return;
                default:
                    // WHAT DID YOU DOOOOOOOOOOO?????????
                    return;
            }
        }

    }
}

//
// Reads nbytes from the specified file and offset to the given data pointer
// Increments the read/write position of the descriptor by the ammount read
// param fs the F15FS file
// param fname the name of the file to read from
// param data the buffer to write to
// param nbyte the number of bytes to read
// param offset the offset in the file to begin reading from
// return ammount read, < 0 on error
//
ssize_t fs_read_file(F15FS_t *const fs, const char *const fname, void *data, size_t nbyte, size_t offset) {
    ssize_t data_read = -1;
    if (fs && fname && fname[0] && strnlen(fname, FS_PATH_MAX) < FS_PATH_MAX && data && nbyte) {
        search_struct_t file_info;
        locate_file(fs, fname, &file_info);
        if (file_info.valid) {
            inode_t file_inode;
            if (load_inode(fs, file_info.inode, &file_inode)) {
                // Inode found and loaded, validate type and then size/offset
                if (INODE_IS_TYPE(&file_inode, REGULAR) && file_inode.mdata.size > offset) {
                    // Ok, request is to read SOMETHING (request may extend past file)
                    // That offset check should make us return -1 when at EOF

                    // Now we need to figure out our position
                    // Loop till we've read all we can (hopefully the full request)
                    // Update a_time
                    // Flush inode change
                    data_itr_t file_itr;
                    file_itr.inode = file_info.inode;

                    size_t data_to_read = file_inode.mdata.size - offset < nbyte ? (file_inode.mdata.size - offset) : nbyte;
                    data_read = 0;
                    size_t loop_offset = offset & 0x3FF; // only needed once, clear after first itr
                    size_t loop_read_ammount = (nbyte > (BLOCK_SIZE - loop_offset)) ? (BLOCK_SIZE - loop_offset) : nbyte; // must recalc every loop

                    // These functions are so much nicer without mallocs.
                    // The others are just embarassing, but hey, they work.

                    while (data_to_read) {
                        // Locate position. How? Not sure. Uhh, increment offset every iteration I suppose
                        get_position_and_index_tuple(offset, &file_itr);
                        find_block_index(fs, &file_itr);
                        if (file_itr.location) {
                            // load block and pipe it straight to data, woo!
                            if (load_partial_block(fs, file_itr.location, data, loop_read_ammount, loop_offset)) {
                                // Success!
                                // Next block!
                                loop_offset = 0;
                                data_read += loop_read_ammount;
                                data_to_read -= loop_read_ammount;
                                data = INCREMENT_VOID_PTR(data, loop_read_ammount);
                                loop_read_ammount = ((data_to_read > BLOCK_SIZE) ? BLOCK_SIZE : data_to_read);
                                continue;
                            }
                        } else {
                            // Just an FYI
                            fprintf(stderr, "Read position reached invalid block index, something very wrong somewhere.\n");
                        }
                        // Welp.
                        // We may or maynot have read some data from that block. Let's just assume we got no data
                        if (!data_read) {
                            // Assume it's broke if we haven't read anything yet
                            data_read = -1;
                        }
                        data_to_read = 0;
                    }
                    if (data_read) {
                        // Hooray for us, we read something.
                        // Update a_time
                        file_inode.mdata.a_time = time(NULL);
                        if (!(write_inode(fs, file_itr.inode, &file_inode) && (block_store_flush(fs->bs), block_store_errno() == BS_OK))) {
                            fprintf(stderr, "Error writing inode or flushing inode, BS says: %s\n", block_store_strerror(block_store_errno()));
                        }
                    }
                }
            }
        }
    }
    return data_read;
}
///
/// Function to remove the passed in file fs struct
/// \param fs the fs object for the fs system that is in use
/// \param fname the filepath of the file to be removed from filesytem
/// \return 0 on success and < 0 on failure or error
///
 int fs_remove_file(F15FS_t *const fs, const char *const fname)
{
	//Error check the passed in parameters, also checking the file path is a valid in size
	if( fs && fname && fname[0] && strnlen(fname, FS_PATH_MAX) < FS_PATH_MAX) 
	{
		search_struct_t file_data;
        locate_file(fs, fname, &file_data); // Locate the file to be removed
        if(file_data.valid) // Error check locate_file 
        {
        	inode_t file_inode;
        	if (load_inode(fs, file_data.inode, &file_inode)) // Load that inode of file to be removed
        	{
        	 	if( (INODE_IS_TYPE(&file_inode, REGULAR))) 
                {
                    fprintf(stderr, "INODE IS TYPE REGULAR for file \"%s\"\n", file_inode.fname);
        	 	     for(size_t i = 0; i < BLOCK_PTR_TOTAL; ++i) // Loop through data_ptrs to release blocks of file
        	 	     {
        	 		    if( (!file_inode.data_ptrs[i]) ){ // Check that the block ptr is valid
        	 			   break;
        	 		    }
        	 		    block_store_release(fs->bs, file_inode.data_ptrs[i]); 
        	 		    if(block_store_errno() == BS_OK)
        	 			   continue;
        	 		    else {
        	 			   fprintf(stderr, "bs release broke. Block store states: \"%s\"\n", block_store_strerror(block_store_errno()));
        	 			   return -1;
        	 		    }
        	 	     }
        	 	     inode_ptr_t parent_inode = file_inode.mdata.parent; // Find the parent inode, should be a directory
        	 	     inode_t parent_inode_data;
        	 	     if(load_inode(fs, parent_inode, &parent_inode_data)) // load directory inode into memory
        	 	     {
        	 		    if(INODE_IS_TYPE(&parent_inode_data, DIRECTORY))  // Error check that it is a directory
        	 		    {	
                            dir_block_t *dir_record = (dir_block_t *)malloc(sizeof(dir_block_t));
        	 			    if(load_block(fs, parent_inode_data.data_ptrs[0], dir_record)) // Load directory block into memory
        	 			    {	
                                size_t file_index_in_dir = -1;
                                for(size_t i = 0; i < DIR_REC_MAX; ++i) 
                                { 	// Traverse through the directory block looking for the file to be removed
        	 					     if(strncmp(file_inode.fname, (dir_record)->entries[i].fname, FNAME_MAX) == 0)
        	 					     {
        	 						        file_index_in_dir = i; // Save this index thus we can push all entries down 1 in
        	 											   //  the directory entries array
        	 						        strcpy((dir_record)->entries[i].fname, "");
        	 						        break;
        	 					     }	
        	 			        }
        	 				      if( !(file_index_in_dir < 0) ) // Error check that entry was found
        	 			   	      {    
                                    // If the file removed  was in the beginning or middle of the entries array we need to bum after thing down by 1 
        	 					     for(size_t j = file_index_in_dir,  i = (file_index_in_dir + 1); i < DIR_REC_MAX && j < DIR_REC_MAX; ++i, ++j)
        	 					     {
        	 						        (dir_record)->entries[j] = (dir_record)->entries[i]; 
        	 					     }
        	 					     memset(&file_inode, 0, sizeof(inode_t)); // Zero out the inode to be written to the inode table
        	 					     if(write_inode(fs, file_data.inode, &file_inode) && write_block(fs, parent_inode_data.data_ptrs[0], dir_record))
                                     {
        	 						    free(dir_record);
        	 						    return 0;
        	 					     }    
        	 				      }
                                  free(dir_record);
                                  fprintf(stderr, "Problem with file index in directory\n");
                            }
                            free(dir_record);
                            fprintf(stderr, "Could not load block\n");
                        }
                        fprintf(stderr, "Parent inode of file is not a directory\n");
                    }
                }
                else if(INODE_IS_TYPE(&file_inode, DIRECTORY)) 
                {
                    dir_block_t *dir_record = (dir_block_t *)malloc(sizeof(dir_block_t));
                    if(load_block(fs, file_inode.data_ptrs[0], dir_record)) {
                        if((dir_record)->mdata.size > 0){
                            fprintf(stderr, "Directory \"%s\" is not empty\n", file_inode.fname);
                            return -1;
                        } 
                        block_store_release(fs->bs, file_inode.data_ptrs[0]);
                        if(block_store_errno() == BS_OK) {

                            memset(&file_inode, 0, sizeof(inode_t));
                            if(write_inode(fs, file_data.inode, &file_inode)){
                                free(dir_record);
                                return 0;
                            }
                            free(dir_record);
                            fprintf(stderr, "Problem with write_inode\n");
                            return -1;
                        }
                        free(dir_record);
                        fprintf(stderr, "Block store release failed. Block store states: \"%s\"", block_store_strerror(block_store_errno()));
                        return -1;
                    }
                    fprintf(stderr, "Could not load_inode(...). Block store states: \"%s\"\n", block_store_strerror(block_store_errno()));
                    return -1; 
                }
                fprintf(stderr, "Problem with the type of the file\n");
                return -1;
        	 }
             fprintf(stderr, "Could not load_inode(...). Block store states: \"%s\"\n", block_store_strerror(block_store_errno()));
             return -1;	
        }
        fprintf(stderr, "Could not located the file\n");
        return -1;

    }
	fprintf(stderr, "Bad fs object or Bad filename: \"%s\"\n", fname); 
	return -1;
}

///
/// function to move the src file to the dest file place
/// \param fs the filesystem in use
/// \param fname_src the file name of the source file that will be moved
/// \param fname_dst
/// \return success is 0 or failure is < 0
///
int fs_move_file(F15FS_t *const fs, const char *const fname_src, const char *const fname_dst)
{
    //Error check the passed in parameters, also checking the file path is a valid in size
    if( fs && fname_src && fname_src[0] && fname_dst && fname_dst[0] && strnlen(fname_src, FS_PATH_MAX) < FS_PATH_MAX && strnlen(fname_dst, FS_PATH_MAX) < FS_PATH_MAX)  
    {
        search_struct_t file_data;
        locate_file(fs, fname_src, &file_data); // Locate the file to be removed
        if(file_data.valid) // Error check locate_file 
        {
            inode_t file_inode;
            if (load_inode(fs, file_data.inode, &file_inode)) // Load that inode of file to be removed
            {
                if( (INODE_IS_TYPE(&file_inode, REGULAR))) 
                {
                     inode_ptr_t parent_inode = file_inode.mdata.parent; // Find the parent inode, should be a directory
                     inode_t parent_inode_data;
                     if(load_inode(fs, parent_inode, &parent_inode_data)) // load directory inode into memory
                     {
                        if(INODE_IS_TYPE(&parent_inode_data, DIRECTORY))  // Error check that it is a directory
                        {   
                            dir_block_t *dir_record = (dir_block_t *)malloc(sizeof(dir_block_t));
                            if(load_block(fs, parent_inode_data.data_ptrs[0], dir_record)) // Load directory block into memory
                            {

                            }
                        }
                    }
                }
                fprintf(stderr, "Can not move a directory");
                return -1; 
            }
            fprintf(stderr, "Problem with load_inode\n");
            return -1;
        }
        fprintf(stderr, "Problem locating the file\n");
        return -1;
    }
    fprintf(stderr, "Problem with function parameters\n");
	return -1;
}

