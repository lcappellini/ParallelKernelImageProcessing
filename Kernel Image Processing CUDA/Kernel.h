#pragma once

#include <cstdint>

class Kernel {
private:
    uint16_t size;
    float* matrix;
public:
    Kernel(uint16_t size, float* matrix);
    uint16_t get_size() const;
    const float* get_raw_data();
    ~Kernel();
};

