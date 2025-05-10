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

# clean exp3 result
find ./Result/exp3 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build
cd ./Prototype
bash ./script/default/reset_default.sh
bash ./setup.sh
cd ../

####
# evaluate ShieldReduce and baselines
####
MAX_RETRIES=3
baselineIDs=(4 0 1 2)
declare -A name_prefixes
name_prefixes[0]="SecureMeGA"
name_prefixes[1]="DEBE"
name_prefixes[2]="ForwardDelta"
name_prefixes[4]="ShieldReduce"

for baselineID in "${baselineIDs[@]}"
do
  retry_count=0
  while [ $retry_count -lt $MAX_RETRIES ]; do
    cd ./Prototype && bash ./recompile.sh
    cd ./bin 
    ./ShieldReduceServer -m $baselineID > serveroutput 2>&1 &
    server_pid=$!
    cd ../../
    echo "wait server start..." && sleep 5 && echo "ok"
    ssh root@172.28.114.116 "cd /root/atc25shieldreduce && bash ./ClientScript/${dataset_name}Up.sh"

    # close server
    echo "wait the server to complete offline delta compression.."
    
    loop_count=0
    max_loops=180
    server_completed=false
    
    while true; do
      if tail -n 1 ./Prototype/bin/serveroutput | grep -q "total key exchange time of client"; then
          echo "Success: Server completed normally."
          server_completed=true
          break
      fi
      
      loop_count=$((loop_count + 1))
      
      if [ $loop_count -ge $max_loops ]; then
          echo "Timeout: Server did not complete in expected time. Killing server and restarting..."
          kill -9 $server_pid 2>/dev/null
          sleep 2
          break
      fi
      
      sleep 1
    done
    
    if [ "$server_completed" = true ]; then
      echo "offline delta compression done."
      kill -9 $server_pid
      sleep 1
      prefix=${name_prefixes[$baselineID]}
      cp ./Prototype/bin/server-log ./Result/exp3/${prefix}_serverlog.csv
      break
    else
      retry_count=$((retry_count + 1))
      if [ $retry_count -lt $MAX_RETRIES ]; then
        echo "Retrying... Attempt $((retry_count + 1)) of $MAX_RETRIES"
      else
        echo "Failed after $MAX_RETRIES attempts."
      fi
    fi
  done
done

# show result
cd ./Result/exp3
echo "------------------------"
echo "Exp#3: inline performance (dataset: ${dataset_name})"
echo "------------------------"
python3 ./ShowResult.py ${dataset_name}
echo "figure has generated in ./Result/exp3"
echo "------------------------"
cd ../../
