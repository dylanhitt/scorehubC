CC=clang

LDLIBS=-lcurl -ljson-c -lcjson -lwiringPi

scorehub: scorehub.o
	$(CC) $< $(LDLIBS) -o $@

.PHONY: scorehub
