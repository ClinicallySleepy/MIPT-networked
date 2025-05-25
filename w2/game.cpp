#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <sstream>

int player_id = 1;

struct PlayerInfo
{
  int id;
  std::string name;
  ENetPeer* peer;
};

std::unordered_map<ENetPeer*, PlayerInfo> players;

void broadcast(const std::string& msg, ENetPacketFlag flag, ENetPeer* except = nullptr)
{
  for (const auto& [peer, info] : players) {
    if (peer != except) {
      ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, flag);
      enet_peer_send(peer, 0, packet);
    }
  }
}

void add_player(ENetPeer* peer)
{
  PlayerInfo info;
  info.id = player_id++;
  info.name = "Player" + std::to_string(info.id);
  info.peer = peer;

  players[peer] = info;

  std::ostringstream oss;
  oss << "new_player:" << info.name << ":" << info.id;

  broadcast(oss.str(), ENET_PACKET_FLAG_RELIABLE, peer);
  {
    std::ostringstream oss;
    oss << "self:";
    oss << players[peer].name << ":" << players[peer].id;

    std::string message = oss.str();

    ENetPacket *packet = enet_packet_create(message.c_str(), message.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 0, packet);
  };

  std::cout << "Player joined: " << info.name << " (ID " << info.id << ")\n";
}

void send_players_list(ENetPeer* recipient) {
    std::ostringstream oss;
    oss << "players_list:";

    bool first = true;
    for (const auto& [peer, player] : players) {
        if (peer == recipient) continue;
        if (!first) oss << "|";
        oss << player.name << ":" << player.id;
        first = false;
    }

    std::string msg = oss.str();
    ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(recipient, 0, packet);
}

void broadcast_pings(ENetHost *server) {
    std::ostringstream oss;
    oss << "pings:";

    bool first = true;
    for (const auto& [peer, info] : players) {
        if (!first) oss << "|";
        // oss << info.id << ":" << peer->roundTripTime;
        oss << info.id << ":" << peer->lastRoundTripTime;
        first = false;
    }

    std::string message = oss.str();
    ENetPacket *packet = enet_packet_create(message.c_str(), message.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast(server, 0, packet);
}

void broadcast_positions(ENetHost *server, ENetPeer *peer, float x, float y) {
    std::ostringstream oss;
    oss << "positions:";
    oss << players[peer].id << ":" << x << ":" << y;

    std::string message = oss.str();
    broadcast(message.c_str(), ENET_PACKET_FLAG_UNSEQUENCED, peer);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10888;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastPingTime = timeStart;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_CONNECT:
          printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
          add_player(event.peer);
          send_players_list(event.peer);
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          printf("Packet received '%s'\n", event.packet->data);
          if (strncmp((char*)event.packet->data, "position:", 9) == 0)
          {
            float x, y;
            sscanf((char*)event.packet->data + 9, "%f:%f", &x, &y);
            printf("x = %f, y = %f\n", x, y);
            broadcast_positions(server, event.peer, x, y);
          }
          enet_packet_destroy(event.packet);
          break;
        default:
          break;
      };
    }

    uint32_t curTime = enet_time_get();

    if (curTime - lastPingTime > 500)
    {
      printf("Send ping\n");
      lastPingTime = curTime;
      broadcast_pings(server);
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

