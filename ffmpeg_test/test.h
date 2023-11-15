#pragma once
#include <iostream>

class Test {
public:
    Test() {
        std::cout << "construct Test..." << std::endl;
    };

    ~Test() {
        std::cout << "destruct Test..." << std::endl;
    }
public:
    void testReadMp4();
    int testRtsp();
    int testCamera();
    int testMp4ToFlv();
private:

};