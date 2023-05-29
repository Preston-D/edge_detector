import sys
import subprocess
import csv
import os
import re

def run_edge_detector(times, image_folder):
    # Check if the given folder exists
    if not os.path.isdir(image_folder):
        print("Image folder not found")
        return

    # Check if run_program.sh exists
    if not os.path.isfile("./run_program.sh"):
        print("run_program.sh not found")
        return

    # Prepare the csv file
    with open('output.csv', 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Seconds"])

        # Loop to run the edge detector for the given number of times
        for i in range(times):
            try:
                print("Run: ", i)
                # Run the shell script with the image folder as an argument
                process = subprocess.Popen(['./run_program.sh', image_folder], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                stdout, stderr = process.communicate()

                # Check for errors in the shell script
                if process.returncode != 0:
                    print(f"Error in run_program.sh: {stderr.decode('utf-8')}")
                    continue
                
                # Extract the time from the output using regular expressions
                match = re.search(r"Total elapsed time: (\d+\.\d+) s", stdout.decode('utf-8'))
                if match:
                    writer.writerow([match.group(1)])
                else:
                    print("Could not extract time from run_program.sh output")

            except Exception as e:
                print(f"Unexpected error: {e}")

if __name__ == "__main__":
    # Check if the correct number of arguments was given
    if len(sys.argv) != 3:
        print("Usage: python3 run_edge_detector.py <times> <image_folder>")
        sys.exit(1)
    # Run the edge detector
    run_edge_detector(int(sys.argv[1]), sys.argv[2])
