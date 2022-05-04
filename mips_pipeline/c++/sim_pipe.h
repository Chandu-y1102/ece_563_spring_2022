//code for integer pipeline
//header file
//coder - Chandu Yuvarajappa
//std_id - 200366211


#ifndef SIM_PIPE_H_
#define SIM_PIPE_H_

#include <stdio.h>
#include <string>

using namespace std;

#define PROGRAM_SIZE 50
#define DEBUG   0
#define UNDEFINED 0xFFFFFFFF //create intirla reg values
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 16
#define NUM_STAGES 5

typedef enum {
    PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

typedef enum {
    LW, SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;

typedef enum {
    IF, ID, EXE, MEM, WB} stage_t;

typedef struct {
    opcode_t opcode;
    unsigned src1;
    unsigned src2;
    unsigned dest;
    unsigned immediate;
    string label;} instruction_t;
typedef enum {
    RAW, WAR, WAW, STRUCTURAL, CONTROL} hazard_type_t;

typedef struct {
    bool in_stall;
    hazard_type_t hazard_type;
    unsigned reg;} stall_t;

class sp_register {
private:
    instruction_t instr_register[NUM_STAGES];
    unsigned spRegisters[NUM_STAGES][NUM_SP_REGISTERS];
public:
    void set_sp_register(sp_register_t reg, stage_t s, unsigned value);
    unsigned get_sp_register(sp_register_t reg, stage_t s);
    void set_ir_register(instruction_t instruction, stage_t s);
    instruction_t get_ir_register(stage_t s);
    instruction_t *get_ir_registers();
    void clean_registers(stage_t s);
    void reset();};

class sim_pipe {
    bool pipe_run;
    bool end;
    sp_register *sp_registers;
    stall_t stall_pipe[NUM_STAGES];
    int latency[NUM_STAGES];
    bool stall_put;
    int total_clock_cycle;
    unsigned int total_instr_count;
    unsigned int stalls;
    unsigned *registerFile;
    instruction_t instr_memory[PROGRAM_SIZE];
    unsigned instr_base_address;
    unsigned char *data_memory;
    unsigned data_memory_size;
    unsigned data_memory_latency;
public:
    sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);
    ~sim_pipe();
    void load_program(const char *filename, unsigned base_address = 0x0);
    void run(unsigned cycles = 0);
    void reset();
    void print_registers();
    void inst_fetch();
    void inst_decode();
    void inst_execute();
    void inst_memory();
    void inst_writeback();
    void checking_Stall(stage_t, instruction_t);
    unsigned get_sp_register(sp_register_t reg, stage_t stage);
    int get_gp_register(unsigned reg);
    void set_gp_register(unsigned reg, int value);
    unsigned get_stalls();
    void print_memory(unsigned start_address, unsigned end_address);
    void write_memory(unsigned address, unsigned value);
    unsigned load_memory(unsigned address);
    float get_IPC();
    unsigned get_instructions_executed();
    unsigned get_clock_cycles();
    bool is_in_stall(stage_t);};

#endif /*SIM_PIPE_H_*/
