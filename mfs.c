/*
Name: Michael Pena
ID: 1001073634
*/


// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line
#define MAX_COMMAND_HISTORY_SIZE 50
#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 11     // Mav shell only supports 10 arguments
#define MAX_PID_SIZE 15 // number of pids to be stored

char *cmd_history[MAX_COMMAND_HISTORY_SIZE]; //50
int cmd_history_count = 0;

int pid_history[MAX_PID_SIZE]; //array to hold the pids for listpids
int  pid_count = 0; //this will be incremented after each succesfull fork

static void handle_signal (int sig )
{
}

//This displays the commmand history in the shell uses the global
void print_command_history()
{
	int i;
    for (i = 0; i < cmd_history_count; i++)
      printf("%d %s\n", i+1, cmd_history[i]);
}

//this prints out the pids into the shell
void listpids()
{
	int i;
  for (i = 0; i < pid_count; i++)
    printf("%d: %d\n", i+1, pid_history[i]);
}

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE ); //dynamically allocating a string of 255 characters
  //infinite loop for shell

	int i;
  while( 1 )
  {
    struct sigaction act;

    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handle_signal;

    //catches ctrl-c
    if (sigaction(SIGINT , &act, NULL) < 0)
    {
      perror ("sigaction: ");
      return -1;
    }

    //catches ctrl-z
    if (sigaction(SIGTSTP , &act, NULL) < 0)
    {
      perror ("sigaction: ");
      return -1;
    }

    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) ); //stores the command into cmd_str

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS]; // 10 arguments

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      char buf;if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    /*** Begining of Changes ***/

    //catches empty buffer prevents from seg fault
    if (token[0] == NULL)
    {
      continue;
    }


  //If user enters quit or exit the program quits with code 0.
  if ( (strcmp(token[0], "quit") == 0) || (strcmp(token[0], "exit") == 0) )
  {
    exit(0);
  }

  //prints the history
  if (strcmp(token[0], "history") == 0 )
  {
    print_command_history();
    continue;
  }

  //lists the past pids
  if (strcmp(token[0], "listpids") == 0)
  {
    listpids();
    continue;
  }

  //bg the LAST process in the pid_history array
  if (strcmp(token[0], "bg") == 0)
  {
    kill(pid_history[pid_count], SIGCONT);
    continue;
  }


  /* This is used to change the directory NOTE: to get to home directory "cd"
  by itself does not work, it needs the '~' character */
  if (strcmp(token[0], "cd") == 0)
  {

    if ( (strcmp(token[1], "~") == 0) || (token[1] == NULL) )
    {
      chdir(getenv("$HOME"));
      continue;
    }
    else if (token[1] != NULL)
    {
      chdir(token[1]);
      continue;
    }
  }


  char word[50];
  strcpy(word, token[0]);
  int cmd_hist_exec;

  if (word[0] == '!')
  {
      for (i = 0; i < strlen(word); i++)
      {
        if (word[i] == '!')
        {
          cmd_hist_exec = word[i+1] - '0';  //converts the number ascii value into it's integer value so the index can be used in the command history array

          //Checks to see if the number entered exceeds the total number of commands present (out of bounds array check)
          if( (cmd_hist_exec > cmd_history_count) || (cmd_hist_exec < 0) )
          {
            printf("Command not in history!\n" );
            break;
          }


          strcpy(word, cmd_history[cmd_hist_exec-1]);

          //Tokenizes the input
          char *working_str_2 = strdup(word);
          token_count = 0;

          while ( ( (arg_ptr = strsep( &working_str_2, WHITESPACE ) ) != NULL) &&
                    (token_count<MAX_NUM_ARGUMENTS))
          {
            token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
            char buf;if( strlen( token[token_count] ) == 0 )
            {
              token[token_count] = NULL;
            }
              token_count++;
          }

          pid_t hist_child_pid;
          int stat_hist, exec_status_hist;

          printf("msh> %s\n", *token);

          hist_child_pid = fork();

          //fork and execute command in history
          if ( hist_child_pid == 0 )
          {
            exec_status_hist = execvp(token[0], token);
          }
          else
          {
            waitpid(hist_child_pid, &stat_hist, 0);
            free( working_root );
          }
        }
      }
      continue;
    }



    pid_t child_pid;
    int status, exec_status;

    child_pid = fork();

    //this stores the child into an array
    if ( child_pid > 0 )
    {
      if (pid_count > 15 ) /*since the history can only hold 15 pids
      after the 15th pid is stored it will start to rewrite pids starting
      from the 1st position in the array*/
      {
        pid_count = 0;
      }

      pid_history[pid_count] = child_pid;
      pid_count++;
    }


    //child process succesfull
    if ( child_pid == 0 )
    {

      exec_status = execvp(token[0], token);

      if (exec_status = -1)
      {
        printf("%s: Command not found\n\n", token[0]);
        continue;
      }

      /*
          Note 1: put execvp code structure into a function
          Note 2: put the tokenization code structure into a function

          Req 7: Your shell shall support suspending the process with ctrl-z    SIGTSTP

          Req 8: Your shell shall background a suspended process with bg

        */
      }
    else
    {
      waitpid(child_pid, &status, 0);
    }

    cmd_history[cmd_history_count] = strdup(cmd_str);
    cmd_history_count++;
    free( working_root );
  }

  free(cmd_history);
  free(pid_history);

  return 0;
}
