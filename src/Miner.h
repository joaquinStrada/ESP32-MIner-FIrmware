#ifndef __MINER_H__
#define __MINER_H__

#include <Arduino.h>

class Miner {
    public:
        void setup(String name, String poolUrl, int poolPort, String walletAddress);
        void run(void);
    private:
        String _name;
        IPAddress _poolIp;
        bool _poolExist = false;
        int _poolPort;
        String _walletAddress;
    protected:
        uint8_t _byteArrayTarget[32];
        byte _interResult[32];
        byte _shaResult[32];
        long unsigned int _validShares;
        long unsigned int _invalidShares;
        uint32_t _timeMining;
};

#endif // __MINER_H__