import subprocess

total_count = 100
priority_inversion_count = 0

for _ in range(total_count):
    calling_output = subprocess.check_output("taskset -c 1 ./test_pthread".split())

    output_list = calling_output.decode().strip().split("\n")
    if output_list[-1].startswith("Thread high func: execution"):
        priority_inversion_count += 1
print(f"{priority_inversion_count} / {total_count} = {priority_inversion_count/total_count}")