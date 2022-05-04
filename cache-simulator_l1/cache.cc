#include "cache.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>
#include <math.h>

using namespace std;

cache::cache(unsigned size,
      unsigned associativity,
      unsigned line_size,
      write_policy_t wr_hit_policy,
      write_policy_t wr_miss_policy,
      unsigned hit_time,
      unsigned miss_penalty,
      unsigned address_width
){
	/* edit here */
memory_accesses = 0;
no_rd = 0;

no_mem_wr = 0;
no_evicts = 0;
cache_size = size/1024;
cache_associativity = associativity;
cache_line_size = line_size;
cache_miss_penalty = miss_penalty;
cache_address_width = address_width;
no_sets = size/(associativity*line_size);
cache_wr_hit_policy = wr_hit_policy;
cache_wr_miss_policy = wr_miss_policy;
cache_hit_time = hit_time;
blk_width = log(line_size)/log(2);
idx_width = log(size/(associativity*line_size))/log(2);
tag_width = address_width - idx_width - blk_width;
no_rd_miss = 0;
num_write = 0;
no_wr_miss = 0;
dty_bit = tag_width+1 ;
vld_bit = dty_bit +1 ;
int i=0;
int j=0;
while( (unsigned)i< cache_associativity){
  while( (unsigned)j< no_sets){
    tag_arr[i][j] = 0;
    tag_dist[i][j] = 0;
j=j+1;  }
i=i+1;  }
}

void cache::print_configuration(){
	/* edit here */
  cout<<"CACHE CONFIGURATION"<<endl;
cout<<"size = " << dec << cache_size << " KB"<<endl;
cout<<"associativity = "<< dec <<cache_associativity << "-way" <<endl;
cout<<"cache line size = "<< dec <<cache_line_size <<" B" <<endl;
if(cache_wr_hit_policy!=true){
cout<<"write hit policy = "<< "write-back"<<endl;
}else if(cache_wr_hit_policy!=false){
cout<<"write hit policy = "<< "write-through"<<endl;
}
if(cache_wr_miss_policy>1 && cache_wr_miss_policy<3 ){
cout<<"write miss policy = "<< "write-allocate" <<endl;
}else if(cache_wr_miss_policy>2 && cache_wr_miss_policy<4){
cout<<"write miss policy = "<< "no-write-allocate" <<endl;
}
cout<<"cache hit time = "<< dec <<cache_hit_time<<" CLK"<<endl;
cout<<"cache miss penalty = "<< dec <<cache_miss_penalty << " CLK"<<endl;
cout<<"memory address width = "<< dec <<cache_address_width <<" bits" <<endl;
}

cache::~cache(){
	/* edit here */
  int i=0;
while( (unsigned)i< cache_associativity){
  int j =0;
  while( (unsigned)j< no_sets){
    tag_arr[i][j] = 0;
    tag_dist[i][j] = 0;
j++;  }
i++;  }
}

void cache::load_trace(const char *filename){
   stream.open(filename);
}

void cache::run(unsigned num_entries){

   unsigned first_access = number_memory_accesses;
   string line;
   unsigned line_nr=0;

   while (getline(stream,line)){

	line_nr++;
        char *str = const_cast<char*>(line.c_str());

        // tokenize the instruction
        char *op = strtok (str," ");
	char *addr = strtok (NULL, " ");
	address_t address = strtoul(addr, NULL, 16);
  switch(*op){
   default:
   if(*op != 'w'){
     read(address);
   }else if (*op != 'r'){
     write(address);
   }}
/* added */
//	cout << "ADDRESS:: @=0x" << hex << address << endl;

	/*
		edit here:
		insert the code to process read and write operations
	   	using the read() and write() functions below

	*/
 memory_accesses=memory_accesses+1;
	number_memory_accesses++;
	if (num_entries!=0 && (number_memory_accesses-first_access)==num_entries)
		break;
   }
}

void cache::print_statistics(){
	cout << "STATISTICS" << endl;
	/* edit here */
  cout << "memory accesses = "<< dec << memory_accesses << endl;
	cout << "read = " << dec << no_rd << endl;
	cout << "read misses = " << dec << no_rd_miss <<endl;
	cout << "write = " << dec << num_write <<endl;
	cout << "write misses = " << dec << no_wr_miss <<endl;
	cout << "evictions = " << dec << no_evicts << endl;
	cout << "memory writes = " << dec << no_mem_wr << endl;
	cout << "average memory access time = " << dec << (cache_hit_time + ((float(no_rd_miss+no_wr_miss)/memory_accesses)*cache_miss_penalty)) << endl;

}

access_type_t cache::read(address_t address){
	/* edit here */
  if(query_tag_array(address) != MISS){
  no_rd=no_rd+1;
  return HIT;
}else {
  int index;
  index = (((((address_t) 1) << idx_width) - 1) & (address >> (blk_width)));
  no_rd=no_rd+1;
  no_rd_miss=no_rd_miss+1;
  int i =0;
  while( (unsigned)i< cache_associativity ){
    if((tag_arr[i][index] & ((address_t) 1)<<vld_bit)==false){
      tag_arr[i][index] = (((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
      tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<vld_bit;
      tag_dist[i][index] = memory_accesses;
      break;
    }
    int cache_check = cache_associativity-1;
     if((i > cache_check-1) && (i < cache_check+1) ){
      i = evict(index);
      if((((((address_t) 1) << 1) - 1) & (tag_arr[i][index] >> (dty_bit)))>0 && (((((address_t) 1) << 1) - 1) & (tag_arr[i][index] >> (dty_bit)))<2){no_mem_wr=no_mem_wr+1;}
      tag_arr[i][index] = (((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
      tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<vld_bit;
      tag_dist[i][index] = memory_accesses;
      break;
    }
i=i+1;  }
return MISS;
}
	return MISS;
}

access_type_t cache::write(address_t address){
	/* edit here */
  if(query_tag_array(address) != MISS){
  num_write=num_write+2-1;
  if(cache_wr_hit_policy!=false){
   no_mem_wr=no_mem_wr+1;
 }else if(cache_wr_hit_policy!=true){
   int index;
   address_t tag;
   index =(((((address_t) 1) << idx_width) - 1) & (address >> (blk_width)));
   tag = (((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
   int i=0;
   while((unsigned)i< cache_associativity){
     if(((((((address_t) 1) << tag_width) - 1) & (tag_arr[i][index] >> (0))) <  (tag+1) )&& ((((((address_t) 1) << tag_width) - 1) & (tag_arr[i][index] >> (0))) >  (tag-1) )){
       tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<dty_bit;
     }
  i=i+1; }

  }
  return HIT;
}else {
  int index;
  index = (((((address_t) 1) << idx_width) - 1) & (address >> (blk_width)));
  num_write=num_write+1;
  no_wr_miss=no_wr_miss+1;
  if(cache_wr_miss_policy>1 && cache_wr_miss_policy<3){
    int i=0;
    while((unsigned)i< cache_associativity ){
      if((tag_arr[i][index] & ((address_t) 1)<<vld_bit)==false){
        tag_arr[i][index] = (((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
        tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<vld_bit;
          tag_dist[i][index] = memory_accesses;
        if(cache_wr_hit_policy!=true){
          tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<dty_bit;
        }
        break;
      }

    int cache_check = cache_associativity-1;
     if((i > cache_check-1) && (i < cache_check+1) ){
      i = evict(index);
      if((((((address_t) 1) << 1) - 1) & (tag_arr[i][index] >> (dty_bit)))>0 && (((((address_t) 1) << 1) - 1) & (tag_arr[i][index] >> (dty_bit)))<2){no_mem_wr=no_mem_wr+1;}
      tag_arr[i][index] = (((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
      tag_arr[i][index] = tag_arr[i][index] | ((address_t) 1)<<vld_bit;
      tag_dist[i][index] = memory_accesses;
        if(cache_wr_hit_policy!=true){//write-back
          tag_arr[i][index] = tag_arr[i][index] |((address_t) 1)<<dty_bit;
        }
      break;
     }
  i=i+1;  }
  }
  if(cache_wr_hit_policy!=false){
   no_mem_wr=no_mem_wr+1;
  }
  return MISS;
}
        return MISS;
}

void cache::print_tag_array(){
	cout << "TAG ARRAY" << endl;
	/* edit here */
  int i=0;
   while ((unsigned)i< cache_associativity ){
	  cout << "BLOCKS " << i << endl;
    if(cache_wr_hit_policy!=true){
      cout << setfill(' ') << setw(7) << "index" << setw(6) << "dirty" << setw(4+tag_width/4) << "tag" << endl;
      int j=0;
      while(j< (int)no_sets ){
       if(tag_arr[i][j] && (((address_t) 1)<vld_bit)){
         cout << setfill(' ') << setw(7) << dec << j << setw(6) << (((((address_t) 1) << 1) - 1) & (tag_arr[i][j] >> (dty_bit))) << "  0x" << hex << (((((address_t) 1) << tag_width) - 1) & (tag_arr[i][j] >> (0))) << endl;
       }
    j=j+1;  }
    }else if(cache_wr_hit_policy!=false){
      cout << setfill(' ') << setw(7) << "index" << setw(4+tag_width/4) << "tag" << endl;
      int k=0;
      while(k< (int)no_sets ){
       if(tag_arr[i][k] && ((address_t) 1)<vld_bit){
         cout << setfill(' ') << setw(7) << dec << k <<"  0x" << hex << setfill('0') << setw(4) << (((((address_t) 1) << tag_width) - 1) & (tag_arr[i][k] >> (0))) << endl;
       }
    k=k+1;  }
    }
  i=i+1; }
}

unsigned cache::evict(unsigned index){
	/* edit here */
  int temp = tag_dist[0][index];
int idx=0,i=0;
while ((unsigned)i< cache_associativity){
   if((unsigned)temp>tag_dist[i][index]) {
      temp=tag_dist[i][index];
      idx=i;
   }
i=i+1;}
no_evicts=no_evicts+1;
return idx;
}

access_type_t cache::query_tag_array(address_t address){
int index;
address_t tag;
index = (((((address_t) 1) << idx_width) - 1) & (address >> (blk_width)));
tag =(((((address_t) 1) << tag_width) - 1) & (address >> (blk_width+idx_width)));
unsigned i=0;
while(i<cache_associativity){
if(((((((address_t) 1) << tag_width) - 1) & (tag_arr[i][index] >> (0))) ==  tag) && (tag_arr[i][index] & ((address_t) 1)<<vld_bit) ){
  tag_dist[i][index] = memory_accesses;
  return HIT;
  break;
}i++;
}
return MISS;
}
