#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include "socket_tools.h"

int main(int argc, const char **argv)
{
  const char *port = "2025";
  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);

  if (sfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  std::string input = "/identify";
  ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
  if (res == -1)
  {
    std::cout << strerror(errno) << std::endl;
  }

  // printf(">");
  // fflush(stdout);

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);

    FD_SET(STDIN_FILENO, &readSet);
    FD_SET(sfd, &readSet);

    int maxFd = std::max(STDIN_FILENO, sfd);
    timeval timeout = { 1, 0 };

    int selectResult = select(maxFd + 1, &readSet, NULL, NULL, &timeout);

    if (selectResult == -1)
    {
      fprintf(stderr, "Error during select: %s\n", strerror(errno));
      return 1;
    }

    if (FD_ISSET(STDIN_FILENO, &readSet))
    {
      std::string input;
      // printf(">");
      std::getline(std::cin, input);

      ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
      if (res == -1)
      {
        std::cout << strerror(errno) << std::endl;
      }
    }

    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr_in sin;
      socklen_t slen = sizeof(sockaddr_in);
      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, (sockaddr*)&sin, &slen);
      if (numBytes > 0)
      {
        printf("%s\n", buffer);
      }
    }
  }
  return 0;
}
