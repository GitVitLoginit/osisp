#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <linux/limits.h>

int ComparedFiles(char *path1, char *path2)
{
	FILE *First = fopen (path1, "r");
	FILE *Second = fopen (path2, "r");
	char *content1 = "", *content2 = "";

	if ((First == NULL) || (Second == NULL)) 
	{
		fprintf (stderr, "Error %s\n", strerror(errno));
		return 1;
	}

	while (fgetc(First) != EOF)
		content1 += fgetc(First);
	

	while (fgetc(Second) != EOF)
		content2 += fgetc(Second);

	if ((fclose (First) == EOF) || (fclose (Second) == EOF)) 
	{
		fprintf (stderr, "Error %s\n", strerror(errno));
		return 1;
	}

	return strcmp(content1, content2);
		
}

void CreateProcess(int processNum, int MAX_PROCESSES, char *path1, char *path2, struct stat st_dir1, struct stat st_dir2)
{
	int numOfBytes = 0;

	if (processNum > MAX_PROCESSES)
	{
		int stat;
		if (wait(&stat))
			processNum--;
	}
	
	pid_t pid; 
 	pid = fork (); 
	
	if (pid == 0)
	{
		numOfBytes = st_dir1.st_size + st_dir2.st_size;
		if(ComparedFiles(path1, path2) == 0)
			printf ("Pid = %d; Path of first file: %s; Path of second file: %s; Total size in bytes: %d; Result of comparing: files are similar\n", getpid(), path1, path2, numOfBytes);
		else
    		printf ("Pid = %d; Path of first file: %s; Path of second file: %s; Total size in bytes: %d; Result of comparing: files are different\n", getpid(), path1, path2, numOfBytes);
		exit(0);
	}
	else
		if(pid > 0)
			processNum++;
}

int CompareFiles(char *dir_1, char *dir_2, int MAX_PROCESSES)
{
	int processNum = 0;
	DIR *directory1 = opendir(dir_1); 
	DIR *directory2 = opendir(dir_2);
	struct dirent *currDirectory1, *currDirectory2;
	struct stat st_dir1, st_dir2;
	char path1[PATH_MAX], path2[PATH_MAX];

	if (directory1 == NULL)
	{
		fprintf(stderr,"Error opening directory:%s", dir_1);
		return 1;
	} 
	if (directory2 == NULL)
	{
		fprintf(stderr,"Error opening directory:%s", dir_2);
		return 1;
	}

	strcat(dir_1, "/");
	strcat(dir_2, "/");
	while ((currDirectory1 = readdir(directory1)) != NULL)
	{
		strcpy(path1, dir_1);	
		strcat(path1, currDirectory1->d_name);
		
		if (!stat(path1, &st_dir1)) 
		{
			if	(S_ISREG(st_dir1.st_mode))
			{
				rewinddir(directory2);
				while ((currDirectory2 = readdir(directory2)) != NULL)
				{
					strcpy(path2, dir_2);	
					strcat(path2, currDirectory2->d_name);
			
					if (!stat(path2, &st_dir2)) 
					{
						if	(S_ISREG(st_dir2.st_mode))
							CreateProcess(processNum, MAX_PROCESSES, path1, path2, st_dir1, st_dir2);
					}					
					else
					{
						fprintf(stderr, "Statting error %s: %s\n", dir_2, strerror( errno ));
						return 1;
					}	
				}
			}			
		}
		else
		{
			fprintf(stderr, "Statting error %s: %s\n", dir_1, strerror( errno ));
			return 1;
		}
	}	
	closedir(directory1);
	closedir(directory2);
}

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		puts("Wrong number of parameters\n");
		return 1;
	}
	
	char *path_dir_1 = realpath(argv[1], NULL); 
	char *path_dir_2 = realpath(argv[2], NULL);
		
	if (path_dir_1 == NULL)
	{
		fprintf(stderr,"Error opening directory:%s, %s", argv[1], strerror(errno));
		return 1;
	}
	if (path_dir_2 == NULL)
	{
		fprintf(stderr,"Error opening directory:%s, %s", argv[2], strerror(errno));
		return 1;
	}
		
	int MAX_PROCESSES = atoi(argv[3]); 
	if ( MAX_PROCESSES < 1)
	{
		fprintf(stderr, "Wrong value of argument.\n");
		return 1;
	}
		
	CompareFiles(path_dir_1, path_dir_2, MAX_PROCESSES);
	while (wait(NULL) > 0){}
	return 0;	
}
