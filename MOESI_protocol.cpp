#include "MOESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESI_protocol::MOESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
  // Initialize lines to not have data yet
  this->state = MOESI_CACHE_I;
}

MOESI_protocol::~MOESI_protocol ()
{
}

void MOESI_protocol::dump (void)
{
    const char *block_states[10] = {"X","I","S","E","O","M", "IM", "ISE", "SM", "OM"};
    fprintf (stderr, "MOESI_protocol - state: %s\n", block_states[state]);
}

void MOESI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
    case MOESI_CACHE_I:   do_cache_I (request); break;
    case MOESI_CACHE_S:   do_cache_S (request); break;
    case MOESI_CACHE_E:   do_cache_E (request); break;
    case MOESI_CACHE_O:   do_cache_O (request); break;
    case MOESI_CACHE_M:   do_cache_M (request); break;
    case MOESI_CACHE_IM:  do_cache_IM (request); break;
    case MOESI_CACHE_ISE: do_cache_ISE (request); break;
    case MOESI_CACHE_SM:  do_cache_SM (request); break;
    case MOESI_CACHE_OM:  do_cache_OM (request); break;
    default:
        fatal_error ("Invalid Cache State for MOESI Protocol\n");
    }
}

void MOESI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
    case MOESI_CACHE_I:   do_snoop_I (request); break;
    case MOESI_CACHE_S:   do_snoop_S (request); break;
    case MOESI_CACHE_E:   do_snoop_E (request); break;
    case MOESI_CACHE_O:   do_snoop_O (request); break;
    case MOESI_CACHE_M:   do_snoop_M (request); break;
    case MOESI_CACHE_IM:  do_snoop_IM (request); break;
    case MOESI_CACHE_ISE: do_snoop_ISE (request); break;
    case MOESI_CACHE_SM:  do_snoop_SM (request); break;
    case MOESI_CACHE_OM:  do_snoop_OM (request); break;
    default:
    	fatal_error ("Invalid Cache State for MOESI Protocol\n");
    }
}
//DEBUG: There is an extra outgoing request going to processor....FIXED: E state intervenes on the bus after snooping a GETS/read
//DEBUG: There is an unexpected stall at clock: 130018 for the 8proc. FIXED: do_cache_S needs to send data to processor on local read!
// The 16proc val. is running for too many cycles, too many cache misses & accesses, too many silent upgrades, and too many cachce to cache transfers
//Could it be data being waited on but not being sent? FIXED: do_cache_S needs to send data to processor on local read!
inline void MOESI_protocol::do_cache_I (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
      // (1.1.) Add your code here
      send_GETS(request->addr);
      state = MOESI_CACHE_ISE;
      Sim->cache_misses++;
      break;
    case STORE:
      // (1.2.) Add your code here
      send_GETM(request->addr);
      state = MOESI_CACHE_IM;
      Sim->cache_misses++;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: I state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_S (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
      // (2.1.) Add your code here
      send_DATA_to_proc(request->addr);
      set_shared_line();
      break;
    case STORE:
      // (2.2.) Add your code here
      send_GETM(request->addr);
      state = MOESI_CACHE_SM;
      Sim->cache_misses++;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: S state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_E (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
      // (3.1.) Add your code here
      //send data when local read sent
      send_DATA_to_proc(request->addr);
      break;
    case STORE:
      // (3.2.) Add your code here
      //go to M state when local write sent
      //no getM message used -> silent upgrade
      send_DATA_to_proc(request->addr);
      state = MOESI_CACHE_M;
      Sim->silent_upgrades++;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: S state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_O (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
      // (4.1.) Add your code here
      send_DATA_to_proc(request->addr);
      break;
    case STORE:
      // (4.2.) Add your code here
      //write-back data (dirty data) and send write request
      //write miss
      //send_DATA_on_bus(request->addr, request->src_mid);
      send_GETM(request->addr);
      state = MOESI_CACHE_OM;
      Sim->cache_misses++;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: S state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_M (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
    case STORE:
      // (5.) Add your code here
      send_DATA_to_proc(request->addr);
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: M state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_IM (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
    case STORE:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error("Should only have one outstanding request per processor 1!");
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: IM state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_ISE (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
    case STORE:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error("Should only have one outstanding request per processor 2!");
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: IS state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_SM (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
    case STORE:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error("Should only have one outstanding request per processor 3!");
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: I state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_cache_OM (Mreq *request)
{
  switch (request->msg) {
    case LOAD:
    case STORE:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error("Should only have one outstanding request per processor! 4");
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: I state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_I (Mreq *request)
{
  switch (request->msg) {
    case GETS:
    case GETM:
    case DATA:
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: I state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_S (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (6.1.) Add your code here
      set_shared_line();
      break;
    case GETM:
      // (6.2.) Add your code here
      set_shared_line();
      state = MOESI_CACHE_I;
      break;
    case DATA:
      fatal_error ("Should not see data for this line!  I have the line!");
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: S state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_E (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (7.1.) Add your code here
      //intervene: provide data on bus; downgrade to S state
      send_DATA_on_bus(request->addr, request->src_mid);
      set_shared_line();
      state = MOESI_CACHE_S;
      break;
    case GETM:
      // (7.2.) Add your code here
      //Write back and invalidate
      send_DATA_on_bus(request->addr, request->src_mid);
      set_shared_line();
      state = MOESI_CACHE_I;
      break;
    case DATA:
      fatal_error ("Should not see data for this line!  I have the line!");
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: M state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_O (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (8.1.) Add your code here
      //Intervene with data
      send_DATA_on_bus(request->addr, request->src_mid);
      break;
    case GETM:
      // (8.2.) Add your code here
      //set shared bit to one
      //write back dirty data
      //Invalidate
      set_shared_line();
      send_DATA_on_bus(request->addr, request->src_mid);
      state = MOESI_CACHE_I;
      break;
    case DATA:
      fatal_error ("Should not see data for this line!  I have the line!");
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: S state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_M (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (9.1.) Add your code here
      //line is shared on chip
      //data is dirty so write back to memory
      // change is state O: Shared read access, dirty
      set_shared_line();
      send_DATA_on_bus(request->addr, request->src_mid);
      state = MOESI_CACHE_O;
      break;
    case GETM:
      // (9.2.) Add your code here
      //had this line so set shared bit to 1, Writeback, invalidate
      set_shared_line();
      send_DATA_on_bus(request->addr, request->src_mid);
      state = MOESI_CACHE_I;
      break;
    case DATA:
      fatal_error ("Should not see data for this line!  I have the line!");
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: M state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_IM (Mreq *request)
{
  switch (request->msg) {
    case GETS:
    case GETM:
      break;
    case DATA:
      // (10.) Add your code here
      //supply data and finish at M state
      send_DATA_to_proc(request->addr);
      state = MOESI_CACHE_M;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: I state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_ISE (Mreq *request)
{
  switch (request->msg) {
    case GETS:
    case GETM:
      break;
    case DATA:
      // (11.) Add your code here

      //check if the block is already shared by another cache.
      //If so, transition to S state, instead of E state, provide data, and set shared bit to 1.
      //If not, provide data and finish in E state.
      if(get_shared_line()){
        set_shared_line();
        send_DATA_to_proc(request->addr);
        state = MOESI_CACHE_S;
      }else{
        send_DATA_to_proc(request->addr);
        state = MOESI_CACHE_E;
      }
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: IS state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_SM (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (12.1.) Add your code here
      set_shared_line();
      break;
    case GETM:
      // (12.2.) Add your code here
      set_shared_line();
      break;
    case DATA:
      // (12.3.) Add your code here
      send_DATA_to_proc(request->addr);
      state = MOESI_CACHE_M;
      break;
    default:
    //error coming from here: waiting for data, receiving default? What does that mean?
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: SM state shouldn't see this message\n");
  }
}

inline void MOESI_protocol::do_snoop_OM (Mreq *request)
{
  switch (request->msg) {
    case GETS:
      // (13.1.) Add your code here
      set_shared_line();
      send_DATA_to_proc(request->addr);
      state = MOESI_CACHE_M;
      break;
    case GETM:
      set_shared_line();
      //write back dirty data
      send_DATA_on_bus(request->addr,request->src_mid);
      if (request->src_mid != this->my_table->moduleID) {
        // Someone else sent a GETM request before our GETM request, so we need
        // to invalidate our contents, and indicate that we should finish at M
        state = MOESI_CACHE_IM;
      }
      break;
    case DATA:
      // (13.2.) Add your code here
      //waiting for data, receive data and send it to the processor. Finish in M state.
      send_DATA_to_proc(request->addr);
      state = MOESI_CACHE_M;
      break;
    default:
      request->print_msg (my_table->moduleID, "ERROR");
      fatal_error ("Client: OM state shouldn't see this message\n");
  }
}
