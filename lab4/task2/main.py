import afl
import sys

def uridecode(s):
    ret = []
    i = 0
    while i < len(s):
        if s[i] == '%':
            a = s[i + 1]
            b = s[i + 2]
            char_code = (int(a, 16) * 16) + int(b, 16)
            ret.append(chr(char_code))
            i += 3
        elif s[i] == '+':
            ret.append(' ')
        else:
            ret.append(s[i])
            i += 1
    return ''.join(ret)

if __name__ == '__main__':
    afl.init()
    print(uridecode(sys.stdin.read()))
