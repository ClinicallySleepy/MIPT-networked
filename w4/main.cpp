#include <functional>
#include <algorithm> // min/max
#include "raylib.h"
#include <enet/enet.h>

#include <vector>
#include "entity.h"
#include "protocol.h"
#include <cstdio>


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;
static std::map<uint16_t, int> scoresMap;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float radius = 0.f;
  deserialize_snapshot(packet, eid, radius, x, y);
  // TODO: Direct adressing, of course!
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.radius = radius;
      e.x = x;
      e.y = y;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 800;
  int height = 600;
  InitWindow(width, height, "w4 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 1.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;

    bool eaten = false;
    bool grown = false;
    float radius; float x; float y;

    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          printf("new it\n");
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          printf("got it\n");
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        case E_SERVER_TO_CLIENT_GROWN:
          deserialize_grown(event.packet, radius);
          grown = true;
          break;
        case E_SERVER_TO_CLIENT_EATEN:
          deserialize_eaten(event.packet, radius, x, y);
          eaten = true;
          break;
        case E_SERVER_TO_CLIENT_SCORES_MAP:
          deserialize_scores_map(event.packet, scoresMap);
          break;
        };
        break;
      default:
        break;
      };
    }

    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      for (Entity &e : entities)
        if (e.eid == my_entity)
        {
          if (grown) {
            e.radius = radius;
          }
          if (eaten) {
            e.radius = radius;
            e.x = x;
            e.y = y;
          }
          // Update
          e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 100.f;
          e.y += ((up ? -dt : 0.f) + (down ? +dt : 0.f)) * 100.f;

          // Send
          send_entity_state(serverPeer, my_entity, e.radius, e.x, e.y);
        }
    }

    BeginDrawing();
      ClearBackground(GRAY);
      {
        int y = 20;
        int x = 20;
        for (const auto& [playerId, score] : scoresMap)
        {
          DrawText(TextFormat("ID %d: Score %d", playerId, score), x, y, 20, BLACK);
          y += 20;
        }
      };
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
          DrawCircleV(Vector2{e.x, e.y}, e.radius, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
