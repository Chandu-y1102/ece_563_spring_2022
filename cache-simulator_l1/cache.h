#ifndef CACHE_H_
#define CACHE_H_

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

#define UNDEFINED 0xFFFFFFFFFFFFFFFF //constant used for initialization

typedef enum {WRITE_BACK, WRITE_THROUGH, WRITE_ALLOCATE, NO_WRITE_ALLOCATE} write_policy_t;

typedef enum {HIT, MISS} access_type_t;

typedef long long address_t; //memory address type

class cache{

	/* Add the data members required by your simulator's implementation here */
	unsigned cache_miss_penalty;
  unsigned cache_address_width;
  unsigned blk_width;
  unsigned idx_width;
  unsigned tag_width;
	unsigned no_mem_wr;
  address_t tag_arr[32][1000000];
  unsigned tag_dist[32][1000000];
  unsigned dty_bit;
	write_policy_t cache_wr_hit_policy;
  write_policy_t cache_wr_miss_policy ;
  unsigned cache_hit_time;
	unsigned vld_bit;
  unsigned no_sets;
  unsigned no_evicts;
	unsigned cache_size;
	unsigned cache_associativity ;
  unsigned cache_line_size;
	unsigned memory_accesses;
  unsigned no_rd;
  unsigned no_rd_miss;
	unsigned no_wr_miss;
  unsigned num_write;
	/* number of memory accesses processed */
	unsigned number_memory_accesses;

	/* trace file input stream */
	ifstream stream;


public:

	/*
	* Instantiates the cache simulator
        */
	cache(unsigned cache_size, 		// cache size (in bytes)
              unsigned cache_associativity,     // cache associativity (fully-associative caches not considered)
	      unsigned cache_line_size,         // cache block size (in bytes)
	      write_policy_t write_hit_policy,  // write-back or write-through
	      write_policy_t write_miss_policy, // write-allocate or no-write-allocate
	      unsigned cache_hit_time,		// cache hit time (in clock cycles)
	      unsigned cache_miss_penalty,	// cache miss penalty (in clock cycles)
	      unsigned address_width            // number of bits in memory address
	);

	// de-allocates the cache simulator
	~cache();

	// loads the trace file (with name "filename") so that it can be used by the "run" function
	void load_trace(const char *filename);

	// processes "num_memory_accesses" memory accesses (i.e., entries) from the input trace
	// if "num_memory_accesses=0" (default), then it processes the trace to completion
	void run(unsigned num_memory_accesses=0);

	// processes a read operation and returns hit/miss
	access_type_t read(address_t address);

	// processes a write operation and returns hit/miss
	access_type_t write(address_t address);
	access_type_t query_tag_array(address_t address);

	// returns the next block to be evicted from the cache
	unsigned evict(unsigned index);

	// prints the cache configuration
	void print_configuration();

	// prints the execution statistics
	void print_statistics();

	//prints the metadata information (including "dirty" but, when applicable) for all valid cache entries
	void print_tag_array();
};

#endif /*CACHE_H_*/
