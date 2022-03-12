#! /bin/bash

rm -rf application_dvs

mkdir -p application_dvs/cnn/net
mkdir -p application_dvs/tcn/net

# we set --number_of_deployed_layers=10 to leave out the final GlobalAvgPooling layer
python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/cnn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=10 --optional mixed-hw --perf_layer Yes
cp -r application application_dvs/cnn
cp -r logs application_dvs/cnn
cp -r examples/Quantlab_examples/dvs_cnn/cnn/* application_dvs/cnn/net

python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/tcn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=3 --optional mixed-hw --perf_layer Yes
cp -r application application_dvs/tcn
cp -r logs application_dvs/tcn/
cp -r examples/Quantlab_examples/dvs_cnn/tcn/* application_dvs/tcn/net
