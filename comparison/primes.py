
sieve = [2]
for n in range(3, 1000000):
    for i in sieve:
        if n % i == 0:
            break
        if i*i > n:
            sieve.append(n)
            break

print "Calculated %u primes" % len(sieve)

