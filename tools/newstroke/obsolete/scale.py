with open('CKJ_lib.lib') as fi, open('CKJ_wide.lib', 'w') as fo:
    for line in fi:
        tok = line.split()
        if tok:
            if tok[0] == 'P':
                tok[3:-2:2] = [str(int(t) * 1.5) for t in tok[3:-2:2]]
            elif tok[0] == 'X' and int(tok[3]) > 0:
                tok[3] = '1450'

        print(' '.join(tok), file=fo)
