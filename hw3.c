#include "ll.h"
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <termios.h>
#include <errno.h>
#include <sys/prctl.h>

sigset_t blocked;
pid_t shellpid;

struct TokenList{
  char*** argumentList;
  int* numWords;
  jobaction* jobacts;
  int listsize;
};

struct termios terminal;
void sigchldHandler(int signum, siginfo_t *info, void* context);
char* trimws(char* buffer);
struct TokenList tokenizer(char* str);
void backgroundn(char** arg, int argsize);
void background(char** arg, int argsize);
void foreground(char** arg, int argsize);
void killjob(char** arg, int argsize);
void execute(char** arg, int argsize);
int main(int argc, const char* argv[]){
  char *command;
  char line[BUFSIZ];
  char** arg;
  jobaction action;
  int executing = true;
  int prompt, readInput, processInput;

  tcgetattr(STDIN_FILENO, &terminal);
  shellpid = getpid();

  struct sigaction child, sint;
  struct TokenList argTuple;

  sigset_t blocked;
  sigfillset(&blocked);
  
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGSTOP, SIG_IGN);
 
  child.sa_sigaction = sigchldHandler;
  child.sa_flags = SA_SIGINFO;
  sigaction(SIGCHLD, &child, NULL);

  /* sint.sa_sigaction = sigintHandler;
  sint.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &sint, NULL);
  */
  
  while (executing){
    prompt = true;
    //print report queue
    if (prompt){
      printf("[mysh]$ ");
      fflush(stdout);
      prompt = false;
      readInput = true;
    }
    if (readInput){
      if (fgets(line, BUFSIZ, stdin) != NULL){
	command = trimws((char*)line);
	readInput = false;
      } else {
	readInput = true;
	continue;
      }
      processInput = true;
    }
    
    if (processInput){
      argTuple = tokenizer(command);
      char*** argList = argTuple.argumentList;
      jobaction* jList = argTuple.jobacts;
      int size = argTuple.listsize;
      int* numWords = argTuple.numWords;
	
      for (int i = 0; i < size; i++){
	action = jList[i];
	arg = argList[i];
	int argsize = numWords[i];

        if (strcmp(arg[0], "") == 0) continue;

	if (action == EXT){
	  printf("logout\n");
	  exit(0);
	}
	
	else if (action == BGN){
	  backgroundn(arg, argsize);
	  
	}
	else if (action == BGR){
	  background(arg, argsize);
	}
	
	else if (action == FGR){
	  foreground(arg, argsize);
	}
	
	else if (action == LST){
	  printList();
	}
	
	else if (action == KLL){
	  killjob(arg, argsize);
	}
	
	else if (action == EXC){
	  execute(arg, argsize);
	}
	
	else {
	  //print error
	  perror("Error from not having job status!\n");
	}
	
      }
      
      
      processInput = false;
    }
    //executing = false;
  }

  if (argTuple.listsize > 0){
    free(argTuple.jobacts);
    for (int i = 0; i < argTuple.listsize; i++){
      for (int j = 0; j < argTuple.numWords[i]+1; j++){
	free(argTuple.argumentList[i][j]);
      }
      free(argTuple.argumentList[i]);
    }
    free(argTuple.argumentList);
    free(argTuple.numWords);
  } 
  
  struct node* curr;
  while ((curr = head) != NULL){
    head = head->next;
    for (int i = 0; i < curr->argsize; i++){
      free(curr->cmd[i]);
    }
    free(curr->cmd);
    free(curr);
  }
  
  free(command);
  
  return 0;
}


void sigchldHandler(int signum, siginfo_t *info, void* context){
  int code = info->si_code;
  pid_t pid = info->si_pid;
  if (code == CLD_EXITED){
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    struct node *top = deletePid(pid);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    if (top != NULL){
      if (top->status != BGR){
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
	tcsetpgrp(STDIN_FILENO, getpid());
      }
      printf("[%d]\tDone\t\t", top->key);
      printArg(top->cmd, top->argsize);
      printf("\n");
    }
    
  } else if (code == CLD_STOPPED){
    printf("\n");
    if (length() == 0){
      return;
    }
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    struct node* stp = deletePid(pid);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
      tcgetattr(STDIN_FILENO, &(stp->term));
      kill(stp->data, SIGSTOP);
      tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
      int pgres = tcsetpgrp(STDIN_FILENO, shellpid);
      if (pgres == 0){
	printf("[%d]\tStopped\t\t", stp->key);
	printArg(stp->cmd, stp->argsize);
	printf("\n");
	sigprocmask(SIG_BLOCK, &blocked, NULL);
        insertFirst(stp->key, stp->data, SUS, stp->cmd, stp->argsize, stp->term);
	sigprocmask(SIG_UNBLOCK, &blocked, NULL);
      } else {
	printf("-mysh: cannot suspend process\n");
      }
  } else if (code == CLD_CONTINUED){
    
  } else if (code == CLD_KILLED){
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    struct node* dlt = deletePid(pid);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    printf("\n[%d]\tTerminated\t\t", dlt->key);
    printArg(dlt->cmd, dlt->argsize);
    printf("\n");
  } else if (code == CLD_DUMPED){
    printf("-mysh: child %d just got dumped!\n", pid);
  } else if (code == CLD_TRAPPED){
    printf("-mysh: child %d just got trapped!\n", pid);
  } else {
    printf("-mysh: no si_code recognized!\n");
  }
  return;
}

/* void sigintHandler(int signum, siginfo_t *info, void* context){
  printf("in sigint handler\n");
  int code = info->si_code;
  pid_t pid = info->si_pid;
  
  sigprocmask(SIG_BLOCK, &blocked, NULL);
  struct node *top = deletePid(pid);
  sigprocmask(SIG_UNBLOCK, &blocked, NULL);
  if (top != NULL){
    if (top->status != BGR){
      tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
      tcsetpgrp(STDIN_FILENO, getpid());
    }
    printf("[%d]\tDone\t\t", top->key);
    printArg(top->cmd, top->argsize);
    printf("\n");
  }
}
*/

char* trimws(char* buffer){
  //trim leading space
  while (isspace(*buffer)){
    buffer++;
  }
  if (*buffer == 0){
    return buffer;
  }
  
  //trim trailing space
  char* end = buffer + strlen(buffer)-1;
  if (end[0] == '\n'){
    end--;
  } //get rid of \n
  while (end > buffer && isspace(*end)){
    end--;
  }

  *(end+1) = 0;

  return buffer;
}

/* Tokenizer to parse command lines:
   ignore spaces for & and record occurence of % for fg and bg
*/
struct TokenList tokenizer(char* str){
  str = trimws(str);
  struct TokenList res;
  res.listsize = 0;
  if (strcmp(str,"") == 0){
    return res;
  }
  int prevPos = -1;
  int length = strlen(str)+1;

  int numArg = 0;
  for (int i = 0; i < length; i++){
    if ((str[i] == '&') || (str[i] ==  ';') ||( str[i] == '\0')){
      //if ((str[i] == ';') || (str[i] == '\0')){
      numArg++;
    }
  }

  char*** argumentList = (char***) malloc(numArg*sizeof(char**));
  int idxArgList = 0;
  jobaction* jobacts = (jobaction*) malloc (numArg*sizeof(jobaction));
  int* numWordsList = (int*)malloc (numArg*sizeof(int));
  
  for (int i = 0; i < length; i++){
    if ((str[i] == '&') || (str[i] ==  ';')){
      char* temp = (char*)malloc((i-prevPos+1)*sizeof(char));
      memcpy(temp, &str[prevPos+1], i-prevPos-1);
      temp[i-prevPos] = '\0';
      prevPos = i;

      temp = trimws(temp);
      int j = 0;
      int numWords = 0;
      while (j < strlen(temp)){
	if (temp[j] == ' '){
	  if (temp[j+1] != ' '){
	    numWords++;
	  }
	} else if (j == strlen(temp)-1){
	  numWords++;
	}
	j++;
      }

      if (numWords > 0){
	numWordsList[idxArgList] = numWords;
      
	char** cmdList = (char**) malloc(sizeof(char*)*(numWords+1));
	cmdList[numWords] = '\0';
	int index = 0;
	j = 0;
	int startWord = 0;
	int inWord = true;
      
	while (j <= strlen(temp)){
	  if (temp[j] == ' ' || temp[j] == '\0'){
	    if (inWord == true){
	      cmdList[index] = (char*)malloc(sizeof(char)*(j-startWord + 2));
	      memcpy(cmdList[index], &temp[startWord], j-startWord);
	      cmdList[index][j-startWord+1] = '\0';
	      index++;
	      inWord = false;
	    }
	  } else {
	    if (inWord == false && temp[j] != ' '){
	      startWord = j;
	      inWord = true;
	    }
	  }
	  j++;
	}

	argumentList[idxArgList] = cmdList;

	if (str[i] == '&'){
	  jobacts[idxArgList] = BGN;
	} 
	if (strcmp(cmdList[0], "bg") == 0){
	  jobacts[idxArgList] = BGR;
	} else if (strcmp(cmdList[0], "fg") == 0){
	  jobacts[idxArgList] = FGR;
	} else if (strcmp(cmdList[0], "exit") == 0){
	  jobacts[idxArgList] = EXT;
	} else if (strcmp(cmdList[0], "jobs") == 0){
	  jobacts[idxArgList] = LST;
	} else if (strcmp(cmdList[0], "kill") == 0){
	  jobacts[idxArgList] = KLL;
	}
	idxArgList++;
      }
      free(temp);
    } else {
      if (i == length-1){
	char* temp = (char*)malloc(sizeof(char)*(i-prevPos+1));
	memcpy(temp, &str[prevPos+1], i-prevPos-1);
	temp[i-prevPos] = '\0';
	prevPos = i;

	temp = trimws(temp);
	int j = 0;
	int numWords = 0;
	
	while (j < strlen(temp)){
	  if (temp[j] == ' '){
	    if (temp[j+1] != ' '){
	      numWords++;
	    }
	  } else if (j == strlen(temp)-1){
	    numWords++;
	  }
	  j++;
	}

	if (numWords > 0){
	  numWordsList[idxArgList] = numWords;

	  char** cmdList = (char**) malloc(sizeof(char*)*(numWords + 1));
	  cmdList[numWords] = '\0';
	
	  int index = 0;
	  j = 0;
	  int startWord = 0;
	  int inWord = true;

	  while (j <= strlen(temp)){
	    if (temp[j] == ' ' || temp[j] == '\0'){
	      if (inWord == true){
		cmdList[index] = (char*)malloc(sizeof(char)*(j-startWord + 2));
		memcpy(cmdList[index], &temp[startWord], j-startWord);
		cmdList[index][j-startWord+1] = '\0';
		index++;
		inWord = false;
	      }
	    } else {
	      if (inWord == false && temp[j] != ' '){
		startWord = j;
		inWord = true;
	      }
	    }
	    j++;
	  }
	  argumentList[idxArgList] = cmdList;

	  

	  if (strcmp(cmdList[0], "bg") == 0){
	    jobacts[idxArgList] = BGR;
	  } else if (strcmp(cmdList[0], "fg") == 0){
	    jobacts[idxArgList] = FGR;
	  } else if (strcmp(cmdList[0], "exit") == 0){
	    jobacts[idxArgList] = EXT;
	  } else if (strcmp(cmdList[0], "jobs") == 0){
	    jobacts[idxArgList] = LST;
	  } else if (strcmp(cmdList[0], "kill") == 0){
	    jobacts[idxArgList] = KLL;
	  }
	  idxArgList++;
	}
	free(temp);
      }
    }
  }
  res.argumentList = argumentList;
  res.jobacts = jobacts;
  res.listsize = idxArgList;
  res.numWords = numWordsList;
  
  /* printf("parsed string: ");
  for (int i = 0; i < res.listsize; i++){
    printArg(res.argumentList[i], res.numWords[i]);
  }
  printf("done parsing \n");
  */
  return res;
}

void backgroundn(char** arg, int argsize){
  pid_t pid = fork();
  struct termios term;

  if (pid == 0){
    setpgid(getpid(), getpid());
    for (int i = 0; i < 32; i++){
      signal(i, SIG_DFL);
    }
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    execvp(arg[0], arg);
  
  } else if (pid > 0){
    setpgid(getpid(), getpid());
    setpgid(pid, pid);
    
    int idxinsert;
    if (head == NULL){
      idxinsert = 1;
    } else {
      idxinsert = head->key + 1;
    }
    tcgetattr(STDIN_FILENO, &term);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    insertFirst(idxinsert, pid, BGR, arg, argsize, term);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    printf("[%d] %d\n", idxinsert, pid);
  } else {
    perror("Error when forking child for background process with &!\n");
  }
}

/* Bring process to background:
   if background with bg: find last job suspended, resume
   register sigchldHandler to notify parent of background job status
*/
void background(char** arg, int argsize){
  if (argsize == 1){
    struct node* lastsus = resumeBG();
    if (lastsus == NULL) {
      printf("-mysh: bg: no suspended job\n");
      return;
    }
    int killres = kill(lastsus->data, SIGCONT);
    if (killres == 0){
      sigprocmask(SIG_BLOCK, &blocked, NULL);
      lastsus->status = BGR;
      delete(lastsus->key);
      insertFirst(lastsus->key, lastsus->data, lastsus->status, lastsus->cmd, lastsus->argsize, lastsus->term);
      sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    } else {
      printf("-mysh: error for SIGCONT background: %s\n", strerror(errno));
    }
  } else {
    for (int i = 1; i < argsize; i++){
      if (arg[i][0] == '%'){
	char* numjobchar = strtok(arg[i], "%");
        int numjob = atoi(numjobchar);
	struct node* resume = find(numjob);
	if (resume == NULL){
	  printf("-mysh: bg: no suspended job\n");
	  return;
	}
	int killres = kill(resume->data, SIGCONT);
	if (killres == 0){
	  sigprocmask(SIG_BLOCK, &blocked, NULL);
	  resume->status = BGR;
	  delete(resume->key);
	  insertFirst(resume->key, resume->data, resume->status, resume->cmd, resume->argsize, resume->term);
	  sigprocmask(SIG_UNBLOCK, &blocked, NULL);
	} else {
	  printf("-mysh: error for SIGCONT background: %s\n", strerror(errno));
	}
      } else {
	printf("-mysh: bg: no such job\n");
	return;
      }
    }
  }
  return;
}

/* Bring process to foreground:
   Send SIGCONT signal
*/
void foreground(char** arg, int argsize){
  int status;
  if (argsize == 1){
    struct node* last = resumeFG();
    if (last == NULL) {
      printf("-mysh: fg: no current job\n");
      return;
    }

    int fgres = tcsetpgrp(STDIN_FILENO, last->data);
    
    if (last->status == SUS){
      tcsetattr(STDIN_FILENO, TCSANOW, &(last->term));
      int contres = kill(last->data, SIGCONT);
      if (contres != 0){
	printf("-mysh: error for SIGCONT foreground: %s\n", strerror(errno));
	return;
      }
    }
    
    if (fgres == 0){
      sigset_t waitblock;
      sigfillset(&waitblock);
      sigdelset(&waitblock, SIGINT);
      sigdelset(&waitblock, SIGTERM);
      sigdelset(&waitblock, SIGTSTP);
      sigdelset(&waitblock, SIGQUIT);
      sigprocmask(SIG_BLOCK, &waitblock, NULL);
      int reswait = waitpid(last->data, &status, WUNTRACED);
      sigprocmask(SIG_UNBLOCK, &waitblock, NULL);
      tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
      tcsetpgrp(STDIN_FILENO, getpid());
 
      return;
    } else {
      printf("-mysh: cannot foreground process: %d\n", last->data);
    }
    return;
    
  } else {
    for (int i = 1; i < argsize; i++){
      if (arg[i][0] == '%'){
	char* numjobchar = strtok(arg[i], "%");
        int numjob = atoi(numjobchar);
	struct node* last = find(numjob);
	if (last == NULL){
	  printf("-mysh: fg: no suspended job\n");
	  return;
	}
	
	int fgres = tcsetpgrp(STDIN_FILENO, last->data);
    
	if (last->status == SUS){
	  tcsetattr(STDIN_FILENO, TCSANOW, &(last->term));
	  int contres = kill(last->data, SIGCONT);
	  if (contres != 0){
	    printf("-mysh: error for SIGCONT foreground: %s\n", strerror(errno));
	    return;
	  }
	}

	if (fgres == 0){
	  sigset_t waitblock;
	  sigfillset(&waitblock);
	  sigdelset(&waitblock, SIGINT);
	  sigdelset(&waitblock, SIGTERM);
	  sigdelset(&waitblock, SIGTSTP);
	  sigdelset(&waitblock, SIGQUIT);
	  sigprocmask(SIG_BLOCK, &waitblock, NULL);
	  int reswait = waitpid(last->data, &status, WUNTRACED);
	  sigprocmask(SIG_UNBLOCK, &waitblock, NULL);
	  tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
	  tcsetpgrp(STDIN_FILENO, getpid());
	  return;
	} else {
	  printf("-mysh: cannot foreground process: %d\n", last->data);
	}
	
      } else {
	printf("-mysh: bg: no such job\n");
	return;
      }
    }
  }
  return;
}

/* Kill process by sending SIGTERM or kill(pid, SIGKILL)
 */
void killjob(char** arg, int argsize){
  if (argsize <= 1){
    printf("kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]\n");
    return;
  }
  if (strcmp(arg[1],"-9") == 0){
    for (int i = 2; i < argsize; i++){
      if (arg[i][0] == '%'){
	char* numjobchar = strtok(arg[i], "%");
        int numjob = atoi(numjobchar);
	struct node* jobkilled = find(numjob);
	if (jobkilled == NULL){
	  printf("-mysh: kill: %s: no such job\n", arg[i]);
	  return;
	}
	int killres = kill(jobkilled->data, SIGKILL);
	if (killres == 0){
	  /* sigprocmask(SIG_BLOCK, &blocked, NULL);
	  struct node* dlt = delete(numjob);
	  sigprocmask(SIG_UNBLOCK, &blocked, NULL); */

	} else {
	  printf("-mysh: error for SIGKILL kill job: %s\n", strerror(errno));
	}
      } else {
	printf("-mysh: kill: %s: arguments must be job IDs\n", arg[i]);
	return;
      }
    }
  } else {
    for (int i = 1; i < argsize; i++){
      if (arg[i][0] == '%'){
	char* numjobchar = strtok(arg[i], "%");
        int numjob = atoi(numjobchar);
	struct node* jobkilled = find(numjob);
	if (jobkilled == NULL){
	  printf("-mysh: kill: %s: no suspended job\n", arg[i]);
	  return;
	}
	int killres = kill(jobkilled->data, SIGTERM);
	if (killres == 0){
	  /* sigprocmask(SIG_BLOCK, &blocked, NULL);
	  struct node* dlt = delete(numjob);
	  sigprocmask(SIG_UNBLOCK, &blocked, NULL); */
	} else {
	  printf("-mysh: error for SIGTERM kill job: %s\n", strerror(errno));
	}
      } else {
	printf("-mysh: kill: %s: arguments must be process or job IDs\n", arg[i]);
	return;
      }
    }
  }
  return;
}

/* Execute normal processes
 */
void execute(char** arg, int argsize){
  int status;
  pid_t pid = fork();
  int result = 0;
  struct termios term;
  
  if (pid == 0){

    setpgid(getpid(), getpid());
    
    //reset signals
    for (int i = 0; i < 32; i++){
      signal(i, SIG_DFL);
    }
    
    tcsetpgrp(STDIN_FILENO, getpid());
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    result = execvp(arg[0], arg);
    if (result < 0){
      printf("-mysh: ");
      printf(strerror(errno));
      printf("\n");
      exit(1);
      //return;
    }
  } else if (pid > 0){
    setpgid(pid, pid);
    setpgid(getpid(), getpid());

    int idxinsert;
    if (head == NULL){
      idxinsert = 1;
    } else {
      idxinsert = head->key + 1;
    }
    tcgetattr(STDIN_FILENO, &term);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    insertFirst(idxinsert, pid, FGR, arg, argsize, term);
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
    //printList();
   
    tcsetpgrp(STDIN_FILENO, pid);
    waitpid(pid, &status, WUNTRACED);
    
  } else {
    perror("Error from forking in execution!\n");
  }
  return;
}

