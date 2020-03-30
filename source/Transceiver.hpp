#include <iostream>

#ifndef LANFT_Transceiver_HPP
#define LANFT_Transceiver_HPP

class Transceiver {
protected:
    /**
     * tar a directory provided to tempFilePackage.tar.gz
     * @param path std::string
     * @return true if success, false otherwise
     */
    bool packFile(const std::string &path);

    /**
     * untar tempFilePackage.tar.gz
     */
    void unpackFile();

    /**
     * delete tempFilePackage.tar.gz
     */
    void deleteFile();

    /**
     * calculate the shasum 256 of tempFilePackage.tar.gz
     * @return shasum as std::string
     */
    std::string shasum() noexcept(false);
};

#endif //LANFT_Transceiver_HPP
