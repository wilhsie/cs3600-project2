/*
 * CS3600, Spring 2013
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This file contains all of the basic functions that you will need 
 * to implement for this project.  Please see the project handout
 * for more details on any particular function, and ask on Piazza if
 * you get stuck.
 */

#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#define _POSIX_C_SOURCE 199309

#include <time.h>
#include <fuse.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <sys/statfs.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "3600fs.h"
#include "disk.h"


// Parses path, seperating file name from directory 
// Returns 0 if there is only one slash, path becomes file name
// Returns -1 if there is zero or more than one slash, path remains the same.
int filename_parser(char* path){
    int found = 0;
    int numslash = 0;
  
    char* temp = path;  // Temp copy of path to traverse through.
    char* firstslash = path; // Will point to location of first slash.
    while(*temp){
      if(*temp == '/'){
        numslash++;
        if(found == 0){
          found = 1;
          firstslash = temp;
        }
      }
      temp++;
    }
    if(numslash==1){
      path = firstslash;
      return 0;
    }
   return -1;
 }


/*
 * Initialize filesystem. Read in file system metadata and initialize
 * memory structures. If there are inconsistencies, now would also be
 * a good time to deal with that. 
 *
 * HINT: You don't need to deal with the 'conn' parameter AND you may
 * just return NULL.
 *
 */
static void* vfs_mount(struct fuse_conn_info *conn) {
  fprintf(stderr, "vfs_mount called\n");

  // Do not touch or move this code; connects the disk
  dconnect();
  
  /* 3600: YOU SHOULD ADD CODE HERE TO CHECK THE CONSISTENCY OF YOUR DISK
           AND LOAD ANY DATA STRUCTURES INTO MEMORY */

  vcb volblock;
  char temp[BLOCKSIZE];
  memset(temp,0,BLOCKSIZE);
  dread(0,temp);
  memcpy(&volblock,temp,sizeof(volblock));

  if(volblock.disk_id != MAGICNUM){
   fprintf(stderr,"Invalid disk: Invalid magic number.");
   dunconnect();
  }
  if(volblock.mounted != 0){
   fprintf(stderr,"Invalid disk: Disk did not unmount correctly.");    
   // For now dismount, we will handle bad sectors/blocks later (heh ehe.. heh)
   dunconnect();
  }
  // TODO: Possibly check dirents for invalid info 
  // (e.g. missing data in dirents). Deal with them maybe.
  // Else, magic number is correct (its our drive) and was unmounted correctly
  // So we set mounted to be 1 and write vcb to disk
  volblock.mounted = 1;
  memcpy(temp,&volblock,sizeof(BLOCKSIZE));
  dwrite(0,temp);
}

/*
 * Called when your file system is unmounted.
 *
 */
static void vfs_unmount (void *private_data) {
  fprintf(stderr, "vfs_unmount called\n");

  vcb volblock;
  char temp[BLOCKSIZE];
  memset(temp,0,BLOCKSIZE);
  dread(0,temp);
  memcpy(&volblock,temp,sizeof(volblock));
  
  volblock.mounted = 0; // Flip mounted bit, indicating safe dismount.

  memcpy(&volblock,temp,sizeof(volblock));
  dwrite(0, temp);

  /* 3600: YOU SHOULD ADD CODE HERE TO MAKE SURE YOUR ON-DISK STRUCTURES
           ARE IN-SYNC BEFORE THE DISK IS UNMOUNTED (ONLY NECESSARY IF YOU
           KEEP DATA CACHED THAT'S NOT ON DISK */

  // Do not touch or move this code; unconnects the disk
  dunconnect();
}

/* 
 *
 * Given an absolute path to a file/directory (i.e., /foo ---all
 * paths will start with the root directory of the CS3600 file
 * system, "/"), you need to return the file attributes that is
 * similar stat system call.
 *
 * HINT: You must implement stbuf->stmode, stbuf->st_size, and
 * stbuf->st_blocks correctly.
 *
 */
static int vfs_getattr(const char *path, struct stat *stbuf) {
  fprintf(stderr, "vfs_getattr called\n");

  // Do not mess with this code 
  stbuf->st_nlink = 1; // hard links
  stbuf->st_rdev  = 0;
  stbuf->st_blksize = BLOCKSIZE;

  /* 3600: YOU MUST UNCOMMENT BELOW AND IMPLEMENT THIS CORRECTLY */

  // TODO: Parse path so that it contains only the filename

  // Create a temp block and format it to be a directory entry.
  dirent direntblock;
  vcb volblock;
  char temp[BLOCKSIZE];
  memset(temp,0,BLOCKSIZE);
  
  if(filename_parser(path) == 0){ // if we are requesting a valid path
    fprintf(stderr, "%s", path);
    if(strlen(path) == 1){ // if we are requesting attr of root directory
      dread(0, temp);
      memcpy(&volblock, temp, sizeof(BLOCKSIZE));
      
      struct tm * tm1;
      struct tm * tm2;
      struct tm * tm3;
      tm1 = localtime(&((direntblock.access_time).tv_sec));
      tm2 = localtime(&((direntblock.modify_time).tv_sec));
      tm3 = localtime(&((direntblock.create_time).tv_sec));
      
      stbuf->st_mode = (volblock.mode & 0x0000ffff) | S_IFDIR; // TODO: Check to ensure volblock.mode might always be 0777
      
      stbuf->st_uid = volblock.userid;
      stbuf->st_gid = volblock.groupid;
      stbuf->st_atime = mktime(tm1); 
      stbuf->st_mtime = mktime(tm2);
      stbuf->st_ctime = mktime(tm3);
      stbuf->st_size = BLOCKSIZE;
      stbuf->st_blocks = 1;
    } 
    else { // if we are requesting attr of a file
      path++; // increase the pointer of path, to account for the leading /
      // Traverse through directory entries
      for(int i = 1; i < 101; i++){
        dread(i,temp);
        // Mold temp data to dirent structure direntblock
        memcpy(&direntblock, temp, sizeof(BLOCKSIZE));
        if(direntblock.valid && (strcmp(direntblock.name, path) == 0)){
          // We have a match, path == direntblock.name
          // Break and copy direntblock info into memory
          struct tm * tm1;
          struct tm * tm2;
          struct tm * tm3;
          tm1 = localtime(&((direntblock.access_time).tv_sec));
          tm2 = localtime(&((direntblock.modify_time).tv_sec));
          tm3 = localtime(&((direntblock.create_time).tv_sec));

          stbuf->st_mode  = (direntblock.mode & 0x0000ffff) | S_IFREG;
      
          stbuf->st_uid = direntblock.userid; 
          stbuf->st_gid = direntblock.groupid;
          
          stbuf->st_atime = mktime(tm1); 
          stbuf->st_mtime = mktime(tm2);
          stbuf->st_ctime = mktime(tm3);
          
          stbuf->st_size = direntblock.size;
          stbuf->st_blocks = (int) (direntblock.size / BLOCKSIZE);
          // We have updated stbuf, return success.
          return 0;
	}
      }
      // Traversed through all 100 dirents and we did not write to st_buf then...
      return -ENOENT;
    }
  }else // if we have a bad path
     return -1;
}
/*
 * Given an absolute path to a directory (which may or may not end in
 * '/'), vfs_mkdir will create a new directory named dirname in that
 * directory, and will create it with the specified initial mode.
 *
 * HINT: Don't forget to create . and .. while creating a
 * directory.
 */
/*
 * NOTE: YOU CAN IGNORE THIS METHOD, UNLESS YOU ARE COMPLETING THE 
 *       EXTRA CREDIT PORTION OF THE PROJECT.  IF SO, YOU SHOULD
 *       UN-COMMENT THIS METHOD.
static int vfs_mkdir(const char *path, mode_t mode) {

  return -1;
} */

/** Read directory
 *
 * Given an absolute path to a directory, vfs_readdir will return 
 * all the files and directories in that directory.
 *
 * HINT:
 * Use the filler parameter to fill in, look at fusexmp.c to see an example
 * Prototype below
 *
 * Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *                                 const struct stat *stbuf, off_t off);
 *			   
 * Your solution should not need to touch offset and fi
 *
 */
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi){

    return 0;
}

/*
 * Given an absolute path to a file (for example /a/b/myFile), vfs_create 
 * will create a new file named myFile in the /a/b directory.
 *
 */
static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {      
  // Next, we want to check the path for errors and extract file name.

  if(filename_parser(path) != 0) // Check for valid path and set filename.
    return -1;

  printf("%s", path);

  // Create a temp block and format it to be a directory entry.
  dirent direntblock;
  char temp[BLOCKSIZE];
  memset(temp,0,BLOCKSIZE);

  // Traverse through directory entries.
  for(int i = 1; i < 101; i++){
    dread(i,temp);
    // Put temp data into dirent structure direntblock
    memcpy(&direntblock,temp,sizeof(BLOCKSIZE));
    // Compare current valid directory entry name to argument string 'path'
    if(direntblock.valid && (strcmp(direntblock.name, path) == 0)){
      // The path already exists so we cannot create it.
      printf("Error: File already exists.");
      return -EEXIST;
    }
  }

  // Traverse through directory entries, we now know path does not exist.
  for(int i = 1; i < 101; i++){
    dread(i, temp);
    memcpy(&direntblock,temp,sizeof(BLOCKSIZE));
    // Find first invalid block, flip valid bit, create dirent with path as name
    if(!direntblock.valid){
      direntblock.valid = 1;
      direntblock.userid = getuid();
      direntblock.groupid = getgid();
      direntblock.mode = mode;
      clock_gettime(CLOCK_REALTIME, &direntblock.access_time);
      clock_gettime(CLOCK_REALTIME, &direntblock.modify_time);
      clock_gettime(CLOCK_REALTIME, &direntblock.create_time);
      strcpy(direntblock.name, path);
      
      // Write direntblock to a character buffer then write to disk
      memcpy(temp,&direntblock,sizeof(BLOCKSIZE));
      dwrite(i, temp);
      return 0;
    }
  }
  // If there is no more room on the disk return -1;
  return -1;
}

/*
 * The function vfs_read provides the ability to read data from 
 * an absolute path 'path,' which should specify an existing file.
 * It will attempt to read 'size' bytes starting at the specified
 * offset (offset) from the specified file (path)
 * on your filesystem into the memory address 'buf'. The return 
 * value is the amount of bytes actually read; if the file is 
 * smaller than size, vfs_read will simply return the most amount
 * of bytes it could read. 
 *
 * HINT: You should be able to ignore 'fi'
 *
 */
static int vfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{

    return 0;
}

/*
 * The function vfs_write will attempt to write 'size' bytes from 
 * memory address 'buf' into a file specified by an absolute 'path'.
 * It should do so starting at the specified offset 'offset'.  If
 * offset is beyond the current size of the file, you should pad the
 * file with 0s until you reach the appropriate length.
 *
 * You should return the number of bytes written.
 *
 * HINT: Ignore 'fi'
 */
static int vfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{

  /* 3600: NOTE THAT IF THE OFFSET+SIZE GOES OFF THE END OF THE FILE, YOU
           MAY HAVE TO EXTEND THE FILE (ALLOCATE MORE BLOCKS TO IT). */

  return 0;
}

/**
 * This function deletes the last component of the path (e.g., /a/b/c you 
 * need to remove the file 'c' from the directory /a/b).
 */
static int vfs_delete(const char *path)
{

  /* 3600: NOTE THAT THE BLOCKS CORRESPONDING TO THE FILE SHOULD BE MARKED
           AS FREE, AND YOU SHOULD MAKE THEM AVAILABLE TO BE USED WITH OTHER FILES */
    return 0;
}

/*
 * The function rename will rename a file or directory named by the
 * string 'oldpath' and rename it to the file name specified by 'newpath'.
 *
 * HINT: Renaming could also be moving in disguise
 *
 */
static int vfs_rename(const char *from, const char *to)
{

    return 0;
}


/*
 * This function will change the permissions on the file
 * to be mode.  This should only update the file's mode.  
 * Only the permission bits of mode should be examined 
 * (basically, the last 16 bits).  You should do something like
 * 
 * fcb->mode = (mode & 0x0000ffff);
 *
 */
static int vfs_chmod(const char *file, mode_t mode)
{

    return 0;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid)
{

    return 0;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2])
{

    return 0;
}

/*
 * This function will truncate the file at the given offset
 * (essentially, it should shorten the file to only be offset
 * bytes long).
 */
static int vfs_truncate(const char *file, off_t offset)
{

  /* 3600: NOTE THAT ANY BLOCKS FREED BY THIS OPERATION SHOULD
           BE AVAILABLE FOR OTHER FILES TO USE. */

    return 0;
}


/*
 * You shouldn't mess with this; it sets up FUSE
 *
 * NOTE: If you're supporting multiple directories for extra credit,
 * you should add 
 *
 *     .mkdir	 = vfs_mkdir,
 */
static struct fuse_operations vfs_oper = {
    .init    = vfs_mount,
    .destroy = vfs_unmount,
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .create	 = vfs_create,
    .read	 = vfs_read,
    .write	 = vfs_write,
    .unlink	 = vfs_delete,
    .rename	 = vfs_rename,
    .chmod	 = vfs_chmod,
    .chown	 = vfs_chown,
    .utimens	 = vfs_utimens,
    .truncate	 = vfs_truncate,
};

int main(int argc, char *argv[]) {
    /* Do not modify this function */
    umask(0);
    if ((argc < 4) || (strcmp("-s", argv[1])) || (strcmp("-d", argv[2]))) {
      printf("Usage: ./3600fs -s -d <dir>\n");
      exit(-1);
    }
    return fuse_main(argc, argv, &vfs_oper, NULL);
}

