struct hashtable {
  struct tuple *t;
  int len;
};

struct tuple {
  long id;
  void * v;
};


void create_ht(struct hashtable *ht,  int count) ;
void add_ht(struct hashtable *ht, unsigned int id, void* value);
void* get_ht(struct hashtable *ht, unsigned int id);
void del_ht(struct hashtable *ht, unsigned int id);
void show_ht(struct hashtable *ht);
