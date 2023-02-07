#include <stdio.h>
#include <stdlib.h>

#include "Viob_plic_top.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

// other macros
#define CLK_PERIOD 10 // 10 ns

#define DATA_W 32
#define SOURCES 8
#define TARGETS 2
#define PRIORITY_BITS 8
#define HAS_THRESHOLD 1
#define HAS_CONFIG_REG 1
//Configuration Bits
#define MAX_SOURCES_BITS   16
#define MAX_TARGETS_BITS   16
#define MAX_PRIORITY_BITS  MAX_SOURCES_BITS
#define HAS_THRESHOLD_BITS 1
//How many CONFIG registers are there (only 1)
#define CONFIG_REGS (HAS_CONFIG_REG == 0 ? 0 : (MAX_SOURCES_BITS + MAX_TARGETS_BITS + MAX_PRIORITY_BITS + HAS_THRESHOLD_BITS +  DATA_W -1) /  DATA_W)
//Amount of Edge/Level registers
#define EDGE_LEVEL_REGS ((SOURCES +  DATA_W -1) /  DATA_W)
//Amount of Interrupt Enable registers
#define IE_REGS (EDGE_LEVEL_REGS * TARGETS)
//Each PRIORITY field starts at a new nibble boundary
//Get the number of nibbles in 'PRIORITY_BITS' ?
#define PRIORITY_NIBBLES ((PRIORITY_BITS +3 -1) / 4)
//How many PRIORITY fields fit in 1 register?
#define PRIORITY_FIELDS_PER_REG (DATA_W / (PRIORITY_NIBBLES*4))
//Amount of Priority registers
#define PRIORITY_REGS ((SOURCES + PRIORITY_FIELDS_PER_REG -1) / PRIORITY_FIELDS_PER_REG)
//Amount of Threshold registers
#define PTHRESHOLD_REGS (HAS_THRESHOLD == 0 ? 0 : TARGETS)
//Amount of ID registers
#define ID_REGS TARGETS
//Total amount of registers
#define TOTAL_REGS (CONFIG_REGS + EDGE_LEVEL_REGS + IE_REGS + PRIORITY_REGS + PTHRESHOLD_REGS + ID_REGS)

vluint64_t main_time = 0;
VerilatedVcdC* tfp = NULL;
Viob_plic_top* dut = NULL;

void write_zero_to_regs();
void test_simple_target_source();

double sc_time_stamp(){
  return main_time;
}

void Timer(unsigned int ns){
  for(int i = 0; i<ns; i++){
    if(!(main_time%(CLK_PERIOD/2))){
      dut->clk = !(dut->clk);
    }
    dut->eval();
#ifdef VCD
    tfp->dump(main_time);
#endif
    main_time += 1;
  }
}

int wait_responce(){
  while(dut->ready != 1){
    Timer(CLK_PERIOD);
  }
  return dut->rdata;
}

int set_inputs(int address, int data, int strb){
  dut->valid = 1;
  dut->address = address;
  dut->wdata = data;
  dut->wstrb = strb;
  Timer(CLK_PERIOD);
  dut->valid = 0;
  return wait_responce();
}

int main(int argc, char **argv, char **env){
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
  dut = new Viob_plic_top;

#ifdef VCD
  tfp = new VerilatedVcdC;

  dut->trace(tfp, 1);
  tfp->open("iob_plic.vcd");
#endif
  main_time = 0;

  dut->clk = 0;
  dut->rst = 0;
  dut->valid = 0;
  dut->address = 0;
  dut->wdata = 0;
  dut->wstrb = 0;
  dut->wstrb = 0;
  dut->srip = 0;

  dut->eval();
#ifdef VCD
  tfp->dump(main_time);
#endif
  printf("\nTestbench started!\n\n");

  // Reset sequence
  Timer(CLK_PERIOD);
  dut->rst = !(dut->rst);
  Timer(CLK_PERIOD);
  dut->rst = !(dut->rst);

  //write_zero_to_regs();
  test_simple_target_source();

  Timer(CLK_PERIOD);

  printf("\nTestbench finished!\n\n");
  dut->final();
#ifdef VCD
  tfp->close();
#endif
  delete dut;
  dut = NULL;
  exit(0);

}

void test_simple_target_source(){
    printf("\nWIP\n");
}

void write_zero_to_regs(){
    int err = 0;
    int i = 0;
    int el_base_address, //Edge/Level registers base address
        pr_base_address, //Priority registers base address
        ie_base_address, //Interrupt Enable registers base address
        th_base_address, //Threshold register base address
        id_base_address; //ID registers base address

    //Set base register addresses ...
    el_base_address = CONFIG_REGS;
    pr_base_address = el_base_address + EDGE_LEVEL_REGS;
    ie_base_address = pr_base_address + PRIORITY_REGS;
    th_base_address = ie_base_address + IE_REGS;
    id_base_address = th_base_address + PTHRESHOLD_REGS;

    //clear all IE
    for (i=0; i < IE_REGS; i++)
        err = set_inputs(ie_base_address+i, 0, 0xF);
    //clear all EL
    for (i=0; i < EDGE_LEVEL_REGS; i++)
        err = set_inputs(el_base_address+i, 0, 0xF);
    //set all threshold to '0'
    for (i=0; i < PTHRESHOLD_REGS; i++)
        err = set_inputs(th_base_address+i, 0, 0xF);
    //set priority for all sources to '1'; '0' means 'never interrupt'
    for (i=0; i < PRIORITY_REGS; i++)
        err = set_inputs(pr_base_address+i, 1, 0xF);

}