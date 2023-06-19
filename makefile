PRP_SEND_TEST_TARG = prpstest
PRP_RECEIVE_TEST_TARG = prprtest
TSN_SEND_TEST_TARG = tsnstest
TSN_RECEIVE_TEST_TARG = tsnrtest

all:
	make $(PRP_SEND_TEST_TARG)
	make $(PRP_RECEIVE_TEST_TARG)
	make $(TSN_SEND_TEST_TARG)
	make $(TSN_RECEIVE_TEST_TARG)

clean:
	rm -f *.o $(PRP_SEND_TEST_TARG) $(PRP_RECEIVE_TEST_TARG)

generics.o: generics.c generics.h
	gcc -c -o $@ $<

packetio.o: packetio.c packetio.h
	gcc -c -o $@ $<

prp.o: prp.c prp.h
	gcc -c -o $@ $<

ethframes.o: ethframes.c ethframes.h
	gcc -c -o $@ $<

$(PRP_SEND_TEST_TARG): packetio.o generics.o prp.o ethframes.o sendtest.o
	gcc $^ -lpthread -o $@
	
sendtest.o: sendtest.c packetio.h ethframes.h
	gcc -c -o $@ $<

$(PRP_RECEIVE_TEST_TARG): packetio.o prpreceive.o generics.o receivetest.o
	gcc $^ -lpthread -o $@

receivetest.o: receivetest.c prpreceive.h
	gcc -c -o $@ $<

prpreceive.o: prpreceive.c prpreceive.h
	gcc -c -o $@ $<

$(TSN_SEND_TEST_TARG): packetio.o generics.o tsn.o ethframes.o tsnsendtest.o
	gcc $^ -lpthread -o $@

tsnsendtest.o: tsnsendtest.c packetio.h ethframes.h
	gcc -c -o $@ $<