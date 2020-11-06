#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "kv_store.h"

#define VALID 1
#define INVALID 0
#define SYNCED 3
#define MODIFIED 4
#define LRU 5
#define LFU 6
#define READER 7
#define WRITER 8
#define MAX_KEY_SIZE 256
#define MAX_VAL_SIZE 256
#define SUCCESS 200
#define ERROR 240


struct cache_meta {
	int size;
	int policy; // LRU or LFU
	long long int timer;
	int reader_count;
	pthread_mutex_t lock;
	pthread_mutex_t write_lock;
} *cache_meta;

struct cache {
	int hash;
	struct kv *kv_pair;
	int state; // valid or invalid
	int status;	// modified or synced
	long long int criteria;
} **cache;

int kv_cache_init(int size, int policy) {
	if(size < 1) return -1;
	
	kv_init(); // db file init

	cache_meta = (struct cache_meta *)malloc(sizeof(struct cache_meta));
	cache_meta->size = size;
	cache_meta->policy = policy;
	cache_meta->timer = 0;
	pthread_mutex_init(&cache_meta->lock, NULL);
	pthread_mutex_init(&cache_meta->write_lock, NULL);

	cache = (struct cache **)malloc(size * sizeof(struct cache *));
	for(int i = 0; i < cache_meta->size; i++) {
		cache[i] = (struct cache *)malloc(sizeof(struct cache));
		cache[i]->state = INVALID;
		cache[i]->kv_pair = (struct kv *)malloc(sizeof(struct kv));
	}
	return 1;
}

int get_hash(char *key) {
	int hash = 0;
	for(int i = 0; i < MAX_KEY_SIZE && key[i] != '\0'; i++) {
		hash += key[i];
	}
	return hash;
}

int equals(char *a, char *b) {
	for(int i = 0; i < MAX_KEY_SIZE; i++) {
		if(a[i] != b[i]) return 0;
	}
	return 1;
}

void get_lock(int requster) {
	if(requster == READER) {
		pthread_mutex_lock(&cache_meta->lock);
		if(cache_meta->reader_count == 0) {
			pthread_mutex_lock(&cache_meta->write_lock);
			cache_meta->reader_count += 1;
		} else {
			cache_meta->reader_count += 1;
		}
		pthread_mutex_unlock(&cache_meta->lock);
	} else {
		pthread_mutex_lock(&cache_meta->write_lock);
	}
}

void release_lock(int requster) {
	if(requster == READER) {
		pthread_mutex_lock(&cache_meta->lock);
		if(cache_meta->reader_count == 1) {
			pthread_mutex_unlock(&cache_meta->write_lock);
			cache_meta->reader_count -= 1;
		} else {
			cache_meta->reader_count -= 1;
		}
		pthread_mutex_unlock(&cache_meta->lock);
	} else {
		pthread_mutex_unlock(&cache_meta->write_lock);
	}
}

int get_replacement_idx() {
	long long int min;
	int index = -1;
	if(cache_meta->policy == LRU) {
		min = cache_meta->timer + 1;
	} else {
		min = 1ll << 60; // max frequency
	}
	for(int i = 0; i < cache_meta->size; i++) {
		if(cache[i]->state == INVALID) return i;
		if(cache[i]->criteria < min) {
			min = cache[i]->criteria;
			index = i;
		}
	}
	return index;
}

void replmt_criteria_update(int index) {
	if(cache_meta->policy == LRU) {
		cache_meta->timer += 1;
		cache[index]->criteria = cache_meta->timer;
	} else {
		cache[index]->criteria += 1;
	}
}

int search_cache(char *key) {
	int hash = get_hash(key);
	for(int i = 0; i < cache_meta->size; i++) {
		if(cache[i]->state == VALID && hash == cache[i]->hash && equals(key, cache[i]->kv_pair->key)) {
			return i;
		}
	}
	return -1;
}

void set_key(char *key, struct kv *kv) {
	for(int i = 0; i < MAX_KEY_SIZE; i++) {
		kv->key[i] = key[i];
	}
}

void set_value(char *value, struct kv *kv) {
	for(int i = 0; i < MAX_VAL_SIZE; i++) {
		kv->val[i] = value[i];
	}
}

void set_kv_pair(struct kv *from, struct kv *to) {
	for(int i = 0; i < MAX_KEY_SIZE; i++) {
		to->key[i] = from->key[i];
		to->val[i] = from->val[i];
	}
}

void write_back_dirty(int index) {
	if(cache[index]->state == VALID && cache[index]->status == MODIFIED) {
		//printf("Writing back dirty: key: %s, value: %s\n", cache[index]->kv_pair->key, cache[index]->kv_pair->val);
		int status = kv_write(cache[index]->kv_pair);
		//printf("Status code: %d\n", status);
	}
}

int get(struct kv *req_pair) {

	int hash = get_hash(req_pair->key);
	int idx = search_cache(req_pair->key);
	if(idx >= 0) {
		get_lock(READER);
		if(cache[idx]->state == VALID && equals(req_pair->key, cache[idx]->kv_pair->key)) {
			set_value(cache[idx]->kv_pair->val, req_pair);
			replmt_criteria_update(idx);
			release_lock(READER);
			return SUCCESS;
		}
		release_lock(READER);
	}

	get_lock(WRITER); // cache need to be updated.
	idx = search_cache(req_pair->key);
	if(idx >= 0) { // when cuncurrent request could not find entry to read one of them could have updated.
		set_value(cache[idx]->kv_pair->val, req_pair);
		replmt_criteria_update(idx);
		release_lock(WRITER);
		return SUCCESS;
	}
	int status = kv_read(req_pair);
	if(status == SUCCESS) {
		idx = get_replacement_idx();
		write_back_dirty(idx);
		cache[idx]->hash = hash;
		set_kv_pair(req_pair, cache[idx]->kv_pair);
		cache[idx]->state = VALID;
		cache[idx]->status = SYNCED;
		cache[idx]->criteria = 0;// LRU does not need initialization but LFU needs
		replmt_criteria_update(idx);
	}
	release_lock(WRITER);
	return status;
}

int put(struct kv *req_pair) {
	
	get_lock(WRITER);
	int idx = search_cache(req_pair->key);
	if(idx >= 0) {
		set_value(req_pair->val, cache[idx]->kv_pair);
		cache[idx]->status = MODIFIED;
		replmt_criteria_update(idx);
		release_lock(WRITER);
		return SUCCESS;
	}
	idx = get_replacement_idx();
	write_back_dirty(idx);
	int hash = get_hash(req_pair->key);
	cache[idx]->hash = hash;
	set_kv_pair(req_pair, cache[idx]->kv_pair);
	cache[idx]->state = VALID;
	cache[idx]->status = MODIFIED;
	cache[idx]->criteria = 0;// LRU does not need initialization but LFU needs
	replmt_criteria_update(idx);
	release_lock(WRITER);
	return SUCCESS;
}

int del(struct kv *req_pair) {

	get_lock(WRITER);
	int idx = search_cache(req_pair->key);
	if(idx >= 0) { // it may not be in DB because of write back cache
		cache[idx]->state = INVALID;
		kv_del(req_pair);
		release_lock(WRITER);
		return SUCCESS;
	}
	int status = kv_del(req_pair);
	release_lock(WRITER);
	return status;
}

//////////////////////////////////////////////////////////////////////////////////////////   TESTING   ////////////////////////////////////////////////////////

/*void print_entry(int idx) {
	printf("Index: %d, hash: %d, Key: %s, Value: %s, state: %d, status: %d, criteria %lld\n", idx, cache[idx]->hash, cache[idx]->kv_pair->key, cache[idx]->kv_pair->val, cache[idx]->state, cache[idx]->status, cache[idx]->criteria);
}
void print_cache() {
	printf("____________________PRINTING CACHE______________________\n");
	printf("Size: %d, Policy: %d, Timer: %lld, Reader Count: %d\n", cache_meta->size, cache_meta->policy, cache_meta->timer, cache_meta->reader_count);
	for(int i = 0; i < cache_meta->size; i++) {
		print_entry(i);
	}
	printf("--------------------PRINTED CACHE-----------------------\n");
}



void fill_zeros(char *a) {
	int i = 0;
	for(; i < MAX_KEY_SIZE && a[i] != '\0'; i++);
	printf("Appending %d null char\n", MAX_KEY_SIZE-i-1);
	for(; i < MAX_KEY_SIZE; i++)
		a[i] = '\0';
}

void main() {
	int size, policy;
	printf("Enter Cache size and policy 5. LRU 6. LFU\n");
	scanf("%d %d", &size, &policy);
	kv_cache_init(size, policy);
	kv_init();
	int a=1;
	struct kv *req = (struct kv *)malloc(sizeof(struct kv));
	while(a > 0) {
		printf("1. GET, 2. PUT, 3. DEL 4. PRINT CACHE 0. EXIT # don't use spaces it is just for testing\n");
		scanf("%d", &a);
		if(a == 1) {
			printf("Enter key: ");
			scanf("%s", req->key);
			fill_zeros(req->key);
			int status = get(req);
			printf("Status code: %d\n", status);
			if(status == SUCCESS)
				printf("Value: %s\n", req->val);
		} else if(a == 2) {
			printf("Enter key: ");
			scanf("%s", req->key);
			fill_zeros(req->key);
			printf("Enter value: ");
			scanf("%s", req->val);
			fill_zeros(req->val);
			int status = put(req);
			printf("Status code: %d\n", status);
		} else if(a == 3) {
			printf("Enter key: ");
			scanf("%s", req->key);
			fill_zeros(req->key);
			int status = del(req);
			printf("Status code: %d\n", status);
		}
		print_cache();
	}
}*/