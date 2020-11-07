#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_LENGTH 80
#define CMD_HISTORY_SIZE 10 

static char* cmd_history[CMD_HISTORY_SIZE];
static int cmd_history_count = 0;

static inline void error(const char* msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

// simulating pipe() function
void ssPipe(char* argv1[], char* argv2[])
{
	pid_t pid1;
	pid_t pid2;
	int fd[2];// file descriptors for input and output

	pipe(fd);
	pid1 = fork();//create fork

	//child
	if (pid1 == 0)
	{
		// Child 1 executing.. 
		// It only needs to write at the write end 
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		execvp(argv2[0], argv2);
	}
	//parent
	else
	{
		// Parent executing 
		pid2 = fork();

		// Child 2 executing.. 
	   // It only needs to read at the read end 
		if (pid2 == 0)
		{
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			execvp(argv1[0], argv1);
		}
		// parent executing, waiting for two children 
		close(fd[1]);
		close(fd[0]);
		waitpid(-1, NULL, 0);
		waitpid(-1, NULL, 0);
	}
}

//split element function
char* strsep(char** stringp, const char* delim) {
	char* rv = *stringp;
	if (rv) {
		*stringp += strcspn(*stringp, delim);
		if (**stringp)
			*(*stringp)++ = '\0';
		else
			*stringp = 0;
	}
	return rv;
}

//split cmd into array of params
int parse_cmd(char* cmd, char** params) { 
	int i, n = -1;
	for (i = 0; i < 10; i++) {
		params[i] = strsep(&cmd, " ");
		n++;
		if (params[i] == NULL) break;
	}
	return(n);
}

void identify_pipe(char* line) {
	int k, x; //k is index of '|', x is 
	char* params[11];
	int j = parse_cmd(line, params); //split line into array of params           

	for (k = 0; k < j; k++) {    
		if (strcmp(params[k], "|") == 0) {
			break;
		}
	}
	//argv1 is a fist child 
	//argv2 is a second child
	char* argv1[11] = { 0 };
	char* argv2[11] = { 0 };

	//initialization of fist child
	for (x = 0; x < k; x++) {
		argv1[x] = params[x];
	}

	//inittialization of second child
	int z = 0;
	for (x = k + 1; x < j; x++) {
		argv2[z] = params[x];
		z++;
	}

	//processing
	ssPipe(argv1, argv2);
}

void exec_cmd(const char* line)
{
	char* CMD = strdup(line);
	char* params[10];
	int argc = 0;

	// parsing command parameters

	params[argc++] = strtok(CMD, " ");
	while (params[argc - 1] != NULL) {	// if tokens can still be get
		params[argc++] = strtok(NULL, " ");
	}

	argc--; // assure that token won't receive NULL

	// controlling background processes

	int background = 0;
	if (strcmp(params[argc - 1], "&") == 0) {
		background = 1; 
		params[--argc] = NULL;	// remove character "&' from the params array
	}

	int fd[2] = { -1, -1 };	// file descriptors for input and output

	while (argc >= 3) {

		// controlling parameters

		if (strcmp(params[argc - 2], ">") == 0) {	// output

			// mở tập tin
			fd[1] = open(params[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
			if (fd[1] == -1) {
				perror("open");
				free(CMD);
				return;
			}

			// updatin params[]

			params[argc - 2] = NULL;
			argc -= 2;
		}
		else if (strcmp(params[argc - 2], "<") == 0) { // input
			fd[0] = open(params[argc - 1], O_RDONLY);
			if (fd[0] == -1) {
				perror("open");
				free(CMD);
				return;
			}
			params[argc - 2] = NULL;
			argc -= 2;
		}
		else {
			break;
		}
	}

	int status;
	pid_t pid = fork();	// forking process
	switch (pid) {
	case -1:	// an error occured
		perror("fork");
		break;
	case 0:	// child
		if (fd[0] != -1) {	// there is a child process
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO) {
				perror("dup2");
				exit(1);
			}
		}
		if (fd[1] != -1) {
			if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
				perror("dup2");
				exit(1);
			}
		}
		execvp(params[0], params);
		perror("execvp");
		exit(0);
	default: // there is a parents process
		close(fd[0]); close(fd[1]);
		if (!background)
			waitpid(pid, &status, 0);
		break;
	}
	free(CMD);
}

// add command lines to history array

static void add_to_history(const char* cmd) {
	if (cmd_history_count == (CMD_HISTORY_SIZE - 1)) {	// checking overflow
		int i;
		free(cmd_history[0]);	// free the first command in the array

		// left shift

		for (i = 1; i < cmd_history_count; i++)
			cmd_history[i - 1] = cmd_history[i];
		cmd_history_count--;
	}
	cmd_history[cmd_history_count++] = strdup(cmd);	// add command to the array
}

// run from history function

static void run_from_history(const char* cmd) {
	int index = 0;
	if (cmd_history_count == 0) {
		printf("No commands in history\n");
		return;
	}

	if (cmd[1] == '!') // get the command which has just been processed after the second '!' character
		index = cmd_history_count - 1;
	else {
		index = atoi(&cmd[1]) - 1;	// 
		if ((index < 0) || (index > cmd_history_count)) { // if none of the commands were in the history, error
			fprintf(stderr, "No such command in history.\n");
			return;
		}
	}
	printf("%s\n", cmd_history[index]);	// print out the command
	exec_cmd(cmd_history[index]);	// redo it
}

// list 10 newest commands

static void list_history() {
	int i = cmd_history_count - 1;
	for (i; i >= 0; i--)
		printf("%i %s\n", i + 1, cmd_history[i]);
}

int main(int argc, char* argv[]) {
	// cấp phát buffer cho dòng lệnh
	size_t line_size = 100;
	char* line = (char*)malloc(sizeof(char) * line_size);
	if (line == NULL) {
		error("Failed to create pipe.");
		perror("malloc");
		return 1;
	}

	int inter = 0; // flag for retrieving
	while (1) {
		if (!inter)
			printf("simpleshell > ");
		if (getline(&line, &line_size, stdin) == -1) {	// read input
			if (errno == EINTR) { 
				clearerr(stdin); // erase the error string
				inter = 1;	 // set flag = 1
				continue;
			}
			perror("getline");
			break;
		}
		inter = 0; // reset flag

		int line_len = strlen(line);
		if (line_len == 1) {	// if get enter
			continue;
		}
		line[line_len - 1] = '\0'; // delete new line


		if (strcmp(line, "exit") == 0) { // if command is "exit" then quit the loop and close the shell
			break;
		}
		else if (strcmp(line, "history") == 0) { // if command is "history" then call function list_history()
			list_history();
		}
		else if (line[0] == '!') { // if the first character of the command is "!" then call function run_from_history()
			run_from_history(line);
		}
		else if (strchr(line, '|') == NULL) {
			add_to_history(line); // push the command into history array
			exec_cmd(line); // execution
		}
		else {
			//Do pipe
			add_to_history(line);
			identify_pipe(line);
		}
	}
	free(line);
	return 0;
}

