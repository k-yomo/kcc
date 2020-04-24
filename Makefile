CFLAGS=-std=c11 -g -static

kcc: kcc.c

.PHONY: setup
setup:
	docker build -t linux .

.PHONY: build
build:
	docker run --rm -v $(shell pwd):/kcc/ linux bash -c "cd /kcc/; make"

.PHONY: test
test:
	docker run --rm -v $(shell pwd):/kcc/ linux bash -c "cd /kcc/; ./test.sh"

.PHONY: clean
clean:
	rm -f kcc *.o *~ tmp*
