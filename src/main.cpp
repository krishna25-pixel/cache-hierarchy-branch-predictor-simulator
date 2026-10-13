#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using u64=uint64_t;
struct Config {size_t size=32768, line=64, ways=8; std::string policy="lru";};
struct Stats {u64 accesses=0,hits=0,misses=0;};
struct Line {bool valid=false;u64 tag=0;u64 last=0;};
class Cache {
 Config c; size_t sets; std::vector<std::vector<Line>> data; std::vector<std::vector<bool>> tree; u64 clock=0;
 size_t victim(size_t s){
  if(c.policy=="lru") { for(size_t i=0;i<c.ways;i++) if(!data[s][i].valid) return i; size_t v=0;for(size_t i=1;i<c.ways;i++)if(data[s][i].last<data[s][v].last)v=i;return v; }
  for(size_t i=0;i<c.ways;i++) if(!data[s][i].valid)return i;
  size_t node=0,way=0;for(size_t span=c.ways/2;span;span/=2){bool go=tree[s][node];way+=go?span:0;node=2*node+1+(go?1:0);}return way;
 }
 void touch(size_t s,size_t w){data[s][w].last=++clock; if(c.policy=="plru"){size_t node=0;for(size_t span=c.ways/2;span;span/=2){bool right=(w&span)!=0;tree[s][node]=!right;node=2*node+1+(right?1:0);}}}
 public:Stats stats;
 explicit Cache(Config x):c(x){if(!c.size||!c.line||!c.ways||c.ways>c.size/c.line||c.size%(c.line*c.ways))throw std::runtime_error("Invalid cache geometry");sets=c.size/(c.line*c.ways);if(c.policy!="lru"&&c.policy!="plru")throw std::runtime_error("Policy must be lru or plru");if(c.policy=="plru"&&(c.ways&(c.ways-1)))throw std::runtime_error("PLRU requires power-of-two ways");data.resize(sets,std::vector<Line>(c.ways));if(c.policy=="plru")tree.resize(sets,std::vector<bool>(c.ways-1,false));}
 bool probe(u64 addr){stats.accesses++;u64 block=addr/c.line;size_t set=block%sets;u64 tag=block/sets;for(size_t w=0;w<c.ways;w++)if(data[set][w].valid&&data[set][w].tag==tag){stats.hits++;touch(set,w);return true;}stats.misses++;return false;}
 void insert(u64 addr){u64 block=addr/c.line;size_t set=block%sets;u64 tag=block/sets;for(size_t w=0;w<c.ways;w++)if(data[set][w].valid&&data[set][w].tag==tag){touch(set,w);return;}size_t w=victim(set);data[set][w]={true,tag,0};touch(set,w);}
};
class Predictor {public:virtual ~Predictor()=default;virtual bool predict(u64 pc)=0;virtual void update(u64 pc,bool taken)=0;};
class Bimodal:public Predictor {std::vector<uint8_t> t;public:explicit Bimodal(size_t bits):t(1ULL<<bits,1){}bool predict(u64 pc)override{return t[(pc>>2)&(t.size()-1)]>=2;}void update(u64 pc,bool y)override{auto &x=t[(pc>>2)&(t.size()-1)];x=y?std::min<int>(3,x+1):std::max<int>(0,x-1);}};
class Gshare:public Predictor {std::vector<uint8_t> t;u64 hist=0,mask;public:explicit Gshare(size_t bits):t(1ULL<<bits,1),mask((1ULL<<bits)-1){}bool predict(u64 pc)override{return t[((pc>>2)^hist)&mask]>=2;}void update(u64 pc,bool y)override{auto &x=t[((pc>>2)^hist)&mask];x=y?std::min<int>(3,x+1):std::max<int>(0,x-1);hist=((hist<<1)|y)&mask;}};
class Tournament:public Predictor {Bimodal b;Gshare g;std::vector<uint8_t> choice;public:explicit Tournament(size_t bits):b(bits),g(bits),choice(1ULL<<bits,1){}bool predict(u64 pc)override{size_t i=(pc>>2)&(choice.size()-1);return choice[i]>=2?g.predict(pc):b.predict(pc);}void update(u64 pc,bool y)override{size_t i=(pc>>2)&(choice.size()-1);bool bp=b.predict(pc),gp=g.predict(pc);if(bp!=gp){auto &v=choice[i];v=gp==y?std::min<int>(3,v+1):std::max<int>(0,v-1);}b.update(pc,y);g.update(pc,y);}};
static u64 number(const std::string&s){size_t i=0;auto v=std::stoull(s,&i,0);if(i!=s.size())throw std::runtime_error("Bad integer "+s);return v;}
static void help(){std::cout<<"Usage: simulator --trace path [--l1-size bytes] [--l2-size bytes] [--l3-size bytes] [--line bytes] [--ways n] [--policy lru|plru] [--predictor bimodal|gshare|tournament] [--history bits] [--out report.csv]\nTrace lines: M <address>, B <pc> <T|N>, I <count>; addresses decimal or 0x hex; # comments. I records supply dynamic instruction counts for MPKI.\n";}
int main(int argc,char**argv){try{
 Config c;std::string file,out="results.csv",prediction="tournament";size_t history=10;size_t sizes[3]={32768,262144,8388608};
 for(int i=1;i<argc;i++){std::string k=argv[i];if(k=="--help"){help();return 0;}if(i+1>=argc)throw std::runtime_error("Missing argument for "+k);std::string v=argv[++i];if(k=="--trace")file=v;else if(k=="--out")out=v;else if(k=="--l1-size")sizes[0]=number(v);else if(k=="--l2-size")sizes[1]=number(v);else if(k=="--l3-size")sizes[2]=number(v);else if(k=="--line")c.line=number(v);else if(k=="--ways")c.ways=number(v);else if(k=="--policy")c.policy=v;else if(k=="--predictor")prediction=v;else if(k=="--history")history=number(v);else throw std::runtime_error("Unknown option: "+k);}
 if(file.empty())throw std::runtime_error("--trace required");
 if(history<1||history>20)throw std::runtime_error("history must be 1..20");
 std::vector<std::unique_ptr<Cache>> caches;for(auto sz:sizes){c.size=sz;caches.emplace_back(std::make_unique<Cache>(c));}
 std::unique_ptr<Predictor> pred;if(prediction=="bimodal")pred=std::make_unique<Bimodal>(history);else if(prediction=="gshare")pred=std::make_unique<Gshare>(history);else if(prediction=="tournament")pred=std::make_unique<Tournament>(history);else throw std::runtime_error("Unknown predictor");
 std::ifstream in(file);if(!in)throw std::runtime_error("Cannot open "+file);u64 mem=0,branches=0,correct=0,instructions=0;bool explicitInstructions=false;std::string line;size_t lineNum=0;
 while(std::getline(in,line)){lineNum++;auto comment=line.find('#');if(comment!=std::string::npos)line.resize(comment);std::istringstream ss(line);std::string type,a,b,extra;if(!(ss>>type))continue;
  if(type=="M"){if(!(ss>>a)||(ss>>extra))throw std::runtime_error("Invalid M record");u64 address=number(a);mem++;size_t level=0;for(;level<caches.size();level++)if(caches[level]->probe(address))break; // refill previously missed levels; no enforced inclusion or coherence
   for(size_t j=0;j<level;j++)caches[j]->insert(address);
  }else if(type=="B"){if(!(ss>>a>>b)||(ss>>extra)||(b!="T"&&b!="N"))throw std::runtime_error("Invalid B record");u64 pc=number(a);bool y=b=="T";branches++;correct+=pred->predict(pc)==y;pred->update(pc,y);
  }else if(type=="I"){if(!(ss>>a)||(ss>>extra))throw std::runtime_error("Invalid I record");explicitInstructions=true;instructions+=number(a);
  }else throw std::runtime_error("Invalid record type");
 }
 if(!explicitInstructions)instructions=mem+branches; // explicit proxy, NOT true instruction count
 if(in.bad())throw std::runtime_error("Error reading trace");
 std::ofstream os(out);if(!os)throw std::runtime_error("Cannot write "+out);os<<"metric,value\n";auto write=[&](std::string name,double val){os<<name<<","<<std::setprecision(12)<<val<<"\n";};write("memory_accesses",mem);write("branches",branches);write("instructions",instructions);write("instruction_count_estimated",!explicitInstructions);for(size_t j=0;j<3;j++){auto &s=caches[j]->stats;std::string p="l"+std::to_string(j+1)+"_";write(p+"accesses",s.accesses);write(p+"hits",s.hits);write(p+"misses",s.misses);write(p+"local_miss_rate",s.accesses?double(s.misses)/s.accesses:0);write(p+"mpki",instructions?1000.0*s.misses/instructions:0);}write("branch_correct",correct);write("branch_accuracy",branches?double(correct)/branches:0);write("branch_mpki",instructions?1000.0*(branches-correct)/instructions:0);
 std::cout<<"Trace "<<file<<"; memory="<<mem<<", branches="<<branches<<", instructions="<<instructions<<(explicitInstructions?" (trace)":" (proxy)")<<"\n";for(size_t j=0;j<3;j++){auto&s=caches[j]->stats;std::cout<<"L"<<j+1<<" accesses="<<s.accesses<<" misses="<<s.misses<<" local_miss_rate="<<std::fixed<<std::setprecision(4)<<(s.accesses?double(s.misses)/s.accesses:0)<<"\n";}std::cout<<"Branch accuracy="<<(branches?double(correct)/branches:0)<<"; CSV: "<<out<<"\n";
 }catch(const std::exception&e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}}
