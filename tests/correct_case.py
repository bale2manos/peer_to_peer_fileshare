import subprocess
import time
import threading


# Function to send instructions to the client shell and print them
def send_instruction(instruction, process):
    process.stdin.write(instruction + '\n')
    process.stdin.flush()
    print(instruction)


# Function to print output from client shell
def print_output(process):
    for line in process.stdout:
        print(line.strip())


# Function to handle errors and print stderr output
def print_errors(process):
    for line in process.stderr:
        print(line.strip())
    process.stderr.close()


# Remove all the files and subfolders of ../database
subprocess.run(['rm', '-rf', './database'])
subprocess.run(['mkdir', './database'])
# Use try-except to suppress error message
try:
    subprocess.run(['rm', './current_username.txt'], stderr=subprocess.DEVNULL)
except FileNotFoundError:
    pass

print("Borrando base de datos")
time.sleep(1)  # Para ver la base de datos vacía

# Start the client shells
processes = []
output_threads = []
error_threads = []
correct_processes_calls = ['python3', 'client.py', '-s', 'localhost', '-p', '4500']
n_processes = 100

for _ in range(n_processes):
    process = subprocess.Popen(correct_processes_calls, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    processes.append(process)

    # Start a separate thread to print output and errors
    output_thread = threading.Thread(target=print_output, args=(process,))
    output_thread.start()
    output_threads.append(output_thread)

    error_thread = threading.Thread(target=print_errors, args=(process,))
    error_thread.start()
    error_threads.append(error_thread)

# Wait for the client shells to start
time.sleep(1)  # Adjust this delay according to your project's startup time

instructions = []
half_processes = n_processes / 2
for i in range(n_processes):
    # Define instructions for each process, do REGISTER usuario + n_process
    user = "usuario" + str(i)
    if (i < half_processes):
        neighbor = int(i + half_processes)
        process_instr = ["REGISTER " + user,
                         "CONNECT " + user,
                         "PUBLISH title" + str(i) + " content of the file" + str(i),
                         "LIST_USERS",
                         "GET_FILE usuario" + str(neighbor) + " title" + str(neighbor) + " copia" + str(neighbor),
                         "QUIT"]
    else:
        neighbor = int(i - half_processes)
        process_instr = ["REGISTER " + user,
                         "CONNECT " + user,
                         "PUBLISH title" + str(i) + " content of the file" + str(i),
                         "LIST_CONTENT usuario" + str(neighbor),
                         "GET_FILE usuario" + str(neighbor) + " title" + str(neighbor) + " copia" + str(neighbor),
                         "QUIT"]
    instructions.append(process_instr)

# Send instructions with a delay of 3 seconds between each
for i in range(len(instructions[0])):
    for j in range(n_processes):
        send_instruction(instructions[j][i], processes[j])
        time.sleep(0.01)

# Close the client shells
for process in processes:
    process.stdin.close()

print("Procesos cerrados")
