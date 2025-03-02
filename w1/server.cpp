#include <algorithm>
#include <iterator>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "socket_tools.h"

struct Duel {
  int first;
  int second;
  int answer;
};

int main(int argc, const char **argv)
{
  const char *port = "2025";
  std::vector<sockaddr_in> clients;
  std::vector<Duel> duels;
  Duel incompleteDuel;
  bool pendingDuel = false;

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
  {
    printf("cannot create socket\n");
    return 1;
  }
  printf("listening!\n");

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 };
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


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
        std::string initialKeyword = "/identify";
        std::string broadcastKeyword = "/c";
        std::string duelKeyword = "/duel";
        std::string answerKeyword = "/ans";

        printf("(%s:%d) %s\n", inet_ntoa(sin.sin_addr), sin.sin_port, buffer); // assume that buffer is a string

        if (!strncmp(buffer, initialKeyword.c_str(), initialKeyword.length())) {
          const char *response = "Welcome!";
          printf("welcoming\n");
          ssize_t sentBytes = sendto(sfd, response, strlen(response), 0, (sockaddr*)&sin, slen);

          if (sentBytes == -1) {
            perror("Error sending response");
          }

          clients.push_back(sin);
        }
        else if (!strncmp(buffer, broadcastKeyword.c_str(), broadcastKeyword.length())) {
          std::string response = std::to_string(sin.sin_port) + ">" + std::string(buffer + broadcastKeyword.length());
          printf("broadcasting\n");

          for (const auto& client : clients) {
            if (client.sin_port != sin.sin_port) {
              ssize_t sentBytes = sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&client, slen);
            }
          }
        }
        else if (!strncmp(buffer, duelKeyword.c_str(), duelKeyword.length())) {
          printf("dueling\n");
          std::string response;

          bool alreadyInGame = std::find_if(duels.begin(), duels.end(), [&sin, &clients](const Duel &duel) {
              return clients[duel.first].sin_port == sin.sin_port || clients[duel.second].sin_port == sin.sin_port ;
          }) != duels.end() || (pendingDuel && clients[incompleteDuel.first].sin_port == sin.sin_port);

          if (alreadyInGame) {
            response = "Already in a duel";
            sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&sin, slen);
            continue;
          }

          const auto &client = std::find_if(clients.begin(), clients.end(), [&sin](const sockaddr_in &client) {
              return client.sin_port == sin.sin_port;
            });
          int index = std::distance(clients.begin(), client);

          if (!pendingDuel) {
            incompleteDuel.first = index;
            response = "Duel is pending";
            sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&clients[incompleteDuel.first], slen);
            pendingDuel = true;
          }
          else {
            incompleteDuel.second = index;
            incompleteDuel.answer = 34;
            response = "37 * 3 - 77 = ?";
            sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&clients[incompleteDuel.first], slen);
            sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&clients[incompleteDuel.second], slen);
            duels.push_back(incompleteDuel);
            pendingDuel = false;
          };
        }
        else if (!strncmp(buffer, answerKeyword.c_str(), answerKeyword.length())) {
          std::string response;
          printf("answering\n");

          const auto &duel = std::find_if(duels.begin(), duels.end(), [&sin, &clients](const Duel &duel) {
              return clients[duel.first].sin_port == sin.sin_port || clients[duel.second].sin_port == sin.sin_port ;
          });
          bool partOfDuel = duel != duels.end();

          if (partOfDuel) {
            int answer = std::stoi(buffer + answerKeyword.length());
            if (duel->answer == answer) {
              int winner = sin.sin_port == clients[duel->first].sin_port ? clients[duel->first].sin_port : clients[duel->second].sin_port;
              response = std::to_string(winner) + " won the duel";
              // sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&clients[incompleteDuel.first], slen);
              // sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&clients[incompleteDuel.second], slen);
              for (const auto& client : clients) {
                ssize_t sentBytes = sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&client, slen);
              }
              duels.erase(duel);
            }
            else {
              response = "Wrong answer";
              sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&sin, slen);
            };
          }
          else {
            response = "You are not participating in any duel";
            sendto(sfd, response.c_str(), response.length(), 0, (sockaddr*)&sin, slen);
            continue;
          };
        }
        else {
          printf("incorrect\n");
        }
      }
    }
  }
  return 0;
}
