import time
import subprocess

import psutil


class ProcessTimer:
    def __init__(self, command):
        self.command = command
        self.execution_state = False

    def execute(self):
        self.avg_memory = 0
        self.avg_cpu = 0
        self.avg_step = 0

        self.max_cpu_percent = 0
        self.max_vms_memory = 0
        self.max_rss_memory = 0

        self.time_start = time.time()
        self.time_end = None

        self.tracked_subprocess = subprocess.Popen(self.command, shell=False, stdout=subprocess.PIPE)
        self.execution_state = True

    def poll(self) -> bool:
        if not self.check_execution_state():
            return False

        self.time_end = time.time()

        try:
            pp = psutil.Process(self.tracked_subprocess.pid)
            descendants = list(pp.children(True)) + [pp]
            vms = 0
            rss = 0
            cpu_per = 0

            for descendant in descendants:
                mem_info = descendant.memory_full_info()

                cpu_per += descendant.cpu_percent(0.1)
                vms += mem_info[1]
                rss += mem_info[0]

            self.avg_step += 1
            self.max_vms_memory = max(self.max_vms_memory, vms)
            self.max_rss_memory = max(self.max_rss_memory, rss)
            self.max_cpu_percent = max(self.max_cpu_percent, cpu_per)

            self.avg_cpu = (self.avg_cpu*(self.avg_step-1) + cpu_per) / self.avg_step
            self.avg_memory = (self.avg_memory*(self.avg_step-1) + rss) / self.avg_step

        except psutil.NoSuchProcess:
            pass

        except psutil.NoSuchProcess:
            return self.check_execution_state()

        return self.check_execution_state()

    def is_running(self) -> bool:
        return psutil.pid_exists(self.tracked_subprocess.pid) and self.tracked_subprocess.poll() == None

    def check_execution_state(self) -> bool:
        if not self.execution_state:
            return False
        if self.is_running():
            return True
        self.execution_state = False
        self.time_end = time.time()
        return False

    def close(self, kill=False):
        try:
            pp = psutil.Process(self.tracked_subprocess.pid)
            if kill:
                pp.kill()
            else:
                pp.terminate()
        except psutil.NoSuchProcess:
            pass


def track_run(container: str, count_entries: int):
    ptimer = ProcessTimer([
        './build/build/bin/mem_bench',
        '--count_entries', str(count_entries),
        '--container', container,
    ])

    try:
        ptimer.execute()
        while ptimer.poll():
            pass
    finally:
        ptimer.close()

        print('#')
        print(container, count_entries, '\n')
        print('Time:', ptimer.time_end - ptimer.time_start)
        print('Average CPU:', ptimer.avg_cpu, '%')
        print('Max CPU:', ptimer.max_cpu_percent, '%')
        print('Average Memory:', ptimer.avg_memory / 1024 / 1024, 'MB')
        print('Max RSS Memory:', ptimer.max_rss_memory / 1024 / 1024, 'MB')
        print('Max VMS Memory:', ptimer.max_vms_memory / 1024 / 1024, 'MB')
        print('#\n')


def main():
    containers = [
        'std::unordered_map', 'tsl::sparse_map', 'tsl::hopscotch',
        'tsl::robin_map', 'google::dense_hash_map', 'google::sparse_hash_map',
    ]
    sizes = [1e6, 2e6, 4e6, 8e6]

    for container in containers:
        for count_entries in sizes:
            track_run(container, int(count_entries))


if __name__ == '__main__':
    main()
