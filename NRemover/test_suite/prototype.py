#prototype fuction tested in Python

def NoN(s):
    max_end = 0
    max_start = 0;
    current_end = 0;
    current_start = 0;

    for c in range(0, len(s)):
        if (s[c] == 'N'):
            if c - current_start + 1 > max_end - max_start:
                max_end = c;
                max_start = current_start;
                current_start = c + 1;
        
    if c - current_start > max_end - max_start:
        max_end = c + 1
        max_start = current_start
    print s[max_start:max_end]
def main():
    s = "NGNTAGAAAAAAAAAAAAAAAAAAAAAAAGTAAAAACAAAGN"
    NoN(s)
    s = "GAGAAAAAAAAAAAAAAAAAAAAAAAGTAAAAACAAAGNNNNNNNNNNNNNNN"
    NoN(s)
    s = "GAGAAAAAAAAAAAAAAAAAAAAAAAGTAAAAACAAAG"
    NoN(s)
    s = "NNNNNGAGAAAAAAAAAAAAAAAAAAAAAAAGTAAAAACAAAG"
    NoN(s)
    s = "NNNNNNNNNNNNNNNNNNNNNNNNNNANNNNNNNNNNNNNNNN"
    NoN(s)


main()
