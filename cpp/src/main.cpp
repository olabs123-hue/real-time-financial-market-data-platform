#include "book.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#ifdef WITH_KAFKA
#include <librdkafka/rdkafka.h>
#endif
int main(int argc,char**argv) {
 bool benchmark=false;uint64_t count=10000;std::string brokers="redpanda:9092",topic="market.updates";
 for(int i=1;i<argc;i++) {std::string a=argv[i];if(a=="--benchmark")benchmark=true;
 else if(a=="--count" && i+1<argc)count=std::stoull(argv[++i]);
 else if(a=="--brokers" && i+1<argc)brokers=argv[++i];
 else if(a=="--topic" && i+1<argc)topic=argv[++i];
 else {std::cerr<<"Unknown argument: "<<a<<"\n";return 2;}}
 if(count==0){std::cerr<<"count must be positive\n";return 2;}
 Book book;
 #ifdef WITH_KAFKA
 rd_kafka_t* producer=nullptr;rd_kafka_topic_t* kt=nullptr;
 if(!benchmark){char err[512];rd_kafka_conf_t*conf=rd_kafka_conf_new();
 rd_kafka_conf_set(conf,"bootstrap.servers",brokers.c_str(),err,sizeof(err));
 producer=rd_kafka_new(RD_KAFKA_PRODUCER,conf,err,sizeof(err));
 if(!producer){std::cerr<<err<<"\n";return 1;}kt=rd_kafka_topic_new(producer,topic.c_str(),nullptr);}
 #else
 if(!benchmark){std::cerr<<"Built without Kafka; run with --benchmark\n";return 2;}
 #endif
 auto start=std::chrono::steady_clock::now();
 for(uint64_t i=1;i<=count;i++) {
  Update u{"ACME",i,(i%2?'B':'A'),(i%2?9900:10000)+(int64_t)(i%50),1+(int64_t)(i%100)};
  book.apply(u);auto payload=json(u,book);
  #ifdef WITH_KAFKA
  if(!benchmark){while(rd_kafka_produce(kt,RD_KAFKA_PARTITION_UA,RD_KAFKA_MSG_F_COPY,(void*)payload.data(),payload.size(),u.symbol.data(),u.symbol.size(),nullptr)==-1){if(rd_kafka_last_error()!=RD_KAFKA_RESP_ERR__QUEUE_FULL){std::cerr<<rd_kafka_err2str(rd_kafka_last_error())<<"\n";return 1;}rd_kafka_poll(producer,100);}rd_kafka_poll(producer,0);}
  #endif
 }
 auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"messages="<<count<<" elapsed_seconds="<<elapsed<<" operations_per_second="<<(count/elapsed)<<" best_bid="<<book.best_bid()<<" best_ask="<<book.best_ask()<<"\n";
 #ifdef WITH_KAFKA
 if(!benchmark){auto remaining=rd_kafka_flush(producer,30000);rd_kafka_topic_destroy(kt);rd_kafka_destroy(producer);if(remaining){std::cerr<<"undelivered messages="<<remaining<<"\n";return 1;}}
 #endif
}
