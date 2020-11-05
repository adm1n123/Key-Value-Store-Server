all: server client 
server: test_server.c decode-encode.c read_config.c
		gcc test_server.c -o test_server read_config.c decode-encode.c -lpthread

client: client.c decode-encode.c
		gcc client.c -o client decode-encode.c

run_server: test_server
		./test_server
run_client: client
		./client