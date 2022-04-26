#! /bin/bash

set -e

rm -rf application_dvs

mkdir -p application_dvs/cnn/net
mkdir -p application_dvs/tcn/net
mkdir -p application_dvs/dvs

# generate CNN; we set --number_of_deployed_layers=10 to leave out the final GlobalAvgPooling layer
python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/dvs/cnn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=10 --optional mixed-hw --perf_layer Yes --number_of_test_inputs=5
cp -r application application_dvs/cnn
cp -r logs application_dvs/cnn
cp -r examples/Quantlab_examples/dvs_cnn/cnn/* application_dvs/cnn/net

# generate TCN
python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/dvs/tcn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=3 --optional mixed-hw --perf_layer Yes --number_of_test_inputs=1
cp -r application application_dvs/tcn
cp -r logs application_dvs/tcn/
cp -r examples/Quantlab_examples/dvs_cnn/tcn/* application_dvs/tcn/net

# generate the full DVS network and copy all necessary files in the output directory
cp application_dvs/cnn/application/network_template_data.json dvs_generation/network_template_data_cnn.json
cp application_dvs/cnn/application/makefile_template_data.json dvs_generation/makefile_template_data_cnn.json
cp application_dvs/tcn/application/network_template_data.json dvs_generation/network_template_data_tcn.json
cp application_dvs/tcn/application/makefile_template_data.json dvs_generation/makefile_template_data_tcn.json
cd dvs_generation
python dvs_generate.py --network_template_data_cnn='./network_template_data_cnn.json' --network_template_data_tcn='./network_template_data_tcn.json' \
                    --makefile_template_data_cnn='./makefile_template_data_cnn.json' --makefile_template_data_tcn='./makefile_template_data_tcn.json'
cd ..
cp -r dvs_generation/application application_dvs/dvs
# note: while copying the layer files, we assume that the two nets have layers with different names. But this is the case here, so no problem
cp application_dvs/cnn/application/DORY_network/inc/layer*.h application_dvs/dvs/application/DORY_network/inc
cp application_dvs/cnn/application/DORY_network/inc/pulp_nn_*.h application_dvs/dvs/application/DORY_network/inc
cp application_dvs/cnn/application/DORY_network/src/layer*.c application_dvs/dvs/application/DORY_network/src
cp application_dvs/cnn/application/DORY_network/src/xpulp_nn*.c application_dvs/dvs/application/DORY_network/src
cp application_dvs/cnn/application/DORY_network/*weights.hex application_dvs/dvs/application/DORY_network
cp application_dvs/cnn/application/DORY_network/inputs*.hex application_dvs/dvs/application/DORY_network
cp application_dvs/tcn/application/DORY_network/inc/layer*.h application_dvs/dvs/application/DORY_network/inc
cp application_dvs/tcn/application/DORY_network/src/layer*.c application_dvs/dvs/application/DORY_network/src
cp application_dvs/cnn/application/DORY_network/src/xpulp_nn_*.c application_dvs/dvs/application/DORY_network/src
cp application_dvs/tcn/application/DORY_network/src/xpulp_nn_conv1d_*.c application_dvs/dvs/application/DORY_network/src
cp application_dvs/tcn/application/DORY_network/*weights.hex application_dvs/dvs/application/DORY_network

rm -r application_dvs/cnn application_dvs/tcn
mv application_dvs/dvs/application/* application_dvs
rm -r application_dvs/dvs
rm -r application
