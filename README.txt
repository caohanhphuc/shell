Stephanie Cao
CS355, HW3

README.txt:
1. My shell is implemented to first parse the user input, and based on symbols and words parsed to assign a job action (including background, foreground, execute, job list, kill and exit) to each job. For each type of job action, a corresponding function is implemented to handle it.
In terms of signal handlers, I implemented one handler for SIGCHLD, because in all cases a SIGCHLD gets sent from a child to the shell with different codes when the status of a process is changed.
The basic data structure is a linked list stored in struct node that inserts jobs with process id, job id, job action, most recent terminal settings, and the argument to be executed. Jobs are added after execution or first background execution with &, and deleted when a SIGINT is sent or when they get killed by users. Everytime a job is added or deleted, the code that changes the job list is wrapped by sigprocmask which blocks all signals before making changes to the list and unblocks them afterwards to avoid race conditions.

2. Fully implemented features:
   - Background process with &
   - Background process with bg [arg]: only works with % before the job
   - Foreground process with fg [arg]: only works with % before the job
   - Suspend process with Ctrl Z without suspending the shell
   - Terminate process with Ctrl C without closing the shell
   - Kill processes with or without flag -9: All suspended jobs can only be killed with flag -9. Can only kill jobs by job ids, not process ids.
   - Multiple arguments separated by & or ;
   
  Partially implemented features:
   - List of all jobs are implemented, but without the + and - signs
   - Processes that take user inputs when terminated right after opened makes stdout corrupt. For example cat then Ctrl C or sleep then Ctrl C. However, these work fine with Ctrl Z. I haven't been able to figure out why since the corrupted stdout that keeps printing out the prompt  makes it very hard to debug. 

  Other errors: I tried to free memory, but as I freed anything inside the tokenizer (even the temporary variables), the whole program acts weirdly, and I don't understand why or how to deal with that. But I freed the rest of the memory.
   
3. I tested my shell with different combinations of commands. First I tested each implemented feature separately, then put them together, for example:
   - Start several backgrounded and suspended processes, then foreground one in the middle, and ctrl-z/ ctrl-c to check the job list
   - Always check consistency between job list and "ps"
   - Try invalid commands
