#include "util.h"

using namespace std;

vector<string> split_string(const string &s, char delim)
{
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

int to_int(string str)
{
    int result;
    stringstream ss(str);
    ss >> result;
    return result;
}

string my_to_string(int num)
{
    ostringstream convert;
    convert << num;
    return convert.str();
}

vector<string> get_dir_list(string base_dir)
{
    vector<string> result;
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    if ((dir = opendir(base_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            string dir_name(ent->d_name);
            if (dir_name[0] == '.')
                continue;
            string full_dir = base_dir + SLASH + dir_name;
            if (stat(full_dir.c_str(), &st) == -1)
                continue;
            const bool is_directory = (st.st_mode & S_IFDIR) != 0;
            if (!is_directory)
                continue;
            result.push_back(dir_name);
        }
        closedir(dir);
    }
    else
    {
        perror("");
        return result;
    }
    return result;
}

vector<string> get_file_list(string base_dir)
{
    vector<string> result;
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    if ((dir = opendir(base_dir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            string file_name(ent->d_name);
            if (file_name[0] == '.')
                continue;

            string full_file = base_dir + SLASH + file_name;
            if (stat(full_file.c_str(), &st) == -1)
                continue;
            const bool is_directory = (st.st_mode & S_IFDIR) != 0;
            if (is_directory)
                continue;
            result.push_back(file_name);
        }
        closedir(dir);
    }
    else
    {
        perror("");
        return result;
    }
    return result;
}

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr *cmsg;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1)
    {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        *((int *)CMSG_DATA(cmsg)) = fd;
    }
    else
    {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        printf("not passing fd\n");
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        perror("sendmsg");
    return size;
}

ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd)
{
    ssize_t size;

    if (fd)
    {
        struct msghdr msg;
        struct iovec iov;
        union {
            struct cmsghdr cmsghdr;
            char control[CMSG_SPACE(sizeof(int))];
        } cmsgu;
        struct cmsghdr *cmsg;

        iov.iov_base = buf;
        iov.iov_len = bufsize;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg(sock, &msg, 0);
        if (size < 0)
        {
            perror("recvmsg");
            exit(1);
        }
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
        {
            if (cmsg->cmsg_level != SOL_SOCKET)
            {
                fprintf(stderr, "invalid cmsg_level %d\n",
                        cmsg->cmsg_level);
                exit(1);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS)
            {
                fprintf(stderr, "invalid cmsg_type %d\n",
                        cmsg->cmsg_type);
                exit(1);
            }

            *fd = *((int *)CMSG_DATA(cmsg));
        }
        else
            *fd = -1;
    }
    else
    {
        size = read(sock, buf, bufsize);
        if (size < 0)
        {
            perror("read");
            exit(1);
        }
    }
    return size;
}