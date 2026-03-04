#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char *args[]) {
	char *writefile;
	char *writestr;
	int fd;
	ssize_t nr;

	openlog(NULL, 0, LOG_USER);

	if (argc != 3) {
		syslog(LOG_ERR, "Please provide writefile and writestr. e.g ./writer <writefile> <writestr>");
		return -1;
	}

	writefile = args[1];
	writestr = args[2];

	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

	fd = creat(writefile, 0644);
	if (fd == -1) {
		syslog(LOG_ERR, "Can not create a file");
		return -1;
	}

	nr = write(fd, writestr, strlen(writestr));
	if (nr == -1) {
		syslog(LOG_ERR, "Write failed");
		return -1;
	}

	return 0;
}

