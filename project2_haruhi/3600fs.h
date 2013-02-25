/*
 * CS3600, Spring 2013
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600FS_H__
#define __3600FS_H__

#define MAGICNUM 63923392

typedef struct vcb_s {
    int disk_id; // Unique disk identifier.

    int mounted:1; // Flag that is set to 1 when disk is mounted.
		   // Used to detect unmounting errors.

    // Disk layout metadata.
    int blocksize; // Size of a block in the disk.
    
    int de_start;  // Starting location of directory entry blocks.
    int de_length;   // Length of directory entry block section.

    int fat_start; // Starting location of file allocation table.
    int fat_length;  // Length of file allocation table.

    int db_start;  // Starting location of the data block section.

    // Root directory metadata.
    uid_t userid;
    gid_t groupid;
    mode_t mode;

    struct timespec access_time;
    struct timespec modify_time;
    struct timespec create_time;
} vcb;

typedef struct dirent_s {
    unsigned int valid;
    unsigned int first_block;
    unsigned int size;

    uid_t userid;
    gid_t groupid;
    mode_t mode;

    struct timespec access_time;
    struct timespec modify_time;
    struct timespec create_time;

    char name[27];
} dirent;

typedef struct fatent_s {
    unsigned int used:1;
    unsigned int eof:1;
    unsigned int next:30;
} fatent;
#endif

