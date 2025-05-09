import csv
import os

def extract_values_from_file(file_path, required_fields):
    """
    Extract the required field values from the last row of the CSV file.
    
    Args:
        file_path: Path to the CSV file
        required_fields: List of field names to extract
    
    Returns:
        Dictionary of field names and their values
    """
    try:
        with open(file_path, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            # Get the last row
            for row in reader:
                last_row = row
            
            # Extract required fields
            result = {}
            for field in required_fields:
                if field in last_row:
                    result[field] = last_row[field]
                else:
                    result[field] = "N/A"
            
            return result
    except Exception as e:
        print(f"Error processing file {file_path}: {e}")
        return {field: "N/A" for field in required_fields}

def main():
    """
    Main function to process all CSV files and print the required values.
    """
    # Define file paths
    debe_file = "DEBE_sgxlog.csv"
    forward_delta_file = "ForwardDelta_sgxlog.csv"
    shield_reduce_file = "ShieldReduce_sgxlog.csv"
    
    # Define required fields for each file
    common_fields = [" Inline Ecall", " Index queries Ocall", " Data transfers Ocall", " Total Inline Ocall"]
    shield_reduce_extra_fields = [" Index updates Ocall", " Offline Ecall", " Offline_Ocall"]
    
    debe_fields = common_fields
    forward_delta_fields = common_fields
    shield_reduce_fields = common_fields + shield_reduce_extra_fields
    
    # Define the order of output
    output_order = [" Inline Ecall", " Index queries Ocall", " Index updates Ocall", 
                   " Data transfers Ocall", " Total Inline Ocall", " Offline Ecall", " Offline_Ocall"]
    
    # Process DEBE
    print("DEBE values:")
    debe_values = extract_values_from_file(debe_file, debe_fields)
    for field in output_order:
        if field in debe_values:
            print(f"{field.strip()}: {debe_values[field]}")
    
    # Process ForwardDelta
    print("\nForwardDelta values:")
    forward_delta_values = extract_values_from_file(forward_delta_file, forward_delta_fields)
    for field in output_order:
        if field in forward_delta_values:
            print(f"{field.strip()}: {forward_delta_values[field]}")
    
    # Process ShieldReduce
    print("\nShieldReduce values:")
    shield_reduce_values = extract_values_from_file(shield_reduce_file, shield_reduce_fields)
    for field in output_order:
        if field in shield_reduce_values:
            print(f"{field.strip()}: {shield_reduce_values[field]}")

if __name__ == "__main__":
    main()