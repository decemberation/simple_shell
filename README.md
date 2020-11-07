# simple_shell
This project is organized into several parts:
1. Creating the child process and executing the command in the child
2. Providing a history feature
3. Adding support of input and output redirection
4. Allowing the parent and child processes to communicate via a pipe

However, please note that this version of simple shell does not handling communication via signal as well as communication via multiple pipes.
> How to use it?
1) Make sure that you have gcc installed
  > You can get my CentOS VM which has already built gcc compiler and gcc debugger here: https://drive.google.com/file/d/1X9ZJhpq0QRkBqZSiyhnFislZe1mHM0qa/view?usp=sharing
2) Open Terminal (Ctrl + Alt + T)
3) <cd> to the Directory which has the .c file (in my case the command should be cd Desktop)
4) Compile the program
    > gcc -o shell shell.c
5)  Run it
    > ./shell
