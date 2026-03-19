#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#define PORT "9000" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

#define MAXDATASIZE 4096 // max number of bytes we can get at once
#define TMP_DATA_FILE "/var/tmp/aesdsocketdata"

static void sig_handler(int s) {
  syslog(LOG_INFO, "Caught signal, exiting\n");
  if (remove(TMP_DATA_FILE) != 0) {
    perror("remove");
  }

  exit(EXIT_SUCCESS);
}

void sigchld_handler(int s) {
  (void)s; // quiet unused variable warning

  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  // listen on sock_fd, new connection on new_fd
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address info
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  bool d_mode = false;

  if (argc == 2 && strcmp("-d", argv[1]) == 0) {
    d_mode = true;
  }

  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }

  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }

  openlog(NULL, 0, LOG_USER);
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL) {
    syslog(LOG_ERR, "server: failed to bind\n");
    exit(EXIT_FAILURE);
  }

  int pid;
  if (d_mode) {
    pid = fork();
  }
  // deamon and not a child
  if (d_mode && pid != 0) {
    return 0;
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  syslog(LOG_INFO, "server: waiting for connections...\n");

  while (1) { // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    syslog(LOG_INFO, "Accepted connection from %s\n", s);

    if (!fork()) {   // this is the child process
      close(sockfd); // child doesn't need the listener
                     // file buffer
      int numbytes, fd, nr;
      char buf[MAXDATASIZE];

      // socket buffer
      size_t capacity = 16; // Start small
      size_t length = 0;
      char *buffer = malloc(capacity);
      char c;

      if (buffer == NULL) {
        syslog(LOG_ERR, "Buffer allocation failed\n");
        exit(EXIT_FAILURE);
      }

      fd = open(TMP_DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror("creat");
        syslog(LOG_ERR, "Can not create a file");
        exit(EXIT_FAILURE);
      }
      while (recv(new_fd, &c, 1, 0) > 0) {
        if (length + 1 >= capacity) {
          capacity *= 2;
          char *new_ptr = realloc(buffer, capacity);
          if (new_ptr == NULL) {
            syslog(LOG_ERR, "Package too long\n");
            free(buffer);
            exit(EXIT_FAILURE);
          }
          buffer = new_ptr;
        }
        buffer[length++] = c;

        if (c == '\n') {
          break;
        }
      }

      buffer[length] = '\0';
      syslog(LOG_INFO, "server: got message %s\n", buf);

      nr = write(fd, buffer, length);
      if (nr == -1) {
        perror("write");
        syslog(LOG_ERR, "Write failed");
        exit(EXIT_FAILURE);
      }

      free(buffer);
      close(fd);

      fd = open(TMP_DATA_FILE, O_RDONLY);
      while ((numbytes = read(fd, buf, MAXDATASIZE)) > 0) {
        if (send(new_fd, buf, numbytes, 0) == -1) {
          perror("send");
        }
      }

      close(fd);
      close(new_fd);
      syslog(LOG_INFO, "Closed connection from %s", s);
      exit(EXIT_SUCCESS);
    }
    close(new_fd); // parent doesn't need this
  }

  return 0;
}
