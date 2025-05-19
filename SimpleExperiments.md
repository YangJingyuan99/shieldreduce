# Evaluation on Our Hardware
Here are the instructions to perform the same experiments in our paper **Quickly**. 
- If you are using the machines we provide, then you can run the scripts in `./ServerScript` directly to reproduce experiments.
- If you are using your own configured machine, scripts will not work directly. Nevertheless, we believe that the scripts we provide are still usable, only requiring you to make slight changes to the scripts (e.g. the IP addresses that appear in the scripts)

**Note that:** In order to fully reproduce the data in our paper, you would need to process **hundreds** of versions of backups for each dataset, which would **take a very large amount of time**. For example, processing 209 versions of Linux backups once would take about 30 hours (you would also need to test baselines and ShieldReduce with different parameters, which means an experiment would take hundreds of hours...). **Therefore, the script we provide will only process 10 versions of backups, which will lead to a discrepancy between the numbers and that presented in our paper on some experiments. Nevertheless, we believe that the data obtained from the short test will still reflect the same trends as the data in our paper and will not affect the correctness of the conclusions in our paper.**

## Storage Efficiency

### Exp#1 (Analysis of data reduction)
Runtime estimation:
- Linux: about 15 min;
- Docker: about 15 min;
- SimOS: about 90 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp1/exp1.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp1/exp1.sh docker
```

Once completed, you can get the data from the terminal:
```
# 'a07' means the result of alpha=0.7
------------------------
Exp#1: overall data reduction ratio (dataset: docker)
------------------------
DEBE: 3.0695
ShieldReduce: 4.08434
a05: 4.08365
a07: 4.02177
a10: 3.71886
SecureMeGA: 4.01989
ForwardDelta: 4.09648
------------------------
```

Can see:
- ShieldReduce has a similar data reduction ratio as ForwardDelta;
- ShieldReduce has a higher data reduction ratio than SecureMeGA and DEBE;
- The data reduction ratio of ShieldReduce generally decreases with $\alpha$.

## Performance

### Exp#2 (Microbenchmarks)
Runtime estimation:
- Linux: about 5 min;
- Docker: about 5 min;
- SimOS: about 20 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp2/exp2.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp2/exp2.sh docker
```

Once completed, you can get the data from the terminal:
```
------------------------
Exp#2: microbenchmarks (dataset: docker)
------------------------
Chunking: 0.852017 ms/MB
Secure session setup: 0.20652 ms/MB
Deduplication: 1.54624 ms/MB
Feature generation: 5.26728 ms/MB
Locality detection: 0.0961592 ms/MB
Delta compression: 0.723937 ms/MB
Local compression: 0.820704 ms/MB
Encryption: 0.185165 ms/MB
------------------------
```

### Exp#3 (Inline performance)
Runtime estimation:
- Linux: about 10 min;
- Docker: about 10 min;
- SimOS: about 60 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp3/exp3.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp3/exp3.sh docker
```

Once completed, you can see the following output and get the figure from `./Result/exp3/Exp3-{dataset name}.png`:
```
------------------------
Exp#3: inline performance (dataset: docker)
------------------------
Plot saved as Exp3-docker.png
------------------------
```

Can see:
- DEBE is faster than other approaches;
- ShieldReduce is faster than ForwardDelta;
- The speeds of ShieldReduce and SecureMeGA are similar.

### Exp#4 (Multi-client performance)
Runtime estimation:
- Unique: about 5 min;
- Redundant: about 10 min;

**Note that:** due to the number of machines, we only tested the performance of one and two clients uploading and downloading at the same time.

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp4/exp4.sh <dataset name>, you can choose: 'unique', 'redundant'.
$ bash ./ServerScript/exp4/exp4.sh unique
```

Once completed, you can get the result from `./Result/exp4/result`:
```
------------------------
Exp#4: multi-client performance (dataset: unique)
------------------------
One Client
Upload execution time: 27.496 seconds
Aggregate upload speed: 74.484 MiB/s
Download execution time: 23.164 seconds
Aggregate download speed: 88.411 MiB/s
------------------------
------------------------
Two Clients
Upload execution time: 28.328 seconds
Aggregate upload speed: 144.59 MiB/s
Download execution time: 23.169 seconds
Aggregate download speed: 176.78 MiB/s
------------------------
```

### Exp#5 (Sensitivity of configurable parameters)

**Note that:** due to the limited number of versions, the impact of $t$ on inline performance is limited.  

Runtime estimation:
- Linux: about 45 min;
- Docker: about 45 min;
- SimOS: about 180 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp5/exp5.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp5/exp5.sh docker
```

Once completed, you can see the following output and get the figure from `./Result/exp5/impact_t_{dataset name}.png` and `./Result/exp5/impact_alpha_{dataset name}.png`:
```
------------------------
Exp#5: sensitivity of configurable parameters (dataset: docker)
------------------------
Extracted from a00_serverlog.csv: AverageOfflineTime = 3.31315 seconds
Extracted from a05_serverlog.csv: AverageOfflineTime = 3.36803 seconds
Extracted from a07_serverlog.csv: AverageOfflineTime = 2.98712 seconds
Extracted from a09_serverlog.csv: AverageOfflineTime = 1.59908 seconds
Extracted from a10_serverlog.csv: AverageOfflineTime = 0.0328414 seconds
Bar chart has been created and saved as 'impact_alpha_docker.png'
Processed t00_serverlog.csv: x=0.0, offline=15.2338, inline=1.98205
Processed t001_serverlog.csv: x=0.01, offline=11.2971, inline=2.01413
Processed t009_serverlog.csv: x=0.09, offline=0.0674066, inline=2.03759
Processed t007_serverlog.csv: x=0.07, offline=0.156601, inline=2.04765
Processed t004_serverlog.csv: x=0.04, offline=0.306945, inline=2.05218
Processed t005_serverlog.csv: x=0.05, offline=0.210373, inline=2.05046
Processed t003_serverlog.csv: x=0.03, offline=3.29613, inline=2.21088
Processed t008_serverlog.csv: x=0.08, offline=0.114065, inline=2.03818
Processed t006_serverlog.csv: x=0.06, offline=0.186803, inline=2.06455
Processed t002_serverlog.csv: x=0.02, offline=6.87465, inline=2.04493
Processed t01_serverlog.csv: x=0.1, offline=0.0671348, inline=2.03095
Plot saved as impact_t_docker.png
------------------------
```

Can see:
-  As $t$ increases, ShieldReduce takes more (less) time in the inline (offline) stage.
-  As $\alpha$ increases, ShieldReduce takes less time in the offline stage.

### Exp#6 (Index overhead)
**Note that:** due to the limited number of versions, the feature index overhead may be higher, since the base chunk takes a larger percentage in unique chunks.

Runtime estimation:
- Linux: about 15 min;
- Docker: about 15 min;
- SimOS: about 90 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp6/exp6.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp6/exp6.sh docker
```

Once completed, you can see the following output and get the figure from `./Result/exp5/impact_alpha_{dataset name}.png` and `./Result/exp5/index_overhead_{dataset name}.png`:

```
------------------------
Exp#6: analysis of index overhead (dataset: docker)
------------------------
File: a00_indexsizelog.csv, Alpha: 0.0, Delta Index: 0.0763432
File: a05_indexsizelog.csv, Alpha: 0.5, Delta Index: 0.0763432
File: a07_indexsizelog.csv, Alpha: 0.7, Delta Index: 0.0720723
File: a09_indexsizelog.csv, Alpha: 0.9, Delta Index: 0.0571937
File: a10_indexsizelog.csv, Alpha: 1.0, Delta Index: 0.0449739
Chart saved as impact_alpha_docker.png
Bar chart saved as impact_t_docker.png
------------------------
```

Can see:
-  As $\alpha$ increases, the logical size of delta index decreases.

### Exp#7 (Enclave overhead)

**Note that:** due to the limited number of versions, the gain from "with offloading" design is limited.

Runtime estimation:
- Linux: about 15 min;
- Docker: about 15 min;
- SimOS: about 60 min;

Run the following command:

```shell
# **Ensure you are in the root path of the repository**
# bash ./ServerScript/exp7/exp7.sh <dataset name>, you can choose: 'linux', 'docker' or 'simos'.
$ bash ./ServerScript/exp7/exp7.sh docker
```

Once completed, you can get the data from the terminal:

```
------------------------
Exp#7: enclave overhead (dataset: docker)
------------------------
Enclave overhead
DEBE values:
Inline Ecall: 1838
Index queries Ocall: 1622
Data transfers Ocall: 366
Total Inline Ocall: 1988

ForwardDelta values:
Inline Ecall: 1838
Index queries Ocall: 3244
Data transfers Ocall: 997
Total Inline Ocall: 5493

ShieldReduce values:
Inline Ecall: 1838
Index queries Ocall: 3244
Index updates Ocall: 1150
Data transfers Ocall: 972
Total Inline Ocall: 5366
Offline Ecall: 5
Offline_Ocall: 233168
------------------------
Average amount of data reduced by delta compression per OCall for data transfers and index updates (KiB) with and without delta compression offloading in ShieldReduce
------------------------
Without Offloading: 84.8189
With Offloading: 84.9261
------------------------
```

Can see:
-  ShieldReduce (with offloading) has more data reduction per OCall compared to without offloading.