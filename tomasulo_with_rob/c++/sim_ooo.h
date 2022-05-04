//header file for tomasulo Simulator
//Owner -- Chandu Yuvarajappa


#ifndef SIM_OO_H_
#define SIM_OO_H_

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <cstring>
#include <sstream>

using namespace std;

#define UNDEFINED 0xFFFFFFFF //constant used for initialization
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 24
#define NUM_STAGES 4
#define MAX_UNITS 10
#define PROGRAM_SIZE 50

// instructions supported
typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, AND, MULT, DIV, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, LWS, SWS, ADDS, SUBS, MULTS, DIVS} opcode_t;

// reservation stations types
typedef enum {INTEGER_RS, ADD_RS, MULT_RS, LOAD_B} res_station_t;

// execution units types
typedef enum {INTEGER, ADDER, MULTIPLIER, DIVIDER, MEMORY} exe_unit_t;

// stages names
typedef enum {ISSUE, EXECUTE, WRITE_RESULT, COMMIT} stage_t;

//
typedef enum {RS, ROB} DS_t;

// instruction data type
typedef struct{
        opcode_t opcode;
        unsigned src1;
        unsigned src2;
        unsigned dest;
        unsigned immediate;
        string label;
} instruction_t;

typedef struct{
  unsigned result;
  unsigned cycles;
	bool ready;
	unsigned pc;
	stage_t state;
	unsigned destination;
	unsigned value;
}rob_entry_t;

// entry in the "instruction window"
typedef struct{
  unsigned wr;
	unsigned commit;
	unsigned pc;
	unsigned issue;
	unsigned exe;
} instr_window_entry_t;

// execution unit
typedef struct{
        unsigned clean_exec_program_counter;
        unsigned imm;
        opcode_t opcode;
        exe_unit_t type;
        unsigned latency;
        unsigned busy;
        unsigned pc;
} unit_t;

// ROB
typedef struct{
	unsigned num_entries;
	rob_entry_t *entries;
} rob_t;



//instruction window
typedef struct{
	unsigned num_entries;
	instr_window_entry_t *entries;
} instr_window_t;

// reservation station entry
typedef struct{
  unsigned address;
	unsigned result;
	unsigned write_result;
	res_station_t type;
	unsigned name;
	unsigned pc;
	unsigned value1;
	unsigned value2;
	unsigned tag1;
	unsigned tag2;
	unsigned destination;
}res_station_entry_t;

// reservation stations
typedef struct{
	unsigned num_entries;
	res_station_entry_t *entries;
}res_stations_t;

class sim_ooo{
  unsigned gp_int[NUM_GP_REGISTERS];
  unsigned gp_int_tag[NUM_GP_REGISTERS];
  float gp_fp[NUM_GP_REGISTERS];
  unsigned gp_fp_tag[NUM_GP_REGISTERS];

	unsigned stall[2];
  unsigned free_res_stat;
  unsigned free_unit;
  unsigned count;

	/* end added data members */

	//issue width
	unsigned issue_width;

	//instruction window
	instr_window_t pending_instructions;

	//reorder buffer
	rob_t rob;
  unsigned load_proceed;
  bool store_com_bypass;
  unsigned str_bypass_write[10];
  unsigned str_bypass_rdy[10];
  unsigned str_bypass_write_ready[10];
  unsigned str_bypass_value[10];
  unsigned clean_str_execution;
	//reservation stations
	res_stations_t reservation_stations;
  unsigned clear_res_stat_idx[10];
  unsigned wax_idx;
  bool branch_end_of_program;
	//execution units
        unit_t exec_units[MAX_UNITS];
        unsigned num_units;

	//instruction memory
	instruction_t instr_memory[PROGRAM_SIZE];

        //base address in the instruction memory where the program is loaded
        unsigned instr_base_address;

	//data memory - should be initialize to all 0xFF
	unsigned char *data_memory;

	//memory size in bytes
	unsigned data_memory_size;

	//instruction executed
	unsigned instructions_executed;

	//clock cycles
	unsigned clock_cycles;
  unsigned waw[2];
  bool end_of_program;
  unsigned clean_rb_idx;
	//execution log
	stringstream log;

public:

	/* Instantiates the simulator
          	Note: registers must be initialized to UNDEFINED value, and data memory to all 0xFF values
        */
	sim_ooo(unsigned mem_size, 		// size of data memory (in byte)
		unsigned rob_size, 		// number of ROB entries
                unsigned num_int_res_stations,	// number of integer reservation stations
                unsigned num_add_res_stations,	// number of ADD reservation stations
                unsigned num_mul_res_stations, 	// number of MULT/DIV reservation stations
                unsigned num_load_buffers,	// number of LOAD buffers
		unsigned issue_width=1		// issue width
        );

	//de-allocates the simulator
	~sim_ooo();

        // adds one or more execution units of a given type to the processor
        // - exec_unit: type of execution unit to be added
        // - latency: latency of the execution unit (in clock cycles)
        // - instances: number of execution units of this type to be added
        void init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances=1);

	//related to functional unit
	unsigned get_free_unit(opcode_t opcode);

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0)
	void run(unsigned cycles=0);

	//resets the state of the simulator
        /* Note:
	   - registers should be reset to UNDEFINED value
	   - data memory should be reset to all 0xFF values
	   - instruction window, reservation stations and rob should be cleaned
	*/
	void reset();
  bool clean_exec_program_counter;
  unsigned branch_target;
  bool branch_picked;
  bool eop;

       //returns value of the specified integer general purpose register
        int get_int_register(unsigned reg);

        //set the value of the given integer general purpose register to "value"
        void set_int_register(unsigned reg, int value);

        //returns value of the specified floating point general purpose register
        float get_fp_register(unsigned reg);

        //set the value of the given floating point general purpose register to "value"
        void set_fp_register(unsigned reg, float value);

	// returns the index of the ROB entry that will write this integer register (UNDEFINED if the value of the register is not pending
	unsigned get_int_register_tag(unsigned reg);

	// returns the index of the ROB entry that will write this floating point register (UNDEFINED if the value of the register is not pending
	unsigned get_fp_register_tag(unsigned reg);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of clock cycles
	unsigned get_clock_cycles();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

	//prints the values of the registers
	void print_registers();

	//prints the status of processor excluding memory
	void print_status();

	// prints the content of the ROB
	void print_rob();

	//prints the content of the reservation stations
	void print_reservation_stations();

	//print the content of the instruction window
	void print_pending_instructions();

	//initialize the execution log
	void init_log();

	//commit an instruction to the log
	void commit_to_log(instr_window_entry_t iwe);

	//print log
	void print_log();

  unsigned get_free_res_stat(opcode_t opcode);

  //issue stage
	void issue();

  void execution();

  void write_result();

  void add_stall();
  unsigned rb_wr_idx;
  unsigned rb_rd_idx;
  unsigned rb_rd_idx_branch;

  void init_res_stat(opcode_t opcode);

  void init_rb(opcode_t opcode);

  unsigned a_l_u_mem(opcode_t opcode, unsigned value1, unsigned value2, unsigned immediate, unsigned pc);

  void clear_res_stat(unsigned entry);
  void clear_rb(unsigned entry);

  void commit();
  void tag_updt();
  unsigned rb_pick(unsigned src, unsigned fp);

  unsigned load_store_bp(unsigned re_or_buf_idx,unsigned res_stat_idx);

};

#endif /*SIM_OOO_H_*/
