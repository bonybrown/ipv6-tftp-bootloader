CC=gcc

NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova)
NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova)

CFLAGS=-Wall -g $(NOVAPROVA_CFLAGS)

MAIN_SOURCE=  k12_read.c

MAIN_OBJS=	  $(MAIN_SOURCE:.c=.o)

CODE_SOURCE=  target/net.c target/icmpv6.c target/udp.c target/tftp.c

CODE_OBJS=    $(CODE_SOURCE:.c=.o)

all:	$(CODE_OBJS) $(MAIN_OBJS)
	$(LINK.c) -o k12_read $(MAIN_OBJS) $(CODE_OBJS) 

test: testrunner
	./testrunner
        

TEST_SOURCE=  test/net.c test/icmpv6.c test/udp.c test/tftp.c
TEST_OBJS=  $(TEST_SOURCE:.c=.o)

testrunner:  $(TEST_OBJS) $(CODE_OBJS)
	$(LINK.c) -o $@ $(TEST_OBJS) $(CODE_OBJS) $(NOVAPROVA_LIBS)
        
clean:
	$(RM) k12_read testrunner $(CODE_OBJS) $(TEST_OBJS) $(MAIN_OBJS)
                

