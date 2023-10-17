#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <stdint.h>
#include <linux/limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "misc_util.h"

#define CMDSZ 1024
#define BUFSZ PIPE_BUF

void misc_pipeCmd(char *command, char **output)
{
    FILE *fp;
    char buf[BUFSZ];
    int len;

    *output = malloc(1);
    strcpy(*output, "");
    
    if((fp = popen(command, "r")) == NULL)
        return;
    
    while((fgets(buf, BUFSZ, fp)) != NULL)
    {
        len = strlen(*output) + strlen(buf);
        if((*output = realloc(*output, (sizeof(char) * (len+1)))) == NULL)
            return;
        strcat(*output, buf);
    }
    
    pclose(fp);
}

int misc_pipeCmd_ex(const char *cmd, char *output, int maxlen)
{
  FILE *fp;
  char buf[512];
  int len = 0;

  output[0] = '\0';
  if ((fp = popen(cmd, "r")) == NULL)
	return -1;

  while ((fgets(buf, sizeof(buf), fp)) != NULL) {
    int left = maxlen - len;
	len += snprintf(output + len, left, "%s", buf);
  }

  pclose(fp);

  return len;
}

void misc_system(const char *format, ...)
{
	char buf[CMDSZ] = "";
	va_list arg;

	va_start(arg, format);
	vsnprintf(buf, sizeof(buf), format, arg);
	va_end(arg);

	system(buf);
	usleep(1);
}

void misc_procEp(char *pidfile)
{
    FILE *fp;
    pid_t pid = -1;
    
	fp = fopen(pidfile,"r");
	if(fp)
    {
		fscanf(fp, "%d", &pid);
		fclose(fp);
	}
    
	if(pid != -1 && pid != getpid())
		kill(pid, SIGTERM);

	fp = fopen(pidfile, "w");
	pid = getpid();
    
	if(fp)
    {
		fprintf(fp, "%d", pid);
		fclose(fp);
	}
}

void misc_printFile(char *format, ...)
{
    va_list args;
    FILE *fp;

#define O_FILE "/tmp/debug"

    fp = fopen(O_FILE,"a+");
    if(!fp) return;

    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
}

void misc_printConsole(const char *format, ...)
{
    char buf[CMDSZ];
    char buf2[CMDSZ];
    va_list arg;
        
    memset(buf, '\0', sizeof(buf));
    memset(buf2, '\0', sizeof(buf2));
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);

    sprintf(buf2, "/bin/echo \"%s\" > /dev/console", buf);

    system(buf2);
    usleep(1);
}

int misc_isNullStr(const char *str)
{
    if((str == NULL)||str[0] == '\0')
        return 1;

    return 0;
}

/** 
 * Sleep for several seconds use select function
 * 
 * @param sec seconds we should sleep for
 * 
 * @return 0 is success, -1 is error
 */
int misc_selSleep(int sec)
{
    int ret = 0;
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    if(select (0, NULL, NULL, NULL, &tv) < 0)
    {
        perror("select");
        return -1;
    }

    return 0;
}
