#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define LEN 10

struct tuple {
  long id;
  int flag;
  struct node *head;
  struct node *tail;
  uint64_t append_count;
  uint64_t get_count;
};


struct node {
  struct node *next;
  void* data;
};


struct tuple *tuples;
static int len;
static int current_idx = 0;

static int hash(uint64_t id);
static int get_index(uint64_t id);
static void show(void);
static void * prod (void *);
static void * cons (void *);


int main() {
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
{
  int i = 0;
  int id;
  int *data;
  while (1) {
    id = rand() % LEN;
    data = (int *)malloc(sizeof(int));
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
    usleep(10);
  }
}


int create_hashtable(int length) {
  int i ;
  struct node *nd;

  len = length;
  tuples = (struct tuple*)malloc(sizeof(struct tuple) * length);
  for (i = 0; i < length; i++) {
    nd = (struct node*)malloc(sizeof(struct node));
    nd->next = NULL;
    nd->data = NULL;

    tuples[i].id = -1;
    tuples[i].flag = 0;
    tuples[i].head = nd;
    tuples[i].tail = nd;
    tuples[i].append_count = 0;
    tuples[i].get_count = 0;
  }
  return 0;
}

int append (uint64_t id, void * data)
{
  struct node *nd;
  int index;
  
  nd = (struct node*)malloc(sizeof(struct node));
  nd->data = data;
  nd->next = NULL;

  index = get_index(id);

  //  tuples[index];
  tuples[index].id = id;
  tuples[index].flag = 1;
  tuples[index].tail->next = nd;
  tuples[index].tail = nd;
  tuples[index].append_count++;
  return 0;
}

static void show(void) {
  int i;
  int *v;
  struct node *hn, *tn;
  int hd, td;
  struct node *nd;
  for (i = 0; i < len; i++) {
    hd=-1;
    td=-1;
    hn = tuples[i].head;
    if (hn->data != NULL) {
      hd = * (int *)hn->data;;
    }
    tn = tuples[i].tail;
    if (tn->data != NULL) {
      td = *(int *)tn->data;
    } 
    printf("%d:(id:%d, hd:%d, td:%d, ap:%lu, gc:%lu)",
	   i, tuples[i].id, hd, td, 
	   tuples[i].append_count, 
	   tuples[i].get_count);

    nd = tuples[i].head;    
    while (1) {
      nd = nd->next;
      if (nd == NULL) break;
      v = (int *)(nd->data);
      printf("->[%d]", *v);

    }
    printf("\n");
  }
}



int get_size (uint64_t id) {
  int index;
  index = get_index(id);
  return tuples[index].append_count - tuples[index].get_count;
}

static int get_index(uint64_t id) 
{
  int i;
  int index;
  int offset = 0;
  index = hash(id + offset);
  i = 0;
  while (i < len) {
    if (tuples[index].flag == 0) {
      return index;
    } else if  (tuples[index].id == id) {
      return index;
    }
    index = (index + 1) % len;
    i++;
  }
  printf("Hashtable: Enough size of hashtable needed");
  exit(0);
}

static int hash(uint64_t id) 
{
  unsigned int b    = 378551;
  unsigned int a    = 63689;
  unsigned int hash = 0;
  unsigned int i    = 0;
  unsigned int l    = 10;
  for(i = 0; i < l; i++)
    {
      hash = hash * a + id;
      a    = a * b;
    }

  return hash % len;
}

void* get (uint64_t id) 
{
  int index;
  struct node *nd;
  //  struct tuple *tp;
  void* data;

  index = get_index(id);
  if (tuples[index].head == tuples[index].tail) return NULL;
  nd = tuples[index].head->next;
  data = nd->data;
  free(tuples[index].head);
  tuples[index].head = nd;
  tuples[index].get_count++;

  return data;
}

void* get_current(void) 
{
  int index;
  struct node *nd;
  //  struct tuple *tp;
  void* data;

  while (tuples[current_idx].head == tuples[current_idx].tail) {
    current_idx = (current_idx + 1) % len; 
  }


  nd = tuples[current_idx].head->next;
  data = nd->data;
  free(tuples[current_idx].head);
  tuples[current_idx].head = nd;
  tuples[current_idx].get_count++;

  return data;
}

int free_hashtable() {
  int i;
  for (i = 0; i < len; i++) {
    free(tuples[i].head);
  }
  free(tuples);
  return 0;
}
