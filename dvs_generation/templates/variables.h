#ifndef VARIABLES_H
#define VARIABLES_H

static const char * input_files_cnn[] = {
  % for t in range(test_inputs_cnn):
  ${f"\"inputs_{t}.hex\""}${'' if loop.last else ', '}
  % endfor
};
// allocation of buffers with parameters needed by the network execution
static const char * L3_weights_files_cnn[] = {
  ${files_list_cnn}
};
static const char * L3_weights_files_tcn[] = {
  ${files_list_tcn}
};
static int L3_weights_size_cnn[${weights_number_cnn}];
static int L3_weights_size_tcn[${weights_number_tcn}];
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
static int check_activations_cnn[${len(PULP_Nodes_Graph_cnn)}][${test_inputs_cnn}] = {
% for i in range(len(PULP_Nodes_Graph_cnn)):
 {\
 % for j in range(test_inputs_cnn):
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

#endif