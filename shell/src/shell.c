#include "../include/shell.h"

#define MAX_BUFFER_RD_SIZE 1024
#define MAX_CMD_COUNT 50
#define MAX_CMD_LEN 25


// private function prototypes
int dirwalk(char *dir);

// Run the command, given the command struct of which holds the parsed commands from the user
void run_command(Commands_t* cmd){

	if(strncmp(cmd->cmds[0],"cd",strlen("cd") + 1) == 0){ // Error check, then execute change directory
		if(cmd->num_cmds > 2){
			fprintf(stdout, "cd(): Too many arguments");
		}
		else if(cmd->num_cmds < 2){
			fprintf(stdout, "cd(): Not enough arguments");
		}
		else{
			cd(cmd->cmds[1]);
		}
	}
	else if(strncmp(cmd->cmds[0],"pwd",strlen("pwd") + 1) == 0){ // Error check, then execute print working directory
		if(cmd->num_cmds > 1){
			printf("pwd(): Too many arguments");
		}
		else {
			pwd();
		}
	}

	else if(strncmp(cmd->cmds[0],"ls",strlen("ls") + 1) == 0){ // Error check, then execute list directory/file
		if(cmd->num_cmds > 2){
			fprintf(stdout, "ls(): Too many arguments");
		}
		if(ls(cmd->cmds[1]) < 0){
			fprintf(stdout, "ls(): Error with ls command");
		}
	}

	else if(strncmp(cmd->cmds[0],"cat",strlen("cat") + 1) == 0){ // Error check, then execute cat, or print contents of file
		if(cmd->num_cmds > 2){
			fprintf(stdout, "cat(): Too many arguments");
		}
		if(cat(cmd->cmds[1]) < 0){
			fprintf(stdout, "cat(): Error with cat command");
		}
	}
	else if(strncmp(cmd->cmds[0],"join",strlen("join") + 1) == 0){
		
		if(cmd->num_cmds % 2 != 0 || cmd->num_cmds < 4){
			fprintf(stdout, "Insufficient arguments: join <file1> <col> <file2> <col> ... <fileN> <col> <output_file>");
		}
		else{
			join(cmd);
		}
	}
	else{
		printf("Not a command in this shell");
	}
}

// To print the working directory of shell
void pwd(){

	char *cwd;
	char buff[PATH_MAX + 1];
	cwd = getcwd(buff, PATH_MAX + 1 ); // Get the working directory from the system 
	if(cwd != NULL){ // Error check that we were returned a working directory
		fprintf(stdout, "The current working directory is \"%s\"", cwd);
		return;
	}
	else{// Fuck
		fprintf(stdout, "Could not find working directory");
	}
}

// To change directory to incoming parameters
void cd(char *file ){
	if(file) {
		struct stat fileStat;
		if(stat(file, &fileStat) < 0){ // Error check that it is actually a file
			fprintf(stdout,"file \"%s\" does not exist", file);
			return;
		}
		else if(!S_ISDIR(fileStat.st_mode)){ // Error check that is an actually directory we are working with
			fprintf(stdout, "file \"%s\" is not a directory", file);
			return;
		}
		else{
			if(chdir(file)< 0){ // If chdir() returns with a non-zero, we have a problem
				fprintf(stdout, "Error opening directory \"%s\"", file);
				return;
			}
			else{// Success
				fprintf(stdout, "changed into directory \"%s\"", file);
				return;
			}
		}
	}
	fprintf(stdout, "Bad file \"%s\"", file);
}

// List content of working directory if no files/directory
int ls(char *file) {
    if (!file) {
        char *cwd;
        char buff[PATH_MAX + 1];
        cwd = getcwd(buff, PATH_MAX + 1);
        if (dirwalk(cwd) < 0) {
            return -1;
        }
    } else {

        struct stat fileStat;
        if (stat(file, &fileStat) < 0)
            return -1;
        if (!S_ISDIR(fileStat.st_mode)) {
            fprintf(stdout, "%s\n", file);
            return 0;
        } else {
            if (dirwalk(file) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

// to write content of a file to stdout
int cat(char *file) {
    if (!file) { // Error check param
        return -1;
    }
    int fd = open(file, O_RDONLY);
    if (fd < 0) { // Error check file descriptor
        return -1;
    }
    // read contet of file, then write
    char buffer[MAX_BUFFER_RD_SIZE];
    size_t bufferSize = 0;
    bufferSize = read(fd, buffer, MAX_BUFFER_RD_SIZE);
    while (bufferSize >= 1) {
        write(2, buffer, bufferSize);
        bufferSize = read(fd, buffer, MAX_BUFFER_RD_SIZE);
    }
    close(fd);
    return 0;
}

/*
    PRIVATE FUNCTIONS
*/
int dirwalk(char *dir) {
    if (!dir) {
        return -1;
    }

    struct dirent *dp;
    DIR *dfd;

    if ((dfd = opendir(dir)) == NULL) {
        fprintf(stdout, "ls: can't open %s\n", dir);
        return -1;
    }
    while ((dp = readdir(dfd)) != NULL) {
        fprintf(stdout, "%s\n", dp->d_name);
    }
    closedir(dfd);
    return 0;
}

// Parse the user input and store into command struct
bool parse_user_input (const char* input, Commands_t** cmd) {
	
	if(!input || !cmd)
		return false;

	char *string = strdup(input);
	
	*cmd = calloc (1,sizeof(Commands_t));
	(*cmd)->cmds = calloc(MAX_CMD_COUNT,sizeof(char*));

	unsigned int i = 0;
	char *token;
	token = strtok(string, " \n");
	for (; token != NULL && i < MAX_CMD_COUNT; ++i) {
		(*cmd)->cmds[i] = calloc(MAX_CMD_LEN,sizeof(char));
		if (!(*cmd)->cmds[i]) {
			perror("Allocation Error\n");
			return false;
		}	
		strncpy((*cmd)->cmds[i],token, strlen(token) + 1);
		(*cmd)->num_cmds++;
		token = strtok(NULL, " \n");
	}
	free(string);
	return true;
}

// Takes command struct and deconstructs
void destroy_commands(Commands_t** cmd) {

	if(!cmd){
		return;
	}
	
	for (int i = 0; i < (*cmd)->num_cmds; ++i) {
		free((*cmd)->cmds[i]);
	}
	free((*cmd)->cmds);
	free((*cmd));
	*cmd = NULL;
}

void join(Commands_t* cmd) {

	int num_col = (cmd->num_cmds - 2) / 2; // Fnd the number of columns
	int num_files = num_col; // Num of columns should be equal to num of files
	int cols[num_col]; // Store the actually col values
	size_t i; 
	size_t j;

	// Convert all col arguments to valid integers
	for(j = 0, i = 2; i < (cmd->num_cmds) && j < num_col; i += 2, ++j) {

		if( (cols[j] = atoi(cmd->cmds[i])) < 1 ) {
			fprintf(stdout, "atoi(): column \"%s\" argument must be a valid integer that greater than 0", cmd->cmds[i]);
			fprintf(stdout, "\nFormat of join command: \n\n\t > join <file1> <col_num> <file2> <col_num> <outputFile>\n");
			return;
		}
	}
	// Check to see the passed in file is an actually file and exists
	struct stat fileStat;
	int file_size = 0;


	for(j = 0, i = 1; j < num_files && i < (cmd->num_cmds - 1) ; i += 2, ++j){
		if(stat(cmd->cmds[i], &fileStat) < 0){ // Error check that it is actually a file
			fprintf(stdout,"stat(): file \"%s\" does not exist", cmd->cmds[i]);
			return;
		}
		file_size += (int)fileStat.st_size; // Get the size of all files involved
	}
	
	//fprintf(stdout, "all file sizes: %d bytes", file_size);

	int fds[num_files];

	// Check to see if the files actually open and assign file descriptors into array
	for(j = 0, i = 1; j < num_files && i < (cmd->num_cmds - 1); i += 2, ++j){
		fds[j] = open(cmd->cmds[i], O_RDONLY);
		if(fds[j] < 0){
			//fprintf(stdout, "open(): could not open file \"%s\"", cmd->cmds[i]);
			return;
		}
		else{
			//fprintf(stdout, "Opened file \"%s\". Located at file descriptor: %d\n", cmd->cmds[i], fds[j]);
		}
	}

	int shmId; 			// ID of shared memory segment
	key_t shmKey = 123460; 		// key to pass to shmget(), key_t is an IPC key type defined in sys/types
	int shmFlag = IPC_CREAT | 0666; // Flag to create with rw permissions
	char* shm;
	//char** index;

	// Get shared memory segment
	fprintf(stdout, "file_size: %d\n", file_size);
	if((shmId = shmget(shmKey, file_size, shmFlag)) < 0)
	{
		perror("Error shmget(): ");
		fprintf(stderr, "Init: Failed to initialize shared memory (%d)", shmId); 
		return;
	}
	// array to hold pids after fork;
	pid_t pids[num_files];
	int status;

	fprintf(stdout, "\nabout to fork this shit!!\n");
	// Fork as many times as there are passed in files
	for(i = 0; i < num_files; ++i){

		pids[i] = fork();

		// Child process
		if(pids[i] == 0){
			child_worker(fds[i], cols[i], shmId);
		}
		else if(pids[i] < 0){ // Errror
			fprintf(stderr,"Error Occurred during fork()");
			return;
		}
		else{
			do{
				//fprintf(stdout, "Fork() Proccess: %d\n", (int)pids[i]);
				if( (waitpid(pids[i], &status, 0)) < 0){ // Wait for child processes to finish
					perror("Error waitpid(): ");
					return;
				}
				if(status == 9){ // Process was kill'd/interrupted 
					return;
				}
				else{
					//fprintf(stdout, "status: %d\n", status);
				}
			}while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}

	// Accesss shared memory
	if( (shm = (char*)shmat(shmId, NULL, 0)) < 0 ){
		perror("Error shmat(): ");
		return;
	}

	//fprintf(stdout, "About to print shared memory\n");

	// Write to output file
	 FILE *fp = fopen(cmd->cmds[cmd->num_cmds -1], "w");

	for(i = 0; shm != NULL;  ++i, ++shm){
		fprintf(fp, "%s\n", shm);
	}
	
	shmctl( shmId, IPC_RMID, 0); // DeAllocate shared memory

	return;

	/*for(i = 0; i < num_files; ++i){
		fprintf(stdout, "Closing file descriptor: %d\n", fds[i]);
		close(fds[i]);
	}

	for(i = 1, j = 0; j < num_col && i < cmd->num_cmds - 1; i += 2, ++j){
		fprintf(stdout, "file[%d]: %s\tcol_key_[%d]: %d\n", (int)j, cmd->cmds[i], (int)j, cols[j]);
	}*/

}

void child_worker(int fd, int col_key, int shmId){

	char buffer[1024]; // Buffer to read from file
	dyn_array_t* toker; // Array to hold token'd
	FILE *fp = fdopen(fd, "r"); // Need to open up the passed in file descriptor
	char *shm;// Shared memory pointer

	// Get the shared memory object 
	if( (shm = (char*)shmat(shmId, NULL, 0)) < 0){
		perror("Error shmat(): ");
		return;
	}

	// Read each line in given file, tokenize it, and copy it into shared memory
	while(!feof(fp)){
		fgets(buffer, 1024, fp);
		toker = tokenizer(buffer, ",""\n");
		if(dyn_array_size(toker) >= col_key){
			//fprintf(stdout, "\tBefore: shm: %s\n", *index);
			//shm = *(char**)dyn_array_at(toker, col_key-1);
			//sprintf(*index, "%s", *(char**)dyn_array_at(toker, col_key-1));
			//++i;
			memcpy(shm, *(char**)dyn_array_at(toker, col_key-1), sizeof(dyn_array_at(toker, col_key-1)));
		}
		else{ // Error, Passed in col argument was invalid
			fprintf(stdout, "Column %d is not valid", col_key);
			kill(getpid(), SIGKILL);
		}
		/*for(i = 0; i < dyn_array_size(toker); ++i)
			fprintf(stdout,"dyn_array[%d]: %s\n", (int)i, *(char**)dyn_array_at(toker, i));*/
	}
	//sprintf(shm, "%s", NULL);
	shmdt((void *)shm); // Detach shared memory segment	
	fclose(fp); // close file stream
	close(fd); // close file descriptor
 	dyn_tok_destruct(toker); // Destroy tokenizer array
	_exit(0); // Exit successfully
}

void dyn_tok_destruct(void *tok_str) {
    if (tok_str) {
        free(*(char **)tok_str);
    }
}

dyn_array_t *tokenizer(const char *str, const char *delims) {
    // Haha, oh man, this is gross
    dyn_array_t *strings = NULL;
    char *strtok_pos = NULL; // strtok_r uses a pointer of ours to keep track of data, allowing us to be thread safe
    if (str && delims) {
        char *str_copy = strndup(str, MAX_BUFFER_RD_SIZE); // buffer created will be 1024+terminator
        if (str_copy) {
            strings = dyn_array_create(16, sizeof(char *), &dyn_tok_destruct);
            if (strings) {
                char *tok_pos = strtok_r(str_copy, delims, &strtok_pos);
                while (tok_pos != NULL) {
                    char *temp_ptr = strndup(tok_pos, MAX_BUFFER_RD_SIZE);
                    if (temp_ptr) {
                        if (dyn_array_push_back(strings, &temp_ptr)) {
                            tok_pos = strtok_r(NULL, delims, &strtok_pos);
                            continue;
                        }
                        free(temp_ptr);
                        dyn_array_destroy(strings);
                        strings = NULL;
                    }
                    break;
                }
            }
            free(str_copy);
        }
    }
    return strings;
}