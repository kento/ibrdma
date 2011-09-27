#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static int hash(struct hashtable *ht, unsigned int id) ;
static int get_index(struct hashtable *ht, unsigned int id) ;


/*int main() {
  struct hashtable ht;
  int i;
  int len =10;
  int idx[] = {0, 1, 0, 1, 0, 1, 0};
  char *vs[] = {"apple","bread" ,"cute" ,"dead" ,"eye" ,"food" ,"gym" };
  create_hashtable(&ht, len);
  show(&ht);
  for (i = 0; i < 7; i++) {
    add(&ht, idx[i], vs[i]);
  }
  del(&ht, 0);
  show(&ht);
  for (i = 0; i < 7; i++) {
    char *v;
    v = get(&ht, idx[i]);
    printf("%s\n", v);
  }
  show(&ht);
}
*/

/*
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
*/


void create_ht(struct hashtable *ht,  int count) 
{
  int i;
  ht->t = (struct tuple *)malloc(sizeof(struct tuple *) * count);
  ht->len = count;
  for (i = 0; i < count; i++) {
    ht->t[i].id = -1;
    ht->t[i].v  = NULL;
  }
  return;
}

void add_ht(struct hashtable *ht, unsigned int id, void* value)
{
  int index;
  index = get_index(ht, id);
  ht->t[index].v = value;
  ht->t[index].id = id;
  return;
}

void del_ht(struct hashtable *ht, unsigned int id)
{
  int index;
  index = get_index(ht, id);
  ht->t[index].v = NULL;
  ht->t[index].id = -1;
}

void* get_ht(struct hashtable *ht, unsigned int id)
{
  int index;
  index = get_index(ht, id);
  return ht->t[index].v;
}

void show_ht(struct hashtable *ht) 
{
  int i ;
  printf("Hashtable (size=%d):\n", ht->len);
  for (i = 0; i < ht->len; i++) {
    printf("  ht[%d]=(%lu|%p)\n", i, ht->t[i].id, ht->t[i].v);
  }
}


static int get_index(struct hashtable *ht, unsigned int id) 
{
  int i;
  int index;
  int offset = 0;
  index = hash(ht, id + offset);
  i = 0;
  while (i < ht->len) {
    if (ht->t[index].v == NULL) {
      return index;
    } else if  (ht->t[index].id == id) {
      return index;
    }
    index = (index + 1) % ht->len;
    i++;
  }
  printf("Error:Hashtable: Enough size of hashtable needed\n");
  exit(0);
}

static int hash(struct hashtable *ht, unsigned int id) 
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

  return hash % ht->len;
}


