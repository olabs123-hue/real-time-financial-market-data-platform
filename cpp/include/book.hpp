#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
struct Update {std::string symbol; uint64_t sequence; char side; int64_t price; int64_t quantity;};
inline Update parse_csv(const std::string& line) {
 std::istringstream in(line); std::string f[5];
 for(int i=0;i<5;i++) if(!std::getline(in,f[i],',')) throw std::invalid_argument("missing CSV field");
 if(std::getline(in,f[0],',')) throw std::invalid_argument("extra CSV field");
 Update u; u.symbol=f[0];
 if(u.symbol.empty() || u.symbol.size()>20) throw std::invalid_argument("bad symbol");
 auto num=[](const std::string& x)->uint64_t {if(x.empty() || x[0]=='-' || x[0]=='+') throw std::invalid_argument("bad number");size_t n=0; auto v=std::stoull(x,&n);if(n!=x.size())throw std::invalid_argument("bad number");return v;};
 u.sequence=num(f[1]); if(f[2].size()!=1 || (f[2][0]!='B' && f[2][0]!='A'))throw std::invalid_argument("bad side");u.side=f[2][0];
 auto price=num(f[3]),qty=num(f[4]);if(!price || price>INT64_MAX || qty>INT64_MAX)throw std::invalid_argument("bad price/qty");
 u.price=(int64_t)price;u.quantity=(int64_t)qty;return u;
}
class Book {
 std::map<int64_t,int64_t,std::greater<int64_t>> bids_;
 std::map<int64_t,int64_t> asks_;
 uint64_t last_=0;
 public:
 void apply(const Update& u) {
  if(u.sequence!=last_+1)throw std::runtime_error("sequence gap/out-of-order update");
  if(u.quantity>0 && u.side=='B' && !asks_.empty() && u.price>=asks_.begin()->first)throw std::runtime_error("crossed bid update");
  if(u.quantity>0 && u.side=='A' && !bids_.empty() && u.price<=bids_.begin()->first)throw std::runtime_error("crossed ask update");
  if(u.side=='B') {if(u.quantity==0)bids_.erase(u.price);else bids_[u.price]=u.quantity;}
  else {if(u.quantity==0)asks_.erase(u.price);else asks_[u.price]=u.quantity;}
  last_=u.sequence;
 }
 uint64_t sequence()const{return last_;}
 int64_t best_bid()const{return bids_.empty()?0:bids_.begin()->first;}
 int64_t best_ask()const{return asks_.empty()?0:asks_.begin()->first;}
};
inline std::string json(const Update& u,const Book& b) {
 std::ostringstream s;
 s<<"{\"symbol\":\""<<u.symbol<<"\",\"sequence\":"<<u.sequence<<",\"side\":\""<<u.side<<"\",\"price_ticks\":"<<u.price<<",\"quantity\":"<<u.quantity<<",\"best_bid_ticks\":"<<b.best_bid()<<",\"best_ask_ticks\":"<<b.best_ask()<<"}";
 return s.str();
}
