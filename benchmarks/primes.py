
sieve = []
n = 2
while n < 50000:
    if all(n%i for i in sieve):
        sieve.append(n)
    n = n + 1

