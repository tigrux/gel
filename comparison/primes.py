sieve = []
for n in xrange(2, 50000):
    if all(n % i for i in sieve):
        sieve.append(n)

