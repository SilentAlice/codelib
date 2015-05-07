verb-probe: verb-probe.c
	gcc -g -Wall -O0 $^ -o $@ -libverbs
cm-conn: cm-conn.c
	gcc -g -Wall -O0 $^ -o $@ -libverbs -lpthread -lrdmacm
client: client.c
	gcc -g -Wall -O0 $^ -o $@ -libverbs -lpthread -lrdmacm
ibv-rc: ibv-rc.c
	gcc -g -Wall -O0 $^ -o $@ -libverbs -lpthread
