CC := gcc
RM := rm

CCFLAGS += -g -Wall -I src/core -I src/http
LDFLAGS += -D_GNU_SOURCE -D__USE_XOPEN -lpthread
TARGETS := bohttpd
OBJECTS := bohttpd.o config.o epoll.o http.o http_parse.o http_request.o \
		   http_timer.o list.o log.o rbtree.o rio.o threadpool.o utility.o

$(TARGETS) : $(OBJECTS) 
	$(CC) $(OBJECTS) -o $(TARGETS) $(LDFLAGS)
	$(RM) -f $(OBJECTS)

bohttpd.o : src/core/bohttpd.c src/core/bohttpd.h src/core/config.h \
	   		src/core/epoll.h src/core/log.h src/core/threadpool.h \
		   	src/core/utility.h src/http/http.h  src/http/http_request.h \
		   	src/http/http_timer.h
	$(CC) src/core/bohttpd.c $(CCFLAGS) -c

config.o : src/core/config.c src/core/config.h src/core/log.h
	$(CC) src/core/config.c $(CCFLAGS) -c

epoll.o : src/core/epoll.c src/core/epoll.h src/core/log.h
	$(CC) src/core/epoll.c $(CCFLAGS) -c

http.o : src/http/http.c src/core/config.h src/core/epoll.h src/core/log.h \
		 src/core/rio.h src/core/utility.h src/http/http.h \
		 src/http/http_request.h src/http/http_timer.h
	$(CC) src/http/http.c $(CCFLAGS) -c

http_parse.o : src/http/http_parse.c src/core/list.h src/http/http_parse.h \
	   		   src/http/http_request.h
	$(CC) src/http/http_parse.c $(CCFLAGS) -c

http_request.o : src/http/http_request.c src/core/config.h \
	   			 src/core/epoll.h src/core/list.h src/core/log.h \
				 src/http/http.h src/http/http_parse.h \
				 src/http/http_request.h src/http/http_timer.h
	$(CC) src/http/http_request.c $(CCFLAGS) $(LDFLAGS) -c

http_timer.o : src/http/http_timer.c src/core/log.h src/core/rbtree.h \
	   		   src/http/http_request.h src/http/http_timer.h
	$(CC) src/http/http_timer.c $(CCFLAGS) -c

list.o : src/core/list.c src/core/list.h
	$(CC) src/core/list.c $(CCFLAGS) -c

log.o : src/core/log.c src/core/log.h
	$(CC) src/core/log.c $(CCFLAGS) -c

rbtree.o : src/core/rbtree.c src/core/log.h src/core/rbtree.h
	$(CC) src/core/rbtree.c $(CCFLAGS) -c

rio.o : src/core/rio.c src/core/rio.h
	$(CC) src/core/rio.c $(CCFLAGS) -c

threadpool.o : src/core/threadpool.c src/core/log.h src/core/threadpool.h
	$(CC) src/core/threadpool.c $(CCFLAGS) $(LDFLAGS) -c

utility.o : src/core/utility.c src/core/log.h src/core/utility.h
	$(CC) src/core/utility.c $(CCFLAGS) -c

.PHONY: clean
clean:
	$(RM) -f $(OBJECTS)
	$(RM) -f $(TARGETS)


