import sys
import argparse
from argparse import RawTextHelpFormatter
import json
from mako.template import Template
import os

def combine_network_template_dicts(template_data_cnn, template_data_tcn):
    assert template_data_cnn.keys() == template_data_tcn.keys(), 'cnn and tcn dict keys do not match'
    template_data_dvs = {}
    for key in template_data_cnn.keys():
        if key in ['sdk', 'verbose', 'performance', 'verbose_level', 'fc_frequency', 'cl_frequency', 'master_stack', 'slave_stack', 'dma_parallelization', 'l2_buffer_size', 'l1_buffer', 'type', 'test']:
            assert template_data_cnn[key]==template_data_tcn[key], f'cnn and tcn values for key \'{key}\' do not match - cnn: {template_data_cnn[key]}, tcn: {template_data_tcn[key]}'
            template_data_dvs[key]=template_data_cnn[key]
        else:
            template_data_dvs[key+'_cnn']=template_data_cnn[key]
            template_data_dvs[key+'_tcn']=template_data_tcn[key]

    return template_data_dvs

def combine_makefile_template_dicts(template_data_cnn, template_data_tcn):
    assert template_data_cnn.keys() == template_data_tcn.keys(), 'cnn and tcn dict keys do not match'
    template_data_dvs = {}
    for key in template_data_cnn.keys():
        if key in ['platform', 'sdk']:
            assert template_data_cnn[key]==template_data_tcn[key], f'cnn and tcn values for key \'{key}\' do not match - cnn: {template_data_cnn[key]}, tcn: {template_data_tcn[key]}'
            template_data_dvs[key]=template_data_cnn[key]
        elif key in ['test_inputs']:
            template_data_dvs[key]=template_data_cnn[key]
        else:
            # combine files from both dicts
            lst = template_data_cnn[key]+template_data_tcn[key]
            # remove duplicate files
            template_data_dvs[key] = list(dict.fromkeys(lst))
    template_data_dvs['build_layers'].append('network_cnn.c')
    template_data_dvs['build_layers'].append('network_tcn.c')
    return template_data_dvs

def main():
    parser = argparse.ArgumentParser(formatter_class=RawTextHelpFormatter)
    parser.add_argument('--network_template_data_cnn', default='./network_template_data_cnn.json', help='json file which contains the network template data for the cnn net')
    parser.add_argument('--network_template_data_tcn', default='./network_template_data_tcn.json', help='json file which contains the network template data for the tcn net')
    parser.add_argument('--makefile_template_data_cnn', default='./makefile_template_data_cnn.json', help='json file which contains the makefile template data for the cnn net')
    parser.add_argument('--makefile_template_data_tcn', default='./makefile_template_data_tcn.json', help='json file which contains the makefile template data for the tcn net')
    parser.add_argument('--chip', default = 'GAP8v3', help = 'GAP8v2 for fixing DMA issue. GAP8v3 otherise')

    args = parser.parse_args()

    with open(args.network_template_data_cnn, "r") as read_file:
        tk_net_cnn = json.load(read_file)
    with open(args.network_template_data_tcn, "r") as read_file:
        tk_net_tcn = json.load(read_file)

    # write network.c file
    tk_net_dvs = combine_network_template_dicts(tk_net_cnn, tk_net_tcn)
    tmpl = Template(filename="./templates/network_template.c")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/src/network.c'
    with open(save_string, "w") as f:
        f.write(s)

    # write network.h file
    tk_net_dvs = combine_network_template_dicts(tk_net_cnn, tk_net_tcn)
    tmpl = Template(filename="./templates/network.h")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/inc/network.h'
    with open(save_string, "w") as f:
        f.write(s)

    # write network_cnn.c file
    tmpl = Template(filename="./templates/network_cnn_template.c")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/src/network_cnn.c'
    with open(save_string, "w") as f:
        f.write(s)

    # write network_tcn.c file
    tmpl = Template(filename="./templates/network_tcn_template.c")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/src/network_tcn.c'
    with open(save_string, "w") as f:
        f.write(s)

    # write utils.h file
    tmpl = Template(filename="./templates/utils.h")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/inc/utils.h'
    with open(save_string, "w") as f:
        f.write(s)

    # write variables.h file
    tmpl = Template(filename="./templates/variables.h")
    s = tmpl.render(**tk_net_dvs)
    save_string = './application/DORY_network/inc/variables.h'
    with open(save_string, "w") as f:
        f.write(s)

    with open(args.makefile_template_data_cnn, "r") as read_file:
        tk_mf_cnn = json.load(read_file)
    with open(args.makefile_template_data_tcn, "r") as read_file:
        tk_mf_tcn = json.load(read_file)

    # write Makefile
    tk_mf_dvs = combine_makefile_template_dicts(tk_mf_cnn, tk_mf_tcn)
    tmpl = Template(filename="./templates/Makefile_template")
    s = tmpl.render(**tk_mf_dvs)
    save_string = './application/Makefile'
    with open(save_string, "w") as f:
        f.write(s)

    # write dory.h
    tk = {'sdk': tk_mf_dvs['sdk']}
    tmpl = Template(filename="./templates/dory.h")
    s = tmpl.render(**tk)
    save_string = './application/DORY_network/inc/dory.h'
    with open(save_string, "w") as f:
        f.write(s)

    # write mchan_test.h
    tmpl = Template(filename="./templates/mchan_test.h")
    s = tmpl.render(**tk)
    save_string = './application/DORY_network/inc/mchan_test.h'
    with open(save_string, "w") as f:
        f.write(s)

    # write dory.c
    tk = {'chip': args.chip, 'dma_parallelization': tk_net_dvs['dma_parallelization']}
    tmpl = Template(filename="./templates/dory.c")
    s = tmpl.render(**tk)
    save_string = './application/DORY_network/src/dory.c'
    with open(save_string, "w") as f:
        f.write(s)

if __name__ == '__main__':
    main()
