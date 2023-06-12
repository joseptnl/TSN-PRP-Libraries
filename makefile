SEND_TEST_TARG = stest
RECEIVE_TEST_TARG = rtest

all:
	make $(SEND_TEST_TARG)
	make $(RECEIVE_TEST_TARG)

$(SEND_TEST_TARG): packetio.o generics.o prp.o sendtest.o
	gcc $^ -lpthread -o $@
	
sendtest.o: sendtest.c packetio.h
	gcc -c -o $@ $<

packetio.o: packetio.c packetio.h
	gcc -c -o $@ $<

prp.o: prp.c prp.h
	gcc -c -o $@ $<

$(RECEIVE_TEST_TARG): prpreceive.o generics.o receivetest.o
	gcc $^ -lpthread -o $@

receivetest.o: receivetest.c prpreceive.h
	gcc -c -o $@ $<

prpreceive.o: prpreceive.c prpreceive.h
	gcc -c -o $@ $<

generics.o: generics.c generics.h
	gcc -c -o $@ $<

clean:
	rm -f *.o $(SEND_TEST_TARG) !$(RECEIVE_TEST_TARG)