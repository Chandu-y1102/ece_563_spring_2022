//source file for tomasulo Simulator
//Owner -- Chandu Yuvarajappa


#include "sim_ooo.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

using namespace std;

//used for debugging purposes
static const char *stage_names[NUM_STAGES] = {"ISSUE", "EXE", "WR", "COMMIT"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "AND", "MULT", "DIV", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "LWS", "SWS", "ADDS", "SUBS", "MULTS", "DIVS"};
static const char *res_station_names[5]={"Int", "Add", "Mult", "Load"};

/* =============================================================

   HELPER FUNCTIONS (misc)

   ============================================================= */


/* convert a float into an unsigned */
inline unsigned float2unsigned(float value){
	unsigned result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert an unsigned into a float */
inline float unsigned2float(unsigned value){
	float result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert integer into array of unsigned char - little indian */
inline void unsigned2char(unsigned value, unsigned char *buffer){
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = (value >> 24) & 0xFF;
}

/* convert array of char into integer - little indian */
inline unsigned char2unsigned(unsigned char *buffer){
       return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
}

/* the following six functions return the kind of the considered opcdoe */

bool is_branch(opcode_t opcode){
        return (opcode == BEQZ || opcode == BNEZ || opcode == BLTZ || opcode == BLEZ || opcode == BGTZ || opcode == BGEZ || opcode == JUMP);
}

bool is_memory(opcode_t opcode){
        return (opcode == LW || opcode == SW || opcode == LWS || opcode == SWS);
}

bool is_int_r(opcode_t opcode){
        return (opcode == ADD || opcode == SUB || opcode == XOR || opcode == AND);
}

bool is_int_imm(opcode_t opcode){
        return (opcode == ADDI || opcode == SUBI );
}

bool is_int(opcode_t opcode){
        return (is_int_r(opcode) || is_int_imm(opcode));
}

bool is_fp_alu(opcode_t opcode){
        return (opcode == ADDS || opcode == SUBS || opcode == MULTS || opcode == DIVS);
}

unsigned is_fp(opcode_t opcode){
        return (opcode == ADDS || opcode == SUBS || opcode == MULTS || opcode == DIVS || opcode == LWS || opcode == SWS);
}

void sim_ooo::clear_rb(unsigned entry){
        rob.entries[entry].ready=false;
        rob.entries[entry].pc=UNDEFINED;
        rob.entries[entry].state=ISSUE;
        rob.entries[entry].destination=UNDEFINED;
        rob.entries[entry].value=UNDEFINED;
}


void sim_ooo::clear_res_stat(unsigned entry){
        reservation_stations.entries[entry].pc=UNDEFINED;
        reservation_stations.entries[entry].value1=UNDEFINED;
        reservation_stations.entries[entry].value2=UNDEFINED;
        reservation_stations.entries[entry].tag1=UNDEFINED;
        reservation_stations.entries[entry].tag2=UNDEFINED;
        reservation_stations.entries[entry].destination=UNDEFINED;
        reservation_stations.entries[entry].address=UNDEFINED;
        reservation_stations.entries[entry].result=UNDEFINED;
        reservation_stations.entries[entry].write_result=UNDEFINED;
}

/* clears an entry if the instruction window */
void clean_instr_window(instr_window_entry_t *entry){
        entry->pc=UNDEFINED;
        entry->issue=UNDEFINED;
        entry->exe=UNDEFINED;
        entry->wr=UNDEFINED;
        entry->commit=UNDEFINED;
}

/* implements the ALU operation
   NOTE: this function does not cover LOADS and STORES!
*/
unsigned alu(opcode_t opcode, unsigned value1, unsigned value2, unsigned immediate, unsigned pc){
	unsigned result;
	switch(opcode){
			case ADD:
				result = value1+value2;
				break;
			case ADDI:
				result = value1+immediate;
				break;
			case SUB:
				result = value1-value2;
				break;
			case SUBI:
				result = value1-immediate;
				break;
			case XOR:
				result = value1 ^ value2;
				break;
			case AND:
				result = value1 & value2;
				break;
			case MULT:
				result = value1 * value2;
				break;
			case DIV:
				result = value1 / value2;
				break;
			case ADDS:
				result = float2unsigned(unsigned2float(value1) + unsigned2float(value2));
				break;
			case SUBS:
				result = float2unsigned(unsigned2float(value1) - unsigned2float(value2));
				break;
			case MULTS:
				result = float2unsigned(unsigned2float(value1) * unsigned2float(value2));
				break;
			case DIVS:
				result = float2unsigned(unsigned2float(value1) / unsigned2float(value2));
				break;
			case JUMP:
				result = pc + 4 + immediate;
				break;
			default: //branches
				int reg = (int) value1;
				bool condition = ((opcode == BEQZ && reg==0) ||
				(opcode == BNEZ && reg!=0) ||
  				(opcode == BGEZ && reg>=0) ||
  				(opcode == BLEZ && reg<=0) ||
  				(opcode == BGTZ && reg>0) ||
  				(opcode == BLTZ && reg<0));
				if (condition){
	 				result = pc+4+immediate;}
				else {result = pc+4;}
				break;
	}
	return 	result;
}

unsigned sim_ooo::a_l_u_mem(opcode_t opcode, unsigned value1, unsigned value2, unsigned immediate, unsigned pc){
	unsigned result;
	switch(opcode){
		case ADD:
case ADDI:
case SUB:
case SUBI:
case XOR:
case AND:
case BEQZ:
case BNEZ:
case BLTZ:
case BGTZ:
case BLEZ:
case BGEZ:
case JUMP:
case MULT:
case DIV:
case EOP:
case ADDS:
case SUBS:
case MULTS:
case DIVS:
case SW:
case SWS:
			case LW:
			case LWS:
        result = char2unsigned(&(data_memory[immediate]));
	}
	return 	result;
}

/* writes the data memory at the specified address */
void sim_ooo::write_memory(unsigned address, unsigned value){
	unsigned2char(value,data_memory+address);
}

/* =============================================================

   Handling of FUNCTIONAL UNITS

   ============================================================= */

/* initializes an execution unit */
void sim_ooo::init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
        for (unsigned i=0; i<instances; i++){
                exec_units[num_units].type = exec_unit;
                exec_units[num_units].latency = latency;
                exec_units[num_units].busy = 0;
                exec_units[num_units].pc = UNDEFINED;
                num_units++;
        }
}

/* returns a free unit for that particular operation or UNDEFINED if no unit is currently available */
unsigned sim_ooo::get_free_unit(opcode_t opcode){
	if (num_units == 0){
		cout << "ERROR:: simulator does not have any execution units!\n";
		exit(-1);
	}
	for (unsigned u=0; u<num_units; u++){
		switch(opcode){
			//Integer unit
			case ADD:
			case ADDI:
			case SUB:
			case SUBI:
			case XOR:
			case AND:
			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
			case JUMP:
				if (exec_units[u].type==INTEGER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			//memory unit
			case LW:
			case SW:
			case LWS:
			case SWS:
				if (exec_units[u].type==MEMORY && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// FP adder
			case ADDS:
			case SUBS:
				if (exec_units[u].type==ADDER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// Multiplier
			case MULT:
			case MULTS:
				if (exec_units[u].type==MULTIPLIER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// Divider
			case DIV:
			case DIVS:
				if (exec_units[u].type==DIVIDER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			default:
				cout << "ERROR:: operations not requiring exec unit!\n";
				exit(-1);
		}
	}
	return UNDEFINED;
}



/* ============================================================================

   Primitives used to print out the state of each component of the processor:
   	- registers
	- data memory
	- instruction window
        - reservation stations and load buffers
        - (cycle-by-cycle) execution log
	- execution statistics (CPI, # instructions executed, # clock cycles)

   =========================================================================== */


/* prints the content of the data memory */
void sim_ooo::print_memory(unsigned start_address, unsigned end_address){
	cout << "DATA MEMORY[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": ";
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3){
			cout << endl;
		}
	}
}

/* prints the value of the registers */
void sim_ooo::print_registers(){
        unsigned i;
	cout << "GENERAL PURPOSE REGISTERS" << endl;
	cout << setfill(' ') << setw(8) << "Register" << setw(22) << "Value" << setw(5) << "ROB" << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++){
                if (get_int_register_tag(i)!=UNDEFINED)
			cout << setfill(' ') << setw(7) << "R" << dec << i << setw(22) << "-" << setw(5) << get_int_register_tag(i) << endl;
                else if (get_int_register(i)!=(int)UNDEFINED)
			cout << setfill(' ') << setw(7) << "R" << dec << i << setw(11) << get_int_register(i) << hex << "/0x" << setw(8) << setfill('0') << get_int_register(i) << setfill(' ') << setw(5) << "-" << endl;
        }
	for (i=0; i< NUM_GP_REGISTERS; i++){
                if (get_fp_register_tag(i)!=UNDEFINED)
			cout << setfill(' ') << setw(7) << "F" << dec << i << setw(22) << "-" << setw(5) << get_fp_register_tag(i) << endl;
                else if (get_fp_register(i)!=UNDEFINED)
			cout << setfill(' ') << setw(7) << "F" << dec << i << setw(11) << get_fp_register(i) << hex << "/0x" << setw(8) << setfill('0') << float2unsigned(get_fp_register(i)) << setfill(' ') << setw(5) << "-" << endl;
	}
	cout << endl;
}

/* prints the content of the ROB */
void sim_ooo::print_rob(){
	cout << "REORDER BUFFER" << endl;
	cout << setfill(' ') << setw(5) << "Entry" << setw(6) << "Busy" << setw(7) << "Ready" << setw(12) << "PC" << setw(10) << "State" << setw(6) << "Dest" << setw(12) << "Value" << endl;
	for(unsigned i=0; i< rob.num_entries;i++){
		rob_entry_t entry = rob.entries[i];
		instruction_t instruction;
		if (entry.pc != UNDEFINED) instruction = instr_memory[(entry.pc-instr_base_address)>>2];
		cout << setfill(' ');
		cout << setw(5) << i;
		cout << setw(6);
		if (entry.pc==UNDEFINED) cout << "no"; else cout << "yes";
		cout << setw(7);
		if (entry.ready) cout << "yes"; else cout << "no";
		if (entry.pc!= UNDEFINED ) cout << "  0x" << hex << setfill('0') << setw(8) << entry.pc;
		else	cout << setw(12) << "-";
		cout << setfill(' ') << setw(10);
		if (entry.pc==UNDEFINED) cout << "-";
		else cout << stage_names[entry.state];
		if (entry.destination==UNDEFINED) cout << setw(6) << "-";
		else{
			if (instruction.opcode == SW || instruction.opcode == SWS)
				cout << setw(6) << dec << entry.destination;
			else if (entry.destination < NUM_GP_REGISTERS)
				cout << setw(5) << "R" << dec << entry.destination;
			else
				cout << setw(5) << "F" << dec << entry.destination-NUM_GP_REGISTERS;
		}
		if (entry.value!=UNDEFINED) cout << "  0x" << hex << setw(8) << setfill('0') << entry.value << endl;
		else cout << setw(12) << setfill(' ') << "-" << endl;
	}
	cout << endl;
}

/* prints the content of the reservation stations */
void sim_ooo::print_reservation_stations(){
	cout << "RESERVATION STATIONS" << endl;
	cout  << setfill(' ');
	cout << setw(7) << "Name" << setw(6) << "Busy" << setw(12) << "PC" << setw(12) << "Vj" << setw(12) << "Vk" << setw(6) << "Qj" << setw(6) << "Qk" << setw(6) << "Dest" << setw(12) << "Address" << endl;
	for(unsigned i=0; i< reservation_stations.num_entries;i++){
		res_station_entry_t entry = reservation_stations.entries[i];
	 	cout  << setfill(' ');
		cout << setw(6);
		cout << res_station_names[entry.type];
		cout << entry.name + 1;
		cout << setw(6);
		if (entry.pc==UNDEFINED) cout << "no"; else cout << "yes";
		if (entry.pc!= UNDEFINED ) cout << setw(4) << "  0x" << hex << setfill('0') << setw(8) << entry.pc;
		else	cout << setfill(' ') << setw(12) <<  "-";
		if (entry.value1!= UNDEFINED ) cout << "  0x" << setfill('0') << setw(8) << hex << entry.value1;
		else	cout << setfill(' ') << setw(12) << "-";
		if (entry.value2!= UNDEFINED ) cout << "  0x" << setfill('0') << setw(8) << hex << entry.value2;
		else	cout << setfill(' ') << setw(12) << "-";
		cout << setfill(' ');
		cout <<setw(6);
		if (entry.tag1!= UNDEFINED ) cout << dec << entry.tag1;
		else	cout << "-";
		cout <<setw(6);
		if (entry.tag2!= UNDEFINED ) cout << dec << entry.tag2;
		else	cout << "-";
		cout <<setw(6);
		if (entry.destination!= UNDEFINED ) cout << dec << entry.destination;
		else	cout << "-";
		if (entry.address != UNDEFINED ) cout <<setw(4) << "  0x" << setfill('0') << setw(8) << hex << entry.address;
		else	cout << setfill(' ') << setw(12) <<  "-";
		cout << endl;
	}
	cout << endl;
}

/* prints the state of the pending instructions */
void sim_ooo::print_pending_instructions(){
	cout << "PENDING INSTRUCTIONS STATUS" << endl;
	cout << setfill(' ');
	cout << setw(10) << "PC" << setw(7) << "Issue" << setw(7) << "Exe" << setw(7) << "WR" << setw(7) << "Commit";
	cout << endl;
	for(unsigned i=0; i< pending_instructions.num_entries;i++){
		instr_window_entry_t entry = pending_instructions.entries[i];
		if (entry.pc!= UNDEFINED ) cout << "0x" << setfill('0') << setw(8) << hex << entry.pc;
		else	cout << setfill(' ') << setw(10)  << "-";
		cout << setfill(' ');
		cout << setw(7);
		if (entry.issue!= UNDEFINED ) cout << dec << entry.issue;
		else	cout << "-";
		cout << setw(7);
		if (entry.exe!= UNDEFINED ) cout << dec << entry.exe;
		else	cout << "-";
		cout << setw(7);
		if (entry.wr!= UNDEFINED ) cout << dec << entry.wr;
		else	cout << "-";
		cout << setw(7);
		if (entry.commit!= UNDEFINED ) cout << dec << entry.commit;
		else	cout << "-";
		cout << endl;
	}
	cout << endl;
}


/* initializes the execution log */
void sim_ooo::init_log(){
	log << "EXECUTION LOG" << endl;
	log << setfill(' ');
	log << setw(10) << "PC" << setw(7) << "Issue" << setw(7) << "Exe" << setw(7) << "WR" << setw(7) << "Commit";
	log << endl;
}

/* adds an instruction to the log */
void sim_ooo::commit_to_log(instr_window_entry_t entry){
                if (entry.pc!= UNDEFINED ) log << "0x" << setfill('0') << setw(8) << hex << entry.pc;
                else    log << setfill(' ') << setw(10)  << "-";
                log << setfill(' ');
                log << setw(7);
                if (entry.issue!= UNDEFINED ) log << dec << entry.issue;
                else    log << "-";
                log << setw(7);
                if (entry.exe!= UNDEFINED ) log << dec << entry.exe;
                else    log << "-";
                log << setw(7);
                if (entry.wr!= UNDEFINED ) log << dec << entry.wr;
                else    log << "-";
                log << setw(7);
                if (entry.commit!= UNDEFINED ) log << dec << entry.commit;
                else    log << "-";
                log << endl;
}
/* prints the content of the log */
void sim_ooo::print_log(){
	cout << log.str();
}

/* prints the state of the pending instruction, the content of the ROB, the content of the reservation stations and of the registers */
void sim_ooo::print_status(){
	print_pending_instructions();
	print_rob();
	print_reservation_stations();
	print_registers();
}

float sim_ooo::get_IPC(){return (float)instructions_executed/clock_cycles;}
unsigned sim_ooo::get_instructions_executed(){return instructions_executed;}
unsigned sim_ooo::get_clock_cycles(){return clock_cycles;}



/* ============================================================================

   PARSER

   =========================================================================== */


void sim_ooo::load_program(const char *filename, unsigned base_address){
   instr_base_address = base_address;
	  pending_instructions.entries[0].pc = instr_base_address;
   map<string, opcode_t> opcodes;
   map<string, unsigned> labels;
   for (int i=0; i<NUM_OPCODES; i++)
	 opcodes[string(instr_names[i])]=(opcode_t)i;
   ifstream fin(filename, ios::in | ios::binary);
   if (!fin.is_open()) {
      cerr << "error: open file " << filename << " failed!" << endl;
      exit(-1);
   }
   string line;
   unsigned instruction_nr = 0;
   while (getline(fin,line)){
	char *str = const_cast<char*>(line.c_str());
	char *token = strtok (str," \t");
	map<string, opcode_t>::iterator search = opcodes.find(token);
        if (search == opcodes.end()){
		string label = string(token).substr(0, string(token).length() - 1);
		labels[label]=instruction_nr;
		token = strtok (NULL, " \t");
		search = opcodes.find(token);
		if (search == opcodes.end()) cout << "ERROR: invalid opcode: " << token << " !" << endl;
	}
	instr_memory[instruction_nr].opcode = search->second;
	char *par1;
	char *par2;
	char *par3;
	switch(instr_memory[instruction_nr].opcode){
		case ADD:
		case SUB:
		case XOR:
		case AND:
		case MULT:
		case DIV:
		case ADDS:
		case SUBS:
		case MULTS:
		case DIVS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "RF"));
			instr_memory[instruction_nr].src2 = atoi(strtok(par3, "RF"));
			break;
		case ADDI:
		case SUBI:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].immediate = strtoul (par3, NULL, 0);
			break;
		case LW:
		case LWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case SW:
		case SWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
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
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].label = par2;
			break;
		case JUMP:
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].label = par2;
		default:
			break;

	}

	instruction_nr++;
   }
   int i = 0;
   while(true){
   	instruction_t instr = instr_memory[i];
	if (instr.opcode == EOP) break;
	if (instr.opcode == BLTZ || instr.opcode == BNEZ ||
            instr.opcode == BGTZ || instr.opcode == BEQZ ||
            instr.opcode == BGEZ || instr.opcode == BLEZ ||
            instr.opcode == JUMP
	 ){
		instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
	}
        i++;
   }

}

/* ============================================================================

   Simulator creation, initialization and deallocation

   =========================================================================== */

sim_ooo::sim_ooo(unsigned mem_size,
                unsigned rob_size,
                unsigned num_int_res_stations,
                unsigned num_add_res_stations,
                unsigned num_mul_res_stations,
                unsigned num_load_res_stations,
		unsigned max_issue){
	data_memory_size = mem_size;
	data_memory = new unsigned char[data_memory_size];

	issue_width = max_issue;

	rob.num_entries=rob_size;
	pending_instructions.num_entries=rob_size;
	reservation_stations.num_entries= num_int_res_stations+num_load_res_stations+num_add_res_stations+num_mul_res_stations;
	rob.entries = new rob_entry_t[rob_size];
	pending_instructions.entries = new instr_window_entry_t[rob_size];
	reservation_stations.entries = new res_station_entry_t[reservation_stations.num_entries];
	unsigned n=0;
	for (unsigned i=0; i<num_int_res_stations; i++,n++){
		reservation_stations.entries[n].type=INTEGER_RS;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_load_res_stations; i++,n++){
		reservation_stations.entries[n].type=LOAD_B;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_add_res_stations; i++,n++){
		reservation_stations.entries[n].type=ADD_RS;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_mul_res_stations; i++,n++){
		reservation_stations.entries[n].type=MULT_RS;
		reservation_stations.entries[n].name=i;
	}
	num_units = 0;
	reset();
}

sim_ooo::~sim_ooo(){
	delete [] data_memory;
	delete [] rob.entries;
	delete [] pending_instructions.entries;
	delete [] reservation_stations.entries;
}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */


void sim_ooo::run(unsigned cycles){
    if(count>=2){ commit();
     write_result();}
    if(count>=1) execution();
    unsigned i=0;
    while(i<issue_width){
      issue();
      count=count+1;i=i+1;
    }
    if(branch_picked == true){
      count = (branch_target/4);
      rb_wr_idx=0;
      branch_picked = false;
      branch_end_of_program = true;
      unsigned tmp_indx;
      tmp_indx = rb_rd_idx_branch;
			unsigned j =0;
	    while(j< pending_instructions.num_entries){
        if(rob.entries[tmp_indx+j].pc != UNDEFINED){
        commit_to_log(pending_instructions.entries[tmp_indx+j]);
        }else{break;}
        if(tmp_indx+j==5)tmp_indx=tmp_indx-pending_instructions.num_entries;
				j++;
      }
			unsigned f=0;
	    while(f< rob.num_entries){
        clear_rb(f);f++;
      }
			unsigned g=0;
	    while( g< reservation_stations.num_entries){
        clear_res_stat(g);
				g++;
      }
			unsigned u=0;
	    while( u<num_units){
        exec_units[u].busy=0;
        exec_units[u].pc=UNDEFINED;u=u+1;
      }
			unsigned k=0;
      while(k<NUM_GP_REGISTERS){
        gp_fp_tag[k]=UNDEFINED;
        gp_int_tag[k]=UNDEFINED;k=k+1;
      }
    }
    if(clean_rb_idx != UNDEFINED){
    clear_rb(clean_rb_idx);
    clean_rb_idx = UNDEFINED;
    }
		unsigned l=0;
	  while(l< reservation_stations.num_entries){
    if(clear_res_stat_idx[l] == 1){
      clear_res_stat(l);
      clear_res_stat_idx[l] = 0;
    }l=l+1;
    }
    cycles--;
    if((int)cycles == -1 && end_of_program == true){
    cycles++;
    }
   unsigned m=0;
	 while( m< rob.num_entries){
    rob.entries[m].cycles = rob.entries[m].cycles + 1;
    pending_instructions.entries[m].pc=rob.entries[m].pc;
    if(rob.entries[m].pc == UNDEFINED){
    pending_instructions.entries[m].issue = UNDEFINED;
    pending_instructions.entries[m].exe = UNDEFINED;
    pending_instructions.entries[m].wr = UNDEFINED;
    pending_instructions.entries[m].commit = UNDEFINED;
	  }m++;
}
  while(cycles!=0 && end_of_program==false){
	if(count>=2){ commit();
	 write_result();}
	if(count>=1) execution();

  unsigned i=0;
	while(i<issue_width){
		issue();
		count=count+1;i=i+1;
	}
	if(branch_picked == true){
		count = (branch_target/4);
		rb_wr_idx=0;
		branch_picked = false;
		branch_end_of_program = true;
		unsigned tmp_indx;
		tmp_indx = rb_rd_idx_branch;
		unsigned j =0;
		while(j< pending_instructions.num_entries){
			if(rob.entries[tmp_indx+j].pc != UNDEFINED){
			commit_to_log(pending_instructions.entries[tmp_indx+j]);
			}else{break;}
			if(tmp_indx+j==5)tmp_indx=tmp_indx-pending_instructions.num_entries;
			j++;
		}
		unsigned f=0;
		while(f< rob.num_entries){
			clear_rb(f);f++;
		}
		unsigned g=0;
		while( g< reservation_stations.num_entries){
			clear_res_stat(g);
			g++;
		}
		unsigned u=0;
		while( u<num_units){
			exec_units[u].busy=0;
			exec_units[u].pc=UNDEFINED;u=u+1;
		}
		unsigned k=0;
		while(k<NUM_GP_REGISTERS){
			gp_fp_tag[k]=UNDEFINED;
			gp_int_tag[k]=UNDEFINED;k=k+1;
		}
	}
	if(clean_rb_idx != UNDEFINED){
	clear_rb(clean_rb_idx);
	clean_rb_idx = UNDEFINED;
	}
	unsigned l=0;
	while(l< reservation_stations.num_entries){
	if(clear_res_stat_idx[l] == 1){
		clear_res_stat(l);
		clear_res_stat_idx[l] = 0;
	}l=l+1;
	}
	cycles--;
	if((int)cycles == -1 && end_of_program == true){
	cycles++;
	}
 unsigned m=0;
  while( m< rob.num_entries){
	rob.entries[m].cycles = rob.entries[m].cycles + 1;
	pending_instructions.entries[m].pc=rob.entries[m].pc;
	if(rob.entries[m].pc == UNDEFINED){
	pending_instructions.entries[m].issue = UNDEFINED;
	pending_instructions.entries[m].exe = UNDEFINED;
	pending_instructions.entries[m].wr = UNDEFINED;
	pending_instructions.entries[m].commit = UNDEFINED;
}m++;}}}

void sim_ooo::reset(){
	init_log();
	unsigned i=0;
	while ( i<data_memory_size) {data_memory[i]=0xFF;i++;}
  clean_rb_idx = UNDEFINED;
  clean_str_execution = UNDEFINED;
	int j=0;
	while ( j<PROGRAM_SIZE){
		instr_memory[j].opcode=(opcode_t)EOP;
		instr_memory[j].src1=UNDEFINED;
		instr_memory[j].src2=UNDEFINED;
		instr_memory[j].dest=UNDEFINED;
		instr_memory[j].immediate=UNDEFINED;
		j++;
	}
	unsigned k=0;
  while(k<NUM_GP_REGISTERS){
  gp_int[k]= UNDEFINED;
  gp_int_tag[k]= UNDEFINED;
  gp_fp[k]= UNDEFINED;
  gp_fp_tag[k]= UNDEFINED;k=k+1;
  }
	unsigned m=0;
	while(m< pending_instructions.num_entries){
   pending_instructions.entries[m].pc = UNDEFINED;
   pending_instructions.entries[m].issue = UNDEFINED;
   pending_instructions.entries[m].exe = UNDEFINED;
   pending_instructions.entries[m].wr = UNDEFINED;
   pending_instructions.entries[m].commit = UNDEFINED;
	 m++;
  }
	unsigned q=0;
	while( q< rob.num_entries){
   rob.entries[q].pc = UNDEFINED;
   rob.entries[q].destination = UNDEFINED;
   rob.entries[q].value = UNDEFINED;
	 q++;
  }
  unsigned n=0;
	while(n< reservation_stations.num_entries){
	 reservation_stations.entries[n].pc = UNDEFINED;
	 reservation_stations.entries[n].value1 = UNDEFINED;
	 reservation_stations.entries[n].value2 = UNDEFINED;
	 reservation_stations.entries[n].tag1 = UNDEFINED;
	 reservation_stations.entries[n].tag2 = UNDEFINED;
	 reservation_stations.entries[n].destination = UNDEFINED;
	 reservation_stations.entries[n].address = UNDEFINED;
	 reservation_stations.entries[n].result = UNDEFINED;
	 reservation_stations.entries[n].write_result = UNDEFINED;
	 n++;
  }
	clock_cycles = 0;
	instructions_executed = 0;
}
int sim_ooo::get_int_register(unsigned reg){
	return gp_int[reg];
}
void sim_ooo::set_int_register(unsigned reg, int value){
  gp_int[reg] = value;
}
float sim_ooo::get_fp_register(unsigned reg){
	return gp_fp[reg];
}
void sim_ooo::set_fp_register(unsigned reg, float value){
  gp_fp[reg] = value;
}
unsigned sim_ooo::get_int_register_tag(unsigned reg){
	return gp_int_tag[reg] ;
}
unsigned sim_ooo::get_fp_register_tag(unsigned reg){
	return gp_fp_tag[reg] ;
}

unsigned sim_ooo::rb_pick(unsigned src, unsigned fp){
  unsigned tmp_r_indx;
	unsigned i=0;
  tmp_r_indx = rb_wr_idx;
  while( i< rob.num_entries){
    if(rob.entries[tmp_r_indx - i].destination-fp == src){
      return tmp_r_indx-i;
      break;
    }
    if((tmp_r_indx - i)==0)tmp_r_indx = tmp_r_indx+rob.num_entries;
i++;  }
return tmp_r_indx;
}
unsigned sim_ooo::get_free_res_stat(opcode_t opcode){
	unsigned i=0;
	while(i< reservation_stations.num_entries){
		switch(opcode){
			case ADD:
			case ADDI:
			case SUB:
			case SUBI:
			case XOR:
			case AND:
			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
			case JUMP:
				if (reservation_stations.entries[i].type==INTEGER_RS && reservation_stations.entries[i].pc==UNDEFINED) return i;
				break;
			case LW:
			case LWS:
			case SW:
			case SWS:
				if (reservation_stations.entries[i].type==LOAD_B && reservation_stations.entries[i].pc==UNDEFINED) return i;
				break;
			case ADDS:
			case SUBS:
				if (reservation_stations.entries[i].type==ADD_RS && reservation_stations.entries[i].pc==UNDEFINED) return i;
				break;
			case MULT:
			case MULTS:
			case DIV:
			case DIVS:
				if (reservation_stations.entries[i].type==MULT_RS && reservation_stations.entries[i].pc==UNDEFINED) return i;
				break;
			case EOP:
        eop = true;
	      return UNDEFINED;
			default:
				exit(-1);
		}i=i+1;
  }
	return UNDEFINED;
}
void sim_ooo::init_rb(opcode_t opcode){
  rob.entries[rb_wr_idx].ready = 0;
  rob.entries[rb_wr_idx].pc = count*(16);
	rob.entries[rb_wr_idx].pc=rob.entries[rb_wr_idx].pc/4;
  pending_instructions.entries[rb_wr_idx].pc=rob.entries[rb_wr_idx].pc;
  rob.entries[rb_wr_idx].state = ISSUE;
  pending_instructions.entries[rb_wr_idx].issue=rob.entries[rb_wr_idx].cycles;

  if(is_fp(instr_memory[count].opcode)){
    if(instr_memory[count].opcode==SWS){
    }else{
     rob.entries[rb_wr_idx].destination = instr_memory[count].dest + 8+16+4+2+1+1;
     gp_fp_tag[instr_memory[count].dest] = rb_wr_idx;
    }
  }else{
  if(is_branch(instr_memory[count].opcode)){
  }
  else{
   rob.entries[rb_wr_idx].destination = instr_memory[count].dest;
   gp_int_tag[instr_memory[count].dest] = rb_wr_idx;
  }
  }
  rb_wr_idx = (rb_wr_idx + 2-1 ) % rob.num_entries;
}
void sim_ooo::issue(){
  add_stall();
  if (stall[RS] != 1 && stall[ROB] != 1)
  {
    init_res_stat(instr_memory[count].opcode);
    init_rb(instr_memory[count].opcode);
   }else{
    count=count-1;
   }
    tag_updt();
    branch_end_of_program = false;
}

void sim_ooo::add_stall(){
   free_res_stat = get_free_res_stat(instr_memory[count].opcode);
	  if ( free_res_stat != UNDEFINED){
      stall[RS] = 0;
    }
    else{
      stall[RS] = 1;
    }

unsigned j=0;
	while( j< rob.num_entries){
	  if ( rob.entries[j].pc != UNDEFINED){
      stall[ROB] = 1;
    }
    else{
      stall[ROB] = 0;
			break;
    }j=j+1;
  }
}

void sim_ooo::init_res_stat(opcode_t opcode){
    reservation_stations.entries[free_res_stat].pc = count*4;
    reservation_stations.entries[free_res_stat].destination = rb_wr_idx;
		unsigned i=0;
		unsigned j=0;
		unsigned u=0;
		unsigned k=0;
		unsigned m=0;
		unsigned e=0;
		switch(opcode){
			case ADD:
			case SUB:
			case XOR:
			case AND:
				reservation_stations.entries[free_res_stat].value1 = get_int_register(instr_memory[count].src1);
				reservation_stations.entries[free_res_stat].value2 = get_int_register(instr_memory[count].src2);
	     while( i< rob.num_entries){
        if (rob.entries[i].destination == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[i].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[i].value;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = i;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if (rob.entries[i].destination == instr_memory[count].src2){
            waw[1]=waw[1]+1;
          if(rob.entries[i].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[i].value;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = i;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }i++;}
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if(waw[1] >=2){
          waw[1] = instr_memory[count].src2;
          wax_idx=rb_pick(waw[1],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag2 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = wax_idx;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }
          waw[0]=0;
          waw[1]=0;
       break;
			case ADDI:
			case SUBI:
				reservation_stations.entries[free_res_stat].value1 = get_int_register(instr_memory[count].src1);
	     while( j< rob.num_entries){
        if (rob.entries[j].destination == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[j].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[j].value;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = j;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }j++;}
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
          waw[0]=0;
				break;
			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
			case JUMP:
				reservation_stations.entries[free_res_stat].value1 = get_int_register(instr_memory[count].src1);
	     while( u< rob.num_entries){
        if (rob.entries[u].destination == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[u].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[u].value;
          }else{
          reservation_stations.entries[free_res_stat].tag1 = u;
				reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
			}}u=u+1;}
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
          waw[0]=0;
				break;
			case LW:
			case LWS:
				reservation_stations.entries[free_res_stat].value1 = get_int_register(instr_memory[count].src1);
	     while( k< rob.num_entries){
        if (rob.entries[k].destination == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[k].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[k].value;
          }else{
          reservation_stations.entries[free_res_stat].tag1 = k;
				reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
			}}k=k+1;}
        reservation_stations.entries[free_res_stat].address =  instr_memory[count].immediate;
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
          waw[0]=0;
				break;
			case SW:
			case SWS:
				reservation_stations.entries[free_res_stat].value1 = float2unsigned(get_fp_register(instr_memory[count].src1));
				reservation_stations.entries[free_res_stat].value2 = get_int_register(instr_memory[count].src2);
	     while(m< rob.num_entries){
        if (rob.entries[m].destination-(8*4) == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[m].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[m].value;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = m;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if (rob.entries[m].destination == instr_memory[count].src2){
            waw[1]=waw[1]+1;
          if(rob.entries[m].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[m].value;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = m;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }m=m+1;}
        reservation_stations.entries[free_res_stat].address =  instr_memory[count].immediate;
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],32);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if(waw[1] >=2){
          waw[1] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[1],0);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag2 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = wax_idx;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }
          waw[0]=0;
          waw[1]=0;
				break;
			case ADDS:
			case SUBS:
			case MULT:
			case MULTS:
			case EOP:
			case DIV:
			case DIVS:
				reservation_stations.entries[free_res_stat].value1 = float2unsigned(get_fp_register(instr_memory[count].src1));
				reservation_stations.entries[free_res_stat].value2 = float2unsigned(get_fp_register(instr_memory[count].src2));
	     while(e< rob.num_entries){
        if (rob.entries[e].destination-32 == instr_memory[count].src1){
            waw[0]=waw[0]+1;
          if(rob.entries[e].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[e].value;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = e;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if (rob.entries[e].destination-32 == instr_memory[count].src2){
            waw[1]=waw[1]+1;
          if(rob.entries[e].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[e].value;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = e;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }e=e+1;}
        if(waw[0] >=2){
          waw[0] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[0],32);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value1 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag1 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag1 = wax_idx;
				    reservation_stations.entries[free_res_stat].value1 = UNDEFINED;
          }
        }
        if(waw[1] >=2){
          waw[1] = instr_memory[count].src1;
          wax_idx=rb_pick(waw[1],32);
          if(rob.entries[wax_idx].ready != 0){
				    reservation_stations.entries[free_res_stat].value2 = rob.entries[wax_idx].value;
            reservation_stations.entries[free_res_stat].tag2 = UNDEFINED;
          }else{
            reservation_stations.entries[free_res_stat].tag2 = wax_idx;
				    reservation_stations.entries[free_res_stat].value2 = UNDEFINED;
          }
        }
          waw[0]=0;
          waw[1]=0;
				break;
 }

}
void sim_ooo::tag_updt(){
	unsigned j=0;
  while( j< reservation_stations.num_entries){
    if(reservation_stations.entries[j].tag1 != UNDEFINED){
      if(rob.entries[reservation_stations.entries[j].tag1].ready != 0){
        reservation_stations.entries[j].value1 = rob.entries[reservation_stations.entries[j].tag1].value;
        reservation_stations.entries[j].tag1 = UNDEFINED;
      }
    }
    if(reservation_stations.entries[j].tag2 != UNDEFINED){
      if(rob.entries[reservation_stations.entries[j].tag2].ready != 0){
        reservation_stations.entries[j].value2 = rob.entries[reservation_stations.entries[j].tag2].value;
        reservation_stations.entries[j].tag2 = UNDEFINED;
      }
    }
j=j+1;  }
}


void sim_ooo::execution(){
  unsigned index;
	unsigned i = 0;
	while( i< rob.num_entries){
	  for(unsigned j=0; j< reservation_stations.num_entries;j++){
      if(rob.entries[i].pc == reservation_stations.entries[j].pc){
       if(rob.entries[i].state ==ISSUE && rob.entries[i].pc != UNDEFINED){
          if(reservation_stations.entries[j].tag1 == UNDEFINED && reservation_stations.entries[j].tag2 == UNDEFINED){
            index = rob.entries[i].pc/(1+3);
            if((instr_memory[index].opcode == LWS  && str_bypass_write[j] !=0 )||( instr_memory[index].opcode == LW && str_bypass_write[j] !=0)){
              rob.entries[i].state = EXECUTE;
              pending_instructions.entries[i].exe = rob.entries[i].cycles;
              reservation_stations.entries[j].value2 = str_bypass_value[j];
              rob.entries[i].result = str_bypass_value[j];
              reservation_stations.entries[j].write_result = 1;
							str_bypass_value[j] = 0;
              str_bypass_write[j] = 0;
              reservation_stations.entries[j].address = reservation_stations.entries[j].value1 + reservation_stations.entries[j].address;
             }
             if (instr_memory[index].opcode == SWS){
              rob.entries[i].state = EXECUTE;
              pending_instructions.entries[i].exe = rob.entries[i].cycles;
             }else{
           free_unit = get_free_unit(instr_memory[rob.entries[i].pc/4].opcode);
             if (free_unit != UNDEFINED){
                if (instr_memory[rob.entries[i].pc/4].opcode == LWS || instr_memory[rob.entries[i].pc/4].opcode == LW){
                   if( rob.entries[i].state != EXECUTE ){
                  load_proceed = load_store_bp(i,j);
                  if(load_proceed != 0 ){
                    exec_units[free_unit].busy = exec_units[free_unit].latency;
                    exec_units[free_unit].opcode = instr_memory[index].opcode;
                    exec_units[free_unit].imm = instr_memory[index].immediate;
                    exec_units[free_unit].pc = rob.entries[i].pc;
                    rob.entries[i].state = EXECUTE;
                    pending_instructions.entries[i].exe = rob.entries[i].cycles;
                    load_proceed=0;
                  }
             if( str_bypass_write_ready[j] != 0 ){
              rob.entries[i].state = EXECUTE;
							reservation_stations.entries[j].value2 = str_bypass_value[j];
              pending_instructions.entries[i].exe = rob.entries[i].cycles;
              rob.entries[i].result = str_bypass_value[j];
              reservation_stations.entries[j].write_result = 1;
							str_bypass_value[j] = 0;
              str_bypass_write_ready[j] = 0;
              reservation_stations.entries[j].address = reservation_stations.entries[j].value1 + reservation_stations.entries[j].address;
             }
                   }
                }else{
                  exec_units[free_unit].busy = exec_units[free_unit].latency;
                  exec_units[free_unit].opcode = instr_memory[index].opcode;
                  exec_units[free_unit].imm = instr_memory[index].immediate;
                  exec_units[free_unit].pc = rob.entries[i].pc;
                  rob.entries[i].state = EXECUTE;
                  pending_instructions.entries[i].exe = rob.entries[i].cycles;
                }
              }

          if ((instr_memory[rob.entries[i].pc/4].opcode == LWS  && free_unit == UNDEFINED && rob.entries[i].state != EXECUTE)|| (instr_memory[rob.entries[i].pc/4].opcode == LW  && free_unit == UNDEFINED && rob.entries[i].state != EXECUTE)){
            load_proceed = load_store_bp(i,j);
            if(store_com_bypass != false){
              rob.entries[i].state = EXECUTE;
							reservation_stations.entries[j].value2 = str_bypass_value[j];
              pending_instructions.entries[i].exe = rob.entries[i].cycles;
              rob.entries[i].result = str_bypass_value[j];
              reservation_stations.entries[j].write_result = 1;
              reservation_stations.entries[j].address = reservation_stations.entries[j].value1 + reservation_stations.entries[j].address;
							str_bypass_value[j] = 0;
              store_com_bypass = false;
            }
            if( str_bypass_write_ready[j] != 0 ){
              rob.entries[i].state = EXECUTE;
							reservation_stations.entries[j].value2 = str_bypass_value[j];
              pending_instructions.entries[i].exe = rob.entries[i].cycles;
              rob.entries[i].result = str_bypass_value[j];
              reservation_stations.entries[j].write_result = 1;
							str_bypass_value[j] = 0;
              str_bypass_write_ready[j] = 0;
              reservation_stations.entries[j].address = reservation_stations.entries[j].value1 + reservation_stations.entries[j].address;
             }
          }
             }
          }
       }
        if(rob.entries[i].state == EXECUTE){
          if (instr_memory[rob.entries[i].pc/4].opcode == SWS){
          reservation_stations.entries[j].address = reservation_stations.entries[j].value2 + reservation_stations.entries[j].address;
          rob.entries[i].destination = reservation_stations.entries[j].address;
          reservation_stations.entries[j].write_result = 1;
          }else{
				 unsigned u=0;
	        while( u<num_units){
            if (exec_units[u].pc == rob.entries[i].pc){
		          if (exec_units[u].busy != 0) exec_units[u].busy --;
                if((exec_units[u].opcode == LWS  && exec_units[u].busy == exec_units[u].latency-1 )|| (exec_units[u].opcode == LW && exec_units[u].busy == exec_units[u].latency-1 )){
                  reservation_stations.entries[j].address = reservation_stations.entries[j].value1 + reservation_stations.entries[j].address;
                }
		          if (exec_units[u].busy == 0){
                if(exec_units[u].opcode == LWS || exec_units[u].opcode == LW){
                  rob.entries[i].result = a_l_u_mem(exec_units[u].opcode,reservation_stations.entries[j].value1,reservation_stations.entries[j].value2,reservation_stations.entries[j].address,reservation_stations.entries[j].pc);
                  reservation_stations.entries[j].write_result = 1;
                }else{
                    rob.entries[i].result = alu(exec_units[u].opcode,reservation_stations.entries[j].value1,reservation_stations.entries[j].value2,exec_units[u].imm,reservation_stations.entries[j].pc);
                  reservation_stations.entries[j].write_result = 1;
                }            }          }u=u+1;          }      }      }    }  }i=i+1;  }
unsigned u=0;
	while ( u<num_units){
    if(exec_units[u].clean_exec_program_counter != false){
      exec_units[u].pc = UNDEFINED;
      exec_units[u].clean_exec_program_counter=false;
    }u=u+1;}}


void sim_ooo::commit(){
  if(clean_str_execution != UNDEFINED){
    exec_units[clean_str_execution].pc = UNDEFINED;
    clean_str_execution = UNDEFINED;
  }
unsigned u =0;
  if (rob.entries[rb_rd_idx].ready != 0){
   if(instr_memory[(rob.entries[rb_rd_idx].pc)/4].opcode ==  SWS){
     if(rob.entries[rb_rd_idx].state == COMMIT){
	      while ( u<num_units){
          if (exec_units[u].pc == rob.entries[rb_rd_idx].pc){
		        if (exec_units[u].busy !=0) exec_units[u].busy --;
		          if (exec_units[u].busy == 0){
                write_memory(rob.entries[rb_rd_idx].destination,rob.entries[rb_rd_idx].value);
                clean_rb_idx = rb_rd_idx;
                rb_rd_idx = (rb_rd_idx + (2/2) ) % rob.num_entries;
                instructions_executed ++;
                clean_str_execution = u;
              }
            }u=u+1;
        }
     }else{
     free_unit = get_free_unit(instr_memory[rob.entries[rb_rd_idx].pc/4].opcode);
      if (free_unit != UNDEFINED){
exec_units[free_unit].imm = instr_memory[rob.entries[rb_rd_idx].pc/4].immediate;
        exec_units[free_unit].busy = exec_units[free_unit].latency;
        exec_units[free_unit].opcode = instr_memory[rob.entries[rb_rd_idx].pc/4].opcode;
        exec_units[free_unit].pc = rob.entries[rb_rd_idx].pc;
				exec_units[free_unit].busy =exec_units[free_unit].busy -1;
        rob.entries[rb_rd_idx].state = COMMIT;
        pending_instructions.entries[rb_rd_idx].commit = rob.entries[rb_rd_idx].cycles;
        commit_to_log(pending_instructions.entries[rb_rd_idx]);
      }
     }
   }

   else if(is_branch(instr_memory[(rob.entries[rb_rd_idx].pc)/4].opcode)){
      if(rob.entries[rb_rd_idx].value == rob.entries[rb_rd_idx].pc + 2*2 ){
        clean_rb_idx = rb_rd_idx;
        pending_instructions.entries[rb_rd_idx].commit = rob.entries[rb_rd_idx].cycles;
        commit_to_log(pending_instructions.entries[rb_rd_idx]);
        rb_rd_idx = (rb_rd_idx + 16-15 ) % rob.num_entries;
  instructions_executed ++;
      }else{
        pending_instructions.entries[rb_rd_idx].commit = rob.entries[rb_rd_idx].cycles;
        branch_target = rob.entries[rb_rd_idx].value;
        branch_picked = true;
        rb_rd_idx_branch = rb_rd_idx ;
        rb_rd_idx = 0;
  instructions_executed ++;
      }
    }else if(is_fp(instr_memory[(rob.entries[rb_rd_idx].pc)/4].opcode)){
       set_fp_register(rob.entries[rb_rd_idx].destination-8-24, unsigned2float(rob.entries[rb_rd_idx].value));
     if(gp_fp_tag[rob.entries[rb_rd_idx].destination-32] == rb_rd_idx){
       gp_fp_tag[rob.entries[rb_rd_idx].destination-32] = UNDEFINED;
     }
    clean_rb_idx = rb_rd_idx;
    pending_instructions.entries[rb_rd_idx].commit = rob.entries[rb_rd_idx].cycles;
        commit_to_log(pending_instructions.entries[rb_rd_idx]);
    rb_rd_idx = (rb_rd_idx + 4-3 ) % rob.num_entries;
  instructions_executed ++;
    }else{
       set_int_register(rob.entries[rb_rd_idx].destination, rob.entries[rb_rd_idx].value);
     if(gp_int_tag[rob.entries[rb_rd_idx].destination] == rb_rd_idx){
       gp_int_tag[rob.entries[rb_rd_idx].destination] = UNDEFINED;
     }
    clean_rb_idx = rb_rd_idx;
    pending_instructions.entries[rb_rd_idx].commit = rob.entries[rb_rd_idx].cycles;
        commit_to_log(pending_instructions.entries[rb_rd_idx]);
    rb_rd_idx = (rb_rd_idx + (5/5) ) % rob.num_entries;
  instructions_executed ++;
    }
  }

	unsigned i =0;
  while( i< rob.num_entries){
	      for (unsigned u=0; u<num_units; u++){
          if (exec_units[u].pc == rob.entries[i].pc && rob.entries[i].pc != UNDEFINED){
						unsigned j=0;
	          while( j< reservation_stations.num_entries){
              if(exec_units[u].pc == reservation_stations.entries[j].pc){
              break;
              }
              if (j == reservation_stations.num_entries-1){
                 if(exec_units[u].busy == 0 && clean_str_execution ==UNDEFINED )exec_units[u].pc = UNDEFINED;
              }
          j++ ; }
          }
        }i++;
      }
    unsigned q=0;
	  while( q< rob.num_entries){
      if (rob.entries[q].pc == UNDEFINED && eop == true && branch_end_of_program != true){
        end_of_program = true;
        clock_cycles = rob.entries[q].cycles;
      }
      else{
        end_of_program = false;
      break;
      }
  q=q+1;  }
}
void sim_ooo::write_result(){
	unsigned i =0;
	unsigned u =0;
	unsigned j =0;
	while( i< rob.num_entries){
    if (rob.entries[i].state == WRITE_RESULT && instr_memory[rob.entries[i].pc/4].opcode == SWS){
			str_bypass_rdy[i] = 1;}
		i=i+1;
  }
  while( j< reservation_stations.num_entries){
    if(reservation_stations.entries[j].write_result == 1){
	    for(unsigned i=0; i< rob.num_entries;i++){
        if(rob.entries[i].pc == reservation_stations.entries[j].pc ){
          if(instr_memory[rob.entries[i].pc/4].opcode == SWS){
          rob.entries[i].value = reservation_stations.entries[j].value1;
          }else{
          rob.entries[i].value = rob.entries[i].result;
          }
          rob.entries[i].state = WRITE_RESULT;
          pending_instructions.entries[i].wr = rob.entries[i].cycles;
          rob.entries[i].ready = 1;
	        while( u<num_units){
            if (exec_units[u].pc == rob.entries[i].pc && rob.entries[i].pc != UNDEFINED  ){
              exec_units[u].clean_exec_program_counter = 1;
            }u=u+1;
          }
        }
      }
      clear_res_stat_idx[j] = 1;
    }j=j+1;
  }
}



unsigned sim_ooo::load_store_bp(unsigned re_or_buf_idx,unsigned res_stat_idx){
  bool load_pd;
	unsigned k=0;
  while( k< rob.num_entries){
  if(rob.entries[re_or_buf_idx-k].pc == UNDEFINED){
    break;
  }

  if (instr_memory[rob.entries[re_or_buf_idx-k].pc/4].opcode == SWS){
    if (rob.entries[re_or_buf_idx-k].state == EXECUTE){
		unsigned l=0;
	    while( l< reservation_stations.num_entries){
        if(rob.entries[re_or_buf_idx-k].pc == reservation_stations.entries[l].pc){
          if (reservation_stations.entries[l].address == reservation_stations.entries[res_stat_idx].value1 + reservation_stations.entries[res_stat_idx].address){
            load_pd = false;
            break;
          }else{
            load_pd = true;
          }
        }
    l++;  }
        if(load_pd == false)break;
      }

      if (rob.entries[re_or_buf_idx-k].state == ISSUE){
				unsigned q=0;
	      while( q< reservation_stations.num_entries){
          if(rob.entries[re_or_buf_idx-k].pc == reservation_stations.entries[q].pc){
            if(reservation_stations.entries[q].tag2 == UNDEFINED){
            if (reservation_stations.entries[q].address + reservation_stations.entries[q].value2 == reservation_stations.entries[res_stat_idx].value1 + reservation_stations.entries[res_stat_idx].address){
              load_pd = false;
              break;
            }else{
              load_pd = true;
            }
          }else{
           load_pd = false;
           break;
          }
          }
      q=q+1;  }
        if(load_pd == false)break;

      }
      if ((rob.entries[re_or_buf_idx-k].state == COMMIT) && rob.entries[re_or_buf_idx-k].destination == reservation_stations.entries[res_stat_idx].value1 + reservation_stations.entries[res_stat_idx].address ){
       store_com_bypass = true;
       str_bypass_value[res_stat_idx] = rob.entries[re_or_buf_idx-k].value;
       load_pd = false;
       break;
      }
      if(str_bypass_rdy[re_or_buf_idx-k] == 1){
      if ((rob.entries[re_or_buf_idx-k].state == WRITE_RESULT ) && (rob.entries[re_or_buf_idx-k].destination == reservation_stations.entries[res_stat_idx].value1 + reservation_stations.entries[res_stat_idx].address) ){
       str_bypass_write_ready[res_stat_idx] = 1;
       str_bypass_value[res_stat_idx] = rob.entries[re_or_buf_idx-k].value;
       load_pd = false;
       break;
      }}
      if ((rob.entries[re_or_buf_idx-k].state == WRITE_RESULT ) && rob.entries[re_or_buf_idx-k].destination == reservation_stations.entries[res_stat_idx].value1 + reservation_stations.entries[res_stat_idx].address ){
       str_bypass_write[res_stat_idx] = 1;
       str_bypass_value[res_stat_idx] = rob.entries[re_or_buf_idx-k].value;
       load_pd = false;
       break;
      }
    }else{
      load_pd = true;
    }
    if((re_or_buf_idx-k) == rb_rd_idx) break;
    if((re_or_buf_idx - k)==0)re_or_buf_idx = re_or_buf_idx+rob.num_entries;
  k++;}
  return load_pd;

}
