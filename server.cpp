#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <iostream>
#include "constants.h"
#include "util.h"

#define STDIN 0

using namespace std;

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int check_priority_words(std::vector<string> data, std::vector<string> selected_words) {
	int count = 0;
	for(int i = 0; i < data.size(); i++) {
		for(int j = 0; j < selected_words.size(); j++) {
			if(data[i] == selected_words[j]) {
				count++;
			}
		}
	}
	return count;
}

int calc_priority_words(string words, string path) {
	vector<string> split_words     = split_string(words, '?');
	vector<string> first_priority  = split_string(split_words[0], ',');
	vector<string> second_priority = split_string(split_words[1], ',');
	vector<string> third_priority  = split_string(split_words[2], ',');
 	int report_words = 0;
 	int pri_one = 0, pri_two = 0, pri_three = 0;
	ifstream inFile;
	inFile.open(path.c_str());
	if (!inFile) {
		cout << "Unable to open file: " << path << endl;
		return -1;
	}
	string line;
	while (getline(inFile, line)) {
		vector<string> data = split_string(line, ' ');
		pri_one += (check_priority_words(data, first_priority) * PRIORITY_ONE);
		pri_two += (check_priority_words(data, second_priority) * PRIORITY_TWO);
		pri_three += (check_priority_words(data, third_priority) * PRIORITY_THREE);
		report_words = pri_one + pri_two + pri_three;
	}
	inFile.close();
	cout << "\nNumber of words with priority of one   : " << pri_one << " => * PRIORITY_ONE(6)  = " << pri_one*PRIORITY_ONE;
	cout << "\nNumber of words with priority of two   : " << pri_two << " => * PRIORITY_TWO(3)  = " << pri_two*PRIORITY_TWO;
	cout << "\nNumber of words with priority of three : " << pri_three << " => * PRIORITY_THREE(2)  = " << pri_three*PRIORITY_THREE << endl;
	cout << "--------------------------------------------------------------------------------" << endl;
	return report_words;
}

int browse_in_files(string words, string base_dir) {
	int temp = 0;
	vector<string> dir_list = get_dir_list(base_dir);
	vector<string> file_list = get_file_list(base_dir);

	for (int i = 0; i < dir_list.size(); i++) {
		const string child_dir = base_dir + SLASH + dir_list[i];
		int pipefd[2];
		char dir_fine[MAXBUF];
		pipe(pipefd);
		pid_t pid = fork();
		if (pid == 0) {
			cout << "Calculating child: " << child_dir << endl;
			int result = browse_in_files(words, child_dir);
			close(pipefd[0]);
			write(pipefd[1], my_to_string(result).c_str(), strlen(my_to_string(result).c_str()));
			kill(getpid(), SIGTERM);
		}
		else {
			close(pipefd[1]);
			int read_size = read(pipefd[0], dir_fine, MAXBUF);
			dir_fine[read_size] = '\0';
			string childData(dir_fine);
			temp += to_int(childData);
		}
	}

	for (int i = 0; i < file_list.size(); i++) {
		const string file_dir = base_dir + SLASH + file_list[i];
		int pipefd[2];
		char fileFine[MAXBUF];
		pipe(pipefd);
		pid_t pid = fork();
		if (pid == 0) {
			int result = calc_priority_words(words, file_dir);
			close(pipefd[0]);
			write(pipefd[1], my_to_string(result).c_str(), strlen(my_to_string(result).c_str()));
			cout << "Calculating file: " << file_dir << " = " << result << endl;
			kill(getpid(), SIGTERM);
		}
		else {
			close(pipefd[1]);
			int read_size = read(pipefd[0], fileFine, MAXBUF);
			fileFine[read_size] = '\0';
			string fileData(fileFine);
			temp += to_int(fileData);
		}
	}
	return temp;
}

void create_process_P2(int socket[]) {
	// P2 CHILD PROCESS
	pid_t pid = fork();
	if (pid == 0) {
		int fd;
		char buf[MAXBUF];

		while (true) {
			fd = open(OSFIFO.c_str(), O_RDONLY);
			read(fd, buf, MAXBUF);
			string message(buf);
			cout << "P2 got message : " << message << endl;
			if (message == "quit") {
				cout << "Killing P2 ... " << endl;
				close(fd);
				kill(getpid(), SIGTERM);
				break;
			}
			else {
				int fd;
				char buf[16];
				ssize_t size;
				close(socket[1]);
				size = sock_fd_read(socket[0], buf, sizeof(buf), &fd);
				if (fd != -1) {
					send(fd, message.c_str(), strlen(message.c_str()), 0);
					cout << "P2 Response: " << message << endl;
					close(fd);
				}
			}
			// EMPTY BUFFER
			buf[0] = '\0';
			close(fd);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		cout << "\nError : wrong command (The number of inputs is not appropriate)\n" << endl;
		exit(1);
	}
	int sharedSocket[2];
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sharedSocket) < 0) {
		perror("socketpair");
		exit(1);
	}
	
	create_process_P2(sharedSocket);

	mkfifo(OSFIFO.c_str(), 0666);

	fd_set master;
	fd_set read_fds;
	int fdmax;

	int listener;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;

	char buf[MAXBUF];
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

	int yes = 1;
	int i, j, rv;

	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	// argv[1] = system IP
	// argv[2] = PORT
	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}

		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	if (p == NULL) {
		cout << "selectserver: failed to bind" << endl;
		exit(2);
	}

	freeaddrinfo(ai);

	if (listen(listener, 10) == -1) {
		cerr << "listen" << endl;
		exit(3);
	}

	FD_SET(listener, &master);
	FD_SET(STDIN, &master);

	fdmax = listener;

	for (;;) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

					if (newfd == -1) {
						perror("accept");
					}
					else {
						FD_SET(newfd, &master);
						if (newfd > fdmax) {
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on "
							   "socket %d\n", inet_ntop(remoteaddr.ss_family, 
							   	get_in_addr((struct sockaddr *)&remoteaddr),
							   	remoteIP, INET6_ADDRSTRLEN), newfd);
					}
				}
				else if (i == STDIN) {
					char command_c[MAXBUF];
					if (read(STDIN, command_c, MAXBUF) < 0)
						printf("An error occurred in the read.");
					char *pos;
					if ((pos = strchr(command_c, '\n')) != NULL)
						*pos = '\0';

					string command(command_c);
					if (command == "quit") {
						int fd;
						fd = open(OSFIFO.c_str(), O_WRONLY);
						write(fd, "quit", sizeof("quit"));
						close(fd);
						close(listener);
						unlink(OSFIFO.c_str());
						break;
					}
					else {
						cout << "Invalid command!" << endl;
					}
				}
				else {
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
						if (nbytes == 0)
							printf("selectserver: socket %d hung up\n", i);
						else
							perror("recv");
						close(i);
						FD_CLR(i, &master);
					}
					else {
						buf[nbytes] = '\0';
						string request(buf);
						cout << "Request received: " << request << endl;
						vector<string> parse_req = split_string(request, '*');
						if (parse_req.size() == 2) {
							if (parse_req[0] == REQUEST) {
								pid_t pid = fork();
								if (pid == 0) {
									// P1 CHILD PROCESS
									string response = my_to_string(browse_in_files(parse_req[1], BASEDIR));

									// SEND MESSAGE TO P2 NAMED PIPE
									int fd;
									fd = open(OSFIFO.c_str(), O_WRONLY);
									write(fd, response.c_str(), sizeof(response.c_str()));
									close(fd);

									// SEND SOCKET FD TO P2
									close(sharedSocket[0]);
									ssize_t size;
									char temp[] = "1";
									size = sock_fd_write(sharedSocket[1], temp, 1, i);
									kill(getpid(), SIGTERM);
								}
								else {
									cout << "Processing request ..." << endl;
								}
							}
						}
						else {
							string response = "ERROR: Invalid request!";
							send(newfd, response.c_str(), strlen(response.c_str()), 0);
							cout << "Response: " << response << endl;
						}
					}
				}
			}
		}
	}

	return 0;
}