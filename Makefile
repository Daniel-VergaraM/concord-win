PREFIX          ?= /usr/local
DESTINCLUDE_DIR  = $(PREFIX)/include/concord
DESTLIBDIR       = $(PREFIX)/lib

CC            = gcc
# Auto-detect MSYS2 curl on Windows
ifneq ($(wildcard C:/msys64/mingw64/include/curl/curl.h),)
  CURL_INC = -IC:/msys64/mingw64/include
  CURL_LIB = -LC:/msys64/mingw64/lib
else ifneq ($(wildcard C:/Users/thefl/scoop/apps/msys2/current/mingw64/include/curl/curl.h),)
  CURL_INC = -IC:/Users/thefl/scoop/apps/msys2/current/mingw64/include
  CURL_LIB = -LC:/Users/thefl/scoop/apps/msys2/current/mingw64/lib
endif
SRC_DIR       = src
INCLUDE_DIR   = include
LIBDIR        = lib
DOCS_DIR      = docs
GENCODECS_DIR = gencodecs
CORE_DIR      = core
EXAMPLES_DIR  = examples
TEST_DIR      = test

SOFLAGS     = -fPIC
DYFLAGS     = -fPIC 
DEBUG_FLAGS = -O0 -g

GIT_BRANCHES = master dev
GIT_TARGETS  = latest latest-dev

CFLAGS ?= -O2

all: static

static:
	@ CC="$(CC)" CFLAGS="$(CFLAGS) $(CURL_INC)" $(MAKE) -C $(CORE_DIR)
	@ CC="$(CC)" $(MAKE) -C $(GENCODECS_DIR)
	@ CC="$(CC)" CFLAGS="$(CFLAGS)" CURL_INC="$(CURL_INC)" $(MAKE) -C $(SRC_DIR) $@
shared:
	@ CC="$(CC)" CFLAGS="$(SOFLAGS) $(CFLAGS) $(CURL_INC)" $(MAKE) -C $(CORE_DIR)
	@ CC="$(CC)" CFLAGS="$(SOFLAGS)" $(MAKE) -C $(GENCODECS_DIR)
	@ CC="$(CC)" CFLAGS="$(CFLAGS)" CURL_INC="$(CURL_INC)" $(MAKE) -C $(SRC_DIR) $@
shared_osx:
	@ CC="$(CC)" CFLAGS="$(DYFLAGS) $(CFLAGS) $(CURL_INC)" $(MAKE) -C $(CORE_DIR)
	@ CC="$(CC)" CFLAGS="$(DYFLAGS)" $(MAKE) -C $(GENCODECS_DIR)
	@ CC="$(CC)" CFLAGS="$(CFLAGS)" CURL_INC="$(CURL_INC)" $(MAKE) -C $(SRC_DIR) $@

install:
	@ mkdir -p $(DESTLIBDIR)
	@ mkdir -p $(DESTINCLUDE_DIR)
	install -d $(DESTLIBDIR)
	install -m 644 $(LIBDIR)/* $(DESTLIBDIR)
	install -d $(DESTINCLUDE_DIR)
	install -m 644 $(INCLUDE_DIR)/*.h $(CORE_DIR)/*.h $(GENCODECS_DIR)/*.h \
	               $(DESTINCLUDE_DIR)

uninstall:
	rm -rf $(PREFIX)/include/concord
	rm -rf $(PREFIX)/lib/libdiscord.a
	rm -rf $(PREFIX)/lib/libdiscord.so
	rm -rf $(PREFIX)/lib/libdiscord.dylib

docs:
	@ $(MAKE) -C $(GENCODECS_DIR) headers

echo:
	@ echo -e 'CC: $(CC)\n'
	@ echo -e 'PREFIX: $(PREFIX)\n'
	@ echo -e 'CFLAGS: $(CFLAGS)\n'
	@ echo -e 'CURL_INC: $(CURL_INC)\n'

voice:
	@ CC="$(CC)" CFLAGS="$(CFLAGS)" $(MAKE) -C $(SRC_DIR) $@
debug:
	@ CC="$(CC)" CFLAGS="$(DEBUG_FLAGS)" $(MAKE)

test: debug
	@ CC="$(CC)" $(MAKE) -C $(TEST_DIR)
examples: all
	@ CC="$(CC)" $(MAKE) -C $(EXAMPLES_DIR)

clean: 
	@ CC="$(CC)" $(MAKE) -C $(SRC_DIR) $@
	@ CC="$(CC)" $(MAKE) -C $(TEST_DIR) $@
	@ CC="$(CC)" $(MAKE) -C $(EXAMPLES_DIR) $@
	@ CC="$(CC)" $(MAKE) -C $(GENCODECS_DIR) $@

purge: clean
	@ CC="$(CC)" $(MAKE) -C $(SRC_DIR) $@

latest: master
latest-dev: dev

$(GIT_BRANCHES):
	$(MAKE) purge
	git switch $@
	git pull
	$(MAKE)

.PHONY: test examples uninstall install echo clean purge docs static shared shared_osx $(GIT_BRANCHES) $(GIT_TARGETS)
