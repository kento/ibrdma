#include "buffer_table.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define LEN (10)

static void * prod (void* arg) ;
static void * cons (void* arg) ;

int main() 
{
  int *c;
  int i,j;
  pthread_t pr, co;
  create_hashtable(LEN);
  pthread_create(&pr, NULL, prod, NULL);
  pthread_create(&co, NULL, cons, NULL);
  while (1) {
    show();
    printf("\n");
    sleep(1);                                                                                                                                                                                 
  }                                                                                                                                                                                            
}                                                                                                                                                                                               
                                                                                                                                                                                                
static void * prod (void* arg) 
{                                                                                                                                                                                                int i = 0;                                                                                                                                                                                   
  int id;                                                                                                                                                                                      
  int *data;                                                                                                                                                                                      while (1) {                                                                                                                                                                                     id = rand() % LEN;                                                                                                                                                                             data = (int *)malloc(sizeof(int));                                                                                                                                                       
    *data = id + 1000 * i;
    append(id, (void *)data);
    i++;
    usleep(1);
  }
}                                                                                                                                                                                               
                                                                                                                                                                                                
static void * cons (void* arg)                                                                                                                                                                 
{                                                                                                                                                                                              
  int i = 0;
  int id;
  int *data;
  while (1) {
    id = rand() % LEN;
    data = (int*)get_current(); 
    free(data); 
    usleep(8);
  }
} 
