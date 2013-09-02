#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "tc-init.h"
#include "tc-directory.h"


int main(int argc, char const *argv[]) {
	char taskParentDirectory[TC_MAX_BUFF];

	/* Make sure environment is proper */
	tc_init(taskParentDirectory);
	printf("Working in: %s\n",taskParentDirectory );

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
			/* Check if we are already working on a task */

			/* Create a task and store its information */
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