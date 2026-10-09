#include "book.hpp"
#include <cassert>
#include <iostream>
int main(){
 Book b;b.apply({"ACME",1,'B',100,4});b.apply({"ACME",2,'A',102,7});
 assert(b.best_bid()==100 && b.best_ask()==102 && b.sequence()==2);
 b.apply({"ACME",3,'B',100,0});assert(b.best_bid()==0);
 bool gap=false;try{b.apply({"ACME",5,'B',101,1});}catch(const std::runtime_error&){gap=true;}assert(gap);
 bool crossed=false;try{b.apply({"ACME",4,'A',99,2});}catch(const std::runtime_error&){crossed=true;}assert(crossed);assert(b.sequence()==3);
 auto u=parse_csv("ACME,4,B,103,2");assert(u.sequence==4 && u.price==103);
 bool bad=false;try{parse_csv("ACME,4,B,-1,2");}catch(const std::exception&){bad=true;}assert(bad);
 std::cout<<"book tests passed\n";
}
