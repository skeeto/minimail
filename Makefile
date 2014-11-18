CFLAGS  = -std=c99 -Wall -O2
LDFLAGS = -pthread
LDLIBS  = -lsqlite3

minimail : server.o main.o smtp.o pop3.o database.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

run : minimail
	./$^

clean :
	$(RM) *.o minimail

database.o: database.c database.h
main.o: main.c server.h database.h smtp.h pop3.h
pop3.o: pop3.c database.h pop3.h
server.o: server.c server.h
smtp.o: smtp.c database.h
