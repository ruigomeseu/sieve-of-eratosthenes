all:
	touch primesMemory
	gcc -o ./bin/primes primes.c -D_REENTRANT -lpthread -lm -Wall   

clean:
	rm -rf *o primes