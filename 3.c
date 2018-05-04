/*
Вариант 8
Написать программу подсчета всех периодов бит (0 и 1) в файлах заданного каталога и его подкаталогов. Пользователь задаёт имя каталога. Главный процесс открывает каталоги и запускает для каждого файла каталога отдельный процесс подсчета всех периодов бит. Каждый процесс выводит на экран свой pid, полный путь к файлу, общее число просмотренных байт и все периоды бит (0 и 1). Число одновременно работающих процессов не должно превышать N (вводится пользователем). Проверить работу программы для каталога /etc.
*/

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <wchar.h>
#include <locale.h>

#define errorPath "/tmp/error_log.txt"

typedef struct scannedInodes {
    int value;
    struct scannedInodes *next;
} scannedInodes;

void iterateDir(const char *nameOfProgram, FILE *errorLog, const char *nameOfDir, int *countProcesses, int *maxAmountOfProcesses, dev_t rootDev);

void CreateProcess(const char *nameOfProgram, FILE *errorLog, const char *entryPath, int *countProcesses, int *maxAmountOfProcesses);

typedef struct byteStruct{
    int type;        
    int value;
    int repeats;
    struct byteStruct *next;
} byteStruct;

int bitsCount(const char *nameOfProgram, FILE *errorLog, const char *entryPath);

void printList(byteStruct **head, char **result);

void incInList(byteStruct **head, int count, int value);

void printInStderr(FILE *errorLog);

int main(int argc, char *argv[])
{
    char *nameOfProgram = basename(argv[0]);

    FILE *errorLog = NULL;
    if ((errorLog = fopen(errorPath, "w+")) == NULL)
        fprintf(stderr, "%s: Unable create error log: %s\n", nameOfProgram, errorPath);

    if (argc != 3)
    {
        fprintf(stderr, "%s: %s ./%s [%s]\n", nameOfProgram, "Incorrect amount of parameters. Usage:", nameOfProgram, "directory");
        return 1;
    }

    int countProcesses = 0, maxAmountOfProcesses;

    maxAmountOfProcesses = atoi(argv[2]);
    if (maxAmountOfProcesses <= 0)
    {
        fprintf(stderr, "%s: Invalid value of a number of parameters.\n", nameOfProgram);
        return 1;
    }

    DIR *dirRoot;
    struct dirent *entryRoot;
    struct stat entryStatRoot;
    dev_t rootDev;
  
    if (lstat(argv[1], &entryStatRoot) != 0)
    {
        fprintf(errorLog, "%s: %s %s\n", nameOfProgram, argv[1], strerror(errno));
        printInStderr(errorLog);
        return 1;
    }
    else
    {
        rootDev = entryStatRoot.st_dev;
    }

    iterateDir(nameOfProgram, errorLog, argv[1], &countProcesses, &maxAmountOfProcesses, rootDev);

    int status;
    while (countProcesses > 0) 
    {
        wait(&status);
        countProcesses--;
    }

    printInStderr(errorLog);

    return 0;
}

void iterateDir(const char *nameOfProgram, FILE *errorLog, const char *nameOfDir, int *countProcesses, int *maxAmountOfProcesses, dev_t rootDev)
{
    DIR *dir = NULL;
    struct dirent *entry;
    struct stat entryStat;

    scannedInodes *head = NULL;
  // fprintf(errorLog, "!!! %d \n", *countProcesses);
    if ((dir = opendir(nameOfDir)) == NULL)
    {
         // fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
    }
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
            {
                if (entryStat.st_dev != rootDev)
                {    
                    continue;
                }
                // If the file is a directory
                if (entry->d_type == DT_DIR)
                {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;   

                    // Multiple hard links
                    if (entryStat.st_nlink > 1)
                    {
                        int inList = 0;
                        scannedInodes *tmp = head;
                        while (tmp != 0)
                        {
                            if (tmp->value == entryStat.st_ino)
                            {
                                inList = 1;
                            }
                            tmp = tmp->next;
                        }
            
                        // First occurence
                        if (!inList)
                        {
                            scannedInodes *tmp = (scannedInodes*) malloc(sizeof(scannedInodes));
                            tmp->value = entryStat.st_ino;
                            tmp->next = head;
                            head = tmp;
    
                            iterateDir(nameOfProgram, errorLog, entryPath, countProcesses, maxAmountOfProcesses, rootDev);
                   
	                        free(entryPath);
                        }
	                }
                    // Has a single hard link
                    else
                    {
                        iterateDir(nameOfProgram, errorLog, entryPath, countProcesses, maxAmountOfProcesses, rootDev); 

	                    free(entryPath);
                    }
                } 
                else if (entry->d_type == DT_REG)
                // If the file is a regular file
                {     
                    if (lstat(entryPath, &entryStat) != 0)
                    {
                        fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
                    }
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

                                CreateProcess(nameOfProgram, errorLog, entryPath, countProcesses, maxAmountOfProcesses);
                            }
                        }
                        else
                        {
                            CreateProcess(nameOfProgram, errorLog, entryPath, countProcesses, maxAmountOfProcesses);
                        } 
                    }
                    free(entryPath);
                }
            }
        }  

        if (errno == EBADF)
        {
            fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
            return;
        }
    
        if (closedir(dir) == -1)
        {
            fprintf(errorLog, "%s: %s %s\n", nameOfProgram, nameOfDir, strerror(errno));
            return;
        }
    }  
}

void CreateProcess(const char *nameOfProgram, FILE *errorLog, const char *entryPath, int *countProcesses, int *maxAmountOfProcesses)
{
    int status;
    pid_t pid;

    while (*countProcesses >= *maxAmountOfProcesses)
    {
        wait(&status);
        *countProcesses -= 1;
    }

    // Create a child process
    *countProcesses += 1;

    //fprintf(stdout, "%d \n", pid);
    pid = fork();

    // Check if it is the child process
    if (pid == 0) 
    {
        bitsCount(nameOfProgram, errorLog, entryPath);
        // *countProcesses -= 1;    
        //fprintf(stdout, "%d \n", getpid());  
        exit(0);
    }
}

int bitsCount(const char *nameOfProgram, FILE *errorLog, const char *entryPath)
{
    setlocale(LC_ALL, "");
    
    FILE *file = NULL;

    byteStruct *head = NULL;
   
    if ((file = fopen(entryPath, "rb")) == NULL) 
    {
        fprintf(errorLog, "%s: %s %s\n", nameOfProgram, entryPath, strerror(errno));        
        return 1;
    }

    int bits[9];
    int initialMargin = 1;
    int memorisedBit = 0;
    int count;
    int type;
    unsigned char bit = 1;  
    wchar_t c;
    char *result = (char *)calloc(1, sizeof(char));

    struct stat st;
    if (stat(entryPath, &st) == -1)
    {
        perror("stat");
    }
    else 
    {
        unsigned long long countBytes = (unsigned long long) st.st_size ;
    }

    do
    { 
        c = fgetwc(file);
        if ((c == WEOF) && (errno == EILSEQ))
        {
            errno = 0;
            c = fgetc(file);
        }
        else if (c == WEOF)
        {
            break;
        }
   // fprintf(stdout, "%c",c);
        
        int j = 1;
        for (int k = 7; k >= 0; k--)
        {
            bit = 1 << k;
            bits[j] = (bit & c) != 0;
            j++;
        } 
        bits[0] = memorisedBit; 

       /* bits[1] = 1;
        bits[2] = 1;
        bits[3] = 0;
        bits[4] = 0;
        bits[5] = 0;
        bits[6] = 0;
        bits[7] = 0;
        bits[8] = 1; */

        int i = 0 + initialMargin;
        initialMargin = 0;
        count = 1;
        while (i < 8)
        {
            if (bits[i] == bits[i + 1])
            {
                count++;
            }
            else
            {
                type = 0;
                if (bits[i] == type)
                {
                    incInList(&head, count, type);
                }
                else
                {
                    type = 1;
                    if (bits[i] == type)
                    {
                        incInList(&head, count, type);
                    }
                }
                count = 1;
            } 
            i++;
        } 
        memorisedBit = bits[7];


//fprintf(stdout, "%s %d %d %d %d %d %d %d %d %d \n", entryPath, bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7], bits[8]);

    } while (c != WEOF);

    incInList(&head, count, memorisedBit);

   // fprintf(stdout, "%s \n", bits);

    fclose(file); 

    printList(&head, &result);

    fprintf(stdout, "%d %s %s \n", getpid(), entryPath, result);
}

void incInList(byteStruct **head, int count, int type)
{
    byteStruct *tmp = *head;
    int inList = 0;
    while (tmp != 0)
    {
        if ((tmp->value == count) && (tmp->type == type))
        {                                                                      
            tmp->repeats++;
            inList = 1;
            break;
        }
        tmp = tmp->next;
    } 
    if (inList == 0)
    {
        byteStruct *tmp = (byteStruct*)calloc(1, sizeof(byteStruct));
        tmp->type = type;        
        tmp->value = count;
        tmp->repeats = 1;
        tmp->next = *head;
        *head = tmp;
    }
}

int countDigits(int number)
{
    int count = (number == 0) ? 1 : 0;
    while (number != 0)
    {
        number /= 10;
        count++;
    }
    return count;
}

void printList(byteStruct **head, char **result)
{
    char *buffer = (char *)calloc(1, sizeof(char));
    
    for (int i = 0; i < 2; i++) 
    {
        *result = (char *)realloc(*result, sizeof(*result) + 3 * sizeof(char));    
        (i == 0) ? strcat(*result, "0") : strcat(*result, "1");
        strcat(*result, ": ");

        byteStruct *tmp = *head;
        while (tmp != 0)
        {
            if (tmp->type == i)
            {
                buffer = (char *)realloc(buffer, ((countDigits(tmp->value) + 1) * sizeof(char)));
                sprintf(buffer, "%d+", tmp->value);  
                *result = (char *)realloc(*result, (sizeof(*result) + sizeof(buffer)) * sizeof(char));
                strcat(*result, buffer);
                
                buffer = (char *)realloc(buffer, ((countDigits(tmp->repeats) + 1) * sizeof(char)));
                sprintf(buffer, "%d ", tmp->repeats);    
                *result = (char *)realloc(*result, (sizeof(*result) + sizeof(buffer)) * sizeof(char));
                strcat(*result, buffer);
            }
            tmp = tmp->next;
        }
    }
}

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
