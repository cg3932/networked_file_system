/* nfs_test.c 
 * 
 * Written by Robert Vincent for Programming Assignment #3.
 *
 * Usage: nfs_test [host1 port1 [host2 port2]]
 * 
 * I recommend you create a file named something like "nfs_api.c" or
 * "nfs_client.c", which will contain the functions required by this
 * test program. Then you will build the test program, for example, with the 
 * command:
 *
 * gcc -g -o nfs_test nfs_test.c nfs_client.c
 *
 * You will separately need to create and run two instances of your
 * file server program, which will need to be listening on the port
 * numbers you specify on the command line of nfs_test.c
 *
 * Your file server will probably need to link directly to your code
 * for PA1 and PA2 in order to implement the actual file and disk
 * abstraction used by the file server:
 *
 * gcc -g -o nfs_server nfs_server.c sfs_api.c disk_emu.c
 *
 * Your nfs_server.c will need to take an argument to specify the port
 * number on which to listen (the host name is determined by the computer
 * that is running the server). If you want to test locally, you can just
 * use the standard host name 'localhost'.
 *
 * NOTE: If your file server uses the same file name for the emulated disk
 * in all cases, you will probably want to run the two server instances
 * in separate directories.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME1 "test1"
#define NAME2 "test2"
#define TEST1 "0123456789-abcdefghijklmnopqrstuvwxyz"
#define TEST2 "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"


#include "nfs_api.h"

/* The maximum file name length. We assume that filenames can contain
 * upper-case letters and periods ('.') characters. Feel free to
 * change this if your implementation differs.
 */
#define MAX_FNAME_LENGTH 12   /* Assume at most 12 characters (8.3) */

/* The maximum number of files to attempt to open or create.  NOTE: we
 * do not _require_ that you support this many files. This is just to
 * test the behavior of your code.
 */
#define MAX_FD 100 

/* The maximum number of bytes we'll try to write to a file. If you
 * support much shorter or larger files for some reason, feel free to
 * reduce this value.
 */
#define MAX_BYTES 30000 /* Maximum file size I'll try to create */
#define MIN_BYTES 10000         /* Minimum file size */

/* rand_name() - return a randomly-generated, but legal, file name.
 *
 * This function creates a filename of the form xxxxxxxx.xxx, where
 * each 'x' is a random upper-case letter (A-Z). Feel free to modify
 * this function if your implementation requires shorter filenames, or
 * supports longer or different file name conventions.
 * 
 * The return value is a pointer to the new string, which may be
 * released by a call to free() when you are done using the string.
 */
 
char *rand_name() 
{
  char fname[MAX_FNAME_LENGTH+1];
  int i;

  for (i = 0; i < MAX_FNAME_LENGTH; i++) {
    if (i != 8) {
      fname[i] = 'A' + (rand() % 26);
    }
    else {
      fname[i] = '.';
    }
  }
  fname[i] = '\0';
  return (strdup(fname));
}

int test_phase_one(char *filesys)
{
  int i;
  int j;
  int k;
  int chunksize;
  int readsize;
  int error_count = 0;
  int fds[MAX_FD];
  char *names[MAX_FD];
  int tmp;
  int filesize[MAX_FD];
  char fixedbuf[1024];
  char *buffer;

  /* First we open two files and attempt to write data to them.
   */
  for (i = 0; i < 2; i++) {
    names[i] = rand_name();
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] < 0) {
      fprintf(stderr, "ERROR: creating first test file %s\n", names[i]);
      error_count++;
    }
    tmp = nfs_open(filesys, names[i]);
    if (tmp >= 0 && tmp != fds[i]) {
      fprintf(stderr, "ERROR: file %s was opened twice\n", names[i]);
      error_count++;
    }
    filesize[i] = (rand() % (MAX_BYTES-MIN_BYTES)) + MIN_BYTES;
  }

  for (i = 0; i < 2; i++) {
    for (j = i + 1; j < 2; j++) {
      if (fds[i] == fds[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }

  printf("Two files created with zero length:\n");
  nfs_ls(filesys);
  printf("\n");

  for (i = 0; i < 2; i++) {
    for (j = 0; j < filesize[i]; j += chunksize) {
      if ((filesize[i] - j) < 10) {
        chunksize = filesize[i] - j;
      }
      else {
        chunksize = (rand() % (filesize[i] - j)) + 1;
      }

      if ((buffer = malloc(chunksize)) == NULL) {
        fprintf(stderr, "ABORT: Out of memory!\n");
        exit(-1);
      }
      for (k = 0; k < chunksize; k++) {
        buffer[k] = (char) (j+k);
      }
      tmp = nfs_write(fds[i], buffer, chunksize);
      if (tmp != chunksize) {
        fprintf(stderr, "ERROR: Tried to write %d bytes, but wrote %d\n", 
                chunksize, tmp);
        error_count++;
      }
      free(buffer);
    }
  }

  if (nfs_close(fds[1]) != 0) {
    fprintf(stderr, "ERROR: close of handle %d failed\n", fds[1]);
    error_count++;
  }

  /* Sneaky attempt to close already closed file handle. */
  if (nfs_close(fds[1]) == 0) {
    fprintf(stderr, "ERROR: close of stale handle %d succeeded\n", fds[1]);
    error_count++;
  }

  printf("File %s now has length %d and %s now has length %d:\n",
         names[0], filesize[0], names[1], filesize[1]);
  nfs_ls(filesys);

  /* Just to be cruel - attempt to read from a closed file handle. 
   */
  if (nfs_read(fds[1], fixedbuf, sizeof(fixedbuf)) > 0) {
    fprintf(stderr, "ERROR: read from a closed file handle?\n");
    error_count++;
  }

  fds[1] = nfs_open(filesys, names[1]);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < filesize[i]; j += chunksize) {
      if ((filesize[i] - j) < 10) {
        chunksize = filesize[i] - j;
      }
      else {
        chunksize = (rand() % (filesize[i] - j)) + 1;
      }
      if ((buffer = malloc(chunksize)) == NULL) {
        fprintf(stderr, "ABORT: Out of memory!\n");
        exit(-1);
      }
      readsize = nfs_read(fds[i], buffer, chunksize);
      if (readsize != chunksize) {
        fprintf(stderr, "ERROR: Requested %d bytes, read %d\n", chunksize, readsize);
        readsize = chunksize;
      }
      for (k = 0; k < readsize; k++) {
        if (buffer[k] != (char)(j+k)) {
          fprintf(stderr, "ERROR: data error at offset %d in file %s (%d,%d)\n",
                  j+k, names[i], buffer[k], (char)(j+k));
          error_count++;
          break;
        }
      }
      free(buffer);
    }
  }

  for (i = 0; i < 2; i++) {
    if (nfs_close(fds[i]) != 0) {
      fprintf(stderr, "ERROR: closing file %s\n", names[i]);
      error_count++;
    }
    if (nfs_remove(filesys, names[i]) != 0) {
      fprintf(stderr, "ERROR: deleting file %s\n", names[i]);
      error_count++;
    }
    printf("After deleting file %s:\n", names[i]);
    nfs_ls(filesys);
  }

  /* Now try to close and delete the closed and deleted files. Don't
   * care about the return codes, really, but just want to make sure
   * this doesn't cause a problem.
   */
  for (i = 0; i < 2; i++) {
    if (nfs_close(fds[i]) == 0) {
      fprintf(stderr, "Warning: closing already closed file %s\n", names[i]);
    }
    if (nfs_remove(filesys, names[i]) == 0) {
      fprintf(stderr, "Warning: deleting already deleted file %s\n", names[i]);
    }
    free(names[i]);
    names[i] = NULL;
  }
  return (error_count);
}

int
test_phase_two(char *filesys, char *test_str, char **names, int *p_nopen, 
               int *p_ncreate)
{
  int i, j, k;
  int chunksize;
  int readsize;
  char *buffer;
  char fixedbuf[1024];
  int fds[MAX_FD];
  int tmp;
  int nopen;                    /* Number of files simultaneously open */
  int ncreate;                  /* Number of files created in directory */
  int error_count = 0;

  /* Now just try to open up a bunch of files.
   */
  ncreate = 0;
  for (i = 0; i < MAX_FD; i++) {
    names[i] = rand_name();
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] < 0) {
      break;
    }
    nfs_close(fds[i]);
    ncreate++;
  }

  printf("Created %d files in the root directory\n", ncreate);

  nopen = 0;
  for (i = 0; i < ncreate; i++) {
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] < 0) {
      break;
    }
    nopen++;
  }
  printf("Simultaneously opened %d files\n", nopen);

  for (i = 0; i < nopen; i++) {
    tmp = nfs_write(fds[i], test_str, strlen(test_str));
    if (tmp != strlen(test_str)) {
      fprintf(stderr, "ERROR: Tried to write %d, returned %d\n", 
              strlen(test_str), tmp);
      error_count++;
    }
    if (nfs_close(fds[i]) != 0) {
      fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
      error_count++;
    }
  }

  /* Re-open in reverse order */
  for (i = nopen-1; i >= 0; i--) {
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] < 0) {
      fprintf(stderr, "ERROR: can't re-open file %s\n", names[i]);
    }
  }

  /* Now test the file contents.
   */
  for (j = 0; j < strlen(test_str); j++) {
    for (i = 0; i < nopen; i++) {
      char ch;

      if (nfs_read(fds[i], &ch, 1) != 1) {
        fprintf(stderr, "ERROR: Failed to read 1 character\n");
        error_count++;
      }
      if (ch != test_str[j]) {
        fprintf(stderr, "ERROR: Read wrong byte from %s at %d (%d,%d)\n", 
                names[i], j, ch, test_str[j]);
        error_count++;
        break;
      }
    }
  }

  /* Now close all of the open file handles.
   */
  for (i = 0; i < nopen; i++) {
    if (nfs_close(fds[i]) != 0) {
      fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
      error_count++;
    }
  }

  *p_nopen = nopen;
  *p_ncreate = ncreate;
  return (error_count);
}

int 
test_phase_three(char *filesys, char *test_str, char **names, int nopen, 
                 int ncreate)
{
  int i, j, k;
  int chunksize;
  int readsize;
  char *buffer;
  char fixedbuf[1024];
  int fds[MAX_FD];
  int tmp;
  int error_count = 0;

  for (i = 0; i < nopen; i++) {
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] >= 0) {
      readsize = nfs_read(fds[i], fixedbuf, sizeof(fixedbuf));
      if (readsize != strlen(test_str)) {
        fprintf(stderr, "ERROR: Read wrong number of bytes (%d, %d)\n",
                readsize, strlen(test_str));
        error_count++;
      }

      for (j = 0; j < strlen(test_str); j++) {
        if (test_str[j] != fixedbuf[j]) {
          fprintf(stderr, "ERROR: Wrong byte in %s at %d (%d,%d)\n", 
                  names[i], j, fixedbuf[j], test_str[j]);
          error_count++;
          break;
        }
      }

      if (nfs_close(fds[i]) != 0) {
        fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
        error_count++;
      }
    }
  }

  printf("Trying to fill up the disk with repeated writes to %s.\n", names[0]);
  printf("(This may take a while).\n");

  /* Now try opening the first file, and just write a huge bunch of junk.
   * This is just to try to fill up the disk, to see what happens.
   */
  fds[0] = nfs_open(filesys, names[0]);
  if (fds[0] >= 0) {
    for (i = 0; i < 100000; i++) {
      int x;

      if ((i % 100) == 0) {
        fprintf(stderr, "%d\r", i);
      }

      memset(fixedbuf, (char)i, sizeof(fixedbuf));
      x = nfs_write(fds[0], fixedbuf, sizeof(fixedbuf));
      if (x != sizeof(fixedbuf)) {
        /* Sooner or later, this write should fail. The only thing is that
         * it should fail gracefully, without any catastrophic errors.
         */
        printf("Write failed after %d iterations.\n", i);
        printf("If the emulated disk contains just over %d bytes, this is OK\n",
               (i * sizeof(fixedbuf)));
        break;
      }
    }
    nfs_close(fds[0]);
  }
  else {
    fprintf(stderr, "ERROR: re-opening file %s\n", names[0]);
  }

  /* Now, having filled up the disk, try one more time to read the
   * contents of the files we created.
   */
  for (i = 0; i < nopen; i++) {
    fds[i] = nfs_open(filesys, names[i]);
    if (fds[i] >= 0) {
      readsize = nfs_read(fds[i], fixedbuf, sizeof(fixedbuf));
      if (readsize < strlen(test_str)) {
        fprintf(stderr, "ERROR: Read wrong number of bytes (%d %d)\n", 
                readsize, strlen(test_str));
        error_count++;
      }

      for (j = 0; j < strlen(test_str); j++) {
        if (test_str[j] != fixedbuf[j]) {
          fprintf(stderr, "ERROR: Wrong byte in %s at position %d (%d,%d)\n", 
                  names[i], j, fixedbuf[j], test_str[j]);
          error_count++;
          break;
        }
      }

      if (nfs_close(fds[i]) != 0) {
        fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
        error_count++;
      }
    }
  }

  for (i = 0; i < ncreate; i++) {
    nfs_remove(filesys, names[i]);
    free(names[i]);
    names[i] = NULL;
  }
  return (error_count);
}

/* The main testing program
 */
int
main(int argc, char **argv)
{
  int error_count = 0;
  int port1 = 8010;             /* Default port #1 */
  int port2 = 8011;             /* Default port #2 */
  char host1[1024] = {"127.0.0.1"};
  char host2[1024] = {"127.0.0.1"};
  int mounted1 = 0;
  int mounted2 = 0;
  int ncreate1;
  int ncreate2;
  int nopen1;
  int nopen2;
  char *names1[MAX_FD];
  char *names2[MAX_FD];

  if (argc >= 3) {
    strcpy(host1, argv[1]);
    port1 = atoi(argv[2]);
    if (argc >= 5) {
      strcpy(host2, argv[3]);
      port2 = atoi(argv[4]);
    }
  }

  fprintf(stdout, "Connecting to %s:%d as %s and \n", 
          host1, port1, NAME1);
  fprintf(stdout, "  %s:%d as %s\n", 
          host2, port2, NAME2);

  if (nfs_mount(NAME1, host1, port1) != 0) {
    fprintf(stderr, "ERROR: Failed to mount remote file system\n");
    error_count++;
  }
  else {
    mounted1 = 1;
  }

  if (nfs_mount(NAME2, host2, port2) != 0) {
    fprintf(stderr, "ERROR: Failed to mount second remote file system\n");
    error_count++;
  }
  else {
    mounted2 = 1;
  }

  if (mounted1) {
    error_count += test_phase_one(NAME1);
  }

  if (mounted2) {
    error_count += test_phase_one(NAME2);
  }

  if (mounted1) {
    error_count += test_phase_two(NAME1,
                                  TEST1,
                                  names1,
                                  &nopen1,
                                  &ncreate1);

  }

  if (mounted2) {
    error_count += test_phase_two(NAME2,
                                  TEST2,
                                  names2,
                                  &nopen2,
                                  &ncreate2);
  }

  if (mounted1) {
    error_count += test_phase_three(NAME1,
                                    TEST1,
                                    names1,
                                    nopen1,
                                    ncreate1);

  }

  if (mounted2) {
    error_count += test_phase_three(NAME2,
                                    TEST2,
                                    names2,
                                    nopen2,
                                    ncreate2);
  }
  nfs_umount(NAME1);
  nfs_umount(NAME2);

  fprintf(stderr, "Test program exiting with %d errors\n", error_count);
  return (error_count);
}

int
main3(int argc, char **argv)
{
  int error_count = 0;
  int port1 = 8010;             /* Default port #1 */
  int port2 = 8011;             /* Default port #2 */
  char host1[1024] = {"127.0.0.1"};
  char host2[1024] = {"127.0.0.1"};
  int mounted1 = 0;
  int mounted2 = 0;
  int ncreate1;
  int ncreate2;
  int nopen1;
  int nopen2;
  char *names1[MAX_FD];
  char *names2[MAX_FD];

  if (argc >= 3) {
    strcpy(host1, argv[1]);
    port1 = atoi(argv[2]);
    if (argc >= 5) {
      strcpy(host2, argv[3]);
      port2 = atoi(argv[4]);
    }
  }

  fprintf(stdout, "Connecting to %s:%d as %s and \n", 
          host1, port1, NAME1);
  fprintf(stdout, "  %s:%d as %s\n", 
          host2, port2, NAME2);

  if (nfs_mount(NAME1, host1, port1) != 0) {
    fprintf(stderr, "ERROR: Failed to mount remote file system\n");
    error_count++;
  }
  else {
    mounted1 = 1;
  }

  if (nfs_mount(NAME2, host2, port2) != 0) {
    fprintf(stderr, "ERROR: Failed to mount second remote file system\n");
    error_count++;
  }
  else {
    mounted2 = 1;
  }

  int fds[MAX_FD];
  char *name[MAX_FD];
  name[0]=rand_name();
  name[1]=rand_name();
  fds[0] = nfs_open(NAME2, name[0]);
  fds[1] = nfs_open(NAME2, name[1]);
  if (fds[0] < 0) {
    fprintf(stderr, "ERROR: creating first test file %s\n", name[0]);
  }
  if (fds[1] < 0) {
    fprintf(stderr, "ERROR: creating first test file %s\n", name[1]);
  }

  printf("Two files created with zero length:\n");
  nfs_ls(NAME2);
  printf("\n");


  name[0]=rand_name();
  name[1]=rand_name();
  fds[0] = nfs_open(NAME1, name[0]);
  fds[1] = nfs_open(NAME1, name[1]);
  if (fds[0] < 0) {
    fprintf(stderr, "ERROR: creating first test file %s\n", name[0]);
  }
  if (fds[1] < 0) {
    fprintf(stderr, "ERROR: creating first test file %s\n", name[1]);
  }

  printf("Two files created with zero length:\n");
  nfs_ls(NAME1);
  printf("\n");
}
