#ifndef _MISC_UTIL_H_
#define _MISC_UTIL_H_

void misc_pipeCmd(char *command, char **output);
void misc_system(const char *format, ...);

/** 
 * Prevent from restarting the process, kill the process first if it's
 * running, then start and record pid to pidfile.
 * 
 * @param pidfile 
 */
void misc_procEp(char *pidfile);

void misc_printFile(char *format, ...);
void misc_printConsole(const char *format, ...);
int misc_isNullStr(const char *str);

#define SYSTEM misc_system

#endif
