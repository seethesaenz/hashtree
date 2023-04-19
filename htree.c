#include <stdio.h>     
#include <stdlib.h>   
#include <stdint.h>  
#include <inttypes.h>  
#include <errno.h>     // for EINTR
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>


// Print out the usage of the program and exit.
void Usage(char*);
uint32_t jenkins_one_at_a_time_hash(const uint8_t* , uint64_t );
double GetTime();

// block size
#define BSIZE 4096


typedef struct{
  int height;
  int thread_num;
  int thread_pos;
  int num_threads;
  int32_t n;
  int m;
  int fd;
  int start_block;
  int end_block;
  off_t file_size;
} threads;


void* tree(void* arg){
  threads *thread = (threads*) arg;
  int height = thread->height;
  int thread_num = thread->thread_num;
  int thread_pos = thread->thread_pos;
  int32_t n = thread->n;
  int m = thread->m;
  int fd = thread->fd;
  int num_threads = thread->num_threads;
  off_t file_size = thread->file_size;
  thread->start_block = thread_num*m;
  thread->end_block = (thread_num + 1)*m - 1;
  uint32_t hash = 0;
  uint8_t *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if(file_data == MAP_FAILED){
    perror("mmap failed");
    exit(EXIT_FAILURE);
  }
  //top of tree
  if(height == 0){
    uint8_t *start_block = file_data + thread->start_block * BSIZE;
    size_t total_size = (thread-> end_block - thread->start_block+1)*BSIZE;
    hash = jenkins_one_at_a_time_hash(start_block, total_size);
    printf("tnum %d hash compute %" PRIu32 " \n", thread_num, hash);
    printf("tnum %d hash sent to parent %" PRIu32 " \n", thread_num, hash);
    uint32_t *hash_ptr = malloc(sizeof(uint32_t));
    *hash_ptr = hash;
    pthread_exit(hash_ptr);
  } else {
    //not top
    uint8_t *start_block = file_data + thread->start_block * BSIZE;
    size_t total_size = (thread-> end_block - thread->start_block+1)*BSIZE;
    hash = jenkins_one_at_a_time_hash(start_block, total_size);
    printf("tnum %d hash compute %" PRIu32 " \n", thread_num, hash);
    


    //not top
    pthread_t left, right;
    uint32_t left_h, right_h;
    void *left_retval;
    void *right_retval;

    int blocks_per_thread = (thread->end_block - thread->start_block + 1) / 2;
    //left check
    if(thread_num*2+1 < num_threads){
      threads *left_thread = malloc(sizeof(threads));
      *left_thread = (threads){height - 1, thread_num*2+1, thread_pos*2+1 , num_threads, n, m, fd, thread->start_block, thread->start_block + blocks_per_thread - 1, file_size};
      pthread_create(&left, NULL, tree, left_thread);
    }
    
    //right check
    if(thread_num *2 +2 < num_threads){
      threads *right_thread = malloc(sizeof(threads));
      *right_thread = (threads){height - 1, thread_num*2+2, thread_pos*2+2, num_threads, n, m, fd, thread->start_block + blocks_per_thread, thread->end_block, file_size};
      pthread_create(&right, NULL, tree, right_thread);
    }

    //left check
    if(thread_num*2+1 < num_threads){
      pthread_join(left, &left_retval);
      left_h = *(uint32_t*) left_retval;
      printf("tnum %d hash from left child %" PRIu32 " \n", thread_num, left_h);
    }

    //right check
    if(thread_num * 2 + 2 < num_threads){
      pthread_join(right, &right_retval);
      right_h = *(uint32_t*) right_retval;
      printf("tnum %d hash from right child %" PRIu32 " \n", thread_num, right_h);
    }
    if (thread_num*2+1 < num_threads || thread_num*2+2 < num_threads){
      char combined_hash[100];

      if(thread_num * 2 +2 < num_threads){
        sprintf(combined_hash, "%" PRIu32 "%" PRIu32 "%" PRIu32, hash, left_h, right_h);
      }else{
        sprintf(combined_hash, "%" PRIu32 "%" PRIu32, hash, left_h);
      }
      printf("tnum %d concat string %s \n", thread_num, combined_hash);
      
      hash = jenkins_one_at_a_time_hash((uint8_t*) combined_hash, strlen(combined_hash));
    }
    printf("tnum %d hash sent to parent %" PRIu32 " \n", thread_num, hash);

    uint32_t *hash_ptr = malloc(sizeof(uint32_t));
    *hash_ptr = hash;
    pthread_exit(hash_ptr);

  }
  munmap(file_data, file_size);
}

int 
main(int argc, char** argv) 
{
  int32_t fd;
  int32_t nblocks;
  int num_threads = atoi(argv[2]);
  int height = 0;
  while((1<<height) < num_threads){
    height++;
  }
  

  // input checking 
  if (argc != 3)
    Usage(argv[0]);

  // open input file
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  struct stat st;
  fstat(fd, &st);
  off_t file_size = st.st_size;
  // use fstat to get file size
  // calculate nblocks 
  nblocks = file_size / BSIZE;
  int m = nblocks/num_threads;
  printf("num threads = %d\n", num_threads);
  printf("blocks per thread = %d\n", nblocks / num_threads);
  

  double start = GetTime();

  threads thread = {height, 0, 0, num_threads , nblocks, m, fd, 0, m-1, file_size};
  pthread_t tid;
  pthread_create(&tid, NULL, tree, &thread);
  // calculate hash value of the input file
  uint32_t *hash_ptr;
  pthread_join(tid, (void**) &hash_ptr);
  uint32_t hash = *hash_ptr;


  double end = GetTime();
  printf("hash value = %u \n", hash);
  printf("time taken = %f \n", (end - start));
  close(fd);
  return EXIT_SUCCESS;
}

uint32_t 
jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length) 
{
  uint64_t i = 0;
  uint32_t hash = 0;

  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


void 
Usage(char* s) 
{
  fprintf(stderr, "Usage: %s filename num_threads \n", s);
  exit(EXIT_FAILURE);
}

double GetTime(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec+t.tv_usec /1e6;
}
