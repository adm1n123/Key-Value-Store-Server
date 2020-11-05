typedef struct SERVER_CONF{
	int PORT;
	int NUM_THREADS;
	int CACHE_SIZE;
	int POLICY;
	int NUM_FILES;
}SERVER_CONF;

extern SERVER_CONF S1;

void read_conf();