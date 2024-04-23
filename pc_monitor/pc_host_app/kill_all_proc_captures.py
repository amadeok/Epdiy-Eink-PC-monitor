import psutil
def kill_all():
    # Iterate over all running processes
    for process in psutil.process_iter(['pid', 'name']):
        # Check if the process name matches "process_capture.exe"
        if process.info['name'] == 'process_capture.exe':
            # Terminate the process
            try:
                process.terminate()
                print(f"Terminated process {process.pid}: {process.info['name']}")
            except Exception as e:
                print("error terminating process", e)