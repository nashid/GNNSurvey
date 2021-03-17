#!/usr/bin/env python

''' Useful module for reading obtaining tokens '''

from os import path
from easydict import EasyDict as edict

tokens = []

def get_tokens (encode_fname, tvec_fname):
    global tokens
    tokens = []
    if path.isfile(encode_fname):
        with open(encode_fname, 'r') as encd_f:
            for label in encd_f:
                tokens.append(label.strip())
            encd_f.close()

    tvecs = edict()
    if path.isfile(tvec_fname):
        with open(tvec_fname, 'r') as tvec_f:
            for line in tvec_f:
                tvec = edict()
                larr =line.split(',')
                modname = larr[0]
                projmod = path.split(path.dirname(modname))[-1]
                if projmod not in tvecs:
                    tvecs[projmod] = []
                tvec.mod = path.basename(modname)
                tvec.buggy = 1 == int(larr[1])
                tvec.vec = map(lambda i: int(i), larr[2:])
                tvecs[projmod].append(tvec)
            tvec_f.close()
    else:
        print "no existing file", tvec_fname
    return tvecs

def encode_tokens (project = None):
    tokeninfo = get_tokens("token_encoding_mutate.csv", "token_vector_mutate.csv")
    # normalize all tokens
    max = len(tokens)
    buggy = []
    clean = []
    maxvec = 0

    if project:
        if project not in tokeninfo:
            project = "jansson"
        for info in tokeninfo[project]:
            norm = map(lambda x: (1.0 + float(x)) / max, info.vec)
            if maxvec < len(norm):
                maxvec = len(norm)
            if info.buggy:
                buggy.append(norm)
            else:
                clean.append(norm)
    else:
        for proj in tokeninfo:
            for info in tokeninfo[proj]:
                norm = map(lambda x: (1.0 + float(x)) / max, info.vec)
                if maxvec < len(norm):
                    maxvec = len(norm)
                if info.buggy:
                    buggy.append(norm)
                else:
                    clean.append(norm)
    # pad with zero to enforce size
    blen = len(buggy)
    clen = len(clean)
    for i in range(blen):
        buggy[i] = buggy[i] + [0] * (maxvec - len(buggy[i]))

    for i in range(clen):
        clean[i] = clean[i] + [0] * (maxvec - len(clean[i]))

    return (buggy, clean)
