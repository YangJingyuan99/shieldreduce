#!/bin/bash

# check dataset name
if [ $# -lt 1 ]; then
    echo "error: empty dataset name"
    echo "usage: $0 <dataset name>"
    echo "avalible value: linux, docker, simos"
    exit 1
fi

dataset_name=$1
valid_datasets=("linux" "docker" "simos")
is_valid=0

for valid_name in "${valid_datasets[@]}"; do
    if [ "$dataset_name" = "$valid_name" ]; then
        is_valid=1
        break
    fi
done

if [ $is_valid -eq 0 ]; then
    echo "error: invalid dataset '$dataset_name'"
    echo "you can use: linux, docker, simos"
    exit 1
fi

echo "use dataset: $dataset_name"

# check current path
current_dir_name=$(basename "$PWD")
if [ "$current_dir_name" != "atc25shieldreduce" ]; then
  echo "not the root path, please entre the ./atc25shieldreduce first and run the script again (current dir:$current_dir_name)"
fi

# clean exp2 result
find ./Result/exp2 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build
cd ./Prototype
bash ./script/exp2/reset_exp2.sh
bash ./setup.sh
cd ../

# clean client log
ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin && rm client-log"

####
# evaluate ShieldReduce
####
cd ./Prototype/bin
./ShieldReduceServer -m 4 > serveroutput 2>&1 &
server_pid=$!
cd ../../
echo "wait server start..." && sleep 5 && echo "ok"
ssh root@172.28.114.116 "cd /root/atc25shieldreduce && bash ./ClientScript/${dataset_name}Up.sh"

# close server
echo "wait the server to complete offline delta compression.."
while true; do
    if tail -n 1 ./Prototype/bin/serveroutput | grep -q "total key exchange time of client"; then
        break
    fi
    sleep 1
done
echo "offline delta compression done."
kill -9 $server_pid
sleep 1
cp ./Prototype/bin/breakdown-log ./Result/exp2/breakdownlog.csv
scp root@172.28.114.116:/root/atc25shieldreduce/Prototype/bin/client-log ./Result/exp2/clientlog.csv

# show result
cd ./Result/exp2
echo "------------------------"
echo "Exp#2: microbenchmarks (dataset: ${dataset_name})"
echo "------------------------"
python3 ShowResult.py
echo "------------------------"
cd ../../