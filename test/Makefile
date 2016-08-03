CC=gcc

PROJECT= ipv6_bootloader

NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova)
NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova)

CFLAGS= --coverage -Wall -g $(NOVAPROVA_CFLAGS)


CODE_SOURCE=  ../target/net.c ../target/icmpv6.c ../target/udp.c ../target/tftp.c
CODE_OBJS=    $(CODE_SOURCE:.c=.o)
CODE_GCNO=    $(CODE_SOURCE:.c=.gcno)
CODE_GCDA=    $(CODE_SOURCE:.c=.gcda)

test: testrunner
	./testrunner
        

TEST_SOURCE=  net.c icmpv6.c udp.c tftp.c
TEST_OBJS=    $(TEST_SOURCE:.c=.o)
TEST_GCNO=    $(TEST_SOURCE:.c=.gcno)
TEST_GCDA=    $(TEST_SOURCE:.c=.gcda)

testrunner:  $(TEST_OBJS) $(CODE_OBJS)
	$(LINK.c) -o $@ $(TEST_OBJS) $(CODE_OBJS) $(NOVAPROVA_LIBS)
        
clean:
	$(RM) testrunner $(PROJECT).info $(CODE_OBJS) $(TEST_OBJS) $(CODE_GCDA) $(CODE_GCNO) $(TEST_GCDA) $(TEST_GCNO)
                


coverage:  testrunner
	$(RM) $(TEST_GCDA) $(MYCODE_GCDA)
	mkdir -p coverage
	./testrunner
	lcov --base-directory .. --directory .. --capture --output-file $(PROJECT).info
	lcov --output-file $(PROJECT).info --remove $(PROJECT).info '/test*'
	rm -rf coverage/*
	genhtml -o coverage --title "$(PROJECT) test coverage $(shell date)" --num-spaces 4 $(PROJECT).info

.PHONY: clean coverage
