#include "raylib.h"
#include <cstdio>
#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <unordered_map>

struct RemotePlayer
{
  int id;
  std::string name;
  float x = 0.0f;
  float y = 0.0f;
  int ping = 0;
};

int player_ping = 0;
std::string player_name = "";
int player_id = 0;

std::unordered_map<int, RemotePlayer> remotePlayers;

void send_fragmented_packet(ENetPeer *peer)
{
  const char *baseMsg = "Stay awhile and listen. ";
  const size_t msgLen = strlen(baseMsg);

  const size_t sendSize = 2500;
  char *hugeMessage = new char[sendSize];
  for (size_t i = 0; i < sendSize; ++i)
    hugeMessage[i] = baseMsg[i % msgLen];
  hugeMessage[sendSize-1] = '\0';

  ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

  delete[] hugeMessage;
}

void send_micro_packet(ENetPeer *peer)
{
  const char *msg = "dv/dt";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;
  InitWindow(width, height, "w2 MIPT networked");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet\n");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress lobby_address;
  ENetAddress game_address;
  enet_address_set_host(&lobby_address, "localhost");
  lobby_address.port = 10887;

  printf("Connecting to lobby at %u:%u\n", lobby_address.host, lobby_address.port);
  ENetPeer *lobbyPeer = enet_host_connect(client, &lobby_address, 2, 0);
  ENetPeer *gamePeer;
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby\n");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool lobby_connected = false;
  bool game_connected = false;
  float posx = GetRandomValue(100, 700);
  float posy = GetRandomValue(100, 500);
  float velx = 0.f;
  float vely = 0.f;

  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;

    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_CONNECT:
          printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
          if (event.peer == lobbyPeer)
          {
            lobby_connected = true;
          }
          else if (event.peer == gamePeer)
          {
            game_connected = true;
          }
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          if (event.peer == lobbyPeer)
          {
            printf("Packet received from lobby '%s'\n", event.packet->data);
            if (strncmp((char*)event.packet->data, "game:", 5) == 0)
            {
              char ip[64];
              int port;
              sscanf((char*)event.packet->data + 5, "%[^:]:%d", ip, &port);
              printf("Got game server information %s:%u\n", ip, port);

              if (enet_address_set_host(&game_address, ip) != 0)
              {
                printf("Failed to resolve host: %s\n", ip);
                return 1;
              }
              else
              {
                game_address.port = port;
                printf("Connecting to game at %u:%u\n", game_address.host, game_address.port);
                gamePeer = enet_host_connect(client, &game_address, 2, 0);
                if (!gamePeer) {
                  printf("Failed to establish connection with game server\n");
                  return 1;
                }
              }
            }
          }
          else if (event.peer == gamePeer)
          {
            printf("Packet received from game '%s'\n", event.packet->data);
            if (strncmp((char*)event.packet->data, "new_player:", 11) == 0)
            {
              char name[64];
              int id;
              sscanf((char*)event.packet->data + 11, "%[^:]:%d", name, &id);
              remotePlayers[id] = { id, name };
              printf("Added player %s with id %d\n", name, id);
            }
            if (strncmp((char*)event.packet->data, "self:", 5) == 0)
            {
              char name[64];
              int id;
              sscanf((char*)event.packet->data + 5, "%[^:]:%d", name, &id);
              player_id = id;
              player_name = name;
              printf("Self player %s with id %d\n", name, id);
            }
            else if (strncmp((char*)event.packet->data, "players_list:", 13) == 0)
            {
              char* data = (char*)event.packet->data + 13;
              char* token = strtok(data, "|");
              while (token)
              {
                char name[64];
                int id;
                sscanf(token, "%[^:]:%d", name, &id);
                remotePlayers[id] = { id, name };
                printf("Added player %s with id %d\n", name, id);
                token = strtok(nullptr, "|");
              }
            }
            else if (strncmp((char*)event.packet->data, "pings:", 6) == 0)
            {
              char* data = (char*)event.packet->data + 6;
              char* token = strtok(data, "|");

              while (token)
              {
                int id, ping;
                sscanf(token, "%d:%d", &id, &ping);
                if (remotePlayers.find(id) != remotePlayers.end()) {
                  remotePlayers[id].ping = ping;
                  printf("Got ping id:%d ping:%d\n", id, ping);
                }
                else
                {
                  player_ping = 0;
                }
                token = strtok(nullptr, "|");
              }
            }
            else if (strncmp((char*)event.packet->data, "positions:", 10) == 0)
            {
              int id;
              float x, y;
              sscanf((char*)event.packet->data + 10, "%d:%f:%f", &id, &x, &y);
              remotePlayers[id].x = x;
              remotePlayers[id].y = y;
              printf("Got position of id:%d x:%f y:%f\n", id, x, y);
            }
          }

          enet_packet_destroy(event.packet);
          break;
        default:
          break;
      };
    }

    if (game_connected)
    {
      uint32_t curTime = enet_time_get();
      // if (curTime - lastMicroSendTime > 100)
      // {
      //   lastMicroSendTime = curTime;
      //   send_micro_packet(lobbyPeer);
      // }
      char position_message[64];
      snprintf(position_message, sizeof(position_message), "position:%.2f:%.2f", posx, posy);
      ENetPacket *packet = enet_packet_create(position_message, strlen(position_message) + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(gamePeer, 0, packet);
    }

    if (lobby_connected && !game_connected && IsKeyPressed(KEY_ENTER)) {
      const char *start_message = "start";
      ENetPacket *packet = enet_packet_create(start_message, strlen(start_message) + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(lobbyPeer, 0, packet);
    }

    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);

    constexpr float accel = 30.f;
    velx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * accel;
    vely += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * accel;
    posx += velx * dt;
    posy += vely * dt;
    velx *= 0.99f;
    vely *= 0.99f;

    BeginDrawing();
    ClearBackground(BLACK);
    DrawText(TextFormat("Current status: %s", "unknown"), 20, 20, 20, WHITE);
    DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
    DrawText(TextFormat("SELF %d: %s | Ping: %d", player_id, player_name.c_str(), player_ping), 20, 60, 18, GRAY);
    DrawCircleV(Vector2{posx, posy}, 10.f, WHITE);
    DrawText("List of players:", 20, 80, 20, WHITE);
    int y = 100;
    for (const auto& [id, player] : remotePlayers)
    {
      DrawText(TextFormat("ID %d: %s | Ping: %d", player.id, player.name.c_str(), player.ping), 20, y, 18, GRAY);
      DrawCircleV(Vector2{player.x, player.y}, 10.f, RED);
      y += 20;
    }
    EndDrawing();
  }
  return 0;
}
