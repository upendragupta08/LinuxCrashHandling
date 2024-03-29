#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
int ExecuteCommand(char* cmd);

#define BUF_SIZE        1024
#define VERSION         1
#define REVISION        0
#define CORE_FILE_NAME  "coredump"
#define COMPRESS_CMD    "tar -zcvf"
#define FILE_EXT        ".tar.gz"


void Help(void);
void Install(char *path, char *logPath);
void GenerateCorefile(char *path);
void CompressCoreFile(char *path);

int main(int argc, char *argv[])
{
	char path[128];

	/* check for install argument */
	if (argc==3 && strcmp(argv[1], "--install")==0)
	{
		Install(argv[0], argv[2]);
		return 0;
	}

	if (argc==2 && strcmp(argv[1], "--version")==0) 
	{
		printf("linuxCrashHandler v%d.%d\n", VERSION, REVISION);
		return 0;
	}

	if (argc<3) 
	{
		Help();
		return -1;
	}

	if (argc >= 6)
	{
		/* save the core file, alongside the crash_report file */
		snprintf(path, sizeof(path), "%s""/coredump", argv[5]);
	}
	else
	{
		/* save the core file, alongside the crash_report file */
		snprintf(path, sizeof(path), "/home/coredump");
	}

	/* Path is defined now let's generate core dump file */
	GenerateCorefile(path);

    /* Generate zip file */
    CompressCoreFile(argv[5]);

	exit(EXIT_SUCCESS);
}

void Help(void)
{
	printf("Usage: linuxCrashHandler <pid> <sid> <uid> <gid>\n\n");
	printf("Under normal usage, the linuxCrashHandler is called directly\n");
	printf("by the Linux kernel, as is passed paramters as specified\n");
	printf("by /proc/sys/kernel/core_pattern.\n\n");

	printf("However, a few convenience options are provided:\n");
	printf("--install   Install linuxCrashHandler (register with kernel).\n");
	printf("            That is, to install the linuxCrashHandler program\n");
	printf("            on a system, copy the program to <any_dir> and do:\n");
	printf("            Here log_dir_path is path where core file generation is expected\n");
	printf("            $ <any_dir>/linuxCrashHandler --install --logdir=<log_dir_path>\n");
	printf("--version   show version information\n\n");
}

void Install(char *path, char *logPath)
{
	FILE *fp;
	char actualpath[PATH_MAX];
	char *ptr;
	char *logPtr;

	logPtr=strchr(logPath, '=');
	if(logPtr == NULL)
	{
		/*Invalid path applied*/
		logPath[0]='\0';
	}
	else
	{
		logPath=(logPtr+1);
	}

	fp = fopen("/proc/sys/kernel/core_pattern", "w");
	if (!fp) 
	{
		perror("Could not open core_pattern for installation\n");
		exit(1);
	}

	ptr = realpath(path, actualpath);
	if (!ptr) 
	{
		fprintf(stderr, "Couldn't find real path for %s\n", path);
	} 
	else
	{
		/* set the core_pattern */
		fprintf(fp, "|%s %%p %%s %%u %%g %s\n", actualpath, logPath);
	}
    fclose(fp);

	fp = fopen("/proc/sys/kernel/core_pipe_limit", "w");
    if (fp == NULL)
	{
		perror("Could not open core_pipe_limit for installation\n");
        exit(1);
	}

	fprintf(fp, "1\n");
	fclose(fp);
	printf("Installation done\n");
}

void GenerateCorefile(char *path)
{
    int         tot = 0;
    ssize_t     nread;
    char        buf[BUF_SIZE];
    int         core_out_fd;

    /* Open fd to write file */
	core_out_fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0600);

    /* Count bytes in standard input (the core dump) */
	while ((nread = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
	{
		if (core_out_fd>=0) 
		{
            write(core_out_fd, buf, (size_t)nread);
		}
		tot += nread;
	}

	fprintf(stderr, "Total bytes in core dump: %d\n", tot);
	printf("Total bytes in core dump: %d\n", tot);
	if (core_out_fd >= 0)
	{
		close(core_out_fd);
	}
}

int ExecuteCommand(char* cmd)
{
    FILE    *fP;
    int     sts = 0;

    if (NULL == cmd)
    {
        printf("%s :Command pass is NULL\n", __func__);
        return (-1);
    }

    /* open file to execute command */
    fP = popen(cmd, "w");
    if(NULL == fP)
    {
        printf(" popen: Failed to popen file:%s\n", strerror(errno));
        return (-1);
    }

    /*close file*/
    sts = pclose(fP);

    if (-1 == sts)
    {
        printf("popen : Failed to close file :%s\n",strerror(errno));
        return (-1);
    }

    return (WEXITSTATUS(sts));
}

void CompressCoreFile(char *path)
{
    char buff[PATH_MAX];

    /* Create tar.gz file with same name */
    snprintf(buff, PATH_MAX, " cd %s && " COMPRESS_CMD " "CORE_FILE_NAME FILE_EXT" "CORE_FILE_NAME, path);

    ExecuteCommand(buff);

    /* Remove input file */
    snprintf(buff, PATH_MAX, "rm %s/"CORE_FILE_NAME, path);

    ExecuteCommand(buff);
}
