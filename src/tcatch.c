#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "tc-init.h"
#include "tc-directory.h"
#include "tc-task.h"


int main(int argc, char const *argv[]) {
	char tcHomeDirectory[TC_MAX_BUFF];
	struct tc_task working_task;
	int i;
	char taskName[TC_MAX_BUFF];
	time_t rawtime;

	/* Make sure environment is proper */
	tc_init(tcHomeDirectory);
	taskName[0] = '\0';

	/* Initialize task struct*/
	working_task.taskName = malloc(TC_MAX_BUFF*sizeof(char));
	if(working_task.taskName == NULL){
		fprintf(stderr, "%s\n", "Could not allocate memory for task name in working struct. Exiting");
		exit(1);
	}
	working_task.taskInfo = NULL;
	working_task.state = TC_TASK_NOT_FOUND;
	working_task.seqNum = 0;
	working_task.startTime=working_task.pauseTime=working_task.endTime=0;

	/* Determine what we've been asked to do */
	if ( argc <= 1 ) {
		/* Called with no arguments. Display Usage */
		_tc_display_usage(NULL);
	}else if ( argc == 2 ){
		/* Called with just a command, besides view let it go to usage*/
		if ( strcasecmp( argv[1], TC_VIEW_COMMAND) == 0 ) {
			;
		} else {
			_tc_display_usage(argv[1]);
		}

	}else{
		/* Called with command and arguments of some kind*/

		/* Check for the help flag */
		_tc_help_check(argc,argv);

		/* No help requested try to parse the command*/
		if( strcasecmp( argv[1], TC_VIEW_COMMAND ) == 0 ) {
			/* Check for all flag in any position*/
			if( _tc_args_flag_check(argc, argv, TC_VIEW_ALL_LONG, TC_VIEW_ALL_SHORT) == TRUE ){
				; /* Show all tasks */
				exit(1);
			}
			/* If we made it this far, then we can assume we need to resolve a task name */


		}else if ( strcasecmp( argv[1], TC_START_COMMAND ) == 0 ) {
			/* Create the human readable name of the task */
			strcpy(taskName,argv[2]);
			for(i=3; i < argc; ++i)
				sprintf(taskName,"%s %s",taskName,argv[i]);

			/* Check if we are already working on a task */
			_find_current_task(&working_task);
			if(working_task.state != TC_TASK_NOT_FOUND){
				/* If we're working on a task then no. finish it first or pause it */
				fprintf(stderr, "\n%s\n", "There is already a task being worked on. ");
				fprintf(stderr, "%s\n\n", "Finish the current task first or switch tasks.");
				free(working_task.taskName);
				exit(1);
			}

			/* Create a task and store its information */			
			free(working_task.taskName);
			working_task.taskName = taskName;
			/* If we are making a task we're starting it */
			working_task.state = TC_TASK_STARTED;
			/* Set the start time */
			rawtime = time(0); 
			if(rawtime == -1){
				fprintf(stderr, "%s\n", "Could not determine time. Exiting");
				exit(1);
			}
			working_task.startTime = rawtime;
			_tc_task_write(working_task, tcHomeDirectory);
			
			fprintf(stdout, "Task: %s has been started.\n", working_task.taskName);

			/* Task write sets the new task as current automatically */
		}else if (strcasecmp ( argv[1], TC_ADD_INFO_COMMAND ) == 0 ) {
			/* Check if there is a current task */

			/* Retrieve the current task */

			/* Add information to the task */

			/* Save the task */
		}else if (strcasecmp( argv[1], TC_FINISH_COMMAND ) == 0 ) {
			/* Check if there's a current task */

			/* Retrieve the current task */

			/* Finish the task */

			/* Save the task */
		}
	}

	
	return FALSE;
}
