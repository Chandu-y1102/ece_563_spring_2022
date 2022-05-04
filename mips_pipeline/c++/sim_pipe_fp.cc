//code for the floating pipe simulation of the pipeline
//sim_pipe_fp.cc c++ PROGRAM_SIZE
//source file
//coder - Chandu Yuvarajappa
//std_id - 200366211


#include "sim_pipe_fp.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

using namespace std;

static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ","BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP", "LWS", "SWS", "ADDS", "SUBS", "MULTS", "DIVS"};
static const char *unit_names[4] = {"INTEGER", "ADDER", "MULTIPLIER", "DIVIDER"};

inline unsigned float2unsigned(float value) {
    unsigned result;
    memcpy(&result, &value, sizeof value);
    return result;
}

inline float unsigned2float(unsigned value) {
    float result;
    memcpy(&result, &value, sizeof value);
    return result;
}
inline void unsigned2char(unsigned value, unsigned char *buffer) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}
inline unsigned char2unsigned(unsigned char *buffer) {
    return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);}
bool branch_sel(opcode_t opcode) {
    return (opcode == BEQZ || opcode == BNEZ || opcode == BLTZ || opcode == BLEZ || opcode == BGTZ || opcode == BGEZ || opcode == JUMP);}
bool is_memory(opcode_t opcode) {
    return (opcode == LW || opcode == SW || opcode == LWS || opcode == SWS);}
bool is_int_r(opcode_t opcode) {
    return (opcode == ADD || opcode == SUB || opcode == XOR);}
bool is_int_imm(opcode_t opcode) {
    return (opcode == ADDI || opcode == SUBI);}
bool is_int_alu(opcode_t opcode) {
    return (is_int_r(opcode) || is_int_imm(opcode));}
bool fl_point_alu(opcode_t opcode) {
    return (opcode == ADDS || opcode == SUBS || opcode == MULTS || opcode == DIVS);}

/* implements the ALU operations */
unsigned alu(unsigned opcode, unsigned a, unsigned b, unsigned imm, unsigned npc) {
    switch (opcode) {
        case ADD:
            return (a + b);
        case ADDI:
            return (a + imm);
        case SUB:
            return (a - b);
        case SUBI:
            return (a - imm);
        case XOR:
            return (a ^ b);
        case LW:
        case SW:
        case LWS:
        case SWS:
            return (a + imm);
        case BEQZ:
        case BNEZ:
        case BGTZ:
        case BGEZ:
        case BLTZ:
        case BLEZ:
        case JUMP:
            return (npc + imm);
        case ADDS:
            return (float2unsigned(unsigned2float(a) + unsigned2float(b)));
            break;
        case SUBS:
            return (float2unsigned(unsigned2float(a) - unsigned2float(b)));
            break;
        case MULTS:
            return (float2unsigned(unsigned2float(a) * unsigned2float(b)));
            break;
        case DIVS:
            return (float2unsigned(unsigned2float(a) / unsigned2float(b)));
            break;
        default:
            return (-1);
    }
}

/* =============================================================

   CODE PROVIDED - NO NEED TO MODIFY FUNCTIONS BELOW

   ============================================================= */

/* ============== primitives to allocate/free the simulator ================== */

sim_pipe_fp::sim_pipe_fp(unsigned mem_size, unsigned mem_latency) {
    data_memory_size = mem_size;
    data_memory_latency = mem_latency;
    data_memory = new unsigned char[data_memory_size];
    num_units = 0;
    reset();
}

sim_pipe_fp::~sim_pipe_fp() {
    delete[] data_memory;
    delete registerFile;
    delete sp_registers;
}

/* =============   primitives to print out the content of the memory & registers and for writing to memory ============== */

void sim_pipe_fp::print_memory(unsigned start_address, unsigned end_address) {
    cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8)
         << setfill('0') << end_address << "]" << endl;
    for (unsigned i = start_address; i < end_address; i++) {
        if (i % 4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": ";
        cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
        if (i % 4 == 3) {
#ifdef DEBUG_MEMORY
            unsigned u = char2unsigned(&data_memory[i-3]);
            cout << " - unsigned=" << u << " - float=" << unsigned2float(u);
#endif
            cout << endl;
        }
    }
}

void sim_pipe_fp::write_memory(unsigned address, unsigned value) {
    unsigned2char(value, data_memory + address);
}

void sim_pipe_fp::write_memory_fp(unsigned address, float value) {
    unsigned2char(value, data_memory + address);
}

void sim_pipe_fp::print_registers() {
    cout << "Special purpose registers:" << endl;
    unsigned i, s;
    for (s = 0; s < NUM_STAGES; s++) {
        cout << "Stage: " << stage_names[s] << endl;
        for (i = 0; i < NUM_SP_REGISTERS; i++)
            if ((sp_register_t) i != IR && (sp_register_t) i != COND &&
                get_sp_register((sp_register_t) i, (stage_t) s) != UNDEFINED)
                cout << reg_names[i] << " = " << dec << get_sp_register((sp_register_t) i, (stage_t) s) << hex
                     << " / 0x" << get_sp_register((sp_register_t) i, (stage_t) s) << endl;
    }
    cout << "General purpose registers:" << endl;
    for (i = 0; i < NUM_GP_REGISTERS; i++)
        if (get_int_register(i) != (int) UNDEFINED)
            cout << "R" << dec << i << " = " << get_int_register(i) << hex << " / 0x" << get_int_register(i) << endl;
    for (i = 0; i < NUM_GP_REGISTERS; i++)
        if (get_fp_register(i) != UNDEFINED)
            cout << "F" << dec << i << " = " << get_fp_register(i) << hex << " / 0x"
                 << float2unsigned(get_fp_register(i)) << endl;
}


/* =============   primitives related to the functional units ============== */

/* initializes an execution unit */
void sim_pipe_fp::init_exec_unit(exe_unit_t exec_unit, unsigned delayed, unsigned instances) {
    for (unsigned i = 0; i < instances; i++) {
        exec_units[num_units].type = exec_unit;
        exec_units[num_units].delayed = delayed;
        exec_units[num_units].busy = 0;
        exec_units[num_units].instruction.opcode = NOP;
        num_units++;
    }
}

/* returns a free unit for that particular operation or UNDEFINED if no unit is currently available */
unsigned sim_pipe_fp::get_free_unit(opcode_t opcode) {
    if (num_units == 0) {
        cout << "ERROR:: simulator does not have any execution units!\n";
        exit(-1);
    }
    for (unsigned u = 0; u < num_units; u++) {
        switch (opcode) {
            //Integer unit
            case LW:
            case SW:
            case ADD:
            case ADDI:
            case SUB:
            case SUBI:
            case XOR:
            case BEQZ:
            case BNEZ:
            case BLTZ:
            case BGTZ:
            case BLEZ:
            case BGEZ:
            case JUMP:
            case LWS:
            case SWS:
                if (exec_units[u].type == INTEGER && exec_units[u].busy == 0) return u;
                break;
                // FP adder
            case ADDS:
            case SUBS:
                if (exec_units[u].type == ADDER && exec_units[u].busy == 0) return u;
                break;
                // Multiplier
            case MULTS:
                if (exec_units[u].type == MULTIPLIER && exec_units[u].busy == 0) return u;
                break;
                // Divider
            case DIVS:
                if (exec_units[u].type == DIVIDER && exec_units[u].busy == 0) return u;
                break;
            default:
                //cout << "ERROR:: operations not requiring exec unit!\n";
                // exit(-1);
                break;
        }
    }
    return UNDEFINED;
}

/* decrease the amount of clock cycles during which the functional unit will be busy - to be called at each clock cycle  */
void sim_pipe_fp::units_busy_time() {
    for (unsigned u = 0; u < num_units; u++) {
        if (exec_units[u].busy > 0) exec_units[u].busy--;
    }
}


/* prints out the status of the functional units */
void sim_pipe_fp::debug_units() {
    for (unsigned u = 0; u < num_units; u++) {
        cout << " -- unit " << unit_names[exec_units[u].type] << " delayed=" << exec_units[u].delayed << " busy="
             << exec_units[u].busy <<
             " instruction=" << instr_names[exec_units[u].instruction.opcode] << endl;
    }
}
void sim_pipe_fp::load_program(const char *filename, unsigned base_address) {
    instr_base_address = base_address;
    sp_registers->set_sp_register(PC, IF, base_address);
    map<string, opcode_t> opcodes; //for opcodes
    map<string, unsigned> labels;  //for branches
    for (int i = 0; i < NUM_OPCODES; i++)
        opcodes[string(instr_names[i])] = (opcode_t) i;
    ifstream fin(filename, ios::in | ios::binary);
    if (!fin.is_open()) {
        cerr << "error: open file " << filename << " failed!" << endl;
        exit(-1);
    }
    string line;
    unsigned instruction_nr = 0;
    while (getline(fin, line)) {
        char *str = const_cast<char *>(line.c_str());
        char *token = strtok(str, " \t");
        map<string, opcode_t>::iterator search = opcodes.find(token);
        if (search == opcodes.end()) {
            string label = string(token).substr(0, string(token).length() - 1);
            labels[label] = instruction_nr;
            token = strtok(NULL, " \t");
            search = opcodes.find(token);
            if (search == opcodes.end()) cout << "ERROR: invalid opcode: " << token << " !" << endl;
        }
        instr_memory[instruction_nr].opcode = search->second;
        char *par1;
        char *par2;
        char *par3;
        switch (instr_memory[instruction_nr].opcode) {
            case ADD:
            case SUB:
            case XOR:
            case ADDS:
            case SUBS:
            case MULTS:
            case DIVS:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                par3 = strtok(NULL, " \t");
                instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
                instr_memory[instruction_nr].src1 = atoi(strtok(par2, "RF"));
                instr_memory[instruction_nr].src2 = atoi(strtok(par3, "RF"));
                break;
            case ADDI:
            case SUBI:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                par3 = strtok(NULL, " \t");
                instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
                instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
                instr_memory[instruction_nr].immediate = strtoul(par3, NULL, 0);
                break;
            case LW:
            case LWS:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
                instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
                instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
                break;
            case SW:
            case SWS:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].src1 = atoi(strtok(par1, "RF"));
                instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
                instr_memory[instruction_nr].src2 = atoi(strtok(NULL, "R"));
                break;
            case BEQZ:
            case BNEZ:
            case BLTZ:
            case BGTZ:
            case BLEZ:
            case BGEZ:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
                instr_memory[instruction_nr].label = par2;
                break;
            case JUMP:
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].label = par2;
            default:
                break;

        }

        /* increment instruction number before moving to next line */
        instruction_nr++;
    }
    //reconstructing the labels of the branch operations
    int i = 0;
    while (true) {
        instruction_t instr = instr_memory[i];
        if (instr.opcode == EOP) break;
        if (instr.opcode == BLTZ || instr.opcode == BNEZ ||
            instr.opcode == BGTZ || instr.opcode == BEQZ ||
            instr.opcode == BGEZ || instr.opcode == BLEZ ||
            instr.opcode == JUMP
                ) {
            instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
        }
        i++;
    }

}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */


inline void branch_pick(sp_register_fp *sp_registers) {//getting the branch for the pipe jumping around
    sp_registers->set_sp_register(COND, MEM, 1);}
void sim_pipe_fp::run(unsigned cycles) {//running the pipe for execution
    if (cycles == 0) {      pipe_run = true;  }
    while (pipe_run || cycles != 0) {
        inst_writeback();//all the pipe stages
        inst_memory();
        inst_execute();
        inst_decode();
        inst_fetch();
        units_busy_time();
        if (stall_put) {
            stall_put = false;
            stalls++;    }
        total_clock_cycle = total_clock_cycle+ 1;//total time for the design
        if (!pipe_run && cycles > 0) { cycles--; }  }//cycle to be set back to design
}
//
//
//
// WRITE BACK stage written pull back method
//
//
//
void sim_pipe_fp::inst_writeback() {//write back pipe stage of the design
    instruction_t iwr = sp_registers->get_ir_register(WB);
    if (iwr.opcode == NOP) {        return;  }//no operation//check for no operation / stall and wait before sending back
    if (iwr.opcode == EOP) {//end of the operation//check for end of operation
        pipe_run = false;
        return;}
    unsigned ari_log_output = sp_registers->get_sp_register(ALU_OUTPUT, WB);//back to alu , as back ward logic looping
    unsigned wlmd = sp_registers->get_sp_register(LMD, WB);//back to memory, as back logic and clearing the pipe
    switch (iwr.opcode) {//execute the sequence//check for the code remaining in the pipe
        case LW:
            set_int_register(iwr.dest, wlmd);
            break;
        case LWS:
            set_fp_register(iwr.dest, unsigned2float(wlmd));
        case SW:
            break;
        case SWS:
            break;
        case BEQZ:
        break;
        case BGEZ:
        break;
        case BGTZ:
        break;
        case BLEZ:
        break;
        case BLTZ:
        break;
        case BNEZ:
            break;
        case ADDS:
        break;
        case SUBS:
        break;
        case DIVS:
        break;
        case MULTS:
            set_fp_register(iwr.dest, unsigned2float(ari_log_output));
            break;
        default:
            set_int_register(iwr.dest, ari_log_output);}
    checking_Stall(WB, iwr);
    total_instr_count++;}//finall executed stage at the end of the pipe
//
//
//
// LOAD STORE Stage
//
//
//
    void sim_pipe_fp::inst_memory() {//LW/SW unit
        instruction_t imr = sp_registers->get_ir_register(MEM);
        checking_Stall(MEM, imr);
        if (is_in_stall(MEM)) {
            stall_put = true;
            instruction_t stall_for_Reg;
            stall_for_Reg.opcode = NOP;//checking for the design time//stall until current is completed without latency
            stall_for_Reg.src1 = UNDEFINED;
            stall_for_Reg.src2 = UNDEFINED;
            stall_for_Reg.dest = UNDEFINED;
            stall_for_Reg.immediate = UNDEFINED;
            sp_registers->clean_Register(WB);//cleaing for the design pipe
            sp_registers->set_ir_register(stall_for_Reg, WB);
            return;    }
        if (imr.opcode == NOP) {//no operation//stall
            sp_registers->clean_Register(WB);
            sp_registers->set_ir_register(imr, WB);
            return;  }
        if (imr.opcode == EOP) {//end of operation
            sp_registers->clean_Register(WB);
            sp_registers->set_ir_register(imr, WB);
            return;    }
        unsigned ari_log_output = sp_registers->get_sp_register(ALU_OUTPUT, MEM);//check for the alu operation//data sent back for the iteration in the back ward method
        unsigned y = sp_registers->get_sp_register(B, MEM);//second regcheck//check the particualr load store operations
        unsigned condition = sp_registers->get_sp_register(COND, MEM);//if the src is registered//check for branch in the pipeline coming backwards
        sp_registers->clean_Register(WB);
        sp_registers->set_sp_register(ALU_OUTPUT, WB, ari_log_output);//check for the alu condition
        sp_registers->set_sp_register(COND, WB, condition);
        switch (imr.opcode) {//particular load ans store are checked for the operation
          case ADDI:
          break;
          case SUBI:
          break;
          case ADD:
          break;
          case SUB:
          break;
          case XOR:
          break;
          case BEQZ:
          break;
          case BNEZ:
          break;
          case BLTZ:
          break;
          case BGTZ:
          break;
          case BLEZ:
          break;
          case BGEZ:
          break;
          case JUMP:
          break;
          case EOP:
          break;
          case NOP:
          break;
          case ADDS:
          break;
          case SUBS:
          break;
          case MULTS:
          break;
          case DIVS:
          break;
            case SW:
                write_memory(ari_log_output, y);
                break;
            case SWS:
                write_memory_fp(ari_log_output, y);
                break;
            case LW:
            break;
            case LWS:
                unsigned mlmd = load_memory(ari_log_output);
                sp_registers->set_sp_register(LMD, WB, mlmd);
                break;  }
        sp_registers->set_ir_register(imr, WB);}

//
//
//
//FLOATING POOINT EXECUITON
//
//
//
void sim_pipe_fp::inst_execute() {//fp execution unit
    instruction_t ier = sp_registers->get_ir_register(EXE);
    checking_Stall(EXE, ier);//check stall from the previous stage
    if (is_in_stall(MEM)) {  return;  }
    if (ier.opcode == EOP || ier.opcode == NOP) {//check if its integer or fp
        bool exe_complete = true;
        for (int a = 0; a < (int)num_units; ++a) {
            if(exec_units[a].busy !=0){  exe_complete = false;}    }//check if integer is done
        if(exe_complete) {
            sp_registers->clean_Register(MEM);
            sp_registers->set_ir_register(ier, MEM);  }//if done start next step
           else {
            instruction_t stall_for_Reg;
            stall_for_Reg.opcode = NOP;//no operations
            stall_for_Reg.src1 = UNDEFINED;
            stall_for_Reg.src2 = UNDEFINED;
            stall_for_Reg.dest = UNDEFINED;
            stall_for_Reg.immediate = UNDEFINED;
            sp_registers->clean_Register(MEM);
            sp_registers->set_ir_register(stall_for_Reg, MEM);    }  }
       else if (!is_in_stall(EXE)) {//if the execution is stalled check for the design
        int mode = get_free_unit(ier.opcode);//free the pipe if the design is incomplete
        exec_units[mode].busy = exec_units[mode].delayed;
        exec_units[mode].instruction = ier;
        exec_units[mode].next_pc = sp_registers->get_sp_register(NPC, EXE);//next program counter is updated to proceed the pipeline
        exec_units[mode].x = sp_registers->get_sp_register(A, EXE);//source register 1 is set
        exec_units[mode].y = sp_registers->get_sp_register(B, EXE);//source register 2 is set
        exec_units[mode].xyz = sp_registers->get_sp_register(IMM, EXE);// check for the computed value in the pipe
        instruction_t stall_for_Reg;//stall the design for the next reg set
        stall_for_Reg.opcode = NOP;
        stall_for_Reg.src1 = UNDEFINED;//next program counter is updated to proceed the pipeline
        stall_for_Reg.src2 = UNDEFINED;//source register 1 is set
        stall_for_Reg.dest = UNDEFINED;//source register 2 is set
        stall_for_Reg.immediate = UNDEFINED;// check for the computed value in the pipe
        sp_registers->clean_Register(MEM);//clear the reg for next iteration
        sp_registers->set_ir_register(stall_for_Reg, MEM);  }
    for (int n = 0; n < (int)num_units; ++n) {
        unit_t test_unit = exec_units[n];
        if (test_unit.busy == 0 && test_unit.instruction.opcode != NOP) {//check for the exe stepS
            if (!(is_in_stall(EXE) )) {//sta;; tp avoid the hazard
                 if(!(stall_pipe[EXE].hazard_type==WAW)){
                sp_registers->clean_Register(EXE);
                unsigned ari_log_output = alu(test_unit.instruction.opcode, test_unit.x, test_unit.y, test_unit.xyz, test_unit.next_pc);
                if (!(fl_point_alu(test_unit.instruction.opcode))) {
                    inst_execute_int(test_unit.instruction, test_unit.x, test_unit.y, ari_log_output);    }
                else {
                    sp_registers->clean_Register(MEM);// check for the next stage
                    sp_registers->set_sp_register(ALU_OUTPUT, MEM, ari_log_output);
                    sp_registers->set_sp_register(B, MEM, test_unit.y);
                    sp_registers->set_ir_register(test_unit.instruction, MEM);        }
                instruction_t stall_for_Reg;
                stall_for_Reg.opcode = NOP;//clear the design
                stall_for_Reg.src1 = UNDEFINED;
                stall_for_Reg.src2 = UNDEFINED;
                stall_for_Reg.dest = UNDEFINED;
                stall_for_Reg.immediate = UNDEFINED;
                exec_units[n].instruction = stall_for_Reg;    }      }    }}
}

//
//
//
// INTEGER execution
//
//
//
void sim_pipe_fp::inst_execute_int(const instruction_t &ier, unsigned int x, unsigned int y, unsigned int ari_log_output) const {
    sp_registers->clean_Register(MEM);
    sp_registers->set_sp_register(ALU_OUTPUT, MEM, ari_log_output);
    sp_registers->set_sp_register(B, MEM, y);
    switch (ier.opcode) {//check the design for the opcode
        case ADD:
        break;
        case EOP:
        break;
        case SUB:
        break;
        case ADDS:
        break;
        case SUBS:
        break;
        case MULTS:
        break;
        case DIVS:
        break;
        case XOR:
            break;
        case ADDI:
        break;
        case SUBI:
            break;
        case BEQZ:
            if ((int) x == 0) {          branch_pick(sp_registers);        }
            break;
        case BNEZ:
            if ((int) x != 0) {          branch_pick(sp_registers);      }
            break;
        case BLTZ:
            if ((int) x < 0) {      branch_pick(sp_registers);    }
            break;
        case BGTZ:
            if ((int) x > 0) {          branch_pick(sp_registers);      }
            break;
        case BLEZ:
            if ((int) x <= 0) {            branch_pick(sp_registers);        }
            break;
        case BGEZ:
            if ((int) x >= 0) {              branch_pick(sp_registers);          }
            break;
        case JUMP:
            branch_pick(sp_registers);
            break;
        case LW:
        break;
        case SW:
        break;
        case LWS:
        break;
        case SWS:
            sp_registers->set_sp_register(B, MEM, y);
            break;
        case NOP:
            break;  }
    sp_registers->set_ir_register(ier, MEM);}

//
//
//
//DECODE stages
//
//
//
  void sim_pipe_fp::inst_decode() {//getting and updating the register value for the pipeline execution
        instruction_t idr = sp_registers->get_ir_register(ID);//setting the value for the regi struct
        unsigned x = UNDEFINED;//src1
        unsigned y = UNDEFINED;//src2
        unsigned xyz = UNDEFINED;//dest
        if (is_in_stall(MEM)) {  return;    }
        if (is_in_stall(ID)) {
            stall_put = true;
            instruction_t stall_for_Reg;//check for the design
            stall_for_Reg.opcode = NOP;//no operation
            stall_for_Reg.src1 = UNDEFINED;
            stall_for_Reg.src2 = UNDEFINED;
            stall_for_Reg.dest = UNDEFINED;
            stall_for_Reg.immediate = UNDEFINED;
            sp_registers->clean_Register(EXE);//clear the pipe for next step
            sp_registers->set_ir_register(stall_for_Reg, EXE);
            return;    }
        checking_Stall(EXE, idr);
        if (is_in_stall(EXE)) {//check for the previous stall
            stall_put = true;
            instruction_t stall_for_Reg;
            stall_for_Reg.opcode = NOP;//no operation
            stall_for_Reg.src1 = UNDEFINED;
            stall_for_Reg.src2 = UNDEFINED;
            stall_for_Reg.dest = UNDEFINED;
            stall_for_Reg.immediate = UNDEFINED;
            sp_registers->clean_Register(EXE);
            sp_registers->set_ir_register(stall_for_Reg, EXE);
            return;    }
        if (idr.opcode == NOP) {//check for the no operation
            sp_registers->clean_Register(EXE);
            sp_registers->set_ir_register(idr, EXE);
            return;}
        if (idr.opcode == EOP) {// check for the end of the pipe
                sp_registers->clean_Register(EXE);
                sp_registers->set_sp_register(NPC, EXE, sp_registers->get_sp_register(NPC, ID));
                sp_registers->set_ir_register(idr, EXE);
            return;}
        switch (idr.opcode) {
            case ADD:
            break;
            case EOP:
            break;
            case NOP:
            break;
            case SUB:
            break;
            case XOR:
                x = get_int_register(idr.src1);
                y = get_int_register(idr.src2);
                break;
            case ADDI:
            break;
            case SUBI:
                x = get_int_register(idr.src1);
                xyz = idr.immediate;
                break;
            case BEQZ:
            break;
            case BNEZ:
            break;
            case BLTZ:
            break;
            case BGTZ:
            break;
            case BLEZ:
            break;
            case BGEZ:
            break;
            case JUMP:
                x = get_int_register(idr.src1);
                xyz = idr.immediate;
                break;
            case LW:
                x = get_int_register(idr.src1);
                xyz = idr.immediate;
                break;
            case SW:
                x = get_int_register(idr.src2);
                y = get_int_register(idr.src1);
                xyz = idr.immediate;
                break;
            case LWS:
                x = get_int_register(idr.src1);
                xyz = idr.immediate;
                break;
            case SWS:
                y = float2unsigned(get_fp_register(idr.src1));
                x = get_int_register(idr.src2);
                xyz = idr.immediate;
                break;
            case ADDS:
            break;
            case SUBS:
            break;
            case MULTS:
            break;
            case DIVS:
                x = float2unsigned(get_fp_register(idr.src1));
                y = float2unsigned(get_fp_register(idr.src2));
                break;  }
        int next_pc = get_sp_register(NPC, ID);
        sp_registers->clean_Register(EXE);
        sp_registers->set_sp_register(NPC, EXE, next_pc);
        sp_registers->set_sp_register(A, EXE, x);
        sp_registers->set_sp_register(B, EXE, y);
        sp_registers->set_sp_register(IMM, EXE, xyz);
        sp_registers->set_ir_register(idr, EXE);}
//
//
//
//FETCH
//
//
//
    void sim_pipe_fp::inst_fetch() {
        unsigned present_program_Counter = sp_registers->get_sp_register(PC, IF);//update for the next stage with present
        if (get_sp_register(COND, WB) != UNDEFINED) {    present_program_Counter = get_sp_register(ALU_OUTPUT, WB);    }
        unsigned next_program_Counter = present_program_Counter + 4;//check the stage for next
        if ((present_program_Counter - instr_base_address) / 4 > PROGRAM_SIZE || end) {    return;  }
        instruction_t ifr = instr_memory[(present_program_Counter - instr_base_address) / 4];//counter for design
        if (is_in_stall(ID) || is_in_stall(EXE) || is_in_stall(MEM)) {      stall_put = true;
            return;  }
        checking_Stall(ID, ifr);
        if(is_in_stall(EXE) ) {
           if(stall_pipe[EXE].hazard_type==WAW){    stall_put = true;
            return;  }}
        if (is_in_stall(IF)) {
            stall_put = true;
            instruction_t stall_for_Reg;
            stall_for_Reg.opcode = NOP;//update for the execution
            stall_for_Reg.src1 = UNDEFINED;
            stall_for_Reg.src2 = UNDEFINED;
            stall_for_Reg.dest = UNDEFINED;
            stall_for_Reg.immediate = UNDEFINED;
            sp_registers->clean_Register(ID);
            sp_registers->set_ir_register(stall_for_Reg, ID);
            return;
        }
        checking_Stall(IF, ifr);//closing the stall for the design
        if (ifr.opcode == EOP) {
            end = true;
            sp_registers->clean_Register(ID);
            sp_registers->set_sp_register(NPC, ID, present_program_Counter);
            sp_registers->set_ir_register(ifr, ID);
            return;    }
        sp_registers->clean_Register(ID);
        sp_registers->set_sp_register(NPC, ID, next_program_Counter);
        sp_registers->set_sp_register(PC, IF, next_program_Counter);
        sp_registers->set_ir_register(ifr, ID);}

void sim_pipe_fp::reset() {//reset the pipe everytime
    sp_registers = new sp_register_fp();
    sp_registers->reset();
    registerFile = new unsigned[NUM_GP_REGISTERS];
    registerFile_fp = new unsigned[NUM_GP_REGISTERS];

    for (int k = 0; k < NUM_GP_REGISTERS; ++k) {        registerFile[k] = UNDEFINED;    }//get special register for interger
    for (int l = 0; l < NUM_GP_REGISTERS; ++l) {      registerFile_fp[l] = UNDEFINED;  }//get special register for floating point
    for (int m = 0; m < PROGRAM_SIZE; ++m) {
        instr_memory[m].opcode = EOP;//flush the pipe once the design is done
        instr_memory[m].src1 = UNDEFINED;
        instr_memory[m].src2 = UNDEFINED;
        instr_memory[m].dest = UNDEFINED;
        instr_memory[m].immediate = UNDEFINED;
        instr_memory[m].inum = m;    }
    total_clock_cycle = -1;
    total_instr_count = 0;
    stalls = 0;
    for (int c = 0; c < (int)data_memory_size; ++c) {      data_memory[c] = 0xFF;    }
    pipe_run = false;
    end = false;
    for (int a = 0; a < NUM_STAGES; ++a) {        stall_pipe[a].in_stall = false;    }
    memset(delayed, 0, sizeof(int));
    stall_put = false;
    for (int b = 0; b < (int)num_units; ++b) {        exec_units[b].busy = 0;    }
}

unsigned sim_pipe_fp::get_sp_register(sp_register_t reg, stage_t s) {
    return sp_registers->get_sp_register(reg, s); // please modify
}

int sim_pipe_fp::get_int_register(unsigned reg) {
    return registerFile[reg];
}

void sim_pipe_fp::set_int_register(unsigned reg, int value) {
    if (reg < NUM_GP_REGISTERS) {
        registerFile[reg] = value;
    }
}

float sim_pipe_fp::get_fp_register(unsigned reg) {
    if (registerFile_fp[reg] == UNDEFINED) {
        return UNDEFINED;
    }
    return unsigned2float(registerFile_fp[reg]); // please modify
}

void sim_pipe_fp::set_fp_register(unsigned reg, float value) {
    registerFile_fp[reg] = float2unsigned(value);
}


float sim_pipe_fp::get_IPC() {
    return (float) total_instr_count / (float) total_clock_cycle;
}

unsigned sim_pipe_fp::get_instructions_executed() {
    return total_instr_count;
}

unsigned sim_pipe_fp::get_clock_cycles() {
    return total_clock_cycle; // please modify
}

unsigned sim_pipe_fp::get_stalls() {
    return stalls; // please modify
}

unsigned sim_pipe_fp::load_memory(unsigned address) {
    unsigned a;
    memcpy(&a, data_memory + address, sizeof a);
    return a;
}

void sim_pipe_fp::checking_Stall(stage_t sw1, instruction_t ier) {//chossing the stage for stall
    instruction_t ierEXE = sp_registers->get_ir_register(EXE);
    instruction_t ierMEM = sp_registers->get_ir_register(MEM);
    instruction_t ierWB = sp_registers->get_ir_register(WB);
    instruction_t iers[3] = {ierEXE, ierMEM, ierWB};
    switch (sw1) {
        case IF://monitior control hazard as the branch instruction would stall the pipeline for multiple time i,e 2 stall
            if (ier.opcode == BEQZ || ier.opcode == BNEZ || ier.opcode == JUMP) {
                stall_pipe[IF].in_stall = true;
                stall_pipe[IF].hazard_type = CONTROL;
                stall_pipe[IF].reg = ier.src1;    }
            break;
        case ID://wait for the checking of the value of the register to be updated from write back to monitor the read after write hazards
            for (int m = 0; m < 3; ++m) {
                if ((iers[m].dest == ier.src1 || iers[m].dest == ier.src2)
                    && (iers[m].opcode != NOP && iers[m].opcode != EOP && ier.opcode != NOP && ier.opcode != EOP &&
                        iers[m].dest != UNDEFINED)
                    && ((fl_point_alu(ier.opcode) && fl_point_alu(iers[m].opcode)) ||
                        (fl_point_alu(ier.opcode) && iers[m].opcode == LWS))
                    && (!(branch_sel(iers[m].opcode) && ier.opcode == LWS) &&
                        !(branch_sel(ier.opcode) && iers[m].opcode == LWS))) {
                    stall_pipe[ID].in_stall = true;
                    stall_pipe[ID].hazard_type = RAW;
                    stall_pipe[ID].reg = iers[m].dest;
                }
            }
            break;
        case EXE:// check if we are trying the write to memory stage at the same time
            if (is_memory(ier.opcode)) {
                stall_pipe[EXE].in_stall = true;
                stall_pipe[EXE].hazard_type = STRUCTURAL;
                stall_pipe[EXE].reg = UNDEFINED;
            }
            if (delayed[MEM] == 0 ) {//check whether we are the memory stage and creating a WAW hazard
                if(stall_pipe[EXE].hazard_type != WAW){
                    stall_pipe[EXE].in_stall = false;
                }
                stall_pipe[MEM].in_stall = false;
            } else if (stall_pipe[EXE].in_stall && get_free_unit(ier.opcode) != UNDEFINED ){//&& stall_pipe[EXE].hazard_type == STRUCTURAL) {
                stall_pipe[EXE].in_stall = false;
            }
            if (get_free_unit(ier.opcode) == UNDEFINED && ier.opcode != NOP && ier.opcode != EOP) {
                stall_pipe[EXE].in_stall = true;
                stall_pipe[EXE].hazard_type = STRUCTURAL;
                stall_pipe[EXE].reg = UNDEFINED;
            } else {
            }
            break;
        case MEM://waiting for the branch and aly unit to execute and wait along the pipeline
            if (ier.opcode == BEQZ || ier.opcode == BNEZ ||ier.opcode == JUMP) {
                stall_pipe[IF].in_stall = false;
                stall_pipe[IF].reg = UNDEFINED;
            }
            if ((is_memory(ier.opcode)) && (!is_in_stall(MEM))) {
                delayed[MEM] = data_memory_latency;
                stall_pipe[MEM].in_stall = true;
            } else if (is_memory(ier.opcode)) {
                delayed[MEM]--;
            }
            if (delayed[MEM] == 0) {
                stall_pipe[EXE].in_stall = false;
                stall_pipe[MEM].in_stall = false;
            }
            break;
        case WB://carry over the stall and montior the stage for the RAW in the main loop from ID
            for (int j = 0; j < NUM_STAGES; ++j) {
                if (stall_pipe[j].reg == ier.dest && stall_pipe[j].in_stall && stall_pipe[j].hazard_type == RAW) {
                    stall_pipe[j].in_stall = false;
                    stall_pipe[j].reg = UNDEFINED;        }
            }
            for (int b = 0; b < 3; ++b) {
                if ((iers[b].dest == ier.src1 || iers[b].dest == ier.src2)
                    && (iers[b].opcode != NOP && iers[b].opcode != EOP && ier.opcode != NOP && ier.opcode != EOP &&
                        iers[b].dest != UNDEFINED)
                    && ((fl_point_alu(ier.opcode) && fl_point_alu(iers[b].opcode)) ||
                        (fl_point_alu(ier.opcode) && iers[b].opcode == LWS))
                    && (!(branch_sel(iers[b].opcode) && ier.opcode == LWS) &&
                        !(branch_sel(ier.opcode) && iers[b].opcode == LWS))) {
                    stall_pipe[ID].in_stall = true;
                    stall_pipe[ID].hazard_type = RAW;
                    stall_pipe[ID].reg = iers[b].dest;
                }
            }
            break;
    }
}

bool sim_pipe_fp::is_in_stall(stage_t s) {
    return stall_pipe[s].in_stall;
}

void sp_register_fp::set_sp_register(sp_register_t reg, stage_t s, unsigned value) {
    spRegisters[s][reg] = value;
}

unsigned sp_register_fp::get_sp_register(sp_register_t reg, stage_t s) {
    return spRegisters[s][reg];
}

void sp_register_fp::set_ir_register(instruction_t instruction, stage_t s) {
    instr_register[s] = instruction;
}

instruction_t sp_register_fp::get_ir_register(stage_t s) {
    return instr_register[s];
}

instruction_t *sp_register_fp::get_ir_registers() {
    return instr_register;
}

void sp_register_fp::reset() {
    memset(spRegisters, UNDEFINED, sizeof(spRegisters));
    for (int k = 0; k < NUM_STAGES; ++k) {
        instr_register[k].opcode = NOP;
        instr_register[k].src1 = UNDEFINED;
        instr_register[k].src2 = UNDEFINED;
        instr_register[k].dest = UNDEFINED;
        instr_register[k].immediate = UNDEFINED;
    }
}

void sp_register_fp::clean_Register(stage_t s) {
    for (int i = 0; i < NUM_SP_REGISTERS; ++i) {
        spRegisters[s][i] = UNDEFINED;
    }
}
