#!/usr/bin/python3
import os
import subprocess
import time

def find_latest_riddle_file_in_dir(directory):
    # List files in the directory that start with 'riddle' and get their full paths
    riddle_files = [os.path.join(directory, f) for f in os.listdir(directory) 
                    if os.path.isfile(os.path.join(directory, f)) and f.startswith('riddle')]
    
    # If there are no matching files, return None
    if not riddle_files:
        return None
    
    # Find the most recently modified 'riddle' file
    latest_file = max(riddle_files, key=os.path.getctime)
    return latest_file

def main():
    # Open (or create) the file 'myfile' and get its file descriptor
    # Run the subprocess './riddle'
    process = subprocess.Popen(['./riddle'])
    time.sleep(3)
    latest_riddle_file = find_latest_riddle_file_in_dir("/tmp/")
    if latest_riddle_file:
        #hex_offset = input("Please enter the byte offset (in hex) where you want to start writing: ")
        byte_offset = int(0x6f, 16)
        char = input("Please enter a character: ")

        # Open the file and position the write cursor at the given offset, then write the character 512 times
        with open(latest_riddle_file, "w") as f:
            f.seek(byte_offset)
            f.write(char)

            print(f"Wrote '{char}' 512 times starting from byte offset {hex_offset} in {latest_riddle_file}")
    else:
        print("No 'riddle' file found in /tmp/ directory.")


    process.wait()

if __name__ == '__main__':
    main()
