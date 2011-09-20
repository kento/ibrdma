
#define uint64_t unsigned long


int create_hashtable(int length);
int append (uint64_t id, void* data);
void* get (uint64_t id);
void* get_current (void);
int free_hashtable(void);
int get_size (uint64_t id);

