import os
import collections


class PRNG(object):

    def __init__(self):
        self.state = [ord(x) for x in os.urandom(128)]
        self.pos = 0
        self.sauce = 0x55

    def getrandom(self):
        r = self.state[self.pos]
        self.pos += 1
        if self.pos == len(self.state):
            self.cook_random()
            self.pos = 0
        return r

    def cook_random(self):
        self.sauce ^= self.state[self.sauce % len(self.state)]
        newstate = [a ^ self.sauce for a in self.state]
        self.state = newstate
        self.sauce = ((self.sauce << 1) | (self.sauce >> 7)) & 0xFF


prng = PRNG()
ctr = collections.Counter()

for _ in range(20):
    for x in prng.state:
        ctr[x] += 1
        prng.cook_random()
        print(ctr.most_common(20))
