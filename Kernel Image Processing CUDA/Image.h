#pragma once

#include <iostream>
#include <string>

struct pixel8_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class Image {
private:
    pixel8_t* pixels;
    uint16_t width = 0;
    uint16_t height = 0;
    const int nChannels = 3;
public:
    static Image* LoadFromBmp(const std::string& filename);
    Image(uint16_t w, uint16_t h);
    Image(uint16_t w, uint16_t h, uint8_t* pixelDataRGB);
    ~Image();

    uint16_t get_nChannels();
    uint16_t get_width();
    uint16_t get_height();

    void get_raw_AoS(uint8_t*& buffer);
    void get_raw_SoA(uint8_t*& rbuffer, uint8_t*& gbuffer, uint8_t*& bbuffer);

    int SaveAsBmp(const std::string& filename);
};
