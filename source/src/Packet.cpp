#include "../include/Packet.hpp"

std::unique_ptr<unsigned char[]> Packet::serialize(const unsigned char* message, size_t size) {
    if (size + 4 > 0xFFFFFFFF) throw std::runtime_error("Message too long!"); // assert message size is not longer than 4 bytes

    auto serialized = std::make_unique<unsigned char[]>(size + 4); // extra 4 bytes for length as int

    // set the first 4 bytes to the length of the message
    serialized[0] = (size >> 24) & 0xFF;
    serialized[1] = (size >> 16) & 0xFF;
    serialized[2] = (size >> 8) & 0xFF;
    serialized[3] = size & 0xFF;

    // copy the message into the serialized array
    std::copy_n(message, size, serialized.get()+4);

    return serialized;
}

Packet Packet::deserialize(char *message) {
    unsigned int size =  (message[0] << 24) | (message[1] << 16) | (message[2] << 8) | message[3];

    auto deserialized = new char[size];

    std::copy_n(message+4, size, deserialized);

    return {message, deserialized, size};
}

std::unique_ptr<unsigned char[]> Packet::generateTerminationPacket() {
    auto endPacket = std::make_unique<unsigned char[]>(4);
    endPacket[0] = 0xFF;
    endPacket[1] = 0xFF;
    endPacket[2] = 0xFF;
    endPacket[3] = 0xFF;

    return endPacket;
}

Packet::Packet(char *serialized_message, char *deserialized_message, size_t length) {
    this->serialized_message = serialized_message;
    this->deserialized_message = deserialized_message;
    this->length = length;

}

Packet::~Packet() = default;
