CFLAGS := -std=c11 -Wall -g -O2 -D_GNU_SOURCE -fsanitize=thread
LDFLAGS := -lpthread

ALL := test_pthread test_pthread_fix_pi

all: $(ALL)
.PHONY: all

test_%: main.c atomic.h  cond.h  futex.h  mutex.h  spinlock.h
	$(CC) $(CFLAGS) main.c -o $@ $(LDFLAGS)

test_pthread: CFLAGS += -DUSE_PTHREADS
test_pthread_fix_pi: CFLAGS += -DUSE_PTHREADS -DFIX_PI

# Test suite
NAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PRINTF = printf
else
    PRINTF = env printf
endif
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
notice = $(PRINTF) "$(PASS_COLOR)$(strip $1)$(NO_COLOR)\n"

check: $(ALL)
	@$(foreach t,$^,\
	    $(PRINTF) "Running $(t) ... \n"; \
	    sudo taskset -c 1 ./$(t) && $(call notice, [OK]); \
	)

clean:
	$(RM) $(ALL)
.PHONY: clean