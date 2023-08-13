from statistics import mean, median

import matplotlib
import matplotlib.pyplot as plt

# sudo pacman -S pyqt5
# sudo pacman -S tk
matplotlib.use( 'TkAgg' )

insertion_times = []
searching_times = []
remove_times = []

data_path = "random_1M.data"
data_pic_path = f"./test_data/{data_path}.png"
full_data_path = f"./test_data/{data_path}"

with open(full_data_path, "r") as f:
    for line in f.readlines():
        if line.startswith("Insertion Time"):
            interval_time = float(line.strip().split(" ")[2])
            insertion_times.append(interval_time)
        elif line.startswith("Searching Time"):
            interval_time = float(line.strip().split(" ")[2])
            searching_times.append(interval_time)
        elif line.startswith("Remove Time"):
            interval_time = float(line.strip().split(" ")[2])
            remove_times.append(interval_time)

insertion_avg_value = mean(insertion_times)
insertion_median_value = median(insertion_times)
print(f"Insertion Time:\n"
      f"AVG: {insertion_avg_value:.8f}\n"
      f"Median: {insertion_median_value:.8f}\n")
searching_avg_value = mean(searching_times)
searching_median_value = median(searching_times)
print(f"Searching Time:\n"
      f"AVG: {searching_avg_value:.8f}\n"
      f"Median: {searching_median_value:.8f}\n")
remove_avg_value = mean(remove_times)
remove_median_value = median(remove_times)
print(f"Remove Time:\n"
      f"AVG: {remove_avg_value:.8f}\n"
      f"Median: {remove_median_value:.8f}\n")

fig, axs = plt.subplots(3)
fig.suptitle(f'{data_path}', fontsize=16)
insertion_f = axs[0]
searching_f = axs[1]
remove_f = axs[2]

insertion_f.set_title("Insertion") # title
insertion_f.set(xlabel='times', ylabel='interval (sec)')
insertion_f.plot(range(len(insertion_times)), insertion_times)

searching_f.set_title("Searching") # title
searching_f.set(xlabel='times', ylabel='interval (sec)')
searching_f.plot(range(len(searching_times)), searching_times)

remove_f.set_title("Remove") # title
remove_f.set(xlabel='times', ylabel='interval (sec)')
remove_f.plot(range(len(remove_times)), remove_times)
plt.savefig(data_pic_path)
