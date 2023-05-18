CC=gcc
CFLAGS=-Wall -g

OBJS=edge_detector.o

default: edge_detector

edge_detector: $(OBJS)
	$(CC) $(CFLAGS) -o edge_detector $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o edge_detector
