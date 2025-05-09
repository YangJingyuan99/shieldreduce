# AE Detailed Instructions
Here are the detailed instructions to perform the same experiments in our paper.

## Artifact claims
We claim that **the resultant numbers might be different** from in our paper due to various factors (e.g., different machines, different OS, different software packages...). Nevertheless, we expect that ShieldReduce should still outperform the baseline approaches in terms of performance and storage efficiency.

Also, to ensure the results are stable, you may need to disable the swap space in your machine.

## Storage Efficiency
**Note that**: before starting every experiment, please ensure that `./Prototype/config.json` is correctly configured.

**Note that**: please ensure that backups are uploaded in the same order as in `./ClientScript/BackupOrder/` to ensure that the results of the test are consistent with those in our paper.

### Exp#1 (Analysis of data reduction)

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/default/reset_default.sh
$ bash ./setup.sh
```

In the client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
$ cd ./Prototype && bash ./setup.sh && cd ..
```

**Note that**: the compilation on the client machine still requires the machine to have the SGX environment, although the client does not need SGX to execute. For convenience, you can complete the compilation on the server machine and copy the `ShieldReduceClient` and `./key/` to the client machine if the client machine and the server machine have identical hardware and software configurations (except for SGX)

- **Step-2: evaluate ShieldReduce**

In the storage server machine:

```shell
# start the storage server
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 16:27:07 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 16:27:09 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# **Ensure all corresponding datasets are ready in the correct path**
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/server-log ./Result/exp1/ShieldReduce_serverlog.csv
```

- **Step-3: evaluate baselines**

In the storage server machine:

```shell
# reset
$ cd ./Prototype && bash ./recompile.sh
# execute baselines via setting parameter m with different value
# Option for m: 0-SecureMeGA, 1-DEBE, 2-ForwardDelta, 4-ShieldReduce
# Take SecureMeGA as example
$ cd ./bin && ./ShieldReduceServer -m 0
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/server-log ./Result/exp1/{baseline name}_serverlog.csv
```

**Note that**: you need to reset the storage server before each test.

- **Step-4: evaluate ShieldReduce with different $\alpha$**

In the storage server machine:

Change the parameter $\alpha$ in `./Prototype/include/constVar.h`. In our paper, we evaluated the compression ratio of ShieldReduce when $\alpha$ = 0(default), 0.5, 0.7 and 1.
```c
...
#define GREEDY_THRESHOLD 0.0
...
```

Then, perform the following operations:

```shell
# reset
$ cd ./Prototype && bash ./recompile.sh
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/server-log ./Result/exp1/a{alpha value, e.g., 07}_serverlog.csv
```

**Note that**: also, you need to reset the storage server before each test.

- **Step-5: check the result**

In the storage server machine:
```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp1
# check the result, take ShieldReduce_serverlog.csv as an example 
$ cat ./ShieldReduce_serverlog.csv
BackupID, OnlineSpeed(MB/s), Encalve ProcessTime(s), BackupSize, OnlineSize, OfflineSize, OfflineTime(s), OverallReductionRatio
0, 87.0193, 2.28229, 208250880, 100655456, 100655456, 0.000292, 2.06895
1, 125.14, 1.62116, 212725760, 132196507, 132196507, 0.000291, 3.18448
2, 131.67, 1.58933, 219432960, 161329521, 159613353, 1.1541, 4.01226
3, 124.811, 1.71227, 224092160, 194834384, 186673218, 4.86421, 4.6311
...
```

You can obtain the data reduction ratio from `OverallReductionRatio` of the last entry.

Can see:
- ShieldReduce has a similar data reduction ratio as ForwardDelta;
- ShieldReduce has a higher data reduction ratio than SecureMeGA and DEBE;
- The data reduction ratio of ShieldReduce generally decreases with $\alpha$.
  
**Note that:** due to the limited number of versions, the effect of $\alpha$ may not be significant.

- **Step-6: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```

## Performance

### Exp#2 (Microbenchmarks)

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/exp2/reset_exp2.sh
$ bash ./setup.sh
```

In the client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
$ cd ./Prototype && bash ./setup.sh && cd ..
```

**Note that**: the compilation on the client machine still requires the machine to have the SGX environment, although the client does not need SGX to execute. For convenience, you can complete the compilation on the server machine and copy the ShieldReduceClient to the client machine if the client machine and the server machine have identical hardware and software configurations (except for SGX)

- **Step-2: evaluate ShieldReduce**

In the storage server machine:

```shell
# start the storage server
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 16:27:07 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 16:27:09 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# **Ensure all corresponding datasets are ready in the correct path**
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result, note that: if you want to test multiple times, be careful not to let the new results overwrite the old ones (e.g., by setting a test ID to differentiate between different results)
$ cp ./Prototype/bin/breakdown-log ./Result/exp2/breakdownlog{test ID}.csv
```

- **Step-3: Check the result**

In the storage server machine:
```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp2
# check the result
$ cat ./breakdownlog{test ID}.csv
Backup ID,Backup Size(MB),dataTranTime(us/MB),fpTime(ms/MB),freqTime(ms/MB),firstDedupTime(ms/MB),secondDedupTime(ms/MB),total_dedupTime(ms/MB),sfTime(ms/MB),checkTime(ms/MB),lz4compressTime(ms/MB),deltacompressTime(ms/MB),encTime(ms/MB),Total size(MB)
0,198.604,0.318514,1.06632,0.0404571,0.0691579,1.02415,2.20008,9.27286,0.114494,2.54423,0.853112,0.399887,198.603516
1,202.871,0.333931,1.09114,0.0440544,0.0797983,0.846896,2.06189,7.61516,0.137421,1.63697,1.01287,0.302754,401.474609
2,209.268,0.32714,1.08181,0.0442516,0.0833854,0.782329,1.99178,6.99872,0.150227,1.30392,1.09143,0.265204,610.742188
3,213.711,0.332989,1.08945,0.04205,0.0859491,0.734886,1.95234,6.63841,0.155303,1.1789,1.11058,0.246929,824.453125
...
```
  
**Note that**: you need to reset the storage server before each test.

- **Step-4: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```

### Exp#3 (Inline performance)

The testing process for Exp#3 is exactly the same as for Exp#1, you just need to avoid testing ShieldReduce that with different $\alpha$ (step-4).

You can obtain the upload speed from `OnlineSpeed(MB/s)` of each entry in `serverlog.csv`:

```shell
BackupID, OnlineSpeed(MB/s), Encalve ProcessTime(s), BackupSize, OnlineSize, OfflineSize, OfflineTime(s), OverallReductionRatio
0, 87.0193, 2.28229, 208250880, 100655456, 100655456, 0.000292, 2.06895
1, 125.14, 1.62116, 212725760, 132196507, 132196507, 0.000291, 3.18448
2, 131.67, 1.58933, 219432960, 161329521, 159613353, 1.1541, 4.01226
3, 124.811, 1.71227, 224092160, 194834384, 186673218, 4.86421, 4.6311
...
```

Can see:
- DEBE is faster than other approaches;
- ShieldReduce is faster than ForwardDelta;
- The speeds of ShieldReduce and SecureMeGA are similar.

### Exp#4 (Multi-client performance)

**Note that**: you need to deploy multiple clients in different machines and each client has a unique id. Please ensure that `./MultiClient/config.json` is correctly configured.

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./MultiClient
$ bash ./setup.sh
```

In each client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
# create unique file
$ dd if=/dev/urandom of=u{client id} bs=1M count=2048
# copy the data file into the ramdisk (take Redundant as an example)
# **Ensure that each client uploads files with different names**
$ cp ./u{client id} ./ram-client/
```

To reproduce the *redundant* result, you can directly copy simos into ramdisk by:

```shell
$ cp ./Dataset/redundant/vm{client id}-1 ./ram-client/
```

- **Step-2: evaluate the upload speed**

In the storage server machine:

```shell
# start the storage server
$ cd ./bin && ./ShieldReduceServer -m 2
2025-05-07 16:27:07 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 16:27:09 <ShieldReduceServer>: waiting the request from the client.
```

In each client machine:

```shell
# upload data file
$ cd ./MultiClient/bin/ && ./ShieldReduceClient -t o -i ../../ram-client/u{client id}
```

**Note that:** you need to perform uploading in each client at the same time. You can use a script to send the command to all clients or use multiple terminals to send it. In `./ClientScript/`, we created two scripts examples `multiUp_Unique.sh` and `multiDown_Unique.sh` to control 5 client machines for reference.

```shell
# upload file from each client at the same time
$ cd ./ClientScript && bash ./multiUp_Unique.sh
...
========DataSender Info========
total send batch num: 2098
total thread running time: 51.606819
===============================
2025-05-08 15:05:59 <ShieldReduceClient>: total running time: 51.606960
Execution time: 52.438 seconds
```

In the above example, the aggreated upload speed is 2048 * 5 / 52.438 = 195.278 MiB/s

- **Step-3: evaluate the download speed**

**Note that:** do **NOT** stop the storage server after step-2

```shell
# download files from the storage server to each client at the same time
$ cd ./ClientScript && bash ./multiDown_Unique.sh
...
========RestoreWriter Info========
total thread running time: 58.094946
write chunk num: 268719
write data size: 2147483648
==================================
2025-05-08 15:07:56 <ShieldReduceClient>: total running time: 58.095134
Execution time: 58.451 seconds
```

In the above example, the aggreated download speed is 2048 * 5 / 58.451 = 175.189 MiB/s

- **Step-4: clean up**

In each client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```

### Exp#5 (Sensitivity of configurable parameters)

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/default/reset_default.sh
$ bash ./setup.sh
```

In the client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
$ cd ./Prototype && bash ./setup.sh && cd ..
```

**Note that**: the compilation on the client machine still requires the machine to have the SGX environment, although the client does not need SGX to execute. For convenience, you can complete the compilation on the server machine and copy the ShieldReduceClient to the client machine if the client machine and the server machine have identical hardware and software configurations (except for SGX)


- **Step-2: evaluate ShieldReduce with different $t$**

In the storage server machine:

Change the parameter $t$ in `./Prototype/include/constVar.h`. In our paper, we evaluated the compression ratio of ShieldReduce when $t$ = 0, 0.01, 0.02, ..., 0.1.
```c
...
#define OFFLINE_THRESHOLD 0.03
...
```

Then, perform the following operations:

```shell
# reset
$ cd ./Prototype && bash ./recompile.sh
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/server-log ./Result/exp5/t{t value, e.g., 003}_serverlog.csv
```

**Note that:** you need to reset the storage server before each test.

- **Step-3: check the result**

In the storage server machine:
```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp5
# check the result, take t003_serverlog.csv as an example 
$ cat ./t003_serverlog.csv
BackupID, OnlineSpeed(MB/s), OnlineTime(s), AverageOnlineTime(s), BackupSize, OnlineSize, OfflineSize, OfflineTime(s), AverageOfflineTime(s), OverallReductionRatio
0, 80.172, 2.47722, 2.47722, 208250880, 100655456, 100655456, 0.00039, 0.00039, 2.06895
1, 124.434, 1.63035, 2.05378, 212725760, 132196507, 132196507, 0.000373, 0.0003815, 3.18448
2, 125.186, 1.67166, 1.92641, 219432960, 161329521, 159613353, 1.20505, 0.401937, 4.01226
3, 125.965, 1.69659, 1.86895, 224092160, 194834384, 186673218, 5.18774, 1.59839, 4.6311
...
```

You can obtain the average online (offline) time from the `AverageOnlineTime(s)` (`AverageOfflineTime(s)`) of the last entry.

Can see:
-  As $t$ increases, ShieldReduce takes more (less) time in the inline (offline) stage.

- **Step-4: evaluate ShieldReduce with different $\alpha$**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/default/reset_default.sh
```

Change the parameter $\alpha$ in `./Prototype/include/constVar.h`. In our paper, we evaluated the compression ratio of ShieldReduce when $\alpha$ = 0, 0.5, 0.7, 0.9 and 1.

```c
...
#define GREEDY_THRESHOLD 0.00
...
```

Re-compile and execute:

```shell
$ bash ./recompile.sh
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```
In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/server-log ./Result/exp5/a{alpha value, e.g., 07}_serverlog.csv
```

**Note that:** you need to reset the storage server before each test.

- **Step-5: check the result**
In the storage server machine:
```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp5
# check the result, take a07_serverlog.csv as an example 
$ cat ./a07_serverlog.csv
BackupID, OnlineSpeed(MB/s), OnlineTime(s), AverageOnlineTime(s), BackupSize, OnlineSize, OfflineSize, OfflineTime(s), AverageOfflineTime(s), OverallReductionRatio
0, 80.172, 2.47722, 2.47722, 208250880, 100655456, 100655456, 0.00039, 0.00039, 2.06895
1, 124.434, 1.63035, 2.05378, 212725760, 132196507, 132196507, 0.000373, 0.0003815, 3.18448
2, 125.186, 1.67166, 1.92641, 219432960, 161329521, 159613353, 1.20505, 0.401937, 4.01226
3, 125.965, 1.69659, 1.86895, 224092160, 194834384, 186673218, 5.18774, 1.59839, 4.6311
...
```

You can obtain the average offline time from the `AverageOfflineTime(s)` of the last entry.

Can see:
-  As $\alpha$ increases, ShieldReduce takes less time in the offline stage.

- **Step-6: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```

## Resource Overhead

### Exp#6 (Index overhead)

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/default/reset_default.sh
$ bash ./setup.sh
```

In the client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
$ cd ./Prototype && bash ./setup.sh && cd ..
```

**Note that**: the compilation on the client machine still requires the machine to have the SGX environment, although the client does not need SGX to execute. For convenience, you can complete the compilation on the server machine and copy the ShieldReduceClient to the client machine if the client machine and the server machine have identical hardware and software configurations (except for SGX)


- **Step-2: evaluate ShieldReduce with different $\alpha$**

In the storage server machine:

Change the parameter $\alpha$ in `./Prototype/include/constVar.h`. In our paper, we evaluated the compression ratio of ShieldReduce when $\alpha$ = 0(default), 0.5, 0.7, 0.9 and 1.

```c
...
#define GREEDY_THRESHOLD 0.00
...
```

Then, perform the following operations:

```shell
# reset
$ cd ./Prototype && bash ./setup.sh
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/indexsize-log ./Result/exp6/a{alpha value, e.g., 07}_indexsizelog.csv
```

**Note that:** you need to reset the storage server before each test.

- **Step-3: check the result**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp6
# check the result, take a0_indexsizelog.csv as an example 
$ cat ./a0_indexsizelog.csv
Backup ID, FPindex, FPindex (%), SFindex, SFindex (%), Deltaindex, Deltaindex (%)
0,1959600,0.94098,4687040,2.25067,1728,0.000829768
1,3170080,0.75303,6024896,1.43117,522144,0.124032
2,4374240,0.683038,7179968,1.12115,938976,0.146621
3,5549920,0.641979,8282048,0.958014,1321216,0.15283
...
```

You can obtain the fraction of index size over logical size from the `FPindex (%)`, `SFindex (%)` and `Deltaindex (%)` of the last entry.

Can see:
-  As $\alpha$ increases, the logical size of delta index takes decreases.

- **Step-4: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```

### Exp#7 (Enclave overhead)

- **Step-1: preparation**

In the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
# Initialize compilation
$ cd ./Prototype
$ bash ./script/default/reset_default.sh
$ bash ./setup.sh
```

In the client machine:

```shell
# **Ensure you are in the root path of the repository**
# prepare a 10GiB ramdisk in "~/ram-client/"
$ mkdir -p ./ram-client/
$ sudo mount -t tmpfs -o rw,size=10G tmpfs ./ram-client/
$ cd ./Prototype && bash ./setup.sh && cd ..
```

**Note that**: the compilation on the client machine still requires the machine to have the SGX environment, although the client does not need SGX to execute. For convenience, you can complete the compilation on the server machine and copy the ShieldReduceClient to the client machine if the client machine and the server machine have identical hardware and software configurations (except for SGX)

- **Step-2: evaluate ShieldReduce**

In the storage server machine:

```shell
# start the storage server
$ cd ./bin && ./ShieldReduceServer -m 4
2025-05-07 16:27:07 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 16:27:09 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# **Ensure all corresponding datasets are ready in the correct path**
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/sgx-log ./Result/exp7/ShieldReduce_sgxlog.csv
$ cp ./Prototype/bin/reduction-log ./Result/exp7/ShieldReduce_reductionlog.csv
```

- **Step-3: evaluate baselines**

In the storage server machine:

```shell
# reset
$ cd ./Prototype && bash ./recompile.sh
# execute baselines via setting different parameter m
# 1: DEBE, 2: ForwardDelta, 3: without offloading, 4: ShieldReduce (with offloading)
# take DEBE as example
$ cd ./bin && ./ShieldReduceServer -m 1
2025-05-07 22:52:42 <ShieldReduceServer>: In GC, cloud will merge base container
...
...
2025-05-07 22:52:44 <ShieldReduceServer>: waiting the request from the client.
```

In the client machine (take **Linux** as an example to demonstrate the workflow, you can evaluate different datasets by performing different script in `./ClientScript`):

```shell
# Upload ten backups sequentially
$ bash ./ClientScript/linuxUp.sh
2025-05-07 15:42:29 <ShieldReduceClient>: offline
...
...
========DataSender Info========
total send batch num: 197
total thread running time: 1.810931
===============================
```

Save the result in the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ../../
# save the result
$ cp ./Prototype/bin/sgx-log ./Result/exp7/{baseline name}_sgxlog.csv
$ cp ./Prototype/bin/reduction-log ./Result/exp7/{baseline name}_reductionlog.csv
```

**Note that:** you need to reset the storage server before each test.

- **Step-4: check the result**

For the result of enclave overhead, in the storage server machine:

```shell
# **Ensure you are in the root path of the repository**
$ cd ./Result/exp7
# check the result in sgxlog.csv, take ShieldReduce_sgxlog.csv as an example 
$ cat ./ShieldReduce_sgxlog.csv
Backup ID, Inline Ecall, Index queries Ocall, Index updates Ocall, Data transfers Ocall, Total Inline Ocall, Offline Ecall, Offline_Ocall, _Inline_FPOcall, _Inline_SFOcall, _Inline_LocalOcall, _Inline_LoadOcall, _Inline_DeltaOcall, _Inline_RecipeOcall, _Inline_Write_ContainerOcall, _inline_have_similar_chunk_num, _inline_need_load_container_num, _inlineDeltaChunkNum, 
0,195,386,19,85,490,1,5,193,193,1,34,18,24,27,64,33,34
1,394,780,217,149,1146,2,10,390,390,2,65,215,48,36,8227,276,8190
2,599,1186,421,223,1830,3,15,593,593,3,105,418,73,45,17259,727,17219
3,808,1600,629,303,2532,4,20,800,800,4,151,625,98,54,26191,1378,26149
...
```

You can obtain the number of *ECalls* and *OCalls* from the `Inline Ecall`, `Index queries Ocall`, `Index updates Ocall`, `Data transfers Ocall`, `Total Inline Ocall`, `Offline Ecall` and `Offline_Ocall` of the last entry.

For the result of average amount of data reduced by delta compression per OCall:
```shell
# check the result in reductionlog.csv, take ShieldReduce_reductionlog.csv as an example 
$ cat ./ShieldReduce_reductionlog.csv
Backup ID, Online Delta_save, Online Delta_save per OCall, Offline Delta_save, Offline delta time, Offline dedelta time, Offline delete time, Offline Delta chunk num
0,216908,2.03677,0,0,0,0,0
1,74438321,198.617,0,0,0,0,0
2,153174818,236.311,3940843,771,328,22,443
3,214780859,240.26,24526080,5454,2766,71,2688
...
```

You can obtain the result from the `Online Delta_save per OCall` of the last entry.

Can see:
-  ShieldReduce (with offloading) has more data reduction per OCall compared to without offloading.

**Note that:** Due to the limited versions of backup, the result of ShieldReduce (with offloading) may not much better than without offloading.

- **Step-5: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./ram-client/
```