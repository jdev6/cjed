#FOR ALL .c file
OBJS = $(patsubst %.c,%.o,$(shell find -name "*.c"))
#OBJS = main.c

LIBS = -lncurses -lm
OUT = ./cjed
CFLAGS = -Wall -O3 -Wno-unused-but-set-variable
PREFIX = /usr/bin

# link
$(OUT): $(OBJS)
	@echo "[INFO] linking"
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(OUT)

install: $(OUT)
	install $(OUT) $(PREFIX)

debug: CFLAGS += -ggdb
debug: $(OUT)

#deprendencies
-include $(OBJS:.o=.d)

# generate dependendeces and  objetcs
%.o: %.c
	@echo "[INFO] generating deps for $*.o"
	gcc -MM $(CFLAGS) $*.c > $*.d
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@echo

#  clean
clean:
	$(RM) $(OBJS) $%.d

