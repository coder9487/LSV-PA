
import os

# Set the root directory to the current working directory
root_dir = os.getcwd()

# Define the commands to run
commands = [
    #"rm *.aig",
    "yosys ../run1.ys; yosys ../run2.ys   ",
]

# Walk through all subdirectories
for dirpath, dirnames, filenames in os.walk(root_dir):
    for command in commands:
        try:
            print(f"Running command in: {dirpath}")
            print(f"Command: {command}")
            # Change to the current directory
            os.chdir(dirpath)
            # Execute the command
            exit_code = os.system(command)
            if exit_code != 0:
                print(f"Command failed with exit code {exit_code}")
        except Exception as e:
            print(f"Failed to run command '{command}' in {dirpath}: {e}")