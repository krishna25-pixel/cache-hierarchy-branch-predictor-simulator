import csv,matplotlib.pyplot as plt
rows=list(csv.DictReader(open('results/sweep.csv')))
rows=[r for r in rows if r['policy']=='lru' and r['predictor']=='tournament']
ways=[int(r['ways']) for r in rows]
for field,label,file in [('l2_local_miss_rate','L2 local miss rate','results/cache_miss_rate.png'),('branch_accuracy','Branch accuracy','results/branch_accuracy.png')]:
 fig,ax=plt.subplots();ax.plot(ways,[float(r[field]) for r in rows],marker='o');ax.set_xticks(ways);ax.set_xlabel('Associativity (ways)');ax.set_ylabel(label);ax.set_title('Synthetic trace — illustrative, not SPEC');ax.grid(True,alpha=.2);fig.savefig(file,dpi=160,bbox_inches='tight');plt.close(fig);print(file)
