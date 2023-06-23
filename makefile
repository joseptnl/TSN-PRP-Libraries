PRP_SEND_TEST_TARG = prpstest
TSN_SEND_TEST_TARG = tsnstest
RECEIVE_TEST_TARG = rtest

all:
	make $(PRP_SEND_TEST_TARG)
	make $(TSN_SEND_TEST_TARG)
	make $(RECEIVE_TEST_TARG)

clean:
	rm -f *.o $(PRP_SEND_TEST_TARG) $(TSN_SEND_TEST_TARG) $(RECEIVE_TEST_TARG)

generics.o: generics.c generics.h
	gcc -c -o $@ $<

packetio.o: packetio.c packetio.h
	gcc -c -o $@ $<

prp.o: prp.c prp.h
	gcc -c -o $@ $<

ethframes.o: ethframes.c ethframes.h
	gcc -c -o $@ $<

$(PRP_SEND_TEST_TARG): packetio.o generics.o prp.o ethframes.o prpsendtest.o
	gcc $^ -lpthread -o $@
	
prpsendtest.o: prpsendtest.c packetio.h ethframes.h
	gcc -c -o $@ $<

$(TSN_SEND_TEST_TARG): packetio.o generics.o tsn.o ethframes.o tsnsendtest.o
	gcc $^ -lpthread -o $@

tsnsendtest.o: tsnsendtest.c packetio.h ethframes.h
	gcc -c -o $@ $<

$(RECEIVE_TEST_TARG): packetio.o log.o generics.o receivetest.o
	gcc $^ -lpthread -o $@

receivetest.o: receivetest.c log.h
	gcc -c -o $@ $<

log.o: log.c log.h
	gcc -c -o $@ $<
