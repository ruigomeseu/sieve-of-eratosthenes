Sieve of Erastothenes
==================

An implementation of the [Erastothene's Sieve](http://en.wikipedia.org/wiki/Sieve_of_Eratosthenes) for the calculation of all the primes up to a certain number. This was an academic project for the [Operating Systems](https://sigarra.up.pt/feup/en/UCURR_GERAL.FICHA_UC_VIEW?pv_ocorrencia_id=333120) class at FEUP.

##Features

+ Threads for faster computation and simultaneous number sieving
+ Semaphores and Mutexes for synchronization between threads
+ Shared Memory for parallel storing of the final primes

##Usage
Compile primes.c using the Makefile and run it d to the following example:

> ./primes &lt;number&gt;

This will generate a list of all the primes up to the chosen &lt;number&gt;. You can go as high as you want. Expect a few seconds of runtime with numbers higher than 1M.

##Known Issues

Although the source file is compilable under Apple's OSX, the actual program won't run correctly on that Operating System. This is due to the fact that the usage of unnamed semaphores was a required feature and OSX doesn't implement those.

Please run this under a Linux VM if you happen to be using OSX. [Vagrant](http://www.vagrantup.com/) will allow you to setup a Ubuntu Server VM with little to no effort.