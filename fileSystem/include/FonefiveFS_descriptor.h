#ifndef FONEFIVE_H__
#define FONEFIVE_H__

#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <block_store.h>
#include <dyn_array.h>
// Probably other things

typedef struct F15FS F15FS_t;

// Enum to differentiate between different kinds of files
typedef enum {
    REGULAR = 0x01, DIRECTORY = 0x02
} ftype_t;

// Enum for seeking. They do what they sound like. Check the seek man pages
typedef enum {
	FS_SEEK_BEGIN = 0x01, FS_SEEK_CUR = 0x02, FS_SEEK_END = 0x04
} seek_t;

// They are what they sound like, the max filename (not counting terminator)
// and the number of things a directory can contain
// Have to be exposed in the header for the record structure, which is annoying
// But the other option is to add more functions to parse and handle that struct
#define FNAME_MAX 47
#define DIR_REC_MAX 20

// It's a directory entry. Won't really be used internally
typedef struct {
	ftype_t ftype;
	char fname[FNAME_MAX+1];
} dir_entry_t;

// It's a directory record, used to report directory contents to the user
// Won't really be used internally
typedef struct dir_rec {
    unsigned total; // total valid entries
    dir_entry_t contents[DIR_REC_MAX];
} dir_rec_t;

typedef uint32_t block_ptr_t; // 32-bit addressing, woo!
typedef uint8_t inode_ptr_t; // inodes go from 0 - 255

//That's it?
struct F15FS {
	block_store_t *bs;
};

// got 48 bytes to spare
typedef struct {
	uint32_t size; // Meaningless if it's a directory
	uint32_t mode; // Probably won't be used 
	uint32_t c_time; // technically c_time is for modifications to mode and			stuff ... I'll use it as creation
	uint32_t a_time;
	uint32_t m_time;
	inode_ptr_t parent; // handy!
	uint8_t type; // haha, whoops, almost forgot it!
	// I'd prefer it was the actual type enum, but the size of that is...hard to pin down
	// Uhh...26 bytes left...
	uint8_t padding[26];
} mdata_t;

// It's an inode...
typedef struct {
	char fname[FNAME_MAX + 1];
	mdata_t mdata;
	block_ptr_t data_ptrs[8];
} inode_t;

typedef uint8_t data_block_t[1024];// data's just data, also c is weird

// Sadly, not the same size as an mdata_t, but it's close! 44 Bytes
// They can really be cast back and forth safetly since the end is padding anyway
// But if you have an array of them. it could get messy, but why would you ever have one of those?
typedef struct {
	uint32_t size;
	// Number of the VALID entries, ENTRIES ARE NOT CONTIGUOUS
	// Which means all entries will need to be scanned every time
	//Maintaining contiguity can be done, and in a transaction safe way, but it's extra work
	uint32_t mode;
	uint32_t c_time; // technically c_time is for modifications to mode and stuff...we'll use it as creation
	uint32_t a_time;
	uint32_t m_time;
	// May want a parent pointer, but it may be better to only have it in the inode so we don't get out of sync
	// inode_ptr_t parent;
	// Skipping type becasue it's not useful
	// that leaves 24B
	uint8_t padding[24];
} dir_mdata_t;

// directory entry, found inside directory blocks
typedef struct {
	char fname[FNAME_MAX + 1];
	inode_ptr_t inode;
} dir_ent_t;

typedef struct {
	dir_mdata_t mdata;
	dir_ent_t entries[DIR_REC_MAX];
} dir_block_t;

//size of data blocks
#define BLOCK_SIZE 1024

//Total inode blocks
#define INODE_BLOCK_TOTAL 32

//Number of inodes in a block
#define INODE_PER_BLOCK (BLOCK_SIZE / sizeof(inode_t))

//Total number of inodes
#define INODE_TOTAL (INODE_BLOCK_TOTAL * BLOCK_SIZE / sizeof(inode_t))

//Inode blocks start at 8 and go to 40
#define INODE_BLOCK_OFFSET 8

//Where data blocks start; after inode table
#define DATA_BLOCK_OFFSET (INODE_BLOCK_OFFSET + INODE_BLOCK_TOTAL)

//6 direct block ptrs per inode
#define DIRECT_TOTAL 6

//Number of direct block ptrs in a indirect block ptr, same as in inode
#define INDIRECT_TOTAL (BLOCK_SIZE / sizeof(block_ptr_t))

//Number of direct block ptrs in a full double indirect block pt, same as in inode
#define DBL_INDIRECT_TOTAL (INDIRECT_TOTAL * INDIRECT_TOTAL)

//Max size of a file
#define FILE_SIZE_MAX ((DIRECT_TOTAL + INDIRECT_TOTAL + DBL_INDIRECT_TOTAL) * BLOCK_SIZE) 

//Total number of available data blocks in block store object
#define DATA_BLOCK_MAX 65536

//It should actually be less(about 12100)
//But we'll over allocate, preferably not on the stack ...
#define FS_PATH_MAX 13000

//Calc's what block an inode is in
#define INODE_TO_BLOCK(inode) ((inode >> 3) + INODE_BLOCK_OFFSET)

//Calc's the index of an inode is at within a block
#define INODE_INNER_IDX(inode) ((inode) & 0x07)

//Calc's the offset of an inode
#define INODE_INNER_OFFSET(inode) (INODE_INNER_IDX(inode) * sizeof(inode_t))

//Converts a file position to a block index (Note: not block id. index 6 is the 6th block of the file
#define POSITION_TO_BLOCK_INDEX(position) (position >> 10) 

//Position within a block
#define POSITION_TO_INNER_OFFSET(position) ((position) & 0x3FF)

//Validate block id number 
#define BLOCK_IDX_VALID(block_idx) ((block_idx) >= DATA_BLOCK_OFFSET && (block_idx) < DATA_BLOCK_MAX)

//Checks that an inode is the specified type
//Used because the mdata type won't be the same size as the enum & that will irritate the compiler(most likely)
#define INODE_IS_TYPE(inode_ptr, file_type) ((inode_ptr)->mdata.type & (file_type))

//Because you can't increment an incomplete type
//And, writing the cast every time gets tiring
#define INCREMENT_VOID_PTR(v_ptr, increment) (((uint8_t *)v_ptr) + (increment))

///
/// Creates a new F15FS file at the given location
/// \param fname The file to create (or overwrite)
/// \return Error code, 0 for success, < 0 for error
///
int fs_format(const char *const fname);

///
/// Mounts the specified file and returns an F15FS object
/// \param fname the file to load
/// \return An F15FS object ready to use, NULL on error
///
F15FS_t *fs_mount(const char *const fname);

/// Unmounts, closes, and destructs the given object,
///  saving all unwritten contents (if any) to file
/// \param fs The F15FS file
/// \return 0 on success, < 0 on error
///
int fs_unmount(F15FS_t *fs);

///
/// Creates a new file in the given F15FS object
/// \param fs the F15FS file
/// \param fname the file to create
/// \param ftype the type of file to create
/// \return 0 on success, < 0 on error
///
int fs_create_file(F15FS_t *const fs, const char *const fname, const ftype_t ftype);

///
/// Returns the contents of a directory
/// \param fs the F15FS file
/// \param fname the file to query
/// \param records the record object to fill
/// \return 0 on success, < 0 on error
///
int fs_get_dir(F15FS_t *const fs, const char *const fname, dir_rec_t *const records);

///
/// Opens a file for read/write (note: Directories cannot be opened)
/// \param fs the F15FS file
/// \param fname the file to open
/// \return File descriptor on success, < 0 on error
///
int fs_open_file(F15FS_t *const fs, const char * const fname);

///
/// Closes an open file
/// \param fs the F15FS file
/// \param fd the file descriptor
/// \return 0 on success, < 0 on error
///
int fs_close_file(F15FS_t *const fs, const int fd);

///
/// Seeks the read/write position of the given descriptor
/// \param fs the F15FS file
/// \param fd the file descriptor
/// \param offset the offset relative to whence
/// \param whence the position from which the offset is based
/// \return 0 on success < 0 on error
///
int fs_seek_file(F15FS_t *const fs, const int fd, const off_t offset, const seek_t whence);

///
/// Writes nbytes from the given buffer to the specified file and offset
/// Increments the read/write position of the descriptor by the ammount written
/// \param fs the F15FS file
/// \param fd the file descriptor
/// \param data the buffer to read from
/// \param nbyte the number of bytes to write
/// \return ammount written, < 0 on error
///
ssize_t fs_write_file(F15FS_t *const fs, const int fd, const void *data, size_t nbyte);

///
/// Reads nbytes from the specified file and offset to the given data pointer
/// Increments the read/write position of the descriptor by the ammount read
/// \param fs the F15FS file
/// \param fd the file descriptor
/// \param data the buffer to write to
/// \param nbyte the number of bytes to read
/// \return ammount read, < 0 on error
///
ssize_t fs_read_file(F15FS_t *const fs, const int fd, void *data, size_t nbyte);

///
/// Removes a file. (Note: Directories cannot be deleted unless empty)
/// This closes any open descriptors to this file
/// \param fs the F15FS file
/// \param fname the file to remove
/// \return 0 on sucess, < 0 on error
///
int fs_remove_file(F15FS_t *const fs, const char *const fname);

///
/// Moves the file from the source name to the destination name
/// Note: This does not invalidate any file descriptors
/// \param fs the F15FS file
/// \param fname_src the file to move
/// \param fname_dst the file's new location
/// \return 0 on success, < 0 on error
///
int fs_move_file(F15FS_t *const fs, const char *const fname_src, const char *const fname_dst);

///
/// Load inodeTable into memory, thus we can use it for reference
/// \param fs the filesytem in use
/// \param inodeTable array of inodes; size is 256
/// returns 1 on success and returns 0 if error occurs
///
int load_inodeTable(F15FS_t *const fs, inode_t* inodeTable);

///
/// Parses file path of which returns the parent directory of the this filepath
/// \param fname the file path
/// \param p_dir the parent directory we will use to store the found parent directory
/// \returns 1 for success and returns 0 if error occurs
///
int parse_file_path(const char *const fname, char* p_dir);

///
/// Parses the fname for the parent directory if the parent directory is not root
/// \param fname the file path
/// \param dir_start the start of the parent directory(Note: this index will represent the '/' before the parent directory name)
/// \param dir_end the end of the parent directoy(Note: this index will represent the '/' at the end of the parent directory name)
/// \param p_dir parent directory array, thus will be used to store the found parent directory
/// \returns 1 for success and returns 0 if error occurs
///
int parse_for_parent_dir(const char *const fname, size_t dir_start, size_t dir_end, char* p_dir);

///
/// Parse the file path for the filename
/// \param fname the filepath
/// \param filename_target char array we will use to store the found filename in given file path 
/// \returns 1 for success and returns 0 if error occurs
///
int parse_for_filename(const char *const fname, char* filename_target);

///
/// Get the directory block from bs given a block_id
/// \param fs the file system in use
/// \param dir_block_id the block id where the directory block lives
/// \returns pointer to directory block and returns NULL on error
///
dir_block_t* get_dir_block(F15FS_t *const fs, size_t dir_block_id);

///
/// Initialize new inode with info
/// \param *inode pointer to inode that is being created
/// \param filename the name of the file that this inode will represent
/// \param ftype the type of file this inode will be
/// \param file_block_id the allocated block in bs block_id; what the first direct block pointer will point to
/// \param parent_inode the inode parent of this new inode
/// \returns 1 if no errors occurred and return 0 if errors occur
///
int init_inode(inode_t* inode, const char *const filename, ftype_t ftype, size_t file_block_id, uint8_t parent_inode);

///
/// Find an available inode in the inodeTable and return it's index in the inodeTable array
/// \param inodeTable array of inodes from 0 to 255
/// \return the available inode index and return -1 if error occurs
///
int find_avail_inode(inode_t* inodeTable);

///
/// Find the inode index that matches the given filename; This index is where the seeked inode lives in the inodeTable
/// \param inodeTable array of all inodes in the inodeTable; inodeTable goes from 0 to 255
/// \param filename the filename we are seeking in the inodeTable
/// \returns the index of the inode that matches the given filename and returns -1 if error occurs
///
int find_inode(inode_t* inodeTable, const char *const filename);

///
/// Write inodeTable to bs
/// \param fs the file system in use
/// \param inodeTable array of inodes; from 0 to 255
/// \returns 1 for success of writing and returns 0 if error occurs
///
int write_inodeTable(F15FS_t *const fs, inode_t* inodeTable);

///
/// Init a directory entry
/// \param dir_block data block that represents the directory block
/// \param inodeID inode id where this new directory entry lives in the inode table
/// \param filename the name of the new file being added to the dir_block entries
/// \return 1 if success and return 0 if error occurs 
///
int init_dir_entry(dir_block_t* dir_block, int inodeID, const char *const filename);

///
/// Find the directory entry inode in a given directory block
/// \param dir_block the directory block where the given filename lives
/// \param filename the filename we are seeking in the directory entries
/// \return inode idx where filename lives or return -1 if error occurs
///
int find_dir_entry_inode(dir_block_t* dir_block, char* filename);

#endif
