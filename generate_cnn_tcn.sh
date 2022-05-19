#! /bin/bash

set -e

rm -rf application_dvs_separate

mkdir -p application_dvs_separate/cnn/net
mkdir -p application_dvs_separate/tcn/net

# generate CNN; we set --number_of_deployed_layers=10 to leave out the final GlobalAvgPooling layer
python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/cnn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=10 --optional mixed-hw --perf_layer Yes --signed_input
cp -r application application_dvs_separate/cnn
cp -r logs application_dvs_separate/cnn
cp -r examples/Quantlab_examples/dvs_cnn/cnn/* application_dvs_separate/cnn/net

# generate TCN
python network_generate.py --network_dir ./examples/Quantlab_examples/dvs_cnn/tcn/ --sdk pulp_sdk --frontend Quantlab --backend MCU --number_of_deployed_layers=3 --optional mixed-hw --perf_layer Yes
cp -r application application_dvs_separate/tcn
cp -r logs application_dvs_separate/tcn/
cp -r examples/Quantlab_examples/dvs_cnn/tcn/* application_dvs_separate/tcn/net

rm -r application