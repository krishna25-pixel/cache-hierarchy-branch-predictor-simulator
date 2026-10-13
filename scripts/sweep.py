import csv, pathlib, subprocess, argparse
p=argparse.ArgumentParser();p.add_argument('--sim',default='./simulator');p.add_argument('--trace',default='traces/synthetic.trace');p.add_argument('--out',default='results/sweep.csv');args=p.parse_args()
pathlib.Path(args.out).parent.mkdir(parents=True,exist_ok=True)
rows=[]
for ways in [1,2,4,8]:
 for policy in ['lru','plru']:
  if policy=='plru' and ways==1:continue
  for predictor in ['bimodal','gshare','tournament']:
   temp=str(pathlib.Path(args.out).parent/'one_run.csv')
   cmd=[args.sim,'--trace',args.trace,'--ways',str(ways),'--policy',policy,'--predictor',predictor,'--out',temp]
   subprocess.run(cmd,check=True,stdout=subprocess.DEVNULL)
   metrics={r['metric']:r['value'] for r in csv.DictReader(open(temp))}
   rows.append(dict(ways=ways,policy=policy,predictor=predictor,**metrics))
with open(args.out,'w',newline='') as f:
 w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
print('Saved',args.out,'with',len(rows),'configurations')
