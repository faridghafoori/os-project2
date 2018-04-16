#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "constants.h"

std::vector<std::string> split_string(const std::string &s, char delim);
int to_int(std::string str);
std::string my_to_string(int num);
std::vector<std::string> get_dir_list(std::string base_dir);
std::vector<std::string> get_file_list(std::string base_dir);

ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);
ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd);

#endif