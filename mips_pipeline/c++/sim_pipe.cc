//code for integer pipeline
//source file
//coder - Chandu Yuvarajappa
//std_id - 200366211

#include "sim_pipe.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>



//code provided by Professor for the execution
using namespace std;
//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ",
                                               "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};

/* converts integer into array of unsigned char - little indian */
inline void int2char(unsigned value, unsigned char *buffer) {
    memcpy(buffer, &value, sizeof value);
}
/* converts array of char into integer - little indian */
inline unsigned char2int(unsigned char *buffer) {
    unsigned d;
    memcpy(&d, buffer, sizeof d);
    return d;
}
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
            return (a + imm);
        case BEQZ:
        case BNEZ:
        case BGTZ:
        case BGEZ:
        case BLTZ:
        case BLEZ:
        case JUMP:
            return (npc + imm);
        default:
            return (-1);
    }
}
void sim_pipe::load_program(const char *filename, unsigned base_address) {
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
        //reading remaining parameters
        char *par1;
        char *par2;
        char *par3;
        switch (instr_memory[instruction_nr].opcode) {
            case ADD:
            case SUB:
            case XOR:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                par3 = strtok(NULL, " \t");
                instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
                instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
                instr_memory[instruction_nr].src2 = atoi(strtok(par3, "R"));
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
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
                instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
                instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
                break;
            case SW:
                par1 = strtok(NULL, " \t");
                par2 = strtok(NULL, " \t");
                instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
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
        instruction_nr++;
    }
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
void sim_pipe::write_memory(unsigned address, unsigned value) {
    int2char(value, data_memory + address);
}
void sim_pipe::print_memory(unsigned start_address, unsigned end_address) {
    cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8)
         << setfill('0') << end_address << "]" << endl;
    for (unsigned i = start_address; i < end_address; i++) {
        if (i % 4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": ";
        cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
        if (i % 4 == 3) cout << endl;
    }
}

/* prints the values of the registers */
void sim_pipe::print_registers() {
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
        if (get_gp_register(i) != (int) UNDEFINED)
            cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}

/* initializes the pipeline simulator */
sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency) {
    data_memory_size = mem_size;
    data_memory_latency = mem_latency;
    data_memory = new unsigned char[data_memory_size];
    reset();
}

/* deallocates the pipeline simulator */
sim_pipe::~sim_pipe() {
    delete[] data_memory;
    delete registerFile;
    delete sp_registers;
}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */
//code provided by prof ends here



// BRANCH to be allocated when moving across the pipe
inline void branch_pick(sp_register *sp_registers) {
    sp_registers->set_sp_register(COND, MEM, 1);
}

/* body of the simulator */
void sim_pipe::run(unsigned cycles) {
    if (cycles == 0) {
        pipe_run = true;
    }
    while (pipe_run || cycles != 0) {//simulation for the pipe
        inst_writeback();
        inst_memory();
        inst_execute();
        inst_decode();
        inst_fetch();
        if (stall_put) {
            stall_put = false;
            stalls++;
        }
        total_clock_cycle = total_clock_cycle + 1;//incr for every execution of the pipe
        if (!pipe_run && cycles > 0) { cycles--; }}//once the cycle of the pipe completes update into new pipe now
}

/*
==
==
==
write in the pull back method to get exit first and use the pull method
==
==
==
*/
void sim_pipe::inst_writeback() {//write back pipe stage of the design
    instruction_t iwr = sp_registers->get_ir_register(WB);
    if (iwr.opcode == NOP) {      return;  }//check for no operation / stall and wait before sending back
    if (iwr.opcode == EOP) {  pipe_run = false;//check for end of operation
        return;  }
    unsigned ari_log_out = sp_registers->get_sp_register(ALU_OUTPUT, WB);//back to alu , as back ward logic looping
    unsigned wlmd = sp_registers->get_sp_register(LMD, WB);//back to memory, as back logic and clearing the pipe
    switch (iwr.opcode) {//check for the code remaining in the pipe
        case LW:
            set_gp_register(iwr.dest, wlmd);
            break;
        case SW:
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
        default:
            set_gp_register(iwr.dest, ari_log_out);
    }
    checking_Stall(WB, iwr);
    total_instr_count++;}//completed execution of the pipe

//
//
//
// load store staage
//
//
//
    void sim_pipe::inst_memory() {
        instruction_t iqr = sp_registers->get_ir_register(MEM);
        checking_Stall(MEM, iqr);
        if (is_in_stall(MEM)) {
            stall_put = true;//check for the LW/SW
            instruction_t stall_for_reg;
            stall_for_reg.opcode = NOP;//stall until current is completed without latency
            stall_for_reg.src1 = UNDEFINED;
            stall_for_reg.src2 = UNDEFINED;
            stall_for_reg.dest = UNDEFINED;
            stall_for_reg.immediate = UNDEFINED;
            sp_registers->clean_registers(WB);//flush the pipe data
            sp_registers->set_ir_register(stall_for_reg, WB);
            return;  }
        if (iqr.opcode == NOP) {//no openation/stall
            sp_registers->clean_registers(WB);
            sp_registers->set_ir_register(iqr, WB);
            return;    }
        if (iqr.opcode == EOP) {//end of operation // clear the pipeline
            sp_registers->clean_registers(WB);
            sp_registers->set_ir_register(iqr, WB);
            return;  }
        unsigned ari_log_out = sp_registers->get_sp_register(ALU_OUTPUT, MEM);//data sent back for the iteration in the back ward method
        unsigned y = sp_registers->get_sp_register(B, MEM);//check the particualr load store operations
        unsigned condition = sp_registers->get_sp_register(COND, MEM);//check for branch in the pipeline coming backwards
        sp_registers->clean_registers(WB);
        sp_registers->set_sp_register(ALU_OUTPUT, WB, ari_log_out);
        sp_registers->set_sp_register(COND, WB, condition);
        switch (iqr.opcode) {//particular load ans store are checked for the operation
          case ADD:
          break;
          case SUB:
          break;
          case ADDI:
          break;
          case SUBI:
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
            case SW:
                write_memory(ari_log_out, y);
                break;
            case LW:
                unsigned qlmd = load_memory(ari_log_out);
                sp_registers->set_sp_register(LMD, WB, qlmd);
                break;  }
        sp_registers->set_ir_register(iqr, WB);}
//
//
//
//EXECUTE STAGE     ALU_STAGE LOGIC Stage
//
//
//
void sim_pipe::inst_execute() {
    instruction_t i_r = sp_registers->get_ir_register(EXE);//setting the value for the stage
    checking_Stall(EXE, i_r);//check for the stall inserted
    if (is_in_stall(EXE)) {return;  }
    if (i_r.opcode == NOP) {//if stall look into the opcode
        sp_registers->clean_registers(MEM);
        sp_registers->set_ir_register(i_r, MEM);
        return;  }
    if (i_r.opcode == EOP) {//check for end of the pipe and then complete the program by flushing the memory
        sp_registers->clean_registers(MEM);
        sp_registers->set_ir_register(i_r, MEM);
        return;  }
    int next_pc = sp_registers->get_sp_register(NPC, EXE);//next program counter is updated to proceed the pipeline
    int x = (int) sp_registers->get_sp_register(A, EXE);//source register 1 is set
    unsigned y = sp_registers->get_sp_register(B, EXE);//source register 2 is set
    unsigned xyz = sp_registers->get_sp_register(IMM, EXE);// check for the computed value in the pipe
    unsigned ari_log_out = alu(i_r.opcode, x, y, xyz, next_pc);//update the final value into the pipeline
    sp_registers->clean_registers(MEM);
    sp_registers->set_sp_register(ALU_OUTPUT, MEM, ari_log_out);
    sp_registers->set_sp_register(B, MEM, y);
    switch (i_r.opcode) {//check for all the ALU conditions
        case ADD:
        case EOP:
        break;
        case SUB:
        case XOR:
            break;
        case ADDI:
        case SUBI:
            break;
        case BEQZ:
            if (x == 0) {      branch_pick(sp_registers);  }//branch have to be procced by picking the next stage
            break;
        case BNEZ:
            if (x != 0) {      branch_pick(sp_registers);  }
            break;
        case BLTZ:
            if (x < 0) {       branch_pick(sp_registers);    }
            break;
        case BGTZ:
            if (x > 0) {       branch_pick(sp_registers);    }
            break;
        case BLEZ:
            if (x <= 0) {      branch_pick(sp_registers);      }
            break;
        case BGEZ:
            if (x >= 0) {      branch_pick(sp_registers);      }
            break;
        case JUMP:
            branch_pick(sp_registers);
            break;
        case LW:
        case SW:
            sp_registers->set_sp_register(B, MEM, y);
            break;
        case NOP:
            break;    }
    sp_registers->set_ir_register(i_r, MEM);}
//
//
//
// DECODE stage
//
//
//



    void sim_pipe::inst_decode() {//getting and updating the register value for the pipeline execution
        instruction_t i_r = sp_registers->get_ir_register(ID);//setting the value for the regi struct
        unsigned x = UNDEFINED;
        unsigned y = UNDEFINED;
        unsigned xyz = UNDEFINED;
        if (is_in_stall(ID)) {
            stall_put = true;
            instruction_t stall_for_reg;
            stall_for_reg.opcode = NOP;
            stall_for_reg.src1 = UNDEFINED;
            stall_for_reg.src2 = UNDEFINED;
            stall_for_reg.dest = UNDEFINED;
            stall_for_reg.immediate = UNDEFINED;
            sp_registers->clean_registers(EXE);
            sp_registers->set_ir_register(stall_for_reg, EXE);
            return;  }
        if (is_in_stall(EXE)) {      return;  }
        if (i_r.opcode == NOP) {//clear the stage of next cycle for execution if the pipe and load the content to move forward with stall
            sp_registers->clean_registers(EXE);
            sp_registers->set_ir_register(i_r, EXE);
            return;    }
        if (i_r.opcode == EOP) {///clear the stage of next cycle for execution if the pipe and load the content to move forward with end of program
            sp_registers->clean_registers(EXE);
            sp_registers->set_sp_register(NPC, EXE, sp_registers->get_sp_register(NPC, ID));
            sp_registers->set_ir_register(i_r, EXE);
            return;  }
        switch (i_r.opcode) {//loading the register with value based on the opcode
            case ADD:
            case SUB:
            case XOR:
                x = get_gp_register(i_r.src1);
                y = get_gp_register(i_r.src2);
                break;
            case ADDI:
            case SUBI:
                x = get_gp_register(i_r.src1);
                xyz = i_r.immediate;
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
                x = get_gp_register(i_r.src1);
                xyz = i_r.immediate;
                break;
            case LW:
                x = get_gp_register(i_r.src1);
                xyz = i_r.immediate;
                break;
            case SW:
                x = get_gp_register(i_r.src2);
                y = get_gp_register(i_r.src1);
                xyz = i_r.immediate;
                break;
              case EOP:
                 break;
              case NOP:
                  break; }
        int next_pc = get_sp_register(NPC, ID);//propogate the value once defined and move to the pipe
        sp_registers->clean_registers(EXE);
        sp_registers->set_sp_register(NPC, EXE, next_pc);
        sp_registers->set_sp_register(A, EXE, x);
        sp_registers->set_sp_register(B, EXE, y);
        sp_registers->set_sp_register(IMM, EXE, xyz);
        sp_registers->set_ir_register(i_r, EXE);}
//
//
//
//FETCH of the instruction
//
//
//


    void sim_pipe::inst_fetch() {//getting the instruction to execute , head of the pipe, but tail in the implementation
        unsigned current_program_Counter = sp_registers->get_sp_register(PC, IF);//check for the current program counter for next updation
        if (get_sp_register(COND, WB) != UNDEFINED) {
            current_program_Counter = get_sp_register(ALU_OUTPUT, WB);    }
        unsigned next_program_Counter = current_program_Counter + 4;//get the next program counter
        if ((current_program_Counter - instr_base_address) / 4 > PROGRAM_SIZE || end) {    return;  }//if no instruction then exit
        instruction_t i_r = instr_memory[(current_program_Counter - instr_base_address) / 4];
        if (is_in_stall(ID) || is_in_stall(EXE)) {//if alreadu in exe or id stage exit the loop
            if (i_r.opcode == BLTZ || i_r.opcode == BNEZ ||  i_r.opcode == BGTZ || i_r.opcode == BEQZ ||  i_r.opcode == BGEZ || i_r.opcode == BLEZ ||  i_r.opcode == JUMP
            ) {  }    return;  }//getting the opcode based on the design pipe
        checking_Stall(ID, i_r);
        if (is_in_stall(IF)) {
            stall_put = true;
            instruction_t stall_for_reg;
            stall_for_reg.opcode = NOP;
            stall_for_reg.src1 = UNDEFINED;
            stall_for_reg.src2 = UNDEFINED;
            stall_for_reg.dest = UNDEFINED;
            stall_for_reg.immediate = UNDEFINED;
            sp_registers->clean_registers(ID);
            sp_registers->set_ir_register(stall_for_reg, ID);
            return;  }
        checking_Stall(IF, i_r);//check if the stall or bubble is given back
        if (i_r.opcode == EOP) {//check for teh clear to complete the pipe or flush the pipe
            end = true;
            sp_registers->clean_registers(ID);
            sp_registers->set_sp_register(NPC, ID, current_program_Counter);
            sp_registers->set_ir_register(i_r, ID);
            return;  }
        sp_registers->clean_registers(ID);
        sp_registers->set_sp_register(NPC, ID, next_program_Counter);
        sp_registers->set_sp_register(PC, IF, next_program_Counter);
        sp_registers->set_ir_register(i_r, ID);}

void sim_pipe::reset() {//clear the data for reset
    sp_registers = new sp_register();
    sp_registers->reset();

    registerFile = new unsigned[NUM_GP_REGISTERS];
    for (int m = 0; m < NUM_GP_REGISTERS; ++m) {    registerFile[m] = UNDEFINED;  }
    for (int k = 0; k < PROGRAM_SIZE; ++k) {
        instr_memory[k].opcode = EOP;
        instr_memory[k].src1 = UNDEFINED;
        instr_memory[k].src2 = UNDEFINED;
        instr_memory[k].dest = UNDEFINED;
        instr_memory[k].immediate = UNDEFINED;  }
    total_clock_cycle = -1;
    total_instr_count = 0;
    stalls = 0;
    for (int n = 0; n < (int)data_memory_size; ++n) {      data_memory[n] = 0xFF;    }//setting the value of clear into the pipe
    pipe_run = false;
    end = false;
    for (int l = 0; l < NUM_STAGES; ++l) {  stall_pipe[l].in_stall = false;  }
    memset(latency, 0, sizeof(int));
    stall_put = false;}


unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s) {
    return sp_registers->get_sp_register(reg, s);}


int sim_pipe::get_gp_register(unsigned reg) {
    return registerFile[reg];}

void sim_pipe::set_gp_register(unsigned reg, int value) {
    if (reg < NUM_GP_REGISTERS) {
        registerFile[reg] = value;  }}

float sim_pipe::get_IPC() {
    return (float) total_instr_count / (float) total_clock_cycle; }//please modify}

unsigned sim_pipe::get_instructions_executed() {
    return total_instr_count;}

unsigned sim_pipe::get_stalls() {
    return stalls;}

unsigned sim_pipe::get_clock_cycles() {
    return total_clock_cycle; }//please modify}

unsigned sim_pipe::load_memory(unsigned address) {
    unsigned m;
    memcpy(&m, data_memory + address, sizeof m);
    return m;}

void sim_pipe::checking_Stall(stage_t sw1, instruction_t i_r) {//count the stall for the pipe before every pipe stage
    instruction_t ierEXE = sp_registers->get_ir_register(EXE);
    instruction_t ierMEM = sp_registers->get_ir_register(MEM);
    instruction_t ierWB = sp_registers->get_ir_register(WB);
    instruction_t iwrs[3] = {ierEXE, ierMEM, ierWB};
    switch (sw1) {
        case IF://monitior control hazard as the branch instruction would stall the pipeline for multiple time i,e 2 stall
            if (i_r.opcode == BEQZ || i_r.opcode == BNEZ ||   i_r.opcode == JUMP) {
                stall_pipe[IF].in_stall = true;
                stall_pipe[IF].hazard_type = CONTROL;
                stall_pipe[IF].reg = i_r.src1;      }
            break;
        case ID://wait for the checking of the value of the register to be updated from write back to monitor the read after write hazards
            for (int i = 0; i < 3; ++i) {
                if ((iwrs[i].dest == i_r.src1 || iwrs[i].dest == i_r.src2) &&  (iwrs[i].opcode != NOP && iwrs[i].opcode != EOP && i_r.opcode != NOP && i_r.opcode != EOP && iwrs[i].dest != UNDEFINED)) {
                    stall_pipe[ID].in_stall = true;
                    stall_pipe[ID].hazard_type = RAW;
                    stall_pipe[ID].reg = iwrs[i].dest;        }    }
            break;
        case EXE:// check if we are trying the write to memory stage at the same time
            if (ierMEM.opcode == LW || ierMEM.opcode == SW) {
                stall_pipe[EXE].in_stall = true;
                stall_pipe[EXE].hazard_type = STRUCTURAL;
                stall_pipe[EXE].reg = UNDEFINED;    }
            if (latency[MEM] == 0) {
                stall_pipe[EXE].in_stall = false;
                stall_pipe[MEM].in_stall = false;      }
            break;
        case MEM://waiting for the branch and aly unit to execute and wait along the pipeline
            if (i_r.opcode == BEQZ || i_r.opcode == BNEZ ||i_r.opcode == JUMP) {
                stall_pipe[IF].in_stall = false;
                stall_pipe[IF].reg = UNDEFINED;      }
            if ((i_r.opcode == LW || i_r.opcode == SW) && (!is_in_stall(EXE))) {
                latency[MEM] = data_memory_latency;
                stall_pipe[MEM].in_stall = true;
            } else if (i_r.opcode == LW || i_r.opcode == SW) {          latency[MEM]--;      }
            if (latency[MEM] == 0) {
                stall_pipe[EXE].in_stall = false;
                stall_pipe[MEM].in_stall = false;        }
            break;
        case WB://carry over the stall and montior the stage for the RAW in the main loop from ID
            for (int k = 0; k < NUM_STAGES; ++k) {
                if (stall_pipe[k].reg == i_r.dest && stall_pipe[k].in_stall && stall_pipe[k].hazard_type != CONTROL) {
                    stall_pipe[k].in_stall = false;
                    stall_pipe[k].reg = UNDEFINED;              }          }
            for (int l = 0; l < 2; ++l) {
                if ((iwrs[l].dest == i_r.dest) && (iwrs[l].dest == i_r.src1 || iwrs[l].dest == i_r.src2) &&  (iwrs[l].opcode != NOP && iwrs[l].opcode != EOP && i_r.opcode != NOP && i_r.opcode != EOP &&  iwrs[l].dest != UNDEFINED)) {
                    stall_pipe[ID].in_stall = true;
                    stall_pipe[ID].hazard_type = RAW;
                    stall_pipe[ID].reg = iwrs[l].dest;          }}
            break;}}

bool sim_pipe::is_in_stall(stage_t sw1) {
    return stall_pipe[sw1].in_stall;
}


void sp_register::set_sp_register(sp_register_t reg, stage_t s, unsigned value) {
    spRegisters[s][reg] = value;
}

unsigned sp_register::get_sp_register(sp_register_t reg, stage_t s) {
    return spRegisters[s][reg];
}

void sp_register::set_ir_register(instruction_t instruction, stage_t s) {
    instr_register[s] = instruction;
}

instruction_t sp_register::get_ir_register(stage_t s) {
    return instr_register[s];
}

instruction_t *sp_register::get_ir_registers() {
    return instr_register;
}

void sp_register::reset() {
    memset(spRegisters, UNDEFINED, sizeof(spRegisters));
    for (int m = 0; m < NUM_STAGES; ++m) {
        instr_register[m].opcode = NOP;
        instr_register[m].src1 = UNDEFINED;
        instr_register[m].src2 = UNDEFINED;
        instr_register[m].dest = UNDEFINED;
        instr_register[m].immediate = UNDEFINED;  }}

void sp_register::clean_registers(stage_t s) {
    for (int i = 0; i < NUM_SP_REGISTERS; ++i) {
        spRegisters[s][i] = UNDEFINED;  }}
