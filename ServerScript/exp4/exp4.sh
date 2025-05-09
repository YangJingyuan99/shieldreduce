#!/bin/bash

# check dataset name
if [ $# -lt 1 ]; then
    echo "error: empty dataset name"
    echo "usage: $0 <dataset name>"
    echo "avalible value: redundant, unique"
    exit 1
fi

dataset_name=$1
valid_datasets=("redundant" "unique")
is_valid=0

for valid_name in "${valid_datasets[@]}"; do
    if [ "$dataset_name" = "$valid_name" ]; then
        is_valid=1
        break
    fi
done

if [ $is_valid -eq 0 ]; then
    echo "error: invalid dataset '$dataset_name'"
    echo "you can use: redundant, unique"
    exit 1
fi

echo "use dataset: $dataset_name"

# check current path
current_dir_name=$(basename "$PWD")
if [ "$current_dir_name" != "atc25shieldreduce" ]; then
  echo "not the root path, please entre the ./atc25shieldreduce first and run the script again (current dir:$current_dir_name)"
  exit 1
fi

# clean exp4 result
find ./Result/exp4 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build and run
cd ./MultiClient
bash ./setup.sh
cd ./bin
./ShieldReduceServer -m 2 > serveroutput 2>&1 &
server_pid=$!
cd ../../

if [[ "$dataset_name" == "unique" ]]; then
  # use unique dataset

  ####
  # Upload & Download using one client
  ####

  # upload
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*"
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && cp ./Dataset/unique/u1 ./ram-client/"

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/u1"
  end_time=$(date +%s%N)

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 2 * 1 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: unique)"
  echo "------------------------"
  echo "One Client"
  echo "Upload execution time: ${elapsed_seconds} seconds"
  echo "Aggregate upload speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Exp#4: multi-client performance (dataset: unique)" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "One Client" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Upload execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate upload speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  sleep 2

  # download
  sudo sh -c "sync; echo 1 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 2 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*"

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/u1"
  end_time=$(date +%s%N)

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 2 * 1 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: unique)"
  echo "------------------------"
  echo "One Client"
  echo "Download execution time: ${elapsed_seconds} seconds"
  echo "Aggregate download speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "Download execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate download speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  kill -9 $server_pid
  sleep 2

  ####
  # Upload & Download using two clients
  ####

  # reset
  cd ./MultiClient
  bash ./recompile.sh
  cd ./bin
  ./ShieldReduceServer -m 2 > serveroutput 2>&1 &
  server_pid=$!
  cd ../../

  # upload
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID2=$!
  wait $PID1 $PID2
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && cp ./Dataset/unique/u1 ./ram-client/" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && cp ./Dataset/unique/u2 ./ram-client/" &
  PID2=$!
  wait $PID1 $PID2

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/u1" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/u2" &
  PID2=$!
  wait $PID1 $PID2
  end_time=$(date +%s%N)  

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 2 * 2 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: unique)"
  echo "------------------------"
  echo "Two Clients"
  echo "Upload execution time: ${elapsed_seconds} seconds"
  echo "Aggregate upload speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Two Clients" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Upload execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate upload speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  sleep 2

  # download
  sudo sh -c "sync; echo 1 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 2 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID2=$!
  wait $PID1 $PID2

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/u1" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/u2" &
  PID2=$!
  wait $PID1 $PID2
  end_time=$(date +%s%N)  

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 2 * 2 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: redundant)"
  echo "------------------------"
  echo "Two Clients"
  echo "Download execution time: ${elapsed_seconds} seconds"
  echo "Aggregate download speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "Download execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate download speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  kill -9 $server_pid
  sleep 2

else
  # use redundant dataset

  ####
  # Upload & Download using one client
  ####

  # upload
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*"
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && cp ./Dataset/redundant/vm1-1 ./ram-client/"

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/vm1-1"
  end_time=$(date +%s%N)

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 8 * 1 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: redundant)"
  echo "------------------------"
  echo "One Client"
  echo "Upload execution time: ${elapsed_seconds} seconds"
  echo "Aggregate upload speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Exp#4: multi-client performance (dataset: redundant)" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "One Client" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Upload execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate upload speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  sleep 2

  # download
  sudo sh -c "sync; echo 1 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 2 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*"

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/vm1-1"
  end_time=$(date +%s%N)

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 8 * 1 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: redundant)"
  echo "------------------------"
  echo "One Client"
  echo "Download execution time: ${elapsed_seconds} seconds"
  echo "Aggregate download speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "Download execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate download speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  kill -9 $server_pid
  sleep 2

  ####
  # Upload & Download using two clients
  ####

  # reset
  cd ./MultiClient
  bash ./recompile.sh
  cd ./bin
  ./ShieldReduceServer -m 2 > serveroutput 2>&1 &
  server_pid=$!
  cd ../../

  # upload
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID2=$!
  wait $PID1 $PID2
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && cp ./Dataset/redundant/vm1-1 ./ram-client/" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && cp ./Dataset/redundant/vm2-1 ./ram-client/" &
  PID2=$!
  wait $PID1 $PID2

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/vm1-1" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/vm2-1" &
  PID2=$!
  wait $PID1 $PID2
  end_time=$(date +%s%N)  

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 8 * 2 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: redundant)"
  echo "------------------------"
  echo "Two Clients"
  echo "Upload execution time: ${elapsed_seconds} seconds"
  echo "Aggregate upload speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Two Clients" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Upload execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate upload speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  sleep 2

  # download
  sudo sh -c "sync; echo 1 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 2 > /proc/sys/vm/drop_caches"
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  ssh root@172.28.114.116 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce && rm ./ram-client/*" &
  PID2=$!
  wait $PID1 $PID2

  start_time=$(date +%s%N)
  ssh root@172.28.114.116 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/vm1-1" &
  PID1=$!
  ssh root@172.28.114.119 "cd /root/atc25shieldreduce/Prototype/bin/ && ./ShieldReduceClient -t d -i ../../ram-client/vm2-1" &
  PID2=$!
  wait $PID1 $PID2
  end_time=$(date +%s%N)  

  elapsed_time=$((end_time - start_time))
  elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
  speed=$(echo "scale=3; 1024 * 8 * 2 / $elapsed_seconds" | bc)
  echo "------------------------"
  echo "Exp#4: multi-client performance (dataset: redundant)"
  echo "------------------------"
  echo "Two Clients"
  echo "Download execution time: ${elapsed_seconds} seconds"
  echo "Aggregate download speed: ${speed} MiB/s"
  echo "------------------------"

  # record
  echo "Download execution time: ${elapsed_seconds} seconds" >> /root/atc25shieldreduce/Result/exp4/result
  echo "Aggregate download speed: ${speed} MiB/s" >> /root/atc25shieldreduce/Result/exp4/result
  echo "------------------------" >> /root/atc25shieldreduce/Result/exp4/result
  kill -9 $server_pid
  sleep 2
fi

echo "evaluation done, you can review the result in ./Result/exp4/result"