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
static int check_activations_out_cnn[${len(PULP_Nodes_Graph_cnn)}][${len(PULP_Nodes_Graph_cnn[0]['check_sum_out'])}] = {
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
 while(rdDone < (${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions']*len(PULP_Nodes_Graph_cnn[0]['check_sum_in']))} / sizeof(char)))
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
char* L2_input;
char* L2_input_window;
char* L2_weights_1;
char* L2_weights_2;
char* L2_buffer_allocation;
char* L2_buffer_tofree_copy;
int L2_buffer_allocation_end;
${type} *l1_buffer;
uint8_t * bypass_activations;
uint8_t * activation_to_keep;
char *exec_weights, *transfer_weights, *bypass_weights;
int L3_weights_internal;

void network_run(unsigned int L3_weights_size_cnn, unsigned int L3_weights_size_tcn)
{
/*
 - initial buffer allocation L2 and L1
 - variable declaration
*/
/* ---------------------------------- */
/* -------- SECTION 0 BEGIN --------- */
/* ---------------------------------- */
#ifdef PROFILE_APPLICATION
 int perf_cyc;
 int cycle_network_execution = 0;
 int cycles_alloc_l1_l2_buffer = 0;
 int cycles_alloc_weights_input_output = 0;
 int cycles_copy_weights_next_layer = 0;
 int cycles_input_check = 0;
 int cycles_output_check = 0;
 int cycles_dealloc_alloc_weights = 0;
#endif
 uint16_t out_mult = 0;
 uint16_t out_shift = 0;
 uint16_t inmul1 = 0;
 uint16_t inmul2 = 0;
 int branch_active = 0;
 int branch_keep_active = 0;
 int counter = 0;
 int counter_keep = 0;
 int valid = 0;
 static int keeping = 0;
 static int activation_to_keep_delloced = 0;
 int branch_output_index = 0;
 static int keep_index = 0;
 bypass_activations = 0;
 activation_to_keep = 0;
 int bypass_dimension = 0;
 int bypass_to_dealloc = 0;
 int activation_dimension = 0;
 int d_buffering_weights_t = 0;
 int error_presence = 0;
 int bypass_side = 0;
 int bypass_used_as_out = 0;
 int input_used_as_out = 0;
 int valid_keep = 0;
 int bypass_side_keep = 0;
 int d_buffering_weights_e = 0;
 int d_buffering_inputs = 0;
 int d_buffering_outputs = 0;
 int begin_end_n = 1;
 int residual_number = 0;
 pi_cl_ram_alloc_req_t alloc_req_L3;
 pi_cl_ram_req_t buff_req1;
 L3_weights_internal = L3_weights;
 transfer_weights = d_buffering_weights_t ? L2_weights_2 : L2_weights_1;
 exec_weights = d_buffering_weights_e ? L2_weights_2 : L2_weights_1;
 bypass_weights = d_buffering_weights_e ? L2_weights_2 : L2_weights_1;
 pi_cl_alloc_req_t alloc_req = {0};
 pi_cl_free_req_t free_req = {0};

 if (pi_core_id()==0)
 {
#ifdef PROFILE_APPLICATION
   pi_perf_stop();
   pi_perf_reset();
   pi_perf_conf(1<<PI_PERF_CYCLES);
   pi_perf_start();
#endif
   pi_cl_l2_malloc((uint32_t) ${l2_buffer_size}, &alloc_req);
   L2_buffer_allocation = pi_cl_l2_malloc_wait(&alloc_req);
   L2_buffer_tofree_copy = L2_buffer_allocation;
   L2_buffer_allocation_end = L2_buffer_allocation + ${l2_buffer_size};
   l1_buffer = pmsis_l1_malloc((uint32_t) ${l1_buffer});
#ifdef VERBOSE
   printf("\nL2 Buffer alloc initial\t@ 0x%08x:\t%s\n", (unsigned int)L2_buffer_allocation, L2_buffer_allocation?"Ok":"Failed");
   printf("L1 Buffer alloc initial\t@ 0x%08x:\t%s\n\n", (unsigned int)l1_buffer, l1_buffer?"Ok":"Failed");
#endif
#ifdef PROFILE_APPLICATION
   pi_perf_stop();
   perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
   cycles_alloc_l1_l2_buffer += perf_cyc;
   //printf("[%d] L2&L1 Buffer allocation finished - num_cycles: %d\n", pi_core_id(), perf_cyc);
#endif
 }
/* ---------------------------------- */
/* --------- SECTION 0 END ---------- */
/* ---------------------------------- */
/*
 - initial copies from L3 of input
 - copies of weights of first 2 layers of the CNN
*/
/* ---------------------------------- */
/* -------- SECTION 1 BEGIN --------- */
/* ---------------------------------- */

 if(pi_core_id()==0)
 {
#ifdef PROFILE_APPLICATION
   pi_perf_stop();
   pi_perf_reset();
   pi_perf_conf(1<<PI_PERF_CYCLES);
   pi_perf_start();
#endif
/*
 - CNN first layer weights allocation and copy
*/
   dory_L2_alloc(&L2_buffer_allocation,
     &L2_buffer_allocation_end,
     &L2_weights_1,
     ${int(PULP_Nodes_Graph_cnn[0]['weights_dimension'])},
     begin_end_n // begin is 1, end is 0
     );
/*
 - input allocation and copy
*/
% if test:
   dory_L2_alloc(&L2_buffer_allocation,
     &L2_buffer_allocation_end,
     &L2_input,
     ${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions']*len(PULP_Nodes_Graph_cnn[0]['check_sum_in']))},
     begin_end_n // begin is 1, end is 0
     );
   pi_cl_ram_read(&ram, activations_input, L2_input, ${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions'])}, &buff_req1);
   pi_cl_ram_read_wait(&buff_req1);
% else:
   dory_L2_alloc(&L2_buffer_allocation,
     &L2_buffer_allocation_end,
     &L2_input,
     ${int(PULP_Nodes_Graph_cnn[0]['input_activation_dimensions']*len(PULP_Nodes_Graph_cnn[0]['check_sum_in']))},
     begin_end_n // begin is 1, end is 0
     );
% endif
   begin_end_n = !begin_end_n;
   transfer_weights = L2_weights_1;
   exec_weights = L2_weights_1;
   pi_cl_ram_read(&ram, L3_weights_internal, transfer_weights, ${int(PULP_Nodes_Graph_cnn[0]['weights_dimension'])}, &buff_req1);
   pi_cl_ram_read_wait(&buff_req1);

% if 'Gemm' in PULP_Nodes_Graph_cnn[1]['name'] or 'Conv' in PULP_Nodes_Graph_cnn[1]['name']:
/*
 - CNN second layer weights allocation
*/
   d_buffering_weights_t = !d_buffering_weights_t;
   dory_L2_alloc(&L2_buffer_allocation,
     &L2_buffer_allocation_end,
     &L2_weights_2,
     ${int(PULP_Nodes_Graph_cnn[1]['weights_dimension'])}- ${int(PULP_Nodes_Graph_cnn[0]['weights_dimension'])},
     begin_end_n // begin is 1, end is 0
     );
   transfer_weights = d_buffering_weights_t ? L2_weights_2 : L2_weights_1;
% endif
/*
 - output of the first CNN layer allocation
*/
   dory_L2_alloc(&L2_buffer_allocation,
     &L2_buffer_allocation_end,
     &L2_output,
     ${int(PULP_Nodes_Graph_cnn[0]['output_activation_dimensions'])},
     begin_end_n // begin is 1, end is 0
     );
   if(L2_output == NULL) return -1;
   begin_end_n = !begin_end_n;

#ifdef PROFILE_APPLICATION
   pi_perf_stop();
   perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
   cycles_alloc_weights_input_output += perf_cyc;
   //printf("[%d] Allocation for 1st&2nd layer weights, input and output of first layer finished - num_cycles: %d\n", pi_core_id(), perf_cyc);
#endif
 }

/* ---------------------------------- */
/* --------- SECTION 1 END ---------- */
/* ---------------------------------- */

/* MAIN SECTION
 - for loop over all windows that are fed to the CNN network
 - for loop over all the layers of the CNN network
 - double buffering using L3
 - check on layers to be executed from L3
 - residual check at the end of each layer
*/
/* ---------------------------------- */
/* -------- SECTION 2 BEGIN --------- */
/* ---------------------------------- */

 for(int t = 0; t < ${len(PULP_Nodes_Graph_cnn[0]['check_sum_in'])}; t++)
 {
  printf("=====FEEDING WINDOW %d THROUGH THE CNN...=====\n", t);
  L2_input_window = L2_input + t*check_activations_dimension_cnn[0];
  for(int i = 0; i < ${len(PULP_Nodes_Graph_cnn)}; i++)
  {
    if(pi_core_id()==0)
    {
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
  #endif
      // copy of weights of next layers:
      // 1. copy only if we have to allocate the weights (hence not weights tiled from L3 and not pooling/add layer)
      // 2. waits before the read if we want to implement a double buffering, after if not.
      // Waiting based on the fact if layer need or not transfers from L3 memory.
      if(i < ${len(PULP_Nodes_Graph_cnn)-1})
      {
        if (allocate_layer_cnn[i+1] == 1)
        {
          if (L3_layers_cnn[i-1] == 0 && i > 0)
            pi_cl_ram_read_wait(&buff_req1);
          pi_cl_ram_read(&ram, L3_weights_internal + cumulative_weights_dimension_cnn[i+1], transfer_weights, check_weights_dimension_cnn[i+1], &buff_req1);
          if (L3_layers_cnn[i] == 1)
            pi_cl_ram_read_wait(&buff_req1);
        }
      }
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_copy_weights_next_layer += perf_cyc;
      //printf("[%d] Copying of weights of next layer finished - num_cycles: %d\n", pi_core_id(), perf_cyc);
  #endif
    }


  % if verbose_level == 'Check_all+Perf_final':
  #ifdef VERBOSE
    if(pi_core_id()==0)
    {
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
  #endif
      if (L3_input_layers_cnn[i]==1)
        printf("In in L3\n");
      else if (i==0) {
        printf("Checking input of layer %d...\n", i);
        check_layer(L2_input_window, check_activations_cnn[i][t], check_activations_dimension_cnn[i]);
      }
      else if (branch_change_cnn[i-1]==0) {
        printf("Checking input of layer %d...\n", i);
        check_layer(L2_input_window, check_activations_cnn[i][t], check_activations_dimension_cnn[i]);
      }
      else
        printf("Switching branch, already checked activation\n");
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_input_check += perf_cyc;
      //printf("[%d] Checking input of layer %d finished - num_cycles: %d\n", pi_core_id(), i, perf_cyc);
  #endif
    }
  #endif
  % endif

    out_mult = out_mult_vector_cnn[i];
    out_shift = out_shift_vector_cnn[i];
    inmul1 = inmul1_vector_cnn[i];
    inmul2 = inmul2_vector_cnn[i];
    pi_cl_team_barrier(0);
    unsigned int args[13] = {L3_input,
      L3_output,
      L3_weights_internal + cumulative_weights_dimension_cnn[i],
      L2_input_window,
      bypass_activations,
      L2_output,
      exec_weights,
      l1_buffer,
      &ram,
      out_mult,
      inmul1,
      inmul2,
      out_shift};
  % if 'Yes' in performance or 'Perf_final' in verbose_level:
  #ifdef PROFILE_APPLICATION
    if (pi_core_id() == 0){
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
    }
  #endif
  % endif
    switch (i)
    {
  % for i in range(len(PULP_Nodes_Graph_cnn)):
      case ${i}:
        ${func_name_cnn[i]}(args);
        break;
  % endfor
    }
    pi_cl_team_barrier(0);
  % if 'Yes' in performance or 'Perf_final' in verbose_level:
  #ifdef PROFILE_APPLICATION
    if (pi_core_id() == 0){
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycle_network_execution += perf_cyc;
    }
  #endif
  % endif
  % if 'Yes' in performance:
  #ifdef PROFILE_APPLICATION
    int MACs = NODEs_MACS_cnn[i];
    float perf_MAC =  (float)MACs/perf_cyc;
    if (pi_core_id() == 0)
    {
      printf("[%d] Layer %-3d: num_cycles: %-11d,",pi_core_id(), i, perf_cyc);
      printf(" MACs: %-11d,", MACs);
      printf(" MAC/cycle: %-8f,", perf_MAC);
      printf(" n. of Cores: %d\n", NUM_CORES);
    }
  #endif
  % endif

    // prevents error from compiler
    if (pi_core_id()==0)
    {
      asm volatile("": : :"memory");
      unsigned int temp = L3_input;
      L3_input = L3_output;
      asm volatile("": : :"memory");
      L3_output = temp;
      asm volatile("": : :"memory");
    }

  % if verbose_level == 'Check_all+Perf_final':
  #ifdef VERBOSE
    if(pi_core_id()==0)
    {
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
  #endif
      printf("Layer %s %d ended: \n", Layers_name_cnn[i], i);
      if (i < ${len(PULP_Nodes_Graph_cnn) - 1})
      {
        if (L3_output_layers_cnn[i]==1)
          printf("Out in L3\n");
        else {
          printf("Checking output of layer %d...\n", i);
          check_layer(L2_output, check_activations_out_cnn[i][t], check_activations_out_dimension_cnn[i]);
        }
      }
      else
      {
        check_layer_last((int32_t *) L2_output, check_activations_out_cnn[i][t], check_activations_out_dimension_cnn[i]);
      }
      if (i==${check_layer_cnn})
      {
        check_layer_plus(L2_output, check_activations_out_dimension_cnn[i]);
      }
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_output_check += perf_cyc;
      //printf("[%d] Checking output of layer %d finished - num_cycles: %d\n", pi_core_id(), i, perf_cyc);
  #endif
    }
  #endif
  % elif verbose_level == 'Last+Perf_final':
    if(pi_core_id()==0)
    {
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
  #endif
      if (i == ${len(PULP_Nodes_Graph_cnn) - 1})
          check_layer_last((int32_t *) L2_output, check_activations_out_cnn[i][t], check_activations_out_dimension_cnn[i]);
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_output_check += perf_cyc;
      //printf("[%d] Checking output of layer %d finished - num_cycles: %d\n", pi_core_id(), i, perf_cyc);
  #endif
    }
  % else:
  #ifdef VERBOSE
    if(pi_core_id()==0)
    {
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
  #endif
      printf("Layer %s %d ended: \n", Layers_name_cnn[i], i);
  #ifdef PROFILE_APPLICATION
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_output_check += perf_cyc;
      //printf("[%d] Checking output of layer %d finished - num_cycles: %d\n", pi_core_id(), i, perf_cyc);
  #endif
    }
  #endif
  % endif

  #ifdef PROFILE_APPLICATION
    if (pi_core_id() == 0){
      pi_perf_stop();
      pi_perf_reset();
      pi_perf_conf(1<<PI_PERF_CYCLES);
      pi_perf_start();
    }
  #endif
    if (i < ${len(PULP_Nodes_Graph_cnn) - 1})
    {
      if(pi_core_id()==0)
      {
        dory_L2_free(&L2_buffer_allocation,
          &L2_buffer_allocation_end,
          check_activations_dimension_cnn[i],
          begin_end_n // begin is 1, end is 0
          );
        if (branch_input_cnn[i]==1)
          dory_L2_free(&L2_buffer_allocation,
            &L2_buffer_allocation_end,
            check_activations_dimension_cnn[i],
            begin_end_n // begin is 1, end is 0
            );
        // deallocation of weights
        if (layer_with_weights_cnn[i] == 1)
          dory_L2_free(&L2_buffer_allocation,
            &L2_buffer_allocation_end,
            check_weights_dimension_cnn[i],
            begin_end_n // begin is 1, end is 0
            );
        if (layer_with_weights_cnn[i+1] == 1)
        {
          d_buffering_weights_e = !d_buffering_weights_e;
          exec_weights = d_buffering_weights_e ? L2_weights_2 : L2_weights_1;
        }
        if (i < ${len(PULP_Nodes_Graph_cnn) - 1})
        {
          // allocation of weights for next next layer, if necessary.
          if (layer_with_weights_cnn[i+2] == 1)
          {
            if (d_buffering_weights_e==1)
            {
              dory_L2_alloc(&L2_buffer_allocation,
                &L2_buffer_allocation_end,
                &L2_weights_1,
                check_weights_dimension_cnn[i+2],
                begin_end_n // begin is 1, end is 0
                );
            }
            else
            {
              dory_L2_alloc(&L2_buffer_allocation,
                &L2_buffer_allocation_end,
                &L2_weights_2,
                check_weights_dimension_cnn[i+2],
                begin_end_n // begin is 1, end is 0
                );
            }
            d_buffering_weights_t = !d_buffering_weights_t;
            transfer_weights = d_buffering_weights_t ? L2_weights_2 : L2_weights_1;
          }
        }
        L2_input_window = L2_output;
        if (pi_core_id()==0)
        {
          if (branch_input_cnn[i]==1 || branch_change_cnn[i-1] == 1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_free_req_t free_req;
            pi_cl_ram_free(&ram, layers_pointers_cnn[residual_number], (uint32_t) check_activations_dimension_cnn[i], &free_req);
            pi_cl_ram_free_wait(&free_req);
          }
          if (branch_input_cnn[i+1]==1)
          {
            begin_end_n = !begin_end_n;
            dory_L2_alloc(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              &bypass_activations,
              check_activations_out_dimension_cnn[i],
              begin_end_n // begin is 1, end is 0
              );
            begin_end_n = !begin_end_n;
            pi_cl_ram_read_wait(&buff_req1);
            residual_number--;
            pi_cl_ram_read(&ram, layers_pointers_cnn[residual_number], bypass_activations, check_activations_out_dimension_cnn[i], &buff_req1);
            pi_cl_ram_read_wait(&buff_req1);
          }
          if (i>0)
          {
            if (branch_output_cnn[i-1]==1 && L3_input_layers_cnn[i]==1)
            {
              pi_cl_ram_read_wait(&buff_req1);
              pi_cl_ram_alloc_req_t alloc_req;
              pi_cl_ram_alloc(&ram, (uint32_t) 1000000, &alloc_req);
              pi_cl_ram_alloc_wait(&alloc_req, &L3_input);
            }
          }
          if (branch_output_cnn[i]==1 && L3_output_layers_cnn[i]==1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_free_req_t free_req;
            pi_cl_ram_free(&ram, (uint32_t) L3_input + check_activations_out_dimension_cnn[i], (uint32_t) 1000000 - check_activations_out_dimension_cnn[i], &free_req);
            pi_cl_ram_free_wait(&free_req);
            layers_pointers_cnn[residual_number] = L3_input;
            residual_number++;
          }
          else if (branch_output_cnn[i]==1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_alloc_req_t alloc_req;
            int32_t temp_adress;
            pi_cl_ram_alloc(&ram, (uint32_t) check_activations_out_dimension_cnn[i], &alloc_req);
            pi_cl_ram_alloc_wait(&alloc_req, &temp_adress);
            layers_pointers_cnn[residual_number] = temp_adress;
            pi_cl_ram_write(&ram, temp_adress, L2_output, (uint32_t) check_activations_out_dimension_cnn[i], &buff_req1);
            pi_cl_ram_write_wait(&buff_req1);
            residual_number++;
          }
          if (branch_change_cnn[i] == 1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_alloc_req_t alloc_req;
            int32_t temp_adress;
            pi_cl_ram_alloc(&ram, (uint32_t) check_activations_out_dimension_cnn[i], &alloc_req);
            pi_cl_ram_alloc_wait(&alloc_req, &temp_adress);
            layers_pointers_cnn[residual_number] = temp_adress;
            pi_cl_ram_write(&ram, temp_adress, L2_output, (uint32_t) check_activations_out_dimension_cnn[i], &buff_req1);
            pi_cl_ram_write_wait(&buff_req1);
            residual_number++;
          }
          if (branch_change_cnn[i]==1)
          {
            begin_end_n = !begin_end_n;
            dory_L2_free(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              check_activations_out_dimension_cnn[i],
              begin_end_n // begin is 1, end is 0
              );
            dory_L2_alloc(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              &L2_input_window,
              check_activations_dimension_cnn[i+1],
              begin_end_n // begin is 1, end is 0
              );
            begin_end_n = !begin_end_n;
            pi_cl_ram_read_wait(&buff_req1);
            residual_number--;
            residual_number--;
            pi_cl_ram_read(&ram, layers_pointers_cnn[residual_number], L2_input_window, check_activations_dimension_cnn[i+1], &buff_req1);
            pi_cl_ram_read_wait(&buff_req1);
            residual_number++;
            residual_number++;
          }
        }
        dory_L2_alloc(&L2_buffer_allocation,
          &L2_buffer_allocation_end,
          &L2_output,
          check_activations_out_dimension_cnn[i+1],
          begin_end_n // begin is 1, end is 0
          );
        //switching output and input in the buffer for allocation.
        begin_end_n = !begin_end_n;
      }
    }

  #ifdef PROFILE_APPLICATION
    if (pi_core_id() == 0){
      pi_perf_stop();
      perf_cyc =  pi_perf_read(PI_PERF_CYCLES);
      cycles_dealloc_alloc_weights += perf_cyc;
      //printf("[%d] Deallocation of weights & allocation of weights for next layer finished - num_cycles: %d\n", pi_core_id(), perf_cyc);
    }
  #endif
  }
 }
/* ---------------------------------- */
/* --------- SECTION 2 END ---------- */
/* ---------------------------------- */

/* ---------------------------------- */
/* -------- SECTION 3 BEGIN --------- */
/* ---------------------------------- */

% if 'Perf_final' in verbose_level:
#ifdef PROFILE_APPLICATION
 int cid = pi_core_id();
 int MACs = ${MACs_cnn};
 float perf_MAC =  (float)MACs/cycle_network_execution;
 if (cid == 0)
 {
   printf("\n[%d] : num_cycles: %d\n",cid,cycle_network_execution);
   printf("[%d] : MACs: %d\n",cid,MACs );
   printf("[%d] : MAC/cycle: %f\n",cid,perf_MAC );
   printf("[%d] : n. of Cores: %d\n",cid,NUM_CORES);

   printf("\n[%d] cycles_network_execution : %d\n", cid, cycle_network_execution);
   printf("[%d] cycles_alloc_l1_l2_buffer : %d\n", cid, cycles_alloc_l1_l2_buffer);
   printf("[%d] cycles_alloc_weights_input_output : %d\n", cid, cycles_alloc_weights_input_output);
   printf("[%d] cycles_copy_weights_next_layer : %d\n", cid, cycles_copy_weights_next_layer);
   printf("[%d] cycles_input_check : %d\n", cid, cycles_input_check);
   printf("[%d] cycles_output_check : %d\n", cid, cycles_output_check);
   printf("[%d] cycles_dealloc_alloc_weights : %d\n", cid, cycles_dealloc_alloc_weights);
 }
#endif
% endif

 if (pi_core_id()==0)
 {
   pi_cl_l2_free(L2_buffer_tofree_copy, (uint32_t) ${l2_buffer_size}, &free_req);
   pi_cl_l2_free_wait(&free_req);
   pmsis_l1_malloc_free(l1_buffer, (uint32_t) ${l1_buffer} );
 }
/* ---------------------------------- */
/* --------- SECTION 3 END ---------- */
/* ---------------------------------- */
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

