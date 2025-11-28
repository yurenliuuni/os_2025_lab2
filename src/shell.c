#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p){
		if (p->in_file != NULL){
			int fd_in = open(p->in_file, O_RDONLY, 0);
			if (fd_in < 0){
				perror("open input file error");
				exit(EXIT_FAILURE);
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}
		if (p->out_file != NULL){
			int fd_out = open(p->out_file, O_WRONLY| O_CREAT , 0644); //if not exit then create one 
			if (fd_out <0){
				perror("open out_file error ");
				exit(EXIT_FAILURE);
			}
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}
}	
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	pid_t pid;
	pid = fork();
	switch (pid)
	{
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
		break;
	case 0: //child process execute here
		redirection(p);
		//  since it is child process, not need to recover shell stdin and stdout
		
		execvp(p->args[0], p->args);
		break;
	default: 
		//pid is child's id 
		//parent exec here
		wait(NULL); //wait until child process is done
		break;
	}
	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node (struct cmd *cmd){
	struct cmd_node * temp = cmd->head;
	int last_pipe_read_fd = -1;
	pid_t pid_childs[cmd->pipe_num+1];

	for (int i =0; i< cmd->pipe_num +1  ;i++){
		int pipefd[2];
		if (i != (cmd->pipe_num)){ //最後一個child process不用pipe
			if (pipe(pipefd) == -1){
				perror("pipe");
				exit(EXIT_FAILURE);
			}
		}

		pid_childs[i] = fork();

		if (pid_childs[i] == -1){ 
			perror("fork");
			exit(EXIT_FAILURE);
		}else if (pid_childs[i] == 0){
			if (last_pipe_read_fd != -1){ //the first child process
				dup2(last_pipe_read_fd, STDIN_FILENO);
				close(last_pipe_read_fd);
			}

			if (i != (cmd->pipe_num )){
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[1]);
				close(pipefd[0]); //no need for the next child 
			}
			redirection(temp);
            execvp(temp->args[0], temp->args);
            perror("execvp");
            exit(EXIT_FAILURE);
		}else{
			if (i !=0){
				close(last_pipe_read_fd);
			}
			if (i !=  (cmd->pipe_num )){
				close(pipefd[1]);
				last_pipe_read_fd = pipefd[0];
			}

			waitpid(pid_childs[i], NULL, 0);

		}

		temp = temp->next;

	}
	// for (int i = 0; i < cmd->pipe_num+1; i++) {
	// }

	return 1;
}
// ===============================================================

 
void shell()
{ 
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 | out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)  dup2(in, 0);
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
