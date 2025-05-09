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
fi

# clean exp4 result
find ./Result/exp4 -type f ! -name "*.txt" ! -name "*.py" -delete
echo "clean result success."

# init build
cd ./MultiClient
bash ./setup.sh
cd ../

if [[ "$dataset_name" == "unique" ]]; then
  # use unique dataset
else
  # use redundant dataset
fi

