import pandas as pd

def get_chunking_time():
    try:
        client_df = pd.read_csv('clientlog.csv')
        chunking_time_avg = client_df[" chunking time"].mean()
        return chunking_time_avg
    except Exception as e:
        print(f"Error processing clientlog.csv: {e}")
        return None

def get_breakdown_metrics():
    try:
        breakdown_df = pd.read_csv('breakdownlog.csv')
        last_row = breakdown_df.iloc[-1]
        
        metrics = {
            'dataTranTime(ms/MB)': last_row['dataTranTime(ms/MB)'],
            'total_dedupTime(ms/MB)': last_row['total_dedupTime(ms/MB)'],
            'sfTime(ms/MB)': last_row['sfTime(ms/MB)'],
            'deltacompressTime(ms/MB)': last_row['deltacompressTime(ms/MB)'],
            'checkTime(ms/MB)': last_row['checkTime(ms/MB)'],
            'lz4compressTime(ms/MB)': last_row['lz4compressTime(ms/MB)'],
            'encTime(ms/MB)': last_row['encTime(ms/MB)']
        }
        
        return metrics
    except Exception as e:
        print(f"Error processing breakdownlog.csv: {e}")
        return None

def main():
    chunking_time = get_chunking_time()
    
    breakdown_metrics = get_breakdown_metrics()
    
    if chunking_time is not None and breakdown_metrics is not None:
        print(f"Chunking: {chunking_time} ms/MB")
        print(f"Secure session setup: {breakdown_metrics['dataTranTime(ms/MB)']} ms/MB")
        print(f"Deduplication: {breakdown_metrics['total_dedupTime(ms/MB)']} ms/MB")
        print(f"Feature generation: {breakdown_metrics['sfTime(ms/MB)']} ms/MB")
        print(f"Locality detection: {breakdown_metrics['checkTime(ms/MB)']} ms/MB")
        print(f"Delta compression: {breakdown_metrics['deltacompressTime(ms/MB)']} ms/MB")
        print(f"Local compression: {breakdown_metrics['lz4compressTime(ms/MB)']} ms/MB")
        print(f"Encryption: {breakdown_metrics['encTime(ms/MB)']} ms/MB")

if __name__ == "__main__":
    main()