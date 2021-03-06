#include "tc-task.h"
#include "tc-directory.h"

#include <ctype.h>

void _resolve_taskName_from_args(int argc, char const *argv[],char * taskName){
	int i;
	taskName[0] = '\0';
	
	for(i=2; i < argc; ++i)
		if(argv[i][0] != '-')
			sprintf(taskName,"%s %s",taskName,argv[i]);
		else
			continue; /*Ignore any flag value*/

	/* Strip front white space or ending white space */
	trim(taskName);
}

void _find_current_task(struct tc_task * taskStruct){
	/*Returns an error code within the taskStruct to determine success or not*/
	char currentTaskPath[TC_MAX_BUFF];
	char tempBuffer[TC_MAX_BUFF];
	FILE * fp;

	_tc_getCurrentTaskPath(currentTaskPath);

	/* Check if the file exists or not */
	if( _tc_file_exists(currentTaskPath) ){
		/* the current task file is simply the name of the current task and its hash */
		fp = fopen(currentTaskPath, "r");
		if(!fp){
			fprintf(stderr, "%s\n", "Could not open current task file. Exiting");
			taskStruct->state = TC_TASK_FOUND;
			return;
		}

		/* Get the name of the task we want to read */
		fgets(tempBuffer,TC_MAX_BUFF,fp);
		tempBuffer[strlen(tempBuffer)-1] = '\0';
		fclose(fp);

		_tc_task_read(tempBuffer, taskStruct);		
	}else{
		/* Return an error flag that there is no current task */
		taskStruct->state = TC_TASK_NOT_FOUND;
	}

}

void _tc_task_read(char const * taskName, struct tc_task * structToFill){ 
	/* Attempt to fill the structure with data from the file */
	char taskHash[TC_MAX_BUFF];
	char taskSequencePath[TC_MAX_BUFF];
	char taskInfoPath[TC_MAX_BUFF];
	FILE * fp;
	int seqNum, seqState;
	time_t priorTime,seqTime,runningTime;
	int priorState;

	strcpy(structToFill->taskName,taskName);
	_tc_taskName_to_Hash((char *)taskName,taskHash);
	
	/* Read the sequence information for the sequence number and timing info and the last state*/
	/* So to tease out the information we need to read the sequence information. The start time is 
	 * the first time in the sequence file by default, so lets open the sequence file:
	*/

	sprintf(taskSequencePath,"%s/.tc/%s/%s.seq",_tc_getHomePath(),TC_TASK_DIR,taskHash);
	sprintf(taskInfoPath,"%s/.tc/%s/%s.info",_tc_getHomePath(),TC_TASK_DIR,taskHash);
	fp = fopen(taskSequencePath,"r");
	if(!fp){
		fprintf(stderr, "%s\n", "Could not find or open the sequence file for task. Exiting");
		structToFill->state =  TC_TASK_NOT_FOUND;
		return;
	}

	runningTime =  seqState = 0;
	while( fscanf(fp, "%i %i %ld\n", &seqNum, &seqState, &seqTime) != EOF) {
		if (seqNum == 0) {
			structToFill->startTime = seqTime;
			priorTime = seqTime;
			priorState = seqState;
		} else {
			/* Calculate time spent on task */	
			if( priorState == TC_TASK_STARTED && (seqState == TC_TASK_PAUSED || seqState == TC_TASK_FINISHED) ) 
				runningTime = runningTime + (seqTime - priorTime);
			
			priorTime = seqTime;
			priorState = seqState;
			
		}
	}
	
	/* This occurs of the project just started and hasing had any stops yet*/
	if(runningTime == 0 && seqState == TC_TASK_STARTED ){
		runningTime =  time(0) - structToFill->startTime;
	}

	
	structToFill->endTime = seqTime;
	structToFill->state = seqState;
	structToFill->seqNum = seqNum+1;
	structToFill->pauseTime = runningTime;

	fclose(fp);

	/* Cheat a little bit 
	 * It's simple, we stored the NAME of the info file into the structure, then when we want to
	 * display the information we don't read the whole file into memory, but rather we open it
	 * then read it directly into an output stream. This saves us the trouble of realloc-ing memory
	 * or reading the file some buffer size at a time.for no reason.
	*/
	strcpy(structToFill->taskInfo , taskInfoPath);

}

void _tc_taskName_to_Hash(char * taskName, char  * fileHashName){
	unsigned char hash[SHA_DIGEST_LENGTH];
	char tempHashName[TC_MAX_BUFF];
	int looper;

	SHA1((const unsigned char*)taskName,strlen(taskName),hash);
	tempHashName[0] = '\0';
	for(looper = 0; looper < 20; ++looper)
		sprintf(tempHashName,"%s%02x",tempHashName,hash[looper]);
	tempHashName[looper] = '\0';
	
	strcpy(fileHashName,tempHashName);
}

void _tc_task_write(struct tc_task structToWrite, char tcHomeDirectory[]){
	/* Write the task out in a useful format */

	/* The format of a task file name is as follows:
		<taskName sha-1>.seq and <taskName sha-1>.info

		The seq file format is as follows:
		<seq num> <state> <epoch time> [info hash for future]
		<seq num> <state> <epoch time> [info hash for future]
		<seq num> <state> <epoch time> [info hash for future]

		The info file is simply:
		Task Name \n
		[Raw text added through add-info]
		-- ideas for later implementation use info hash for future as info file
		name and then store individual info added to task at each sequence 
		Then the view command could also show the last added piece of info! --
	*/
	char taskSequencePath[TC_MAX_BUFF]; 
	char taskInfoPath[TC_MAX_BUFF];
	char * fileHash;
	time_t rawtime, timeToWrite;
	struct tm * timeinfo;
	char currentDate[TC_MAX_BUFF/2];
	char indexFilePath[TC_MAX_BUFF];
	FILE * fp;
	char currentTaskPath[TC_MAX_BUFF];

	fileHash = malloc(25*sizeof(char)); /* hash is 20 characters .seq is 4 more \0 is 1 more */
	if( fileHash == NULL ){
		fprintf(stderr, "%s\n", "Could not allocate memory for file hash string.");
		exit(1);
	}
	fileHash[0] = '\0';
	
	_tc_taskName_to_Hash(structToWrite.taskName,fileHash);
	
	sprintf(taskSequencePath,"%s/%s/%s.seq",tcHomeDirectory,TC_TASK_DIR,fileHash);
	sprintf(taskInfoPath,"%s/%s/%s.info",tcHomeDirectory,TC_TASK_DIR,fileHash);

	/* If the info file does not exist create it */
	if((_tc_file_exists(taskInfoPath)) == FALSE){
		fp = fopen(taskInfoPath, "w"); /* Note that w overwrites so no security hole here*/
		if (!fp) {    		
			fprintf(stderr,"%s\n", "Could not create task information file. Please check permissions");
			free(fileHash);
			exit(1);    		
    	} else {
    		/* Write out information to the file if there is any*/
    		fprintf(fp, "%s\n", structToWrite.taskName);
    		if(structToWrite.taskInfo != NULL && strstr(structToWrite.taskInfo,fileHash) == NULL)
    			fprintf(fp, "%s\n", structToWrite.taskInfo);
    		fflush(fp);
    		fclose(fp);
    	}
	}else{
		fp = fopen(taskInfoPath, "a+");
		if (!fp) {    		
			fprintf(stderr,"%s\n", "Could not create task sequence file. Please check permissions");
			free(fileHash);
			exit(1);    		
    	} else {
    		/* Write out information to the file if there is any*/

    		if(structToWrite.taskInfo != NULL && strstr(structToWrite.taskInfo,fileHash) == NULL)
    			fprintf(fp, "%s\n", structToWrite.taskInfo);
    		fflush(fp);
    		fclose(fp);
    	}
	}

	/*If the .seq file doesn't exist create it*/
	if((_tc_file_exists(taskSequencePath))==FALSE){
		fp = fopen(taskSequencePath, "w"); /* Note that w overwrites so no security hole here*/
		if (!fp) {
			fprintf(stderr,"%s\n", "Could not create task sequence file. Please check permissions");
			free(fileHash);
			exit(1);    		
		}
	}else{
		fp = fopen(taskSequencePath, "a+"); /* Note that w overwrites so no security hole here*/
		if (!fp) {
			fprintf(stderr,"%s\n", "Could not create task sequence file. Please check permissions");
			free(fileHash);
			exit(1);    		
		}
	}
    if(fp){
		/* Write out information to the sequence */    		
		switch( structToWrite.state ){
			case TC_TASK_PAUSED:
				timeToWrite = structToWrite.pauseTime;
				break;
			case TC_TASK_FINISHED:
				timeToWrite = structToWrite.endTime;
				break;
			case TC_TASK_STARTED:
			default:
				timeToWrite = time(0);
				break;
		}
		fprintf(fp, "%i %i %ld\n", structToWrite.seqNum, structToWrite.state, timeToWrite);
		fflush(fp);
		fclose(fp);
    }

    /* Update the index file */
    rawtime = time(0); /*If you're starting a task at the crux of midnight then maybe this will be wrong*/
	timeinfo = localtime (&rawtime);
	strftime(currentDate,80,"%Y%m%d",timeinfo);
	sprintf(indexFilePath,"%s/%s/%s.index",tcHomeDirectory,TC_INDEX_DIR,currentDate);

	/* We don't need to check for the index file's existence because tc-init takes care of that */
	fp = fopen(indexFilePath, "a");
	if(!fp) {
		/* Weird permission problem? */
		free(fileHash);
		fprintf(stderr, "%s\n", "Could not open index file for appending. Exiting");
		exit(1);
	}

	/* Append information to the index file 
	 * This information is in the format: <task hash> <state>
	*/
	fprintf(fp, "%s %s %i\n", fileHash, structToWrite.taskName, structToWrite.state);
	fflush(fp);
	fclose(fp);

	/* If the structure was just started then it is our current task */
	_tc_getCurrentTaskPath(currentTaskPath);
	fp = fopen(currentTaskPath,"w");
	if(!fp) {
		/* Something went wrong */
		free(fileHash);
		fprintf(stderr, "%s\n", "Could not open current task file for writing. Exiting");
		exit(1);
	}
	fprintf(fp, "%s\n", structToWrite.taskName);
	fprintf(fp, "%s\n", fileHash);
	fprintf(fp, "%i %i %ld\n", structToWrite.seqNum, structToWrite.state, timeToWrite);
	fflush(fp);
	fclose(fp);

	free(fileHash);
	
}

char * _tc_stateToString(int state){
	switch(state){
		case TC_TASK_NOT_FOUND:
			return "Task Not Found";
		case TC_TASK_FOUND:
			return "Task Found";
		case TC_TASK_FINISHED:
			return "Task Complete";
		case TC_TASK_PAUSED:
			return "Task Paused";
		case TC_TASK_STARTED:
			return "Task In Progress";
		default:
			return "Corrupted State";

	}
}

/* http://stackoverflow.com/a/122974/1808164 */
char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if( str == NULL )
            return NULL;

    if( str[0] == '\0' )
            return str;

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address
     * the first non-whitespace characters from
     * each end.
     */
    while( isspace(*(++frontp)) );
    while( isspace(*(--endp)) && endp != frontp );

    if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
            *str = '\0';

    /* Shift the string so that it starts at str so
     * that if it's dynamically allocated, we can
     * still free it on the returned pointer.  Note
     * the reuse of endp to mean the front of the
     * string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
            while( *frontp ) *endp++ = *frontp++;
            *endp = '\0';
    }


    return str;
}

void _tc_task_read_byHashPath(char const * taskHash, struct tc_task * structToFill){ 
	/* Attempt to fill the structure with data from the file */
	char taskSequencePath[TC_MAX_BUFF];
	char taskInfoPath[TC_MAX_BUFF];
	char taskName[TC_MAX_BUFF];
	FILE * fp;
	int seqNum, seqState;
	time_t priorTime,seqTime,runningTime;
	int priorState;

	/* Read the sequence information for the sequence number and timing info and the last state*/
	/* So to tease out the information we need to read the sequence information. The start time is 
	 * the first time in the sequence file by default, so lets open the sequence file:
	*/
	taskName[0] = '\0';
	strcpy(structToFill->taskName,taskName);


	sprintf(taskSequencePath,"%s/.tc/%s/%s.seq",_tc_getHomePath(),TC_TASK_DIR,taskHash);
	sprintf(taskInfoPath,"%s/.tc/%s/%s.info",_tc_getHomePath(),TC_TASK_DIR,taskHash);
	fp = fopen(taskSequencePath,"r");
	if(!fp){
		fprintf(stderr, "%s\n", "Could not find or open the sequence file for task.");
		structToFill->startTime = 0;
		structToFill->pauseTime = 0;
		structToFill->endTime = 0;
		structToFill->state =  TC_TASK_NOT_FOUND;
		return;
	}

	runningTime =  seqState = seqTime = 0;
	while( fscanf(fp, "%i %i %ld\n", &seqNum, &seqState, &seqTime) != EOF) {
		if (seqNum == 0) {
			structToFill->startTime = seqTime;
			priorTime = seqTime;
			priorState = seqState;
		} else {
			/* Calculate time spent on task */	
			if( priorState == TC_TASK_STARTED && (seqState == TC_TASK_PAUSED || seqState == TC_TASK_FINISHED) ) 
				runningTime = runningTime + (seqTime - priorTime);
			
			priorTime = seqTime;
			priorState = seqState;
			
		}
	}
	
	/* This occurs of the project just started and hasing had any stops yet*/
	if(runningTime == 0 && seqState == TC_TASK_STARTED ){
		runningTime =  time(0) - structToFill->startTime;
	}

	
	structToFill->endTime = seqTime;
	structToFill->state = seqState;
	structToFill->seqNum = seqNum+1;
	structToFill->pauseTime = runningTime;

	fclose(fp);


	fp = fopen(taskInfoPath,"r");
	if(!fp){
		fprintf(stderr, "%s\n", "Could not find or open the info file for task.");
		structToFill->state =  TC_TASK_NOT_FOUND;
		structToFill->startTime = 0;
		structToFill->pauseTime = 0;
		structToFill->endTime = 0;
		return;
	}

	/* The first line is the task name*/
	fgets(taskName,TC_MAX_BUFF,fp);
	taskName[strlen(taskName)-1] = '\0'; /* Remove the newline read in*/
	strcpy(structToFill->taskName,taskName);

	fclose(fp);
	/* Cheat a little bit 
	 * It's simple, we stored the NAME of the info file into the structure, then when we want to
	 * display the information we don't read the whole file into memory, but rather we open it
	 * then read it directly into an output stream. This saves us the trouble of realloc-ing memory
	 * or reading the file some buffer size at a time.for no reason.
	*/
	strcpy(structToFill->taskInfo , taskInfoPath);

}