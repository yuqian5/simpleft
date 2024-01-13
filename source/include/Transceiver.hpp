#ifndef FT_Transceiver_HPP
#define FT_Transceiver_HPP

#include <iostream>
#include <cstring>

class Transceiver {
protected:
    /**
     * pack a directory provided to .ft_temp_pack.tar.gz
     * @param path std::string
     * @return true if success, false otherwise
     */
    static bool packFile(const std::string &path);

    /**
     * unpack .ft_temp_pack.tar.gz
     */
    static void unpackFile();

    /**
     * delete .ft_temp_pack.tar.gz
     */
    static void deletePackedFile();

    /**
     * delete .ft_temp_pack_buffer.tar.gz
     */
    static void deletePackedBufferFile();
};

#endif //FT_Transceiver_HPP
