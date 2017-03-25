TARGET=libtest.a
ARM=arm-none-eabi

SOURCES=mem_test.c heap_test.c timer_test.c lib_test.c msgrbuf_test.c thread_test.c sd_test.c demonstrate_test.c
OBJECTS=mem_test.o heap_test.o timer_test.o lib_test.o msgrbuf_test.o thread_test.o sd_test.o demonstrate_test.o
INCLUDE=../include

AS=$(ARM)-gcc

AR=$(ARM)-ar 
AR_FLAGS=rcs

CC=$(ARM)-gcc

OBJ=$(ARM)-objcopy

.PHONY: sources clean

$(TARGET): sources
	$(AR) $(AR_FLAGS) $@ $(OBJECTS)

sources: $(SOURCES)
	$(CC) $(CC_FLAGS) -I$(INCLUDE) -c $(SOURCES) -lgcc

clean:
	rm -rf *.o
