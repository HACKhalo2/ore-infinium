/******************************************************************************
 *   Copyright (C) 2013 by Shaun Reich <sreich@kde.org>                       *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU General Public License as           *
 *   published by the Free Software Foundation; either version 2 of           *
 *   the License, or (at your option) any later version.                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#include "packet.h"

#include "src/packet.pb.h"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "src/debug.h"

#include <iostream>
#include <fstream>

void Packet::serialize(std::stringstream* out, const google::protobuf::Message* message, uint32_t packetType)
{
    google::protobuf::io::OstreamOutputStream raw_out(out);
    google::protobuf::io::CodedOutputStream coded_out(&raw_out);

    std::string s;

    // write packet header, containing type of message we're sending
    PacketBuf::Packet p;
    p.set_type(packetType);
    p.SerializeToString(&s);

    coded_out.WriteVarint32(s.size());
    coded_out.WriteRaw(s.data(), s.size());

    // write actual contents
    message->SerializeToString(&s);

    coded_out.WriteVarint32(s.size());
    coded_out.WriteString(s);
}

uint32_t Packet::deserializePacketType(std::stringstream& in)
{
    google::protobuf::io::IstreamInputStream raw_in(&in);
    google::protobuf::io::CodedInputStream coded_in(&raw_in);

    std::string s;

    //packet header
    uint32_t msgSize;
    coded_in.ReadVarint32(&msgSize);

    assert(msgSize > 0);

    if (coded_in.ReadString(&s, msgSize)) {
        PacketBuf::Packet p;
        p.ParseFromString(s);

        in.clear();
        in.seekg(0, std::ios::beg);

        return p.type();
    } else {
        assert(0);
    }
}

void Packet::deserialize(std::stringstream* in, google::protobuf::Message* message)
{
    google::protobuf::io::IstreamInputStream raw_in(in);
    google::protobuf::io::CodedInputStream coded_in(&raw_in);

    std::string s;

    //packet header
    uint32_t msgSize;
    coded_in.ReadVarint32(&msgSize);
    assert(msgSize > 0);

    if (coded_in.ReadString(&s, msgSize)) {
        //unused, since deserializePacketType exists
        //PacketBuf::Packet p;
        //p.ParseFromString(s);
        //std::cout << "PACKET CONTENTS, PACKET TYPE:: " << p.type() << "\n";
    } else {
        assert(0);
    }

    //packet contents
    coded_in.ReadVarint32(&msgSize);

    if (coded_in.ReadString(&s, msgSize)) {
        message->ParseFromString(s);
    } else {
        assert(0);
    }

    in->clear();
    in->seekg(0, std::ios::beg);
}

void Packet::sendPacket(ENetPeer* peer, const google::protobuf::Message* message, uint32_t packetType, uint32_t enetPacketType)
{
    assert(peer && message);

    std::stringstream ss(std::stringstream::out | std::stringstream::binary);

    Packet::serialize(&ss, message, packetType);

    ENetPacket *packet = enet_packet_create(ss.str().data(), ss.str().size(), enetPacketType);
    assert(packet);

//Debug::log() << "SENDING PACKET";
    enet_peer_send(peer, 0, packet);
}

void Packet::sendPacketBroadcast(ENetHost* host, const google::protobuf::Message* message, uint32_t packetType, uint32_t enetPacketType)
{
    assert(host && message);
//Debug::log() << "SENDING PACKET BROAD";
    std::stringstream ss(std::stringstream::out | std::stringstream::binary);

    Packet::serialize(&ss, message, packetType);

    ENetPacket *packet = enet_packet_create(ss.str().data(), ss.str().size(), enetPacketType);
    assert(packet);

    enet_host_broadcast(host, 0, packet);
}
