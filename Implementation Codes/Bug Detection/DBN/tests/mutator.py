#!/usr/bin/env python

from os import path
import random
import token_call

encode_fname = './csv/token_encoding.csv'
tvec_fname = './csv/token_vector.csv'

tvecs = token_call.get_tokens(encode_fname, tvec_fname)
oldtoks = token_call.tokens
newtoks = []

def add (vec, orig_idx):
    global newtoks, oldtoks
    nv = len(vec)
    noldt = len(oldtoks)
    nnewt = len(newtoks)
    # choose from any of the new tokens or
    # generate a new token with probability (0.2*nv/nnet) assuming uniform random
    tokeni = random.randint(0, nv / 5)
    # where to add new token
    addi = random.randint(-1, nv)
    if tokeni < nnewt:
        tk = noldt + tokeni
    else:
        newtoks.append("mutation{}".format(nnewt))
        tk = noldt + nnewt + 1
    vec.insert(addi, tk)
    # update index of the original tokens
    for i, origi in enumerate(orig_idx):
        if origi >= addi:
            orig_idx[i] = orig_idx[i] + 1

def remove (vec, orig_idx):
    # where to remove token
    nv = len(orig_idx)
    subi = random.randint(0, nv-1)
    # search in the original tokens
    for i in range(len(orig_idx)):
        if i > subi:
            orig_idx[i] = orig_idx[i] - 1
    vec.pop(orig_idx[subi])
    orig_idx.pop(subi)

def edit (vec, orig_idx):
    global newtoks, oldtoks
    nv = len(orig_idx)
    noldt = len(oldtoks)
    nnewt = len(newtoks)
    editi = random.randint(0, nv-1)
    oldtk = vec[orig_idx[editi]]
    oldstr = oldtoks[oldtk]
    if oldstr == "if" or oldstr == "else" or oldstr == "while":
        return # cannot modify conditional tokens
    replacetoks = random.sample(range(noldt+nnewt-1), 4) # select 4 to avoid if, else and while
    for rtk in replacetoks:
        if rtk >= noldt:
            rtkstr = newtoks[rtk-noldt]
        else:
            rtkstr = oldtoks[rtk]
        if rtkstr == "if" or rtkstr == "else" or rtkstr == "while":
            continue
        # replace and exit
        vec[orig_idx[editi]] = rtk
        if rtk >= noldt:
            # if rtk is a new token,
            # then it is no longer one of the original, remove
            orig_idx.pop(editi)
        return

def mutate (vec):
    nv = len(vec)
    orig_idx = range(nv)
    action = [
        lambda x: add(x, orig_idx),
        lambda x: remove(x, orig_idx),
        lambda x: edit(x, orig_idx)
    ]
    # allow up to 10% of the vector size edits
    nedits = random.randint(0, nv / 10)
    for _ in range(nedits):
        action[random.randint(0, 2)](vec)

for proj in tvecs:
    veclist = tvecs[proj]
    nvecs = len(veclist)
    # we want 20% of it defective
    defects = random.sample(xrange(0,nvecs-1), nvecs/5)
    for i, vecinfo in enumerate(veclist):
        if i in defects:
            # make vecinfo defective
            vecinfo.buggy = True
            # add, remove, or edit tokens?
            mutate(vecinfo.vec)

encoding = path.basename(encode_fname).split('.')
vectoring = path.basename(tvec_fname).split('.')
toks = oldtoks + newtoks
with open(encoding[0]+'_mutate.csv', 'w') as mut_encd_f:
    for t in toks:
        mut_encd_f.write(t+'\n')
    mut_encd_f.close()

with open(vectoring[0]+'_mutate.csv', 'w') as mut_vec_f:
    for proj in tvecs:
        for vecinfo in tvecs[proj]:
            vstr = proj+'/'+vecinfo.mod+','
            if vecinfo.buggy:
                vstr = vstr + '1'
            else:
                vstr = vstr + '0'
            for v in vecinfo.vec:
                vstr = vstr + ',{}'.format(v)
            mut_vec_f.write(vstr+'\n')
    mut_vec_f.close()
