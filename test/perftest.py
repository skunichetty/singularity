import math
import pathlib
import subprocess
import sys
import time

NUM_EXECUTIONS = 30


class Timer:
    def __enter__(self):
        self.start = time.perf_counter_ns()
        self.end = None
        self.elapsed = None

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.end = time.perf_counter_ns()
        self.elapsed = self.end - self.start


executable = pathlib.Path(sys.argv[1])
if not executable.exists():
    raise ValueError(f"Invalid executable: {executable}")

times = []
timer = Timer()
for i in range(NUM_EXECUTIONS):
    print(f"Execution {i}")
    with timer:
        subprocess.run([str(executable)])
    print(f"Elapsed Time: {timer.elapsed / 1e9}s")
    times.append(timer.elapsed)

average_time_ns = sum(times) / len(times)
average_time = average_time_ns / 1e9
standard_dev = math.sqrt(sum([(time - average_time_ns) ** 2 for time in times]) / (len(times) - 1)) / 1e9  # unbiased

print(f"Samples: {times}")
print(f"Average Time: {average_time} seconds")
print(f"Std. Dev: {standard_dev} seconds")
