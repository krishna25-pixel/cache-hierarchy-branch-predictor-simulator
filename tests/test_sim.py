import csv, pathlib, subprocess, tempfile, unittest, random
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
 def run_trace(self, lines, *args):
  with tempfile.TemporaryDirectory() as d:
   t=pathlib.Path(d)/'t.trace';o=pathlib.Path(d)/'o.csv';t.write_text(lines)
   p=subprocess.run([str(ROOT/'simulator'),'--trace',str(t),'--out',str(o),*args],capture_output=True,text=True)
   self.assertEqual(p.returncode,0,p.stderr)
   with o.open() as f:
    return {r['metric']:float(r['value']) for r in csv.DictReader(f)}
 def run_failure(self, lines, *args):
  with tempfile.TemporaryDirectory() as d:
   t=pathlib.Path(d)/'t.trace';o=pathlib.Path(d)/'o.csv';t.write_text(lines)
   return subprocess.run([str(ROOT/'simulator'),'--trace',str(t),'--out',str(o),*args],capture_output=True,text=True)
 def test_empty_trace(self):
  m=self.run_trace('# empty\n')
  self.assertEqual(m['memory_accesses'],0)
  self.assertEqual(m['branch_accuracy'],0)
 def test_invalid_trace_fails(self):
  self.assertNotEqual(self.run_failure('M not_an_address\n').returncode,0)
  self.assertNotEqual(self.run_failure('B 0x100 X\n').returncode,0)
 def test_invalid_geometry_fails(self):
  self.assertNotEqual(self.run_failure('M 0\n','--l1-size','64','--ways','8').returncode,0)
  self.assertNotEqual(self.run_failure('M 0\n','--ways','3','--policy','plru').returncode,0)
 def test_exact_bimodal_behavior(self):
  m=self.run_trace('I 100\n'+'B 0x100 T\n'*4,'--predictor','bimodal','--history','2')
  self.assertEqual(m['branch_correct'],3)
  self.assertEqual(m['branch_accuracy'],.75)
  self.assertEqual(m['branch_mpki'],10)
 def test_cache_lru_against_independent_oracle(self):
  rng=random.Random(2030)
  addresses=[rng.randrange(0,12)*64 for _ in range(400)]
  # Single set, two ways, two cache lines; independent MRU list implementation.
  lines=[];misses=0
  for a in addresses:
   if a in lines:lines.remove(a)
   else:
    misses+=1
    if len(lines)==2:lines.pop()
   lines.insert(0,a)
  tr='I 1000\n'+''.join(f'M {a}\n' for a in addresses)
  m=self.run_trace(tr,'--l1-size','128','--l2-size','128','--l3-size','128','--ways','2')
  self.assertEqual(m['l1_misses'],misses)
  self.assertEqual(m['l1_hits'],400-misses)
  self.assertEqual(m['l1_mpki'],misses)
 def test_hierarchy_refills(self):
  # L1 direct-mapped two lines; L2 four lines. A,B,A => third L1 miss and L2 hit.
  tr='I 100\nM 0\nM 128\nM 0\n'
  m=self.run_trace(tr,'--l1-size','128','--l2-size','256','--l3-size','512','--ways','1')
  self.assertEqual(m['l1_misses'],3)
  self.assertEqual(m['l2_misses'],2)
  self.assertEqual(m['l2_hits'],1)
 def test_compulsory_and_hit(self):
  m=self.run_trace('I 100\nM 0x0\nM 0x4\nM 0x40\nM 0x0\n')
  self.assertEqual(m['l1_misses'],2);self.assertEqual(m['l1_hits'],2);self.assertEqual(m['l2_misses'],2);self.assertEqual(m['l3_misses'],2);self.assertEqual(m['l1_mpki'],20)
 def test_lru_eviction(self):
  m=self.run_trace('M 0x0\nM 0x80\nM 0x0\nM 0x100\nM 0x80\n','--l1-size','128','--l2-size','128','--l3-size','128','--ways','2')
  self.assertEqual(m['l1_misses'],4)
 def test_predictor(self):
  m=self.run_trace('I 100\n'+'B 0x100 T\n'*12,'--predictor','bimodal')
  self.assertEqual(m['branches'],12);self.assertGreater(m['branch_accuracy'],.8)
 def test_plru(self):
  m=self.run_trace('M 0x0\nM 0x40\nM 0x80\nM 0xc0\nM 0x0\n','--ways','4','--policy','plru')
  self.assertEqual(m['l1_misses'],4)
if __name__=='__main__':unittest.main()
