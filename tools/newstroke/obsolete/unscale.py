with open('CKJ_wide.lib') as fi, open('CKJ_lib.lib', 'w') as fo:
    for line in fi:
        tok = line.split()
        if tok and tok[0] == 'P':
            tok[3:-2:2] = [str(int(float(t) / 1.5)) for t in tok[3:-2:2]]

        print(' '.join(tok), file=fo)
