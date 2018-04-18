#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <string>

#define MAXBUF 1024
const std::string REQUEST 	   = "REQUEST";
const std::string REQUESTDELIM = "*";
const std::string BASEDIR 	   = "./Data";
const std::string SLASH 	   = "/";
const std::string OSFIFO 	   = "/tmp/osfifo";
const int PRIORITY_ONE 	 	   = 6;
const int PRIORITY_TWO 	       = 3;
const int PRIORITY_THREE       = 2;

#endif