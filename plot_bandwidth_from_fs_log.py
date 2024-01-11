import sys
import matplotlib.style as mplstyle
import matplotlib.pyplot as plt
import json

mplstyle.use('fast')

if len(sys.argv) < 2:
    print("usage: plot_bandwidth_from_fs_log.py <log_file>")
    exit()

time = []
estimate = []
actual = []
smooth_bps = []
loss = []
bfcp_rtt = []
rtcp_rtt = []
min_rtt = []
receive_rate = []

idx = [
        {"Estimated": 5, "rtt_trend": 24, "time": -1, "estimate": 7, "actual_bps": 14, "receive_rate": 14, "loss": 17, "bfcp_rtt": 19, "rtcp_rtt": 21, "min_rtt": 23},
        {"Estimated": 7, "rtt_trend": 26, "time": 2, "estimate": 9, "actual_bps": 16, "receive_rate": 16, "loss": 19, "bfcp_rtt": 21, "rtcp_rtt": 23, "min_rtt": 25},
        {"Estimated": 7, "rtt_trend": 24, "time": 2, "estimate": 9, "actual_bps": 12, "receive_rate": 12, "loss": 15, "bfcp_rtt": 21, "rtcp_rtt": 21, "min_rtt": 21},
        {"Estimated": 7, "rtt_trend": 28, "time": 2, "estimate": 9, "actual_bps": 16, "receive_rate": 21, "loss": 19, "bfcp_rtt": 23, "rtcp_rtt": 25, "min_rtt": 27},
        ]

log_file = open(sys.argv[1], "r")
line = log_file.readline()
i = 0
while line != "":
    columns = line.split(" ")
    tp = -1
    if columns[4].startswith("loss_bwe"):
        tp = 2
    elif columns[20].startswith("receive_rate"):
        tp = 3
    elif columns[0].startswith("202"):
        tp = 0
    elif columns[1].startswith("202"):
        tp = 1
    if tp < 0:
        continue

    if len(columns) > 26 and columns[idx[tp]["Estimated"]] == "Estimated" and columns[idx[tp]["rtt_trend"]] == "rtt_trend":
        i += 1
        time.append(columns[idx[tp]["time"]][:11] if idx[tp]["time"] >= 0 else i)
        estimate.append(int(columns[idx[tp]["estimate"]][1:-2]))
        actual_bps = int(columns[idx[tp]["actual_bps"]][1:-2])
        receive_rate.append(int(columns[idx[tp]["receive_rate"]][1:-2]))
        actual.append(int(columns[idx[tp]["actual_bps"]][1:-2]))
        smooth_bps.append(actual_bps if len(smooth_bps) == 0 else (smooth_bps[-1]*0.9375+actual_bps*0.0625))
        loss.append(float(columns[idx[tp]["loss"]][1:-2]))
        bfcp_rtt.append(float(columns[idx[tp]["bfcp_rtt"]][1:-2]))
        rtcp_rtt.append(float(columns[idx[tp]["rtcp_rtt"]][1:-2]))
        min_rtt_value = columns[idx[tp]["min_rtt"]][1:-1]
        min_rtt_value = min_rtt_value[:-1] if min_rtt_value.endswith(']') else min_rtt_value
        min_rtt.append(float(min_rtt_value))

    line = log_file.readline()

print("lines: ", i)
data = []
for i in range(len(time)):
    m = {"time": time[i], "estimate": estimate[i], "actual": actual[i], "loss":loss[i], "bfcp_rtt":bfcp_rtt[i], "rtcp_rtt":rtcp_rtt[i]}
    data.append(m)
json_file = open(sys.argv[1].split(".")[0]+".json", "w")
json_file.write(json.dumps(data))
json_file.close()

fig, axs = plt.subplots(3,1, sharex=True)
axs[0].plot(time, estimate, time, actual, time, receive_rate)
axs[0].set_ylabel('bandwidth')
axs[0].legend(['estimate', 'actual', 'receive_rate'])

axs[1].plot(time, bfcp_rtt, time, rtcp_rtt, time, min_rtt)
axs[1].set_ylabel('rtt')
axs[1].legend(['bfcp', 'rtcp', "min"])

axs[2].plot(time, loss)
axs[2].set_ylabel('loss')

# plt.plot(time, estimate, time, actual, time, loss, time, bfcp_rtt, time, rtcp_rtt)
plt.xticks(range(1, len(time), 10), rotation=90)
plt.tick_params(axis='x', labelsize=6)
plt.show()
