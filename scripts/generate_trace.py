#!/usr/bin/env python3
"""Deterministic synthetic trace. Not representative of SPEC workloads."""
import argparse, random
p=argparse.ArgumentParser();p.add_argument('--output',default='traces/synthetic.trace');p.add_argument('--accesses',type=int,default=50000);p.add_argument('--seed',type=int,default=42);args=p.parse_args()
rng=random.Random(args.seed)
with open(args.output,'w') as f:
    f.write('# synthetic sequential/hot/random accesses and correlated branches\n')
    f.write(f'I {args.accesses*4}\n')
    for i in range(args.accesses):
        mode=i%100
        addr=(i%2048)*64 if mode<65 else (rng.randrange(128)*64 if mode<90 else rng.randrange(65536)*64)
        f.write(f'M 0x{addr:x}\n')
        if i%4==0:
            pc=0x1000+((i//4)%16)*4
            taken=(i//4)%16 < 12 if rng.random()<0.9 else bool(rng.getrandbits(1))
            f.write(f'B 0x{pc:x} {"T" if taken else "N"}\n')
print(args.output)
