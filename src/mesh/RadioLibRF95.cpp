#if RADIOLIB_EXCLUDE_SX127X != 1
#include "RadioLibRF95.h"
#include "configuration.h"

// From datasheet but radiolib doesn't know anything about this
#define SX127X_REG_TCXO 0x4B

RadioLibRF95::RadioLibRF95(Module *mod) : SX1278(mod) {}

int16_t RadioLibRF95::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength,
                            uint8_t gain)
{
    auto toleratePortduinoSpiWriteFailure = [](const char *op, int16_t rc) -> int16_t {
#if defined(ARCH_PORTDUINO) || defined(ARDUINO_ARCH_PORTDUINO)
        if (rc == RADIOLIB_ERR_SPI_WRITE_FAILED) {
            LOG_WARN("RF95 begin: tolerating SPI write failure in %s (%d)", op, rc);
            return RADIOLIB_ERR_NONE;
        }
#endif
        return rc;
    };

    // execute common part
    uint8_t rf95versions[2] = {0x12, 0x11};
    int16_t state = SX127x::begin(rf95versions, sizeof(rf95versions), syncWord, preambleLength);
    LOG_INFO("RF95 begin: SX127x::begin returned %d", state);
    RADIOLIB_ASSERT(state);

    // current limit was removed from module' ctor
    // override default value (60 mA)
    state = setCurrentLimit(currentLimit);
    LOG_INFO("RF95 begin: setCurrentLimit(%f) -> %d", currentLimit, state);
    state = toleratePortduinoSpiWriteFailure("setCurrentLimit", state);

    // configure settings not accessible by API
    // state = config();
    RADIOLIB_ASSERT(state);

#ifdef RF95_TCXO
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_TCXO, 0x10 | _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_TCXO));
    RADIOLIB_ASSERT(state);
#endif

    // configure publicly accessible settings
    state = setFrequency(freq);
    LOG_INFO("RF95 begin: setFrequency(%f) -> %d", freq, state);
    state = toleratePortduinoSpiWriteFailure("setFrequency", state);
    RADIOLIB_ASSERT(state);

    state = setBandwidth(bw);
    LOG_INFO("RF95 begin: setBandwidth(%f) -> %d", bw, state);
    state = toleratePortduinoSpiWriteFailure("setBandwidth", state);
    RADIOLIB_ASSERT(state);

    state = setSpreadingFactor(sf);
    LOG_INFO("RF95 begin: setSpreadingFactor(%d) -> %d", sf, state);
    state = toleratePortduinoSpiWriteFailure("setSpreadingFactor", state);
    RADIOLIB_ASSERT(state);

    state = setCodingRate(cr);
    LOG_INFO("RF95 begin: setCodingRate(%d) -> %d", cr, state);
    state = toleratePortduinoSpiWriteFailure("setCodingRate", state);
    RADIOLIB_ASSERT(state);

#ifdef USE_RF95_RFO
    state = setOutputPower(power, true);
#else
    state = setOutputPower(power);
#endif
    LOG_INFO("RF95 begin: setOutputPower(%d) -> %d", power, state);
    state = toleratePortduinoSpiWriteFailure("setOutputPower", state);
    RADIOLIB_ASSERT(state);

    state = setGain(gain);
    LOG_INFO("RF95 begin: setGain(%d) -> %d", gain, state);
    state = toleratePortduinoSpiWriteFailure("setGain", state);

    return (state);
}

int16_t RadioLibRF95::setFrequency(float freq)
{
    // RADIOLIB_CHECK_RANGE(freq, 862.0, 1020.0, ERR_INVALID_FREQUENCY);

    // set frequency
    return (SX127x::setFrequencyRaw(freq));
}

#define RH_RF95_MODEM_STATUS_CLEAR 0x10
#define RH_RF95_MODEM_STATUS_HEADER_INFO_VALID 0x08
#define RH_RF95_MODEM_STATUS_RX_ONGOING 0x04
#define RH_RF95_MODEM_STATUS_SIGNAL_SYNCHRONIZED 0x02
#define RH_RF95_MODEM_STATUS_SIGNAL_DETECTED 0x01

bool RadioLibRF95::isReceiving()
{
    // 0x0b == Look for header info valid, signal synchronized or signal detected
    uint8_t reg = readReg(RADIOLIB_SX127X_REG_MODEM_STAT);
    // Serial.printf("reg %x", reg);
    return (reg & (RH_RF95_MODEM_STATUS_SIGNAL_DETECTED | RH_RF95_MODEM_STATUS_SIGNAL_SYNCHRONIZED |
                   RH_RF95_MODEM_STATUS_HEADER_INFO_VALID)) != 0;
}

uint8_t RadioLibRF95::readReg(uint8_t addr)
{
    Module *mod = this->getMod();
    return mod->SPIreadRegister(addr);
}
#endif
