/*##################################_Pier_Luigi_Manfrini_##############################
 *
 *	This program continuously reads from stdin filenames (whatever kind of file) 
 *	and send them, one at a time, to a thread that open the file, reads it
 *	and "sends" its content to another thread in charge of writing (copying)
 *	it into a new file whose name is provided by the user through CLI.
 *
 *	Usage:
 *		<./solution04> <output_filename>
 *
 *
 *	Compile:
 *		gcc solution04.c -o solution04 -Wall -Werror -pthread
 *i			-fsanitize=leak
 *
 *
 * ####################################################################################
 */
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define YELLOW "\033[1;33m" 
#define yellow "\033[0;33m"
#define RED "\033[1;31m"
#define RESET "\033[0m" 

bool run = true;
size_t filesize;
char *filename = NULL;     // secured by 'mut' mutex 
char *file_content = NULL; // secured by 'file_data' mutex (2 mutexex to speed up)
pthread_mutex_t mut;       // used to access the name of the file to be read
pthread_mutex_t file_data; // used to access the binary content of the read file

/* Consumer-Producer logic between main thread
 * and reader thread for the filename */
pthread_cond_t filename_written;
pthread_cond_t filename_read;

/* Consumer-Producer logic between reader thread and writer
 * thread for what concerns the file content (data) */
pthread_cond_t data_written;
pthread_cond_t data_read;

/* The READER reads (opens it and copy its content in memory) the file whose
 * filename is obtained from stdin by the main thread  */
void *reader(void *arg) {
  while (run) { // loop stops when filename=="END"
    pthread_mutex_lock(&mut);
    while (filename == NULL)
      pthread_cond_wait(&filename_written, &mut);
    if (!strcmp(filename, "END")) { // EXIT CONDITION for READER thread
      pthread_cond_signal(&data_written);
      pthread_mutex_unlock(&mut);
      continue;
    }
    int file;
    if((file=open(filename, O_RDONLY))==-1)
	    dprintf(1,RED"Selected file do not exist!\n"RESET);
    dprintf(2, YELLOW"Opened File:\t"RESET"%s\n", filename);
    free(filename);
    filename = NULL;
    pthread_cond_signal(&filename_read);
    pthread_mutex_unlock(&mut);

    pthread_mutex_lock(&file_data);
    while (file_content != NULL)
      pthread_cond_wait(&data_read, &file_data);
    filesize = lseek(file, 0, SEEK_END) - lseek(file, 0, SEEK_SET);
    dprintf(2, YELLOW"Filesize:\t"RESET"%zu\n", filesize);
    file_content = (char *)malloc(filesize);
    read(file, file_content, filesize);
    pthread_cond_signal(&data_written);
    pthread_mutex_unlock(&file_data);
    close(file);
  }
  pthread_exit(EXIT_SUCCESS);
}

/* The WRITER(Consumer) thread reads the file content written by the READER
 * thread(Producer in this case)
 * and write it as a new file in memory whose name is the one passed through CLI
 * as argv[1] */
void *writer(void *arg) {
  while (run) {
    pthread_mutex_lock(&file_data);
    while (file_content == NULL && run == true) {
      pthread_cond_wait(&data_written, &file_data);
    }
    if (!run)
      continue;
    int out_file = open(arg, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    write(out_file, file_content, filesize);
    dprintf(1, yellow"File '"YELLOW"%s"yellow "' (over)written!\n"RESET, (char*)arg);
    free(file_content);
    file_content = NULL;
    pthread_cond_signal(&data_read);
    pthread_mutex_unlock(&file_data);
    close(out_file);
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    dprintf(2, "Wrong Usage: <./solution04> <filename>\n");
    exit(EXIT_FAILURE);
  }
  /* Binary Semaphores (mutexes) initialization */
  pthread_mutex_init(&mut, NULL);
  pthread_mutex_init(&file_data, NULL);
  pthread_cond_init(&filename_read, NULL);
  pthread_cond_init(&filename_written, NULL);
  pthread_cond_init(&data_read, NULL);
  pthread_cond_init(&data_written, NULL);
  /* Threads initialization */
  pthread_t thread1; // reads filename, open the file and import its content
                     // into 'file_content' variable
  pthread_t thread2;
  pthread_attr_t attribute;
  pthread_attr_init(&attribute); // Initialize the attribute for the threads
  pthread_attr_setdetachstate(
      &attribute,
      PTHREAD_CREATE_JOINABLE); // Threads are explicitly being made joinable

  /* Thread Creation */
  dprintf(1,YELLOW"Initializing threads...\n"RESET);
 if( pthread_create(&thread1, &attribute, reader, NULL))
	 perror("reader thread error");
  dprintf(1,yellow"Reader ready!\n"RESET);
 if( pthread_create(&thread2, &attribute, writer, argv[1]))
	 perror("writer thread error");
  dprintf(1,yellow"Writer ready!\n"RESET);
  size_t size = 25;
  while (run) {
    int i = 0;
    pthread_mutex_lock(&mut);
    if (filename != NULL)
      pthread_cond_wait(&filename_read, &mut);
    filename = (char *)malloc(size * sizeof(char));
    do {
      if (i == 45) {
        filename = (char *)realloc(filename, size * 1.5 * sizeof(char));
        size = 1.5 * size;
      }
      read(0, &filename[i], 1);
    } while (filename[i++] != '\n');
    filename[i - 1] = '\0';
    pthread_cond_signal(&filename_written);
    pthread_mutex_unlock(&mut);
    if (!strcmp(filename, "END")) {
      run = false;
      break;
    }
  }
  pthread_join(thread1, NULL);
  dprintf(1,RED "Joined Reader\n");
  pthread_join(thread2, NULL);
  dprintf(1, "Joined Writer\n"RESET);
  return 0;
}
