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

# clean exp6 result
find ./Result/exp6 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build
cd ./Prototype
bash ./script/default/reset_default.sh
bash ./setup.sh
cd ../

####
# evaluate ShieldReduce with different alpha
####
thresholds=(0.0 0.5 0.7 0.9 1.0)

for threshold in "${thresholds[@]}"
do
  cd ./Prototype
  bash ./script/default/reset_default.sh
  sed -i "39s/#define GREEDY_THRESHOLD [0-9]\+\.[0-9]\+/#define GREEDY_THRESHOLD $threshold/" ./include/constVar.h
  bash ./recompile.sh
  cd ./bin 
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
  
  threshold_str=$(echo $threshold | sed 's/0\./a0/g' | sed 's/1\.0/a10/g')
  cp ./Prototype/bin/indexsize-log ./Result/exp6/${threshold_str}_indexsizelog.csv
done

# show result
cd ./Result/exp6
echo "------------------------"
echo "Exp#6: analysis of index overhead (dataset: ${dataset_name})"
echo "------------------------"
python3 ./ShowResult-a.py ${dataset_name}
python3 ./ShowResult-all.py ${dataset_name}
echo "------------------------"
cd ../../
