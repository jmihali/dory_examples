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
% for layer in list_h_cnn:
#include "${layer}"
% endfor
% for layer in list_h_tcn:
#include "${layer}"
% endfor
#include "pmsis.h"
#include "bsp/fs.h"
#include "bsp/fs/readfs.h"
#include "bsp/flash.h"
#include "bsp/ram.h"
#include "bsp/flash/hyperflash.h"
#include "bsp/ram/hyperram.h"

% if sdk == 'pulp_sdk':
#define ICACHE_CTRL_UNIT 0x10201400
#define ICACHE_PREFETCH ICACHE_CTRL_UNIT + 0x1C
% endif
#define FLASH_BUFF_SIZE 128
% if verbose:
#define VERBOSE 1
#define PROFILE_APPLICATION
//#define COUNT_TOTAL_CYCLES
% endif

% if sdk == 'pulp_sdk':
unsigned int PMU_set_voltage(unsigned int Voltage, unsigned int CheckFrequencies)
{
  return 0;
}
% endif

// allocation of buffers with parameters needed by the network execution
const char * L3_weights_files_cnn[] = {
  ${files_list_cnn}
};
const char * L3_weights_files_tcn[] = {
  ${files_list_tcn}
};
int L3_weights_size_cnn[${weights_number_cnn}];
int L3_weights_size_tcn[${weights_number_tcn}];
// todo: need to duplicate these as well?
static int L3_weights;
static int L3_input;
static int bypass_L3_input;
static int L3_output;
static int bypass_L3_output;
static int activations_input;
static int layers_pointers_cnn[${len(PULP_Nodes_Graph_cnn)}];
static int layers_pointers_tcn[${len(PULP_Nodes_Graph_tcn)}];

static char * Layers_name_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
"${PULP_Nodes_Graph_cnn[i]['name']}"${'' if loop.last else ', '}\
% endfor
};
static char * Layers_name_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
"${PULP_Nodes_Graph_tcn[i]['name']}"${'' if loop.last else ', '}\
% endfor
};
static int L3_layers_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if 'L3' in func_name_cnn[i]:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_layers_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if 'L3' in func_name_tcn[i]:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_input_layers_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['L3_input'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_input_layers_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['L3_input'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_output_layers_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['L3_output'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_output_layers_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['L3_output'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_weights_layers_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['L3_weights'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int L3_weights_layers_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['L3_weights'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int allocate_layer_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['L3_allocation']!=1 and ('Gemm' in PULP_Nodes_Graph_cnn[i]['name'] or 'Conv' in PULP_Nodes_Graph_cnn[i]['name'] or 'MatMul' in PULP_Nodes_Graph_cnn[i]['name']):
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int allocate_layer_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['L3_allocation']!=1 and ('Gemm' in PULP_Nodes_Graph_tcn[i]['name'] or 'Conv' in PULP_Nodes_Graph_tcn[i]['name'] or 'MatMul' in PULP_Nodes_Graph_tcn[i]['name']):
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_input_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['branch_in'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_input_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['branch_in'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_output_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['branch_out'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_output_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['branch_out'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_change_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['branch_change'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_change_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['branch_change'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_last_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['branch_last'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int branch_last_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['branch_last'] == 1:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int check_weights_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${PULP_Nodes_Graph_cnn[i]['check_sum_w']}${'' if loop.last else ', '}\
% endfor
};
static int check_weights_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${PULP_Nodes_Graph_tcn[i]['check_sum_w']}${'' if loop.last else ', '}\
% endfor
};
static int check_weights_dimension_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if i == 0:
${int(PULP_Nodes_Graph_cnn[i]['weights_dimension'])}${'' if loop.last else ', '}\
% else:
${int((PULP_Nodes_Graph_cnn[i]['weights_dimension'] - PULP_Nodes_Graph_cnn[i-1]['weights_dimension']))}${'' if loop.last else ', '}\
% endif
% endfor
};
static int check_weights_dimension_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if i == 0:
${int(PULP_Nodes_Graph_tcn[i]['weights_dimension'])}${'' if loop.last else ', '}\
% else:
${int((PULP_Nodes_Graph_tcn[i]['weights_dimension'] - PULP_Nodes_Graph_tcn[i-1]['weights_dimension']))}${'' if loop.last else ', '}\
% endif
% endfor
};
static int cumulative_weights_dimension_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if i == 0:
0${'' if loop.last else ', '}\
% else:
${int((PULP_Nodes_Graph_cnn[i-1]['weights_dimension_L3']))}${'' if loop.last else ', '}\
% endif
% endfor
};
static int cumulative_weights_dimension_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if i == 0:
0${'' if loop.last else ', '}\
% else:
${int((PULP_Nodes_Graph_tcn[i-1]['weights_dimension_L3']))}${'' if loop.last else ', '}\
% endif
% endfor
};
static int check_activations_cnn[${len(PULP_Nodes_Graph_cnn)}][${len(PULP_Nodes_Graph_cnn[0]['check_sum_in'])}] = {
% for i in range(len(PULP_Nodes_Graph_cnn)):
 {\
 % for j in range(len(PULP_Nodes_Graph_cnn[0]['check_sum_in'])):
${PULP_Nodes_Graph_cnn[i]['check_sum_in'][j]}${'' if loop.last else ', '}\
 % endfor
}${'' if loop.last else ', '}
% endfor
};
static int check_activations_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${PULP_Nodes_Graph_tcn[i]['check_sum_in']}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${int(PULP_Nodes_Graph_cnn[i]['input_activation_dimensions'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${int(PULP_Nodes_Graph_tcn[i]['input_activation_dimensions'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_L3_in_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${int(PULP_Nodes_Graph_cnn[i]['input_activation_dimensions_L3'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_L3_in_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${int(PULP_Nodes_Graph_tcn[i]['input_activation_dimensions_L3'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_L3_out_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${int(PULP_Nodes_Graph_cnn[i]['output_activation_dimensions_L3'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_dimension_L3_out_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${int(PULP_Nodes_Graph_tcn[i]['output_activation_dimensions_L3'])}${'' if loop.last else ', '}\
% endfor
};
static int out_mult_vector_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['outmul'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_cnn[i]['outmul']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int out_mult_vector_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['outmul'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_tcn[i]['outmul']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int out_shift_vector_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['outshift'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_cnn[i]['outshift']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int out_shift_vector_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['outshift'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_tcn[i]['outshift']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int inmul1_vector_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['inmul1'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_cnn[i]['inmul1']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int inmul1_vector_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['inmul1'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_tcn[i]['inmul1']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int inmul2_vector_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if PULP_Nodes_Graph_cnn[i]['inmul2'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_cnn[i]['inmul2']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int inmul2_vector_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if PULP_Nodes_Graph_tcn[i]['inmul2'] == 'empty':
0${'' if loop.last else ', '}\
% else:
${PULP_Nodes_Graph_tcn[i]['inmul2']}${'' if loop.last else ', '}\
% endif
% endfor
};
static int check_activations_cnn[${len(PULP_Nodes_Graph_cnn)}][${len(PULP_Nodes_Graph_cnn[0]['check_sum_out'])}] = {
% for i in range(len(PULP_Nodes_Graph_cnn)):
 {\
 % for j in range(len(PULP_Nodes_Graph_cnn[0]['check_sum_out'])):
${PULP_Nodes_Graph_cnn[i]['check_sum_out'][j]}${'' if loop.last else ', '}\
 % endfor
}${'' if loop.last else ', '}
% endfor
};
static int check_activations_out_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${PULP_Nodes_Graph_tcn[i]['check_sum_out']}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_out_dimension_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${int(PULP_Nodes_Graph_cnn[i]['output_activation_dimensions'])}${'' if loop.last else ', '}\
% endfor
};
static int check_activations_out_dimension_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${int(PULP_Nodes_Graph_tcn[i]['output_activation_dimensions'])}${'' if loop.last else ', '}\
% endfor
};
static int layer_with_weights_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
% if 'Gemm' in PULP_Nodes_Graph_cnn[i]['name'] or 'Conv' in PULP_Nodes_Graph_cnn[i]['name'] or 'MatMul' in PULP_Nodes_Graph_cnn[i]['name']:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
static int layer_with_weights_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
% if 'Gemm' in PULP_Nodes_Graph_tcn[i]['name'] or 'Conv' in PULP_Nodes_Graph_tcn[i]['name'] or 'MatMul' in PULP_Nodes_Graph_tcn[i]['name']:
1${'' if loop.last else ', '}\
% else:
0${'' if loop.last else ', '}\
% endif
% endfor
};
% if 'Yes' in performance:
#ifdef PROFILE_APPLICATION
static int NODEs_MACS_cnn[${len(PULP_Nodes_Graph_cnn)}] = {\
% for i in range(len(PULP_Nodes_Graph_cnn)):
${PULP_Nodes_Graph_cnn[i]['MACs']}${'' if loop.last else ', '}\
% endfor
};
#endif
% endif
% if 'Yes' in performance:
#ifdef PROFILE_APPLICATION
static int NODEs_MACS_tcn[${len(PULP_Nodes_Graph_tcn)}] = {\
% for i in range(len(PULP_Nodes_Graph_tcn)):
${PULP_Nodes_Graph_tcn[i]['MACs']}${'' if loop.last else ', '}\
% endfor
};
#endif
% endif

static uint8_t flashBuffer[FLASH_BUFF_SIZE];

static struct pi_hyperflash_conf flash_conf;
static struct pi_hyper_conf ram_conf;
static struct pi_device ram;

% if verbose_level == 'Check_all+Perf_final':
#ifdef VERBOSE
// check for input/output acitvation checksum
static void check_layer(char *output, int check_sum_true, int dim) {
 int checksum = 0;
 char *ptr = (char *) output;
 for(int j=0; j<dim; j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum in/out Layer :\tOk\n");
 else
   printf("Checksum in/out Layer :\tFailed [%u vs. %u]\n", checksum, check_sum_true);
}

static void check_layer_last(int *output, int check_sum_true, int dim) {
 int checksum = 0;
 int *ptr = (int *) output;
 for(int j=0; j<(int)(dim/4); j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum final :\tOk\n");
 else
   printf("Checksum final :\tFailed [%d vs. %d]\n", checksum, check_sum_true);
}

// check for weight checksum
static void check_layer_weight(char *weight, int check_sum_true, int dim) {
 int checksum = 0;
 char *ptr = (char *) weight;
 for(int j=0; j<dim; j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum weight/bias Layer :\tOk\n");
 else
   printf("Checksum weight/bias Layer :\tFailed [%u vs. %u]\n", checksum, check_sum_true);
}
#endif
% endif

% if 'Last' in verbose_level:
static void check_layer_last(int *output, int check_sum_true, int dim) {
 int checksum = 0;
 int *ptr = (int *) output;
 for(int j=0; j<(int)(dim/4); j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum final :\tOk\n");
 else
   printf("Checksum final :\tFailed [%d vs. %d]\n", checksum, check_sum_true);
}
% endif
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
     printf("Layer %-3d: Checksum = %-12d, FLASH %-12d, Check OK\n", layer_number, check_weights_cnn[layer_number], sum_weights);
   else
     printf("Layer %-3d: Checksum = %-12d, FLASH %-12d, Check FAILED\n", layer_number, check_weights_cnn[layer_number], sum_weights);
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
     printf("Layer %-3d: Checksum = %-12d, FLASH %-12d, Check OK\n", layer_number, check_weights_tcn[layer_number], sum_weights);
   else
     printf("Layer %-3d: Checksum = %-12d, FLASH %-12d, Check FAILED\n", layer_number, check_weights_tcn[layer_number], sum_weights);
   layer_number +=1;
% endif
 }

 // load inputs
 file = pi_fs_open(&fs, "inputs.hex", 0);
 if (file == NULL)
 {
   printf("file open failed\n");
   return -1;
 }
 activations_input = L3_weights+rdDone;
 rdDone = 0;
 int flashBuffSize = FLASH_BUFF_SIZE * sizeof(char);
 // loop on chunk in file
 while(rdDone < (${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions'])} / sizeof(char)))
 {
   // read from HyperFlash
   int size = pi_fs_read(file, flashBuffer, flashBuffSize);
   // write to HyperRam
   pi_ram_write(&ram, activations_input+rdDone, flashBuffer, (uint32_t) size);
   rdDone += size / sizeof(char);
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
 int arg[1];
 arg[0] = (unsigned int) L3_weights_size;
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