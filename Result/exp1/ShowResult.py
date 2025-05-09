import os
import csv
import glob

def get_overall_reduction_ratio(file_path):
    try:
        with open(file_path, 'r') as csv_file:
            reader = csv.reader(csv_file)
            headers = next(reader)
            
            ratio_index = -1
            for i, header in enumerate(headers):
                if header.strip() == 'OverallReductionRatio':
                    ratio_index = i
                    break
            
            if ratio_index == -1:
                print(f"file {file_path} no OverallReductionRatio")
                return None
            
            rows = list(reader)
            if not rows:
                return None
            
            last_row = rows[-1]
            if ratio_index < len(last_row):
                return last_row[ratio_index]
            else:
                return None
    except Exception as e:
        print(f"process {file_path} error: {e}")
        return None

def main():
    csv_files = glob.glob('*_serverlog.csv')
    
    if not csv_files:
        print("no *_serverlog.csv here")
        return
    
    desired_order = ["DEBE", "ShieldReduce", "a05", "a07", "a10", "SecureMeGA", "ForwardDelta"]
    
    results = {}
    
    for file_path in csv_files:
        file_prefix = file_path.split('_serverlog.csv')[0]
        
        ratio = get_overall_reduction_ratio(file_path)
        
        if ratio is not None:
            results[file_prefix] = ratio
        else:
            results[file_prefix] = "can not find OverallReductionRatio value"
    
    for prefix in desired_order:
        if prefix in results:
            print(f"{prefix}: {results[prefix]}")
        else:
            print(f"{prefix}: no corresponding data")

if __name__ == "__main__":
    main()