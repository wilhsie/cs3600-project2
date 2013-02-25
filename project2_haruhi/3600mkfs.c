/*
 * CS3600, Spring 2013
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This program is intended to format your disk file, and should be executed
 * BEFORE any attempt is made to mount your file system.  It will not, however
 * be called before every mount (you will call it manually when you format 
 * your disk file).
 */

#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "3600fs.h"
#include "disk.h"

int get_num_dblocks(int size){
  int fat_plus_db = 101 - size;
  return fat_plus_db * (128 / 129);
}

vcb make_volblock(int num_dblocks){

  vcb volblock;
  volblock.disk_id = MAGICNUM;
  volblock.blocksize = BLOCKSIZE;
  
  volblock.de_start = BLOCKSIZE + 1;
  volblock.de_length = 100; 
  
  volblock.fat_start = (BLOCKSIZE*volblock.de_length)+volblock.de_start;
  volblock.fat_length = (int) (num_dblocks/128);

  volblock.db_start = (BLOCKSIZE*volblock.fat_length)+volblock.fat_start;

  volblock.userid = getuid();
  volblock.groupid = getgid();
  volblock.mode = 0777;
  
  clock_gettime(CLOCK_REALTIME, &volblock.access_time);
  clock_gettime(CLOCK_REALTIME, &volblock.modify_time);
  clock_gettime(CLOCK_REALTIME, &volblock.create_time);

  return volblock;
}

// FIXME: We need to account for small sized dirents. Internal frags.
// TODO: Make size do something. Maybe.
dirent make_directent(){
  dirent de;
  de.valid = 0;
  de.size = 0;
  return de;
}
 
void myformat(int size) {
  // Do not touch or move this function
  dcreate_connect();
  
  // Calculate number of data blocks, FAT entries.
  int num_dblocks = get_num_dblocks(size);

  // Format volume control block.
  vcb volblock = make_volblock(num_dblocks);
  char vcbtemp[BLOCKSIZE];
  memset(vcbtemp,0,BLOCKSIZE);
  memcpy(vcbtemp,&volblock,sizeof(vcb));
  dwrite(0,vcbtemp);
  
  // Format directory entries.
  dirent dent = make_directent();
  char dirtemp[BLOCKSIZE]; //FIXME: Eventually will have to be smaller.
  memset(dirtemp,0,BLOCKSIZE);
  memcpy(dirtemp,&dent, sizeof(dirent));
  
  for(int i = 1; i < 101; i++){
    dwrite(i,dirtemp);
  }




/*
  dirent dent;

  char dirblock[BLOCKSIZE];
  memset(dirblock,0,BLOCKSIZE);
  memcpy(dirblock,&dirent,sizeof(dirblock));
  for(int i = 1; i <= 100; i++){
   dwrite(i,dirblock);
  }  */

  /* 3600: FILL IN CODE HERE.  YOU SHOULD INITIALIZE ANY ON-DISK
           STRUCTURES TO THEIR INITIAL VALUE, AS YOU ARE FORMATTING
           A BLANK DISK.  YOUR DISK SHOULD BE size BLOCKS IN SIZE. */

  /* 3600: AN EXAMPLE OF READING/WRITING TO THE DISK IS BELOW - YOU'LL
           WANT TO REPLACE THE CODE BELOW WITH SOMETHING MEANINGFUL. */


/*
  // first, create a zero-ed out array of memory  
  char *tmp = (char *) malloc(BLOCKSIZE);
  memset(tmp, 0, BLOCKSIZE);

  // now, write that to every block
  for (int i=0; i<size; i++) 
    if (dwrite(i, tmp) < 0) 
      perror("Error while writing to disk");
*/
  // voila! we now have a disk containing all zeros

  // Do not touch or move this function
  dunconnect();
}

int main(int argc, char** argv) {
  // Do not touch this function
  if (argc != 2) {
    printf("Invalid number of arguments \n");
    printf("usage: %s diskSizeInBlockSize\n", argv[0]);
    return 1;
  }

  unsigned long size = atoi(argv[1]);
  printf("Formatting the disk with size %lu \n", size);
  myformat(size);
}
