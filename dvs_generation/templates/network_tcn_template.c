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
#include "network.h"
#include "mem_controller.h"
% if sdk == 'gap_sdk':
#include "pulp.h"
% endif
#include "dory.h"
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
#include "utils.h"
#include "variables.h"
#include "network_tcn.h"


void network_run_tcn(void *args, struct pi_device *ram)
{
  // extract the arguments
  unsigned int *real_arg = (unsigned int *) args;
  unsigned int L3_weights =  real_arg[0];
  unsigned int L3_input; // not needed
  unsigned int L3_output; // not needed
  unsigned int activations_input; // not needed
  unsigned int L2_input = real_arg[4];
  unsigned int L2_buffer_allocation = real_arg[5];
  unsigned int L2_buffer_allocation_end = real_arg[6];
  unsigned int L2_buffer_tofree = real_arg[7];
  unsigned int l1_buffer = real_arg[8];
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
 int L2_output;
 int L2_weights_1;
 int L2_weights_2;
 int exec_weights;
 int transfer_weights;
 int bypass_weights;
 int L3_weights_internal;
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
 int bypass_activations = 0;
 int activation_to_keep = 0;
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

/*
 - no L2 & L1 buffer allocations; has been previously done in CNN part
*/

/* ---------------------------------- */
/* --------- SECTION 0 END ---------- */
/* ---------------------------------- */
/*
 - initial copies from L3 of input
 - copies of weights of first 2 layers of the tcn
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
     - No need for initial input allocation - inputs should already be in L2 from the previous CNN inference
    */

    /*
     - tcn first layer weights allocation and copy
    */
    dory_L2_alloc(&L2_buffer_allocation,
      &L2_buffer_allocation_end,
      &L2_weights_1,
      ${int(PULP_Nodes_Graph_tcn[0]['weights_dimension'])},
      begin_end_n // begin is 1, end is 0
      );
    begin_end_n = !begin_end_n;
    transfer_weights = L2_weights_1;
    exec_weights = L2_weights_1;
    pi_cl_ram_read(ram, L3_weights_internal, transfer_weights, ${int(PULP_Nodes_Graph_tcn[0]['weights_dimension'])}, &buff_req1);
    pi_cl_ram_read_wait(&buff_req1);

 % if 'Gemm' in PULP_Nodes_Graph_tcn[1]['name'] or 'Conv' in PULP_Nodes_Graph_tcn[1]['name']:
 /*
  - tcn second layer weights allocation
 */
    d_buffering_weights_t = !d_buffering_weights_t;
    dory_L2_alloc(&L2_buffer_allocation,
      &L2_buffer_allocation_end,
      &L2_weights_2,
      ${int(PULP_Nodes_Graph_tcn[1]['weights_dimension'])-int(PULP_Nodes_Graph_tcn[0]['weights_dimension'])},
      begin_end_n // begin is 1, end is 0
      );
    transfer_weights = d_buffering_weights_t ? L2_weights_2 : L2_weights_1;
 % endif
 /*
  - output of the first tcn layer allocation
 */
    dory_L2_alloc(&L2_buffer_allocation,
      &L2_buffer_allocation_end,
      &L2_output,
      ${int(PULP_Nodes_Graph_tcn[0]['output_activation_dimensions'])},
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
 - for loop over all windows that are fed to the tcn network
 - for loop over all the layers of the tcn network
 - double buffering using L3
 - check on layers to be executed from L3
 - residual check at the end of each layer
*/
/* ---------------------------------- */
/* -------- SECTION 2 BEGIN --------- */
/* ---------------------------------- */

  for(int i = 0; i < ${len(PULP_Nodes_Graph_tcn)}; i++)
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
      if (i < ${len(PULP_Nodes_Graph_tcn)-1})
        {
        if (allocate_layer_tcn[i+1] == 1) // modulus op: when reached the last layer, read weights of first layer
        {
          if (L3_layers_tcn[i-1] == 0 && i > 0)
            pi_cl_ram_read_wait(&buff_req1);
          pi_cl_ram_read(ram, L3_weights_internal + cumulative_weights_dimension_tcn[i+1], transfer_weights, check_weights_dimension_tcn[i+1], &buff_req1);
          if (L3_layers_tcn[i] == 1)
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
      if (L3_input_layers_tcn[i]==1)
        printf("In in L3\n");
      else if (i==0) {
        printf("Checking input of layer %d...\n", i);
        check_layer(L2_input, check_activations_tcn[i], check_activations_dimension_tcn[i]);
      }
      else if (branch_change_tcn[i-1]==0) {
        printf("Checking input of layer %d...\n", i);
        check_layer(L2_input, check_activations_tcn[i], check_activations_dimension_tcn[i]);
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

    out_mult = out_mult_vector_tcn[i];
    out_shift = out_shift_vector_tcn[i];
    inmul1 = inmul1_vector_tcn[i];
    inmul2 = inmul2_vector_tcn[i];
    pi_cl_team_barrier(0);
    unsigned int args[13] = {L3_input,
      L3_output,
      L3_weights_internal + cumulative_weights_dimension_tcn[i],
      L2_input,
      bypass_activations,
      L2_output,
      exec_weights,
      l1_buffer,
      ram,
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
  % for i in range(len(PULP_Nodes_Graph_tcn)):
      case ${i}:
        ${func_name_tcn[i]}(args);
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
    int MACs = NODEs_MACS_tcn[i];
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
      printf("Layer %s %d ended: \n", Layers_name_tcn[i], i);
      if (i < ${len(PULP_Nodes_Graph_tcn) - 1})
      {
        if (L3_output_layers_tcn[i]==1)
          printf("Out in L3\n");
        else {
          printf("Checking output of layer %d...\n", i);
          check_layer(L2_output, check_activations_out_tcn[i], check_activations_out_dimension_tcn[i]);
        }
      }
      else
      {
        check_layer_last((int32_t *) L2_output, check_activations_out_tcn[i], check_activations_out_dimension_tcn[i]);
      }
      if (i==${check_layer_tcn})
      {
        check_layer_plus(L2_output, check_activations_out_dimension_tcn[i]);
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
      if (i == ${len(PULP_Nodes_Graph_tcn) - 1})
          check_layer_last((int32_t *) L2_output, check_activations_out_tcn[i], check_activations_out_dimension_tcn[i]);
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
      printf("Layer %s %d ended: \n", Layers_name_tcn[i], i);
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
    if (i < ${len(PULP_Nodes_Graph_tcn)})
    {
      if(pi_core_id()==0)
      {
        // deallocation of weights
        if (layer_with_weights_tcn[i] == 1)
          dory_L2_free(&L2_buffer_allocation,
            &L2_buffer_allocation_end,
            check_weights_dimension_tcn[i],
            begin_end_n // begin is 1, end is 0
            );
        if (i < ${len(PULP_Nodes_Graph_tcn)-1})
        {
          if (layer_with_weights_tcn[i+1] == 1)
          {
            d_buffering_weights_e = !d_buffering_weights_e;
            exec_weights = d_buffering_weights_e ? L2_weights_2 : L2_weights_1;
          }
        }

        // deallocation of input activations
        dory_L2_free(&L2_buffer_allocation,
          &L2_buffer_allocation_end,
          check_activations_dimension_tcn[i],
          begin_end_n // begin is 1, end is 0
          );
        if (branch_input_tcn[i]==1)
          dory_L2_free(&L2_buffer_allocation,
            &L2_buffer_allocation_end,
            check_activations_dimension_tcn[i],
            begin_end_n // begin is 1, end is 0
            );

        L2_input = L2_output;
        if (pi_core_id()==0)
        {
          if (branch_input_tcn[i]==1 || branch_change_tcn[i-1] == 1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_free_req_t free_req;
            pi_cl_ram_free(ram, layers_pointers_tcn[residual_number], (uint32_t) check_activations_dimension_tcn[i], &free_req);
            pi_cl_ram_free_wait(&free_req);
          }
          if (branch_input_tcn[i+1]==1)
          {
            begin_end_n = !begin_end_n;
            dory_L2_alloc(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              &bypass_activations,
              check_activations_out_dimension_tcn[i],
              begin_end_n // begin is 1, end is 0
              );
            begin_end_n = !begin_end_n;
            pi_cl_ram_read_wait(&buff_req1);
            residual_number--;
            pi_cl_ram_read(ram, layers_pointers_tcn[residual_number], bypass_activations, check_activations_out_dimension_tcn[i], &buff_req1);
            pi_cl_ram_read_wait(&buff_req1);
          }
          if (i>0)
          {
            if (branch_output_tcn[i-1]==1 && L3_input_layers_tcn[i]==1)
            {
              pi_cl_ram_read_wait(&buff_req1);
              pi_cl_ram_alloc_req_t alloc_req;
              pi_cl_ram_alloc(ram, (uint32_t) 1000000, &alloc_req);
              pi_cl_ram_alloc_wait(&alloc_req, &L3_input);
            }
          }
          if (branch_output_tcn[i]==1 && L3_output_layers_tcn[i]==1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_free_req_t free_req;
            pi_cl_ram_free(ram, (uint32_t) L3_input + check_activations_out_dimension_tcn[i], (uint32_t) 1000000 - check_activations_out_dimension_tcn[i], &free_req);
            pi_cl_ram_free_wait(&free_req);
            layers_pointers_tcn[residual_number] = L3_input;
            residual_number++;
          }
          else if (branch_output_tcn[i]==1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_alloc_req_t alloc_req;
            int32_t temp_adress;
            pi_cl_ram_alloc(ram, (uint32_t) check_activations_out_dimension_tcn[i], &alloc_req);
            pi_cl_ram_alloc_wait(&alloc_req, &temp_adress);
            layers_pointers_tcn[residual_number] = temp_adress;
            pi_cl_ram_write(ram, temp_adress, L2_output, (uint32_t) check_activations_out_dimension_tcn[i], &buff_req1);
            pi_cl_ram_write_wait(&buff_req1);
            residual_number++;
          }
          if (branch_change_tcn[i] == 1)
          {
            pi_cl_ram_read_wait(&buff_req1);
            pi_cl_ram_alloc_req_t alloc_req;
            int32_t temp_adress;
            pi_cl_ram_alloc(ram, (uint32_t) check_activations_out_dimension_tcn[i], &alloc_req);
            pi_cl_ram_alloc_wait(&alloc_req, &temp_adress);
            layers_pointers_tcn[residual_number] = temp_adress;
            pi_cl_ram_write(ram, temp_adress, L2_output, (uint32_t) check_activations_out_dimension_tcn[i], &buff_req1);
            pi_cl_ram_write_wait(&buff_req1);
            residual_number++;
          }
          if (branch_change_tcn[i]==1)
          {
            begin_end_n = !begin_end_n;
            dory_L2_free(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              check_activations_out_dimension_tcn[i],
              begin_end_n // begin is 1, end is 0
              );
            dory_L2_alloc(&L2_buffer_allocation,
              &L2_buffer_allocation_end,
              &L2_input,
              check_activations_dimension_tcn[i+1],
              begin_end_n // begin is 1, end is 0
              );
            begin_end_n = !begin_end_n;
            pi_cl_ram_read_wait(&buff_req1);
            residual_number--;
            residual_number--;
            pi_cl_ram_read(ram, layers_pointers_tcn[residual_number], L2_input, check_activations_dimension_tcn[i+1], &buff_req1);
            pi_cl_ram_read_wait(&buff_req1);
            residual_number++;
            residual_number++;
          }
        }
        dory_L2_alloc(&L2_buffer_allocation,
          &L2_buffer_allocation_end,
          &L2_output,
          check_activations_out_dimension_tcn[i+1],
          begin_end_n // begin is 1, end is 0
          );

        if (i < ${len(PULP_Nodes_Graph_tcn) - 2})
        {
          // allocation of weights for next next layer, if necessary.
          if (layer_with_weights_tcn[i+2] == 1)
          {
            if (d_buffering_weights_e==1)
            {
              dory_L2_alloc(&L2_buffer_allocation,
                &L2_buffer_allocation_end,
                &L2_weights_1,
                check_weights_dimension_tcn[i+2],
                begin_end_n // begin is 1, end is 0
                );
            }
            else
            {
              dory_L2_alloc(&L2_buffer_allocation,
                &L2_buffer_allocation_end,
                &L2_weights_2,
                check_weights_dimension_tcn[i+2],
                begin_end_n // begin is 1, end is 0
                );
            }
            d_buffering_weights_t = !d_buffering_weights_t;
            transfer_weights = d_buffering_weights_t ? L2_weights_2 : L2_weights_1;
          }
        }
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
/* ---------------------------------- */
/* --------- SECTION 2 END ---------- */
/* ---------------------------------- */

/* ---------------------------------- */
/* -------- SECTION 3 BEGIN --------- */
/* ---------------------------------- */

% if 'Perf_final' in verbose_level:
#ifdef PROFILE_APPLICATION
 int cid = pi_core_id();
 int MACs = ${MACs_tcn};
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
   pi_cl_l2_free(L2_buffer_tofree, (uint32_t) ${l2_buffer_size}, &free_req);
   pi_cl_l2_free_wait(&free_req);
   pmsis_l1_malloc_free(l1_buffer, (uint32_t) ${l1_buffer} );
 }
/* ---------------------------------- */
/* --------- SECTION 3 END ---------- */
/* ---------------------------------- */
}
