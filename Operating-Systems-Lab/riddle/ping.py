import os

fd1_r, fd1_w=os.pipe()
fd2_r, fd2_w=os.pipe()

os.dup2(fd1_r, 33)
os.dup2(fd1_w, 34)

os.dup2(fd2_r, 53)
os.dup2(fd2_w, 54)

os.execv("./riddle", ["./riddle"]) 
