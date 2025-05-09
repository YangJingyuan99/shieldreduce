start_time=$(date +%s%N)
ssh root@172.28.114.116 "cd /root/client/bin/ && ./ShieldReduceClient -t o -i /root/client/ram-client/u1" &
ssh root@172.28.114.113 "cd /root/client/bin/ && ./ShieldReduceClient -t o -i /root/client/ram-client/u2" &
ssh root@172.28.114.115 "cd /root/client/bin/ && ./ShieldReduceClient -t o -i /root/client/ram-client/u3" &
ssh root@172.28.114.114 "cd /root/client/bin/ && ./ShieldReduceClient -t o -i /root/client/ram-client/u4" &
ssh root@172.28.114.117 "cd /root/client/bin/ && ./ShieldReduceClient -t o -i /root/client/ram-client/u5" &
wait
end_time=$(date +%s%N)

elapsed_time=$((end_time - start_time))
elapsed_seconds=$(echo "scale=3; $elapsed_time / 1000000000" | bc)
echo "Execution time: ${elapsed_seconds} seconds"

