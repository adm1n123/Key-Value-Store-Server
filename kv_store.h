

#define SUCCESS 200
#define ERROR 240

#define MAX_KEY_LEN 256
#define MAX_VAL_LEN 256

#define MAX_HASH_REC 65536 // 2^16
#define SIZE_OF_HASH_REC sizeof(unsigned long)
#define SIZE_OF_HASH_TABLE (MAX_HASH_REC * SIZE_OF_HASH_REC)

#define SIZE_OF_DB lseek(fd, 0, SEEK_END)
#define GOTO_END_OF_DB lseek(fd, 0, SEEK_END)
#define GOTO_BEG_OF_DB lseek(fd, 0, SEEK_SET)
#define GOTO_HASH_REC(x) lseek(fd, (x) * SIZE_OF_HASH_REC, SEEK_SET)
#define GOTO_X_BYTE(x) lseek(fd, (x), SEEK_SET)

typedef short status_t;

extern int fd = -1;

struct kv
{
	char key[MAX_KEY_LEN];
	char val[MAX_VAL_LEN];
};

struct hash_rec
{
	unsigned int hash_key;
	unsigned long rec_ptr;
};

// KV_API to access the persistent records
status_t kv_init(void);
status_t kv_write(struct kv *kv_pair);
status_t kv_read(struct kv *kv_pair);
status_t kv_del(struct kv *kv_pair);
unsigned int kv_find(struct kv *kv_pair);
unsigned long kv_find_empty_node(struct kv *kv_pair);

// functions used in KV_API
unsigned int key_to_hash(struct kv *kv_pair);
struct kv *create_kv_pair(char *key, char *val);


status_t kv_init(void)
{
	if(fd != -1){
		close(fd);
	}

	fd = open("persistent.db", O_CREAT | O_RDWR, 00700); //open db file 
	
	// check if file is newly created(if it is newly created then create the hash table)
	if(SIZE_OF_DB == 0){
		char none = 0;
		int i;
		int size_of_hash_table = SIZE_OF_HASH_TABLE;
		// create a hash table with all records as 0
		for(i = 0; i < size_of_hash_table; i++){
			write(fd, &none, 1);
		}
	} 

	GOTO_BEG_OF_DB;

	return SUCCESS;
}


status_t kv_write(struct kv *kv_pair)
{	
	// if key is already present in the file then replace the old value with new value 
	unsigned long kv_found = kv_find_empty_node(kv_pair);
	

	/*GOTO_X_BYTE(kv_found + MAX_KEY_LEN);
	int i;
	for(i = 0; i < MAX_VAL_LEN; i++){
		write(fd, &(kv_pair->val[i]), 1);
	}*/
	return SUCCESS;
}


status_t kv_read(struct kv *kv_pair)
{
	unsigned long kv_found = kv_find(kv_pair);
	if(!kv_found){
		return ERROR;
	}

	GOTO_X_BYTE(kv_found);
	read(fd, (char*)kv_pair, MAX_KEY_LEN + MAX_VAL_LEN);
	
	return SUCCESS;
}


status_t kv_del(struct kv *kv_pair)
{
	unsigned long kv_found = kv_find(kv_pair);
	if(!kv_found){
		return ERROR;
	}

	GOTO_X_BYTE(kv_found);
	char none = 0;
	for(int i = 0; i < MAX_KEY_LEN+MAX_VAL_LEN; i++){
		write(fd, &none, 1);
	} 

	return SUCCESS;
}


unsigned int kv_find(struct kv *kv_pair)
{
	unsigned long rec_ptr, next, prev;

	struct kv temp_kv;

	struct hash_rec hr = {key_to_hash(kv_pair), 0};
	GOTO_X_BYTE(hr.hash_key * SIZE_OF_HASH_REC);
	
	read(fd, (char*)&rec_ptr, SIZE_OF_HASH_REC);
	
	if(rec_ptr == 0){
		return 0; // kv_pair is not present in the persistent db 
	}

	prev = rec_ptr;
	next = rec_ptr;
	do{
		GOTO_X_BYTE(next);
		read(fd, (char*)&temp_kv, MAX_KEY_LEN + MAX_VAL_LEN);
		prev = next;
		read(fd, (char*)&next, sizeof(next));
	}while(strcmp(temp_kv.key, kv_pair->key) != 0 && next != 0);

	if(strcmp(temp_kv.key, kv_pair->key) != 0 && next == 0){
		return 0;
	}

	return prev;
}


unsigned long kv_find_empty_node(struct kv *kv_pair)
{
	struct hash_rec hr = {key_to_hash(kv_pair), 0};

	unsigned long rec_ptr, next = 0, prev = 0;

	struct kv temp_kv;
	
	GOTO_HASH_REC(hr.hash_key);
	
	read(fd, (char*)&rec_ptr, SIZE_OF_HASH_REC);
	
	if(rec_ptr == 0){
		rec_ptr = GOTO_END_OF_DB;
		write(fd, (char*)kv_pair, MAX_KEY_LEN + MAX_VAL_LEN);
		write(fd, (char*)&next, sizeof(next));
		GOTO_HASH_REC(hr.hash_key);
		write(fd, (char*)&rec_ptr, sizeof(rec_ptr));
		return rec_ptr;
	}

	prev = rec_ptr;
	next = rec_ptr;
	do{
		GOTO_X_BYTE(next);
		read(fd, (char*)&temp_kv, MAX_KEY_LEN + MAX_VAL_LEN);
		prev = next;
		read(fd, (char*)&next, sizeof(next));
	}while(strcmp(temp_kv.key, kv_pair->key) != 0 && strlen(temp_kv.key) != 0 && next != 0);

	if(strcmp(temp_kv.key, kv_pair->key) == 0){
		GOTO_X_BYTE(prev);
		write(fd, (char*)kv_pair, MAX_KEY_LEN + MAX_VAL_LEN);
		return prev;
	}

	if(strlen(temp_kv.key) == 0){
		GOTO_X_BYTE(prev);
		write(fd, (char*)kv_pair, MAX_KEY_LEN + MAX_VAL_LEN);
		return prev;
	}

	rec_ptr = GOTO_END_OF_DB;
	write(fd, (char*)kv_pair, MAX_KEY_LEN + MAX_VAL_LEN);
	write(fd, (char*)&next, sizeof(next));

	GOTO_X_BYTE(prev + MAX_KEY_LEN + MAX_VAL_LEN);
	write(fd, (char*)&rec_ptr, sizeof(rec_ptr));

	return rec_ptr;
}


unsigned int key_to_hash(struct kv *kv_pair)
{
	int i;
	unsigned int hash_key = 0;
	for(i = 0; i < MAX_KEY_LEN; i++){
		hash_key = (hash_key + kv_pair->key[i]) % MAX_HASH_REC;
	}
	return hash_key;
}


struct kv *create_kv_pair(char *key, char *val)
{
	if(key == NULL){
		return NULL;
	}
	int i;
	struct kv *pair = (struct kv *)malloc(sizeof(struct kv));
	for(i = 0; i < MAX_KEY_LEN && key[i] != 0; i++){
		pair->key[i] = key[i];
	}
	for(; i < MAX_KEY_LEN; i++){
		pair->key[i] = 0;
	}

	if(val == NULL){
		for(i = 0; i < MAX_VAL_LEN; i++){
			pair->val[i] = 0;
		}
	}
	else{
		for(i = 0; i < MAX_VAL_LEN && val[i] != 0; i++){
			pair->val[i] = val[i];
		}
		for(; i < MAX_VAL_LEN; i++){
			pair->val[i] = 0;
		}
	}

	return pair;
}