SEND_TEST_TARG = stest
RECEIVE_TEST_TARG = rtest

all:
	make $(SEND_TEST_TARG)
	make $(RECEIVE_TEST_TARG)

$(SEND_TEST_TARG): send.o generics.o sendtest.o
	gcc $^ -o $@
	
sendtest.o: sendtest.c send.h
	gcc -c -o $@ $<

send.o: send.c send.h
	gcc -c -o $@ $<

$(RECEIVE_TEST_TARG): receive.o generics.o receivetest.o
	gcc $^ -lpthread -o $@

receivetest.o: receivetest.c receive.h
	gcc -c -o $@ $<

receive.o: receive.c receive.h
	gcc -c -o $@ $<

generics.o: generics.c generics.h
	gcc -c -o $@ $<

clean:
	rm -f *.o $(SEND_TEST_TARG) $(RECEIVE_TEST_TARG)