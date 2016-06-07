#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>

#include "../src/shell.c"

int main(int argc, char *argv[]){

	char *line = NULL;
	Commands_t* cmd;

	line = readline("> "); // Read in line and parse commands
	while (strncmp(line,"exit", strlen("exit")  + 1) != 0) {
		
		if (!parse_user_input(line,&cmd)) {
			printf("Failed at parsing command\n\n");
		}
		
		if (cmd->num_cmds > 0) {	
			run_command(cmd); // Call command with cmd struct
		}
		if (line) {
			free(line);
		}
		destroy_commands(&cmd);
		line = readline("\n> ");
	}
	free(line);

	return 0;
}