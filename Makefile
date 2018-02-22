$FLAGS = -Wall -Wextra

make: ipk-client.c ipk-server.c
	gcc $(FLAGS) ipk-client.c -o ipk-client
	gcc $(FLAGS) ipk-server.c -o ipk-server

clean:
	rm ipk-client ipk-server
