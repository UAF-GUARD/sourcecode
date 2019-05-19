.PHONY: build test clean

build: libruntime.so

libruntime.so: runtime.o
	g++ $< -m32 -shared -o $@

runtime.o: runtime.cpp
	g++ $< -m32 -fPIC -c -o $@

test: example

example: example.cpp
	g++ $< -L. -lruntime
	LD_LIBRARY_PATH=. ./$@

clean:
	rm -f runtime.o libruntime.so example