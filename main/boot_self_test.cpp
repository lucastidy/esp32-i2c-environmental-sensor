#include <cstdint>

extern "C" {
    bool i2c_bus_init(void);
    bool i2c_bus_probe(uint8_t addr);
}

namespace {
class BootSelfTest {
public:
    static bool run() {
        if (!i2c_bus_init()) return false;

        // Probe sensors
        if (!i2c_bus_probe(0x38)) return false; // AHT20
        if (!i2c_bus_probe(0x77)) return false; // BMP280

        return true;
    }
};
} // namespace

extern "C" bool boot_self_test_run(void) {
    return BootSelfTest::run();
}
