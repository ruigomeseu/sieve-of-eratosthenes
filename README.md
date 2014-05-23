Sieve of Erastothenes
==================

An implementation of the [Erastothene's Sieve](http://en.wikipedia.org/wiki/Sieve_of_Eratosthenes) for the calculation of all the primes up to a certain number. This was an academic project for the [Operating Systems](https://sigarra.up.pt/feup/en/UCURR_GERAL.FICHA_UC_VIEW?pv_ocorrencia_id=333120) class at FEUP.

##Features

+ Threads for faster computation and multiple numbers exclusion
+ Semaphores and Mutexes for synchronization between the threads
+ Usage of Shared Memory for storing the primes from all threads

##Usage
Compile primes.c using the Makefile and run it according to the following example:

> sudo ./primes 100

This will generate a list of all the primes up to 100. You can go as high as you want. Expect a few seconds of runtime with numbers higher than 1M.