import os
import re
import pandas as pd
import matplotlib.pyplot as plt
import glob
import sys

# Function to extract speed data from csv files
def extract_speed_data(file_path):
    """
    Extract OnlineSpeed(MB/s) data from CSV file
    Args:
        file_path: Path to the csv file
    Returns:
        List of speed values
    """
    try:
        # Read the CSV file
        df = pd.read_csv(file_path)
        
        # Find the column with " OnlineSpeed(MB/s)" (note the leading space)
        speed_column = [col for col in df.columns if " OnlineSpeed(MB/s)" in col]
        
        if not speed_column:
            print(f"Warning: No ' OnlineSpeed(MB/s)' column found in {file_path}")
            return []
        
        # Extract the speed values
        speed_values = df[speed_column[0]].tolist()
        return speed_values
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return []

# Function to get legend name from filename
def get_legend_name(filename):
    """
    Extract the legend name from the filename
    Example: "ShieldReduce_serverlog.csv" -> "ShieldReduce"
    """
    # Extract the part before '_serverlog.csv'
    match = re.match(r"(.+)_serverlog\.csv", filename)
    if match:
        return match.group(1)
    return os.path.basename(filename)  # Fallback to filename if pattern doesn't match

# Main function
def main():
    """
    Main function to process CSV files and create the line chart
    """
    
    color_map = {
        "ShieldReduce": "#AD0626",
        "DEBE": "#B79AD1",
        "SecureMeGA": "#F2BE5C",
        "ForwardDelta": "#75B8BF"
    }

    # Check if parameter is provided
    if len(sys.argv) < 2:
        print("Usage: python script.py <parameter>")
        print("Example: python script.py linux")
        return
    
    # Get parameter for output filename
    parameter = sys.argv[1]
    output_file = f"Exp3-{parameter}.png"
    
    # Find all *_serverlog.csv files in the current directory
    csv_files = glob.glob("*_serverlog.csv")
    
    if not csv_files:
        print("No *_serverlog.csv files found in the current directory")
        return
    
    # print(f"Found {len(csv_files)} CSV files: {', '.join(csv_files)}")
    
    # Set font sizes
    SMALL_SIZE = 20
    MEDIUM_SIZE = 20
    LARGE_SIZE = 20
    
    # Apply font sizes to all elements
    plt.rc('font', size=SMALL_SIZE)          # Default text sizes
    plt.rc('axes', titlesize=MEDIUM_SIZE)     # Axes title
    plt.rc('axes', labelsize=LARGE_SIZE)      # X and Y labels
    plt.rc('xtick', labelsize=MEDIUM_SIZE)    # X tick labels
    plt.rc('ytick', labelsize=MEDIUM_SIZE)    # Y tick labels
    plt.rc('legend', fontsize=LARGE_SIZE)     # Legend
    plt.rc('figure', titlesize=LARGE_SIZE)    # Figure title
    
    # Prepare the plot with a white background
    plt.figure(figsize=(12, 8), facecolor='white')
    ax = plt.subplot(111)
    ax.set_facecolor('white')
    
    # Process each file
    for file_path in csv_files:
        # Extract speed data
        speed_values = extract_speed_data(file_path)
        
        if speed_values:
            # Create x-axis values (snapshots)
            snapshots = list(range(1, len(speed_values) + 1))
            
            # Get legend name
            legend_name = get_legend_name(os.path.basename(file_path))
            
            line_color = color_map.get(legend_name, None)
            
            if line_color:
                plt.plot(snapshots, speed_values, linestyle='-', label=legend_name, 
                         linewidth=3.5, color=line_color)
            else:
                plt.plot(snapshots, speed_values, linestyle='-', label=legend_name, 
                         linewidth=3.5)
            
    
    # Set plot labels and title
    plt.xlabel('Snapshot', fontsize=LARGE_SIZE, labelpad=10)
    plt.ylabel('Speed (MiB/s)', fontsize=LARGE_SIZE, labelpad=10)
    plt.title(f'Online Speed Comparison - {parameter}', fontsize=LARGE_SIZE+2)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Create legend with larger font size
    legend = plt.legend(prop={'size': LARGE_SIZE})
    
    # Add some padding around the figure
    plt.tight_layout(pad=2.0)
    
    # Save the plot with transparent=False to ensure white background
    plt.savefig(output_file, facecolor='white', bbox_inches='tight', transparent=False, dpi=300)
    print(f"Plot saved as {output_file}")
    
    # Show the plot
    plt.show()

if __name__ == "__main__":
    main()