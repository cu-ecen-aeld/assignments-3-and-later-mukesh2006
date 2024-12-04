#include <syslog.h>
#include<stdio.h>
#include <errno.h>

#define ARGC 3
#define ERROR 1
#define NO_ERROR 0


int main(int argc, char *argv[])
{
  char ident[]="writer.c program";
  char *file_name=NULL;
  char *string=NULL;
  FILE *fp=NULL;

  openlog(ident, LOG_PID, LOG_USER);

  if(argc !=ARGC)
  {
    syslog(LOG_PERROR, "Usage: ./writer <filename> <string>");
    return ERROR;
  }

  file_name=argv[1];
  // check the validity of the filename 
   if(NULL==file_name|| '\0' == file_name[0])
  {
    syslog(LOG_ERR, "\nEmpty File Name\n");
    return ERROR;
  }
  string = argv[2];
  if(NULL==string  || '\0'== string[0])
  {
    syslog(LOG_ERR, "\nEmpty String to write in the file\n");
    return ERROR;
  }

  fp=fopen(file_name,"w");
  if(NULL==fp)
  {
    syslog(LOG_ERR, "\nFetal Failure in creation of file: %s , Please check the filepath and name is valid.\n", file_name);
    return ERROR;
  }
  else
  {
    fprintf(fp, "%s", string);
    fclose(fp);
    syslog(LOG_INFO, "\nSucces: The string %s written in the file: %s\n", string, file_name);
  }

  return NO_ERROR;
}
