//code for the floating pipe simulation of the pipeline
//sim_pipe_fp.h c++ PROGRAM
//header file
//coder - Chandu Yuvarajappa
//std_id - 200366211

#ifndef SIM_PIPE_FP_H_
#define SIM_PIPE_FP_H_

#include <stdio.h>
#include <string>

using namespace std;

#define PROGRAM_SIZE 50
#define UNDEFINED 0xFFFFFFFF
#define NUM_SP_REGISTERS 9
#define NUM_SP_INT_REGISTERS 15
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 22
#define NUM_STAGES 5
#define MAX_UNITS 10

typedef enum {
    LW,  SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP, LWS, SWS, ADDS, SUBS, MULTS,DIVS
} opcode_t;

typedef enum {
    INTEGER, ADDER, MULTIPLIER, DIVIDER} exe_unit_t;

typedef struct {
    opcode_t opcode;
    unsigned src1;
    unsigned src2;
    unsigned dest;
    unsigned immediate;
    string label;
    unsigned inum;
} instruction_t;

typedef struct {
    exe_unit_t type;
    unsigned delayed;
    unsigned busy;
    int next_pc;
    unsigned x;
    unsigned y;
    unsigned xyz;
    instruction_t instruction;
} unit_t;

typedef enum {
    PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

typedef enum {
    IF, ID, EXE, MEM, WB} stage_t;

typedef enum {
    RAW, WAR, WAW, STRUCTURAL, CONTROL} hazard_type_t;

typedef struct {
    bool in_stall;
    hazard_type_t hazard_type;
    unsigned reg;} stall_t;

class sp_register_fp {
private:
    instruction_t instr_register[NUM_STAGES];
    unsigned spRegisters[NUM_STAGES][NUM_SP_REGISTERS];
public:
    void set_sp_register(sp_register_t reg, stage_t s, unsigned value);
    unsigned get_sp_register(sp_register_t reg, stage_t s);
    void set_ir_register(instruction_t instruction, stage_t s);
    instruction_t get_ir_register(stage_t s);
    instruction_t *get_ir_registers();
    void clean_Register(stage_t s);
    void reset();
};

class sim_pipe_fp {
    bool pipe_run;
    bool end;
    sp_register_fp *sp_registers;
    stall_t stall_pipe[NUM_STAGES];
    int delayed[NUM_STAGES];
    bool stall_put;
    int total_clock_cycle;
    unsigned int total_instr_count;
    unsigned int stalls;
    unsigned *registerFile;
    unsigned *registerFile_fp;
    instruction_t instr_memory[PROGRAM_SIZE];
    unsigned instr_base_address;
    unsigned char *data_memory;
    unsigned data_memory_size;
    unsigned data_memory_latency;
    unit_t exec_units[MAX_UNITS];
    unsigned num_units;
public:
    sim_pipe_fp(unsigned data_mem_size, unsigned data_mem_latency);
    ~sim_pipe_fp();
    void init_exec_unit(exe_unit_t exec_unit, unsigned delayed, unsigned instances = 1);
    void load_program(const char *filename, unsigned base_address = 0x0);
    void run(unsigned cycles = 0);
    void reset();
    unsigned get_sp_register(sp_register_t reg, stage_t stage);
    int get_int_register(unsigned reg);
    void set_int_register(unsigned reg, int value);
    float get_fp_register(unsigned reg);
    void set_fp_register(unsigned reg, float value);
    float get_IPC();
    unsigned get_instructions_executed();
    unsigned get_clock_cycles();
    unsigned get_stalls();
    void print_memory(unsigned start_address, unsigned end_address);
    void write_memory(unsigned address, unsigned value);
    void write_memory_fp(unsigned address, float value);
    unsigned load_memory(unsigned address);
    void print_registers();
    void inst_fetch();
    void inst_decode();
    void inst_execute();
    void inst_memory();
    void inst_writeback();
    void checking_Stall(stage_t, instruction_t);
    bool is_in_stall(stage_t);
private:
    unsigned get_free_unit(opcode_t opcode);
    void units_busy_time();
    void debug_units();
    void inst_execute_int(const instruction_t &ir, unsigned int a, unsigned int b, unsigned int aluoutput) const;
    void check_wr_af_wr(int i, const unit_t &unit, instruction_t ir);
};

#endif /*SIM_PIPE_FP_H_*/
