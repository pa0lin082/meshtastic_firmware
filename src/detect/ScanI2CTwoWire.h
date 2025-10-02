#pragma once

#include "configuration.h"
#if !MESHTASTIC_EXCLUDE_I2C

#include <map>
#include <memory>
#include <stddef.h>
#include <stdint.h>

#include <Wire.h>

#include "ScanI2C.h"

#include "../concurrency/Lock.h"

class ScanI2CTwoWire : public ScanI2C
{
  public:
    void scanPort(ScanI2C::I2CPort) override;

    void scanPort(ScanI2C::I2CPort, uint8_t *, uint8_t) override;

    ScanI2C::FoundDevice find(ScanI2C::DeviceType) const override;

    bool exists(ScanI2C::DeviceType) const override;

    size_t countDevices() const override;

    static TwoWire *fetchI2CBus(ScanI2C::DeviceAddress);

  protected:
    FoundDevice firstOfOrNONE(size_t, DeviceType[]) const override;

  private:
    typedef struct RegisterLocation {
        DeviceAddress i2cAddress;
        RegisterAddress registerAddress;

        RegisterLocation(DeviceAddress deviceAddress, RegisterAddress registerAddress)
            : i2cAddress(deviceAddress), registerAddress(registerAddress)
        {
        }

    } RegisterLocation;

    typedef uint8_t ResponseWidth;

    std::map<ScanI2C::DeviceAddress, ScanI2C::DeviceType> foundDevices;

    // note: prone to overwriting if multiple devices of a type are added at different addresses (rare?)
    std::map<ScanI2C::DeviceType, ScanI2C::DeviceAddress> deviceAddresses;

    concurrency::Lock lock;

    uint16_t getRegisterValue(const RegisterLocation &, ResponseWidth, bool) const;

    DeviceType probeOLED(ScanI2C::DeviceAddress) const;

    static void logFoundDevice(const char *device, uint8_t address);
    bool testDS3231(TwoWire *i2cBus, uint8_t address)
    {
        // Leggi registro dei secondi (0x00)
        i2cBus->beginTransmission(address);
        i2cBus->write(0x00);
        if (i2cBus->endTransmission() != 0)
            return false;

        i2cBus->requestFrom(address, (uint8_t)7); // Leggi 7 registri tempo
        if (i2cBus->available() < 7)
            return false;

        uint8_t data[7];
        for (int i = 0; i < 7; i++) {
            data[i] = i2cBus->read();
        }

        // Verifica formato BCD valido per RTC
        // Secondi: 0-59, Minuti: 0-59, Ore: 0-23
        if ((data[0] & 0x0F) > 9 || ((data[0] >> 4) & 0x07) > 5)
            return false;
        if ((data[1] & 0x0F) > 9 || ((data[1] >> 4) & 0x07) > 5)
            return false;

        return true;
    }
};
#endif