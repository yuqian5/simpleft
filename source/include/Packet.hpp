#ifndef FT_PACKET_HPP
#define FT_PACKET_HPP


#include <string>
#include <cstdlib>
#include <memory>

class Packet {
public:
    size_t length;
    char* serialized_message;
    char* deserialized_message;

    Packet(char* serialized_message, char* deserialized_message, size_t length);

    ~Packet();

    static std::unique_ptr<unsigned char[]> serialize(const unsigned char* message, size_t size);

    static Packet deserialize(char* message);

    static std::unique_ptr<unsigned char[]> generateTerminationPacket();

};


#endif //FT_PACKET_HPP
