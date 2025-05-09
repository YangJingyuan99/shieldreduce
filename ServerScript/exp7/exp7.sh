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
  exit 1
fi

# clean exp7 result
find ./Result/exp7 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build
cd ./Prototype
bash ./script/default/reset_default.sh
bash ./setup.sh
cd ../

####
# evaluate ShieldReduce and baselines
####
baselineIDs=(4 0 1 2 3)
declare -A name_prefixes
name_prefixes[0]="SecureMeGA"
name_prefixes[1]="DEBE"
name_prefixes[2]="ForwardDelta"
name_prefixes[4]="ShieldReduce"
name_prefixes[3]="WithOutOffload"

for baselineID in "${baselineIDs[@]}"
do
  cd ./Prototype && bash ./recompile.sh
  cd ./bin 
  ./ShieldReduceServer -m $baselineID > serveroutput 2>&1 &
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
  prefix=${name_prefixes[$baselineID]}
  cp ./Prototype/bin/sgx-log ./Result/exp7/${prefix}_sgxlog.csv
  cp ./Prototype/bin/reduction-log ./Result/exp7/${prefix}_reductionlog.csv
done

# show result
cd ./Result/exp7
echo "------------------------"
echo "Exp#7: enclave overhead (dataset: ${dataset_name})"
echo "------------------------"
echo "Enclave overhead"
python3 ./ShowResult-baseline.py ${dataset_name}
echo "------------------------"
echo "Average amount of data reduced by delta compression per OCall for data transfers and index updates (KiB) with and without delta compression offloading in ShieldReduce"
python3 ./ShowResult-offload.py ${dataset_name}
echo "------------------------"
cd ../../
