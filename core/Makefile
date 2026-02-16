CC     = gcc
PREFIX = /usr/local

OBJS = cog-utils.o      \
       io_poller.o      \
       user-agent.o     \
       websockets.o     \
       curl-websocket.o \
       jsmn-find.o      \
       json-build.o     \
       log.o            \
       logconf.o        \
       priority_queue.o \
       anomap.o         \
       sha1.o           \
       threadpool.o     \
       queriec.o

WFLAGS  = -Wall -Wextra -Wpedantic
# Auto-detect MSYS2 curl on Windows (only if not already set)
ifeq ($(CURL_INC),)
  ifneq ($(wildcard C:/msys64/mingw64/include/curl/curl.h),)
    CURL_INC = -IC:/msys64/mingw64/include
  else ifneq ($(wildcard C:/Users/thefl/scoop/apps/msys2/current/mingw64/include/curl/curl.h),)
    CURL_INC = -IC:/Users/thefl/scoop/apps/msys2/current/mingw64/include
  endif
endif
CFLAGS += -std=c99 -pthread -D_XOPEN_SOURCE=600 -DLOG_USE_COLOR \
          -I. -I$(PREFIX)/include $(CURL_INC) # $(WFLAGS)

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

echo:
	@ echo -e 'CC: $(CC)\n'
	@ echo -e 'CFLAGS: $(CFLAGS)\n'
	@ echo -e 'OBJS: $(OBJS)\n'

clean: 
	@ rm -f $(OBJS)

.PHONY: echo clean
