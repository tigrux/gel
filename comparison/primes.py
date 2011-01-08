sieve = []
for n in range(2, 50000):
    if all(n % i for i in sieve):
        sieve.append(n)

print "Calculated %u primes" % len(sieve)

