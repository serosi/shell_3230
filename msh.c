/*****************************************************************************
  @file         main.c
  @author       Noah Mullendore, Maggie Nordquist, Patrick Murphy
  @date         25 February 2022
  @brief        MSH (M SHell)
*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */
int msh_cd(char** args);
int msh_help(char** args);
int msh_exit(char** args);
int msh_pwd(char** args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char* builtin_str[] = {
        "cd",
        "help",
        "exit",
        "pwd"
};

int (*builtin_func[]) (char**) = {
        &msh_cd,
        &msh_help,
        &msh_exit,
        &msh_pwd
};

int msh_num_builtins() {
   return sizeof(builtin_str) / sizeof(char*);
}

/*
  Builtin function implementations.
*/

#define MSH_DIRECT_MAX 260
/**
   @brief Built-in command: prints the current working directory
   @param args list of args. args[0] is "pwd"
   @return Always returns 1, to continue executing.
*/
int msh_pwd(char** args)
{
   char cwd[MSH_DIRECT_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
   }
   else {
      perror("cwd error");
   }
   return 1;
}

/**
   @brief Built-in command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int msh_cd(char** args)
{
   if (args[1] == NULL) {
      fprintf(stderr, "msh: expected argument to \"cd\"\n");
   }
   else {
      if (chdir(args[1]) != 0) {
         perror("msh");
      }
   }
   return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int msh_help(char** args)
{
   int i;
   printf("Group 11's MSH\n");
   printf("Type program names and arguments, and hit enter.\n");
   printf("The following are built in:\n");

   for (i = 0; i < msh_num_builtins(); i++) {
      printf("  %s\n", builtin_str[i]);
   }

   printf("Use the man command for information on other programs.\n");
   return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int msh_exit(char** args)
{
   return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int msh_pipe(char** args) {
   int fd[2];
   pid_t pid1;
   pid_t pid2;
   int pipeLoc = -1;                // where the pipe is located
   char inputArgs[1024] = {'\n'};   // first set of args
   char inputArgsV[1024] = {'\n'};  // additional args
   char outputArgs[1024] = {'\n'};  // second set of args
   char outputArgsV[1024] = {'\n'}; // additional args


   strcpy(inputArgs, args[0]);   // set initial value of input args
   for (int i = 1; args[i] != NULL && strcmp(args[i], "|") != 0; i++)
   {
      if (strcmp(inputArgsV, "\n") == 0)
      {
         strcpy(inputArgsV, args[i]);  // need to use strcpy for empty input
      }
      else
      {
         strcat(inputArgsV, " ");
         strcat(inputArgsV, args[i]);
      }
   }


   // find the location of the pipe
   for (int i = 1; args[i] != NULL; i++)
   {
      if (strcmp(args[i], "|") == 0)
      {
         pipeLoc = i;
      }
   }
   pipeLoc++;  // set starting value for outputArgs


   strcpy(outputArgs, args[pipeLoc]);  // set initial value of output args
   for (int i = pipeLoc + 1; args[i] != NULL; i++)
   {
      if (strcmp(outputArgsV, "\n") == 0)
      {
         strcpy(outputArgsV, args[i]); // need to use strcpy for empty input
      }
      else
      {
         strcat(outputArgsV, " ");
         strcat(outputArgsV, args[i]);
      }
   }

   
   // executing pipe command
   if (pipe(fd))
   {
      perror("Pipe error\n");
      return 0;
   }
   pid1 = fork();

   if (pid1 == 0)
   {
      close(fd[1]);
      dup2(fd[0], STDIN_FILENO);
      close(fd[0]);
      if (strcmp(outputArgsV, "\n") == 0)
      {
         execlp(outputArgs, outputArgs, NULL);  // execute without additional args
      }
      else
      {
         execlp(outputArgs, outputArgs, outputArgsV, NULL);
      }
      //execlp("/bin/sh", "/bin/sh", "-c", convertedArgs, NULL);  // cheatsy method
      //execlp("wc", "wc", "-l", NULL);
      //execlp("grep", "grep", "main", NULL);   // executes grep main
   }
   else
   {
      pid2 = fork();

      if (pid2 == 0)
      {
         close(fd[0]);
         dup2(fd[1], STDOUT_FILENO);
         close(fd[1]);
         if (strcmp(inputArgsV, "\n") == 0)
         {
            execlp(inputArgs, inputArgs, NULL); // execute without additional args
         }
         else
         {
            execlp(inputArgs, inputArgs, inputArgsV, NULL);
         }
         //execlp("/bin/sh", "/bin/sh", "-c", convertedArgs, NULL);  // cheatsy method
         //execlp("ls", "ls", NULL);   executes ls
      }

      close(fd[1]);
      close(fd[0]);
      waitpid(-1, NULL, 0);
      waitpid(-1, NULL, 0);
   }

   return 1;
}



int msh_launch(char** args)
{
   pid_t pid;
   int status;

   pid = fork();
   if (pid == 0) {
      // Child process
      if (execvp(args[0], args) == -1) {
         perror("msh");
      }
      exit(EXIT_FAILURE);
   }
   else if (pid < 0) {
      // Error forking
      perror("msh");
   }
   else {
      // Parent process
      do {
         waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   }

   return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int msh_execute(char** args)
{
   int i;

   if (args[0] == NULL) {
      // An empty command was entered.
      return 1;
   }

   for (i = 0; i < msh_num_builtins(); i++) {
      if (strcmp(args[0], builtin_str[i]) == 0) {
         return (*builtin_func[i])(args);
      }
   }

   return msh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char* msh_read_line(void)
{
#ifdef MSH_USE_STD_GETLINE
   char* line = NULL;
   ssize_t bufsize = 0; // have getline allocate a buffer for us
   if (getline(&line, &bufsize, stdin) == -1) {
      if (feof(stdin)) {
         exit(EXIT_SUCCESS);  // We received an EOF
      }
      else {
         perror("msh: getline\n");
         exit(EXIT_FAILURE);
      }
   }
   return line;
#else
#define MSH_RL_BUFSIZE 1024
   int bufsize = MSH_RL_BUFSIZE;
   int position = 0;
   char* buffer = malloc(sizeof(char) * bufsize);
   int c;

   if (!buffer) {
      fprintf(stderr, "msh: allocation error\n");
      exit(EXIT_FAILURE);
   }

   while (1) {
      // Read a character
      c = getchar();

      if (c == EOF) {
         exit(EXIT_SUCCESS);
      }
      else if (c == '\n') {
         buffer[position] = '\0';
         return buffer;
      }
      else {
         buffer[position] = c;
      }
      position++;

      // If we have exceeded the buffer, reallocate.
      if (position >= bufsize) {
         bufsize += MSH_RL_BUFSIZE;
         buffer = realloc(buffer, bufsize);
         if (!buffer) {
            fprintf(stderr, "msh: allocation error\n");
            exit(EXIT_FAILURE);
         }
      }
   }
#endif
}

#define MSH_TOK_BUFSIZE 64
#define MSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char** msh_split_line(char* line)
{
   int bufsize = MSH_TOK_BUFSIZE, position = 0;
   char** tokens = malloc(bufsize * sizeof(char*));
   char* token, ** tokens_backup;

   if (!tokens) {
      fprintf(stderr, "msh: allocation error\n");
      exit(EXIT_FAILURE);
   }

   token = strtok(line, MSH_TOK_DELIM);
   while (token != NULL) {
      tokens[position] = token;
      position++;

      if (position >= bufsize) {
         bufsize += MSH_TOK_BUFSIZE;
         tokens_backup = tokens;
         tokens = realloc(tokens, bufsize * sizeof(char*));
         if (!tokens) {
            free(tokens_backup);
            fprintf(stderr, "msh: allocation error\n");
            exit(EXIT_FAILURE);
         }
      }

      token = strtok(NULL, MSH_TOK_DELIM);
   }
   tokens[position] = NULL;
   return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void msh_loop(void)
{
   char *line;
   char **args;
   int status;
	int flag = 0;

   char *token;    // new
   char *tempLine;

   do {
      printf("$ ");
      line = tempLine = msh_read_line();

      // NEW STUFF ADDED
      while ((token = strtok_r(line, "&", &line)) && status)
      {
         // checks if a pipe is in the input
         flag = 0;
         
         
         // checks tempLine because line is null here already
         if (strchr(tempLine, '|'))
         {
            token = tempLine;
            flag = 1;
         }

         //printf("%s\n", token);    // used to print the current token
         args = msh_split_line(token);
         if (flag == 1)
         {
            status = msh_pipe(args);
         }
         else
         {
            status = msh_execute(args);
         }
         free(args);
      }
      // END NEW STUFF

        
      //args = msh_split_line(line);      
      //status = msh_execute(args);

      //free(line);
      //free(args);
   } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char** argv)
{
   // Load config files, if any.

   // Run command loop.
   msh_loop();

   // Perform any shutdown/cleanup.

   return EXIT_SUCCESS;
}
