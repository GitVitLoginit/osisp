/*
Вариант 8
Подсчитать для заданного каталога (первый аргумент командной строки) и всех его подкаталогов суммарный размер занимаемого файлами на диске пространства в байтах и суммарный размер файлов. Вычислить коэффициент использования дискового пространства в %. Для получения размера занимаемого файлами на диске пространства использовать команду stat.
*/

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#define errorPath "/tmp/error_log.txt"
#define blockSize 512

typedef struct scannedInodes {
    int value;
    struct scannedInodes *next;
} scannedInodes;

scannedInodes *head = NULL;

dev_t rootDev;

void printInStderr(FILE *errorLog)
{
  fseek(errorLog, 0, SEEK_SET);

  int str = fgetc(errorLog);
  while (str != EOF) 
  {
    fputc(str, stderr);
    str = fgetc(errorLog);
  }

  fclose(errorLog);
  remove(errorPath);
}

void interateDir(const char *nameOfProgram, FILE *errorLog, const char *nameOfDir, long *totalSize, long *blockCount,  long *glTotalSize, long *glBlockCount)
{
  DIR *dir;
  struct dirent *entry;
  struct stat entryStat;
  
  if ((dir = opendir(nameOfDir)) == NULL)
    fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
  else
  {
    while ((entry = readdir(dir)) != NULL)
    {
      // Formation of a directory address
      char *entryPath = malloc(strlen(nameOfDir) + strlen(entry->d_name) + 2);
      strcpy(entryPath, nameOfDir);
      if (entryPath[strlen(nameOfDir) - 1] != '/')
        strcat(entryPath, "/");
      strcat(entryPath, entry->d_name); 

      if (lstat(entryPath, &entryStat) != 0)
        fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
      else
        if (entryStat.st_dev != rootDev)
          continue;

      // If the file is a directory
      if (entry->d_type == DT_DIR)
      {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;   

      // Multiple hard links
        if (entryStat.st_nlink > 1)
        {
          int inList = 0;
          scannedInodes *tmp=head;
            while (tmp!=0)
            {
                if (tmp->value == entryStat.st_ino)
                {
                    inList = 1;
                }
                tmp=tmp->next;
            }
            
            // First occurence
            if (!inList)
            {
              scannedInodes *tmp = (scannedInodes*) malloc(sizeof(scannedInodes));
              tmp->value = entryStat.st_ino;
              tmp->next = head;
              head = tmp;
              *totalSize += entryStat.st_size;
              *blockCount += entryStat.st_blocks;
              *glTotalSize += entryStat.st_size;
              *glBlockCount += entryStat.st_blocks;

              long totalSize = 0;
              long blockCount = 0;

              interateDir(nameOfProgram, errorLog, entryPath, &totalSize, &blockCount, glTotalSize, glBlockCount);

              // Printing results
              fprintf(stdout, "%s %ld %ld\n", entryPath, totalSize, blockCount * blockSize);     

              // Clearing variables
              //*totalSize = 0;
              //*blockCount = 0;

	      free(entryPath);
            }
	}
        // Has a single hard link
        else
        {
          *totalSize += entryStat.st_size;
          *blockCount += entryStat.st_blocks;
          *glTotalSize += entryStat.st_size;
          *glBlockCount += entryStat.st_blocks;

          long totalSize = 0;
          long blockCount = 0;

          interateDir(nameOfProgram, errorLog, entryPath, &totalSize, &blockCount, glTotalSize, glBlockCount);

          // Printing results
          fprintf(stdout, "%s %ld %ld\n", entryPath, totalSize, blockCount * blockSize);     

          // Clearing variables
          //*totalSize = 0;
          //*blockCount = 0;

	  free(entryPath);
        }
      }
      else if (entry->d_type == DT_REG)
      // If the file is a regular file
      {     
        if (lstat(entryPath, &entryStat) != 0)
          fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
        else
        {
          // Multiple hard links
          if (entryStat.st_nlink > 1)
          {
            int inList = 0;
            scannedInodes *tmp=head;
            while (tmp!=0)
            {
                if (tmp->value == entryStat.st_ino)
                {
                    inList = 1;
                }
                tmp=tmp->next;
            }
            
            // First occurence
            if (!inList)
            {
              scannedInodes *tmp = (scannedInodes*) malloc(sizeof(scannedInodes));
              tmp->value = entryStat.st_ino;;
              tmp->next = head;
              head = tmp;
              *totalSize += entryStat.st_size;
              *blockCount += entryStat.st_blocks;
              *glTotalSize += entryStat.st_size;
              *glBlockCount += entryStat.st_blocks;
            }
          }
          else
          {
            *totalSize += entryStat.st_size;
            *blockCount += entryStat.st_blocks;
            *glTotalSize += entryStat.st_size;
            *glBlockCount += entryStat.st_blocks;
            //printf("%s %ld %ld\n", entryPath, entryStat.st_size, entryStat.st_blocks);
          } 
        }
        free(entryPath);
      }
      else if (entry->d_type == DT_LNK)
      {
        char link[PATH_MAX];

        if (readlink(entryPath, link, PATH_MAX) == -1)
          fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
        else
        {
          int sizeOfLink = (int)strlen(link);
          
          *totalSize += sizeOfLink;
          *glTotalSize += sizeOfLink;

          *blockCount += sizeOfLink / blockSize;
          if (sizeOfLink % blockSize != 0)
          {
            *blockCount += 1;
          }
          *glBlockCount += sizeOfLink / blockSize;
          if (sizeOfLink % blockSize != 0)
          {
            *glBlockCount += 1;
          }

        }
        free(entryPath);
      }
    }  

    if (errno == EBADF)
      fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
    
    if (closedir(dir) == -1)
      fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
  }  
}

int main(int argc, char *argv[])
{
  char *nameOfProgram = basename(argv[0]);

  long totalSize = 0;
  long blockCount = 0;
  long glTotalSize = 0;
  long glBlockCount = 0;

  FILE *errorLog = NULL;
  if ((errorLog = fopen(errorPath, "w+")) == NULL)
    fprintf(stderr, "%s: Unable create error log: %s\n", nameOfProgram, errorPath);

  if (argc != 2)
  {
    fprintf(stderr, "%s: %s ./%s [%s]\n", nameOfProgram, "Incorrect amount of parameters. Usage:", nameOfProgram, "directory");
    return 1;
  }

  DIR *dirRoot;
  struct dirent *entryRoot;
  struct stat entryStatRoot;
  
    if (lstat(argv[1], &entryStatRoot) != 0)
      fprintf(errorLog, "%s: %s %s\n", nameOfProgram, argv[1], strerror(errno));
    else
      rootDev = entryStatRoot.st_dev;

  interateDir(nameOfProgram, errorLog, argv[1], &totalSize, &blockCount, &glTotalSize, &glBlockCount);
  
  float rate;
  if ((glBlockCount == 0) && (glBlockCount == 0))
    rate = 0;
  else
    rate = ((float)(glTotalSize * 100) / (float)(glBlockCount * blockSize));
  
  fprintf(stdout, "%s %ld %ld\n", argv[1], totalSize, blockCount * blockSize);     
  fprintf(stdout, "%ld %ld %f%%\n", glTotalSize, glBlockCount * blockSize, rate);  
  
  printInStderr(errorLog);
}
