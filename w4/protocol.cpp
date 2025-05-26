#include "protocol.h"
#include <cstring> // memcpy
#include "Bitstream.h"

void send_join(ENetPeer *peer)
{
  Bitstream bs(E_CLIENT_TO_SERVER_JOIN);
  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float radius, float x, float y)
{
  Bitstream bs;
  bs.write(E_CLIENT_TO_SERVER_STATE);
  bs.write(eid);
  bs.write(radius);
  bs.write(x);
  bs.write(y);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float radius, float x, float y)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(radius);
  bs.write(x);
  bs.write(y);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

// pointless here
MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &radius, float &x, float &y)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(eid);
  bs.read(radius);
  bs.read(x);
  bs.read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &radius, float &x, float &y)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(eid);
  bs.read(radius);
  bs.read(x);
  bs.read(y);
}

void send_eaten(ENetPeer *peer, float radius, float x, float y)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_EATEN);
  bs.write(radius);
  bs.write(x);
  bs.write(y);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void deserialize_eaten(ENetPacket *packet, float &radius, float &x, float &y)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(radius);
  bs.read(x);
  bs.read(y);
}

void send_grown(ENetPeer *peer, float radius)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_GROWN);
  bs.write(radius);

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void deserialize_grown(ENetPacket *packet, float &radius)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();
  bs.read(radius);
}

void send_scores_map(ENetPeer *peer, const std::map<uint16_t, int> &scores)
{
  Bitstream bs;
  bs.write(E_SERVER_TO_CLIENT_SCORES_MAP);

  uint16_t count = static_cast<uint16_t>(scores.size());
  bs.write(count);

  for (const auto &[key, value] : scores)
  {
    bs.write(key);
    bs.write(value);
  }

  ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void deserialize_scores_map(ENetPacket *packet, std::map<uint16_t, int> &scores)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.skip<MessageType>();

  uint16_t count = 0;
  bs.read(count);

  scores.clear();
  for (uint16_t i = 0; i < count; ++i)
  {
    uint16_t key;
    int value;
    bs.read(key);
    bs.read(value);
    scores[key] = value;
  }
}
