UNAME_S := $(shell uname -s)
HEADERS := $(wildcard *.h)
VPATH := src:include:tests

ifeq ($(UNAME_S),Darwin)
GCOV := xcrun llvm-cov gcov
else
GCOV := llvm-cov gcov
endif

CC   := clang

CFLAGS += $(if $(COVERAGE), -fprofile-arcs -ftest-coverage )
CFLAGS += $(if $(DEBUG), -DDEBUG=1 )
CFLAGS += -Werror \
	  -Iinclude \
	  -I/opt/homebrew/include/ \
	  -I/opt/X11/include/ \
	  -I/opt/homebrew/opt/curl/include/ \
	  -g

LDLIBS += $(if $(or $(COVERAGE),$(DEBUG)), -g )
LDLIBS += $(if $(COVERAGE), --coverage )
LDLIBS += -L /opt/homebrew/lib/ -L/opt/X11/lib/ -lcurl -ljansson -lXm -lXt -lX11 -g

test_xmdict: LDLIBS += -lcmocka
test_xmdict: xmdict.o test_xmdict.o

.PHONY: test
test: test_xmdict
	./test_xmdict 

xmdict.o: xmdict.h

valgrind_%: %
	valgrind --leak-check=full --error-exitcode=1 ./$* 

coverage: COVERAGE=1
coverage: test
	$(GCOV) $(SRCS)

TAGS: $(SRCS) xmdict.h tests/test_*.[ch]
	ctags $^ /opt/homebrew/include/Xm/*  /opt/homebrew/opt/curl/include/*  /opt/X11/include/X11/*

TAGS_SYSTEM:
	ctags /opt/homebrew/include/Xm/

docs: $(HEADERS)
	doxygen

.PHONY: clean
clean:
	rm -rf *.o *.gcda *.gcno test_xmdict *.dSYM html/ latex/
