#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int		count = 0, max_fd = 0;
int		ids[1111];
char	*msgs[1111];
char	buf[1111];
fd_set	rfds, wfds, afds;

void	fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				fatal_error();
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		fatal_error();
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	notify_others(int author, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, str, strlen(str), 0);
	}
}

void	register_client(int fd)
{
	max_fd = fd > max_fd ? fd : max_fd;
	ids[fd] = count++;
	msgs[fd] = NULL;
	FD_SET(fd, &afds);
	sprintf(buf, "server: client %d just arrived\n", ids[fd]);
	notify_others(fd, buf);
}

void	remove_client(int fd)
{
	close(fd);
	free(msgs[fd]);
	FD_CLR(fd, &afds);
	sprintf(buf, "server: client %d just left\n", ids[fd]);
	notify_others(fd, buf);
}

void	send_message(int fd)
{
	char	*msg;
	while (extract_message(&msgs[fd], &msg))
	{
		sprintf(buf, "client %d: ", ids[fd]);
		notify_others(fd, buf);
		notify_others(fd, msg);
		free(msg);
	}
}

int main(int ac, char **av) {
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	max_fd = sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatal_error();
	bzero(&servaddr, sizeof(servaddr));
	FD_ZERO(&afds);
	FD_SET(sockfd, &afds);

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	if (listen(sockfd, 10) != 0)
		fatal_error();

	while (1)
	{
		rfds = wfds = afds;
		if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();

		
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))
				continue;
			
			if (fd == sockfd)
			{
				len = sizeof(cli);
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0)
					fatal_error();
				register_client(connfd);
				break;
			}
			int read_bytes = recv(fd, buf, 1000, 0);
			if (read_bytes <= 0)
			{
				remove_client(fd);
				break;
			}
			buf[read_bytes] = 0;
			msgs[fd] = str_join(msgs[fd], buf);
			send_message(fd);
		}
	}
}