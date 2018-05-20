import sys

# Map segments to pins
class SegmentMap(object):

    def __init__(self, sm):
        self.sm = sm

    def __getattr__(self, key):
        return 1 << getattr(self.sm, key)


class AdafruitSegmentMap(SegmentMap):
    A = 0
    B = 1
    C = 2
    D = 3
    E = 4
    F = 5
    G1 = 6
    G2 = 7
    H = 8
    J = 9
    K = 10
    L = 11
    M = 12
    N = 13


class BadgeSegmentMap(SegmentMap):
    A = 9
    B = 10
    C = 7
    D = 5
    E = 0
    F = 14
    G1 = 8
    G2 = 4
    H = 13
    J = 12
    K = 11
    L = 1
    M = 3
    N = 2


maps = {
        'Adafruit': AdafruitSegmentMap,
        'Badge': BadgeSegmentMap,
}


def get_segment_map():
    if len(sys.argv) > 1:
        return SegmentMap(maps[sys.argv[1]])
    # Default
    return SegmentMap(BadgeSegmentMap)


SEG = get_segment_map()


LETTERS = {
    '!': SEG.B | SEG.C,
    '"': SEG.F | SEG.J,
    '#': SEG.B | SEG.C | SEG.D | SEG.G1 | SEG.G2 | SEG.J | SEG.M,
    '$': SEG.A | SEG.C | SEG.D | SEG.F | SEG.G1 | SEG.G2 | SEG.J | SEG.M,
    '%': SEG.C | SEG.F | SEG.K | SEG.L,
    '&': SEG.A | SEG.C | SEG.D | SEG.E | SEG.G1 | SEG.H | SEG.J | SEG.N,
    "'": SEG.K,
    '(': SEG.K | SEG.N,
    ')': SEG.H | SEG.L,
    '*': SEG.G1 | SEG.G2 | SEG.H | SEG.J | SEG.K | SEG.L | SEG.M | SEG.N,
    '+': SEG.G1 | SEG.G2 | SEG.J | SEG.M,
    ',': SEG.L,
    '-': SEG.G1 | SEG.G2,
    '/': SEG.K | SEG.L,
    '0': SEG.A | SEG.B | SEG.C | SEG.D | SEG.E | SEG.F | SEG.K | SEG.L,
    '1': SEG.B | SEG.C,
    '2': SEG.A | SEG.B | SEG.D | SEG.E | SEG.G1 | SEG.G2,
    '3': SEG.A | SEG.B | SEG.C | SEG.D | SEG.G2,
    '4': SEG.B | SEG.C | SEG.F | SEG.G1 | SEG.G2,
    '5': SEG.A | SEG.D | SEG.F | SEG.G1 | SEG.N,
    '6': SEG.A | SEG.C | SEG.D | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    '7': SEG.A | SEG.B | SEG.C,
    '8': SEG.A | SEG.B | SEG.C | SEG.D | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    '9': SEG.A | SEG.B | SEG.C | SEG.D | SEG.F | SEG.G1 | SEG.G2,
    ':': SEG.J | SEG.M,
    ';': SEG.J | SEG.L,
    '<': SEG.K | SEG.N,
    '=': SEG.D | SEG.G1 | SEG.G2,
    '>': SEG.H | SEG.L,
    '?': SEG.A | SEG.B | SEG.G2 | SEG.M,
    '@': SEG.A | SEG.B | SEG.D | SEG.E | SEG.F | SEG.G2 | SEG.J,
    'A': SEG.A | SEG.B | SEG.C | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    'B': SEG.A | SEG.B | SEG.C | SEG.D | SEG.G2 | SEG.J | SEG.M,
    'C': SEG.A | SEG.D | SEG.E | SEG.F,
    'D': SEG.A | SEG.B | SEG.C | SEG.D | SEG.J | SEG.M,
    'E': SEG.A | SEG.D | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    'F': SEG.A | SEG.E | SEG.F | SEG.G1,
    'G': SEG.A | SEG.C | SEG.D | SEG.E | SEG.F | SEG.G2,
    'H': SEG.B | SEG.C | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    'I': SEG.J | SEG.M,
    'J': SEG.B | SEG.C | SEG.D | SEG.E,
    'K': SEG.E | SEG.F | SEG.G1 | SEG.K | SEG.N,
    'L': SEG.D | SEG.E | SEG.F,
    'M': SEG.B | SEG.C | SEG.E | SEG.F | SEG.H | SEG.K,
    'N': SEG.B | SEG.C | SEG.E | SEG.F | SEG.H | SEG.N,
    'O': SEG.A | SEG.B | SEG.C | SEG.D | SEG.E | SEG.F,
    'P': SEG.A | SEG.B | SEG.E | SEG.F | SEG.G1 | SEG.G2,
    'Q': SEG.A | SEG.B | SEG.C | SEG.D | SEG.E | SEG.F | SEG.N,
    'R': SEG.A | SEG.B | SEG.E | SEG.F | SEG.G1 | SEG.G2 | SEG.N,
    'S': SEG.A | SEG.C | SEG.D | SEG.F | SEG.G1 | SEG.G2,
    'T': SEG.A | SEG.J | SEG.M,
    'U': SEG.B | SEG.C | SEG.D | SEG.E | SEG.F,
    'V': SEG.E | SEG.F | SEG.K | SEG.L,
    'W': SEG.B | SEG.C | SEG.E | SEG.F | SEG.L | SEG.N,
    'X': SEG.H | SEG.K | SEG.L | SEG.N,
    'Y': SEG.H | SEG.K | SEG.M,
    'Z': SEG.A | SEG.D | SEG.K | SEG.L,
    '[': SEG.A | SEG.D | SEG.E | SEG.F,
    '\\': SEG.H | SEG.N,
    ']': SEG.A | SEG.B | SEG.C | SEG.D,
    '^': SEG.A | SEG.B | SEG.K | SEG.L,
    '_': SEG.D,
    '`': SEG.H,
    'a': SEG.D | SEG.E | SEG.G1 | SEG.M,
    'b': SEG.D | SEG.E | SEG.F | SEG.G1 | SEG.N,
    'c': SEG.D | SEG.E | SEG.G1 | SEG.G2,
    'd': SEG.B | SEG.C | SEG.D | SEG.G2 | SEG.L,
    'e': SEG.D | SEG.E | SEG.G1 | SEG.L,
    'f': SEG.A | SEG.E | SEG.F | SEG.G1,
    'g': SEG.B | SEG.C | SEG.D | SEG.G2 | SEG.K,
    'h': SEG.E | SEG.F | SEG.G1 | SEG.M,
    'i': SEG.M,
    'j': SEG.B | SEG.C | SEG.D,
    'k': SEG.J | SEG.K | SEG.M | SEG.N,
    'l': SEG.E | SEG.F,
    'm': SEG.C | SEG.E | SEG.G1 | SEG.G2 | SEG.M,
    'n': SEG.E | SEG.G1 | SEG.M,
    'o': SEG.C | SEG.D | SEG.E | SEG.G1 | SEG.G2,
    'p': SEG.E | SEG.F | SEG.G1 | SEG.H,
    'q': SEG.B | SEG.C | SEG.G2 | SEG.K,
    'r': SEG.E | SEG.G1,
    's': SEG.D | SEG.G2 | SEG.N,
    't': SEG.D | SEG.E | SEG.F | SEG.G1,
    'u': SEG.C | SEG.D | SEG.E,
    'v': SEG.C | SEG.N,
    'w': SEG.C | SEG.E | SEG.L | SEG.N,
    'x': SEG.G1 | SEG.G2 | SEG.L | SEG.N,
    'y': SEG.C | SEG.D | SEG.N,
    'z': SEG.D | SEG.G1 | SEG.L,
    '{': SEG.A | SEG.D | SEG.G1 | SEG.H | SEG.L,
    '|': SEG.J | SEG.M,
    '}': SEG.A | SEG.D | SEG.G2 | SEG.K | SEG.N,
    '~': SEG.F | SEG.H | SEG.K,
}


def make_font():
    print('#include <stdint.h>')
    print('')
    print('uint16_t fontmap[] = {')
    for i in range(128):
        ch = LETTERS.get(chr(i), 0)
        print('0x{:04x},'.format(ch))
    print('};')


if __name__ == '__main__':
    make_font()
