/*
 * network.c
 * Alessio Burrello <alessio.burrello@unibo.it>
 * Thorir Mar Ingolfsson <thoriri@iis.ee.ethz.ch>
 *
 * Copyright (C) 2019-2020 University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mem_controller.h"
#include "network.h"
% if sdk == 'gap_sdk':
#include "pulp.h"
% endif
#include "dory.h"
#include "pmsis.h"
#include "bsp/fs.h"
#include "bsp/fs/readfs.h"
#include "bsp/flash.h"
#include "bsp/ram.h"
#include "bsp/flash/hyperflash.h"
#include "bsp/ram/hyperram.h"
#include "utils.h"
#include "variables.h"
#include "network_cnn.h"
#include "network_tcn.h"

% if sdk == 'pulp_sdk':
unsigned int PMU_set_voltage(unsigned int Voltage, unsigned int CheckFrequencies)
{
  return 0;
}
% endif

static int L3_weights;
static int L3_input;
static int bypass_L3_input;
static int L3_output;
static int bypass_L3_output;
static int activations_input;

static uint8_t flashBuffer[FLASH_BUFF_SIZE];

static struct pi_hyperflash_conf flash_conf;
static struct pi_hyper_conf ram_conf;
static struct pi_device ram;

// filesystem management functions
void open_filesystem(struct pi_device *flash, struct pi_device *fs)
{
   struct pi_readfs_conf conf;
   struct pi_hyperflash_conf flash_conf;

   /* Init & open flash. */
   pi_hyperflash_conf_init(&flash_conf);
   pi_open_from_conf(flash, &flash_conf);
   if (pi_flash_open(flash))
   {
       printf("Error flash open !\n");
       pmsis_exit(-1);
   }

   /* Open filesystem on flash. */
   pi_readfs_conf_init(&conf);
   conf.fs.flash = flash;
   pi_open_from_conf(fs, &conf);
   if (pi_fs_mount(fs))
   {
       printf("Error FS mounting !\n");
       pmsis_exit(-2);
   }
}

/* Moves the weights and the biases from hyperflash to hyperram */
int network_setup()
{
 pi_task_t task = {0};
 pi_task_block(&task);
 struct pi_device fs;
 struct pi_device flash;
 pi_hyperram_conf_init(&ram_conf);
 open_filesystem(&flash, &fs);
 pi_open_from_conf(&ram, &ram_conf);
 pi_ram_open(&ram);
 pi_fs_file_t *file;
 pi_ram_alloc(&ram, &L3_weights, (uint32_t) 4800000);
 pi_ram_alloc(&ram, &L3_input, (uint32_t) 1000000);
 pi_ram_alloc(&ram, &L3_output, (uint32_t) 1000000);
#ifdef VERBOSE
   printf("\nL3 Buffer alloc initial\t@ %d:\t%s\n", (unsigned int)L3_weights, L3_weights?"Ok":"Failed");
   printf("\nL3 Buffer alloc initial\t@ %d:\t%s\n", (unsigned int)L3_input, L3_input?"Ok":"Failed");
   printf("\nL3 Buffer alloc initial\t@ %d:\t%s\n", (unsigned int)L3_output, L3_output?"Ok":"Failed");
#endif
 unsigned int rdDone = 0;
% if 'Check_all' in verbose_level:
 int layer_number = 0;
 int sum_weights;
% endif
 // load weights of CNN net
 for (int i=0;i<${weights_number_cnn};i++)
 {
% if 'Check_all' in verbose_level:
   if (layer_with_weights_cnn[layer_number]==0)
     layer_number +=1;
% endif
   file = pi_fs_open(&fs, L3_weights_files_cnn[i], 0);
   if (file == NULL)
   {
     printf("file open failed\n");
     return -1;
   }
   L3_weights_size_cnn[i] = file->size + rdDone;
   int flashBuffSize = FLASH_BUFF_SIZE * sizeof(char);
% if 'Check_all' in verbose_level:
   sum_weights = 0;
% endif
   while(rdDone < (L3_weights_size_cnn[i] / sizeof(char)))
   {
     int size = pi_fs_read(file, flashBuffer, flashBuffSize);
% if 'Check_all' in verbose_level:
     for (int t = 0; t < size; t++)
       sum_weights+=flashBuffer[t];
% endif
     pi_ram_write(&ram, L3_weights+rdDone, flashBuffer,size);
     rdDone += size / sizeof(char);
   }
% if 'Check_all' in verbose_level:
   if (check_weights_cnn[layer_number] == sum_weights)
     printf("CNN Layer %-3d: Checksum = %-12d, FLASH %-12d, Check OK\n", layer_number, check_weights_cnn[layer_number], sum_weights);
   else
     printf("CNN Layer %-3d: Checksum = %-12d, FLASH %-12d, Check FAILED\n", layer_number, check_weights_cnn[layer_number], sum_weights);
   layer_number +=1;
% endif
 }

 layer_number = 0;
 // load weights of TCN net
 for (int i=0;i<${weights_number_tcn};i++)
 {
% if 'Check_all' in verbose_level:
   if (layer_with_weights_tcn[layer_number]==0)
     layer_number +=1;
% endif
   file = pi_fs_open(&fs, L3_weights_files_tcn[i], 0);
   if (file == NULL)
   {
     printf("file open failed\n");
     return -1;
   }
   L3_weights_size_tcn[i] = file->size + rdDone;
   int flashBuffSize = FLASH_BUFF_SIZE * sizeof(char);
% if 'Check_all' in verbose_level:
   sum_weights = 0;
% endif
   while(rdDone < (L3_weights_size_tcn[i] / sizeof(char)))
   {
     int size = pi_fs_read(file, flashBuffer, flashBuffSize);
% if 'Check_all' in verbose_level:
     for (int t = 0; t < size; t++)
       sum_weights+=flashBuffer[t];
% endif
     pi_ram_write(&ram, L3_weights+rdDone, flashBuffer,size);
     rdDone += size / sizeof(char);
   }
% if 'Check_all' in verbose_level:
   if (check_weights_tcn[layer_number] == sum_weights)
     printf("TCN Layer %-3d: Checksum = %-12d, FLASH %-12d, Check OK\n", layer_number, check_weights_tcn[layer_number], sum_weights);
   else
     printf("TCN Layer %-3d: Checksum = %-12d, FLASH %-12d, Check FAILED\n", layer_number, check_weights_tcn[layer_number], sum_weights);
   layer_number +=1;
% endif
 }
 // load CNN inputs
 activations_input = L3_weights+rdDone;
 for (int t=0; t<${test_inputs_cnn}; t++)
 {
  file = pi_fs_open(&fs, input_files_cnn[t], 0);
  if (file == NULL)
  {
    printf("file open failed\n");
    return -1;
  }
  int flashBuffSize = FLASH_BUFF_SIZE * sizeof(char);
  rdDone = 0;
  // loop on chunk in file
  while(rdDone < (${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions'])} / sizeof(char)))
  {
    // read from HyperFlash
    int size = pi_fs_read(file, flashBuffer, flashBuffSize);
    // write to HyperRam
    pi_ram_write(&ram, activations_input+t*${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions'])}+rdDone, flashBuffer, (uint32_t) size);
    rdDone += size / sizeof(char);
  }
 }
 return 1;
}

// parallelization of the function given the number of cores
void pulp_parallel(void *arg)
{
 pi_cl_team_fork(NUM_CORES, (void *)cluster_main, arg);
}

void network_run_FabricController()
{
 int arg[2];
 arg[0] = (unsigned int) L3_weights_size_cnn;
 arg[1] = (unsigned int) L3_weights_size_tcn;
 PMU_set_voltage(1000, 0);
 pi_time_wait_us(10000);
 pi_freq_set(PI_FREQ_DOMAIN_FC, ${fc_frequency});
 pi_time_wait_us(10000);
 pi_freq_set(PI_FREQ_DOMAIN_CL, ${cl_frequency});
 pi_time_wait_us(10000);

% if sdk == 'pulp_sdk':
 #if __PLATFORM__ == ARCHI_PLATFORM_FPGA
   *(int*)(ICACHE_PREFETCH) = 0xFFFF;
 #endif
% endif
 struct pi_device cluster_dev = {0};
 struct pi_cluster_conf conf;
 struct pi_cluster_task cluster_task = {0};
 // task parameters allocation
 pi_cluster_task(&cluster_task, pulp_parallel, arg);
 cluster_task.stack_size = ${master_stack};
 cluster_task.slave_stack_size = ${slave_stack};
 // First open the cluster
 pi_cluster_conf_init(&conf);
 conf.id=0;
 pi_open_from_conf(&cluster_dev, &conf);
 if (pi_cluster_open(&cluster_dev))
   return -1;
 // Then offload an entry point, this will get executed on the cluster controller
 pi_cluster_send_task_to_cl(&cluster_dev, &cluster_task);
 // closing of the cluster
 pi_cluster_close(&cluster_dev);
}

int memId;
char* L2_output;
char* L2_output_window;
char* L2_input;
char* L2_input_window_cnn;
char* L2_input_tcn;
char* L2_weights_1;
char* L2_weights_2;
char* L2_buffer_allocation;
char* L2_buffer_tofree;
int L2_buffer_allocation_end;
${type} *l1_buffer;
uint8_t * bypass_activations;
uint8_t * activation_to_keep;
char *exec_weights, *transfer_weights, *bypass_weights;
int L3_weights_internal;

void network_run(unsigned int L3_weights_size_cnn, unsigned int L3_weights_size_tcn)
{
  unsigned int args[9] = {
    L3_weights,
    L3_input,
    L3_output,
    activations_input,
    L2_input,
    L2_buffer_allocation,
    L2_buffer_allocation_end,
    L2_buffer_tofree,
    l1_buffer
  };
  network_run_cnn(args, &ram);
  args[0] = L3_weights + cumulative_weights_dimension_cnn[9]; // set pointer to the TCN weights
  args[4] = args[5] - check_activations_dimension_tcn[0]; // set pointer to the TCN inputs
  network_run_tcn(args, &ram);
}

// on cluster function execution
void cluster_main(void *arg)
{
#ifdef COUNT_TOTAL_CYCLES
 // perf measurement begin
 if (pi_core_id()==0){
   pi_perf_conf(1<<PI_PERF_CYCLES);
   pi_perf_reset();
   pi_perf_stop();
   pi_perf_start();
 }
#endif
 int *real_arg = (int *) arg;
 network_run((unsigned int) real_arg[0], (unsigned int) real_arg[1]);
#ifdef COUNT_TOTAL_CYCLES
 if (pi_core_id()==0){
   // performance measurements: end
   pi_perf_stop();
   int perf_cyc = pi_perf_read(PI_PERF_CYCLES);
   printf("Total cycles for network_run()=%d\n", perf_cyc);
 }
#endif
}

