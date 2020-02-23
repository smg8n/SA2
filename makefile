CC = gcc
CFLAGS = -g -w
TARGET1 = user
TARGET2 = oss
OBJS1 = user.o
OBJS2 = oss.o

all: user oss
$(TARGET1): $(OBJS1)
		$(CC) -o $(TARGET1) $(OBJS1)
$(TARGET2): $(OBJS2)
		$(CC) -o $(TARGET2) $(OBJS2)
user.o: user.c
		$(CC) $(CFLAGS) -c user.c 
oss.o: oss.c
		$(CC) $(CFLAGS) -c oss.c 
clean:
		/bin/rm -f *.o $(TARGET1) $(TARGET2)
