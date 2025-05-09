import csv
import os

def extract_delta_save(file_path):
    """
    Extract the 'Online Delta_save per OCall' value from the last line of the CSV file.
    
    Args:
        file_path (str): Path to the CSV file
    
    Returns:
        float: The value of 'Online Delta_save per OCall' from the last line
    """
    try:
        with open(file_path, 'r') as file:
            # Read all lines to get the CSV headers and data
            csv_reader = csv.DictReader(file)
            
            # Get the last row by iterating through all rows
            last_row = None
            for row in csv_reader:
                last_row = row
            
            if last_row is None:
                return None
            
            # Extract the value from the last row
            delta_save = last_row.get(" Online Delta_save per OCall")
            
            if delta_save is not None:
                return float(delta_save)
            else:
                print(f"Error: Column ' Online Delta_save per OCall' not found in {file_path}")
                return None
                
    except Exception as e:
        print(f"Error reading file {file_path}: {str(e)}")
        return None

def main():
    """
    Main function to process both CSV files and print the results.
    """
    # Define file paths and their display names
    files_info = [
        {"path": "WithOutOffload_reductionlog.csv", "display": "Without Offloading"},
        {"path": "ShieldReduce_reductionlog.csv", "display": "With Offloading"}
    ]
    
    # Process each file
    for file_info in files_info:
        file_path = file_info["path"]
        display_name = file_info["display"]
        
        # Check if the file exists
        if not os.path.exists(file_path):
            print(f"Error: File {file_path} does not exist")
            continue
            
        # Extract the value
        delta_save = extract_delta_save(file_path)
        
        # Print the result
        if delta_save is not None:
            print(f"{display_name}: {delta_save}")

if __name__ == "__main__":
    main()