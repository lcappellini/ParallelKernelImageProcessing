
#include "Image.h"
#include <vector>
#include <fstream>

#pragma pack(push, 1)
typedef struct BITMAP_FILE_HEADER {
    uint16_t type;
    uint32_t size;
    uint16_t reserved[2];
    uint32_t off_bits;
} BITMAP_FILE_HEADER;
#pragma pack(pop)

typedef struct BITMAP_INFO_HEADER {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t x_pels_per_meter;
    int32_t y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;

} BITMAP_INFO_HEADER;


uint32_t greater_multiple(uint32_t value, uint32_t multiple) {
    uint32_t mod = value % multiple;

    if (0 != mod) {
        value += multiple - mod;
    }

    return value;
}

bool LoadBitmapImage(const std::string& filename, std::vector<uint8_t>* output,
    uint32_t* width, uint32_t* height, std::string* error = nullptr) {
    if (filename.empty() || !output) {
        if (error) {
            *error = "Invalid inputs to LoadBitmapImage.";
        }
        return false;
    }

    uint32_t bytes_read = 0;
    BITMAP_INFO_HEADER bih;
    BITMAP_FILE_HEADER bmf_header;

    std::ifstream input_file(filename, ::std::ios::in | ::std::ios::binary);

    if (!input_file.read((char*)&bmf_header, sizeof(BITMAP_FILE_HEADER))) {
        if (error) {
            *error = "Failed to read bitmap file header";
        }
        return false;
    }

    if (!input_file.read((char*)&bih, sizeof(BITMAP_INFO_HEADER))) {
        if (error) {
            *error = "Failed to read bitmap info header.\n";
        }
        return false;
    }

    if (bih.bit_count != 24) {
        if (error) {
            *error = "Unsupported bitmap data format.\n";
        }
        return false;
    }

    uint32_t image_row_pitch = bih.width * 3;
    uint32_t image_size = image_row_pitch * bih.height;

    output->resize(image_size);
    *width = bih.width;
    *height = bih.height;

    /* The BMP format requires each scanline to be 32 bit aligned, so we insert
       padding if necessary. */
    uint32_t scanline_padding = greater_multiple(bih.width * 3, 4) - (bih.width * 3);

    uint32_t row_stride = bih.width * 3;

    for (uint32_t i = 0; i < bih.height; i++) {
        uint32_t y_offset = (bih.height - 1 - i) * row_stride;
        uint8_t* dest_row = &output->at(y_offset);

        if (!input_file.read((char*)dest_row, row_stride)) {
            if (error)
                *error = "Abrupt error reading file.\n";
            return false;
        }

        uint32_t dummy = 0;
        if (!input_file.read((char*)&dummy, scanline_padding)) {
            if (error)
                *error = "Abrupt error reading file.\n";
            return false;
        }

        for (uint32_t j = 0; j < bih.width; j++) {
            uint32_t x_offset = y_offset + j * 3;
            std::swap(output->at(x_offset), output->at(x_offset + 2));
        }
    }

    return true;
}

bool SaveBitmapImage(const std::string& filename, std::vector<uint8_t>* input, uint32_t width,
    uint32_t height, std::string* error = nullptr) {
    if (filename.empty() || input->empty()) {
        if (error) {
            *error = "Invalid inputs to SaveBitmapImage.";
        }
        return false;
    }

    uint32_t bytes_written = 0;
    uint32_t total_image_bytes = (3 * width) * height;
    uint32_t header_size =
        sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER);

    BITMAP_FILE_HEADER bmf_header = { 0x4D42,  // BM
                                          header_size + total_image_bytes, 0, 0,
                                          header_size };

    BITMAP_INFO_HEADER bih = { sizeof(BITMAP_INFO_HEADER),
                                   width,
                                   height,
                                   1,
                                   24,
                                   0,
                                   total_image_bytes,
                                   0,
                                   0,
                                   0,
                                   0 };

    std::ofstream output_file(filename, ::std::ios::out | ::std::ios::binary);

    if (!output_file.write((char*)&bmf_header, sizeof(BITMAP_FILE_HEADER))) {
        if (error) {
            *error = "Failed to write bitmap file header";
        }
        return false;
    }

    if (!output_file.write((char*)&bih, sizeof(BITMAP_INFO_HEADER))) {
        if (error) {
            *error = "Failed to write bitmap info header.\n";
        }
        return false;
    }

    uint32_t image_row_pitch = bih.width * 3;
    uint32_t image_size = image_row_pitch * bih.height;
    uint32_t row_stride = bih.width * 3;

    /* The BMP format requires each scanline to be 32 bit aligned, so we insert
       padding if necessary. */
    uint32_t scanline_padding = greater_multiple(bih.width * 3, 4) - (bih.width * 3);

    for (uint32_t i = 0; i < height; i++) {
        uint32_t y_offset = (height - 1 - i) * row_stride;
        uint8_t* src_row = &input->at(y_offset);

        /* Swap the R and B channels (as BMP stores its data in BGR). */
        for (uint32_t j = 0; j < bih.width; j++) {
            uint32_t x_offset = y_offset + j * 3;
            uint8_t temp_channel = input->at(x_offset + 0);
            input->at(x_offset + 0) = input->at(x_offset + 2);
            input->at(x_offset + 2) = temp_channel;
        }

        if (!output_file.write((char*)src_row, row_stride)) {
            if (error) {
                *error = "Abrupt error writing file.\n";
            }
            return false;
        }

        uint32_t dummy = 0; /* Padding will always be < 4 bytes. */
        if (!output_file.write((char*)&dummy, scanline_padding)) {
            if (error) {
                *error = "Abrupt error writing file.\n";
            }
            return false;
        }
    }

    return true;
}

Image::Image(uint16_t w, uint16_t h) {
    width = w;
    height = h;
    pixels = new pixel8_t[width * height]();
}

Image::Image(uint16_t w, uint16_t h, uint8_t* pixelDataRGB) : Image(w, h) {
    memcpy(pixels, pixelDataRGB, w * h * nChannels);
}

Image::~Image() {
    delete[] pixels;
}

void Image::get_raw_AoS(uint8_t*& buffer) {
    size_t size = width * height * nChannels;
    buffer = new uint8_t[size];
    std::memcpy(buffer, pixels, size);
}

void Image::get_raw_SoA(uint8_t*& rbuffer, uint8_t*& gbuffer, uint8_t*& bbuffer) {
    size_t size = width * height;

    rbuffer = new uint8_t[size];
    gbuffer = new uint8_t[size];
    bbuffer = new uint8_t[size];

    for (size_t i = 0; i < size; ++i) {
        rbuffer[i] = pixels[i].r;
        gbuffer[i] = pixels[i].g;
        bbuffer[i] = pixels[i].b;
    }
}

Image* Image::LoadFromBmp(const std::string& filename) {
    uint32_t src_width = 0;
    uint32_t src_height = 0;
    std::vector<uint8_t> src_image;

    std::string error;
    if (!LoadBitmapImage(filename, &src_image, &src_width, &src_height, &error)) {
        std::cout << error << std::endl;
        return nullptr;
    }
    Image* img = new Image((uint16_t)src_width, (uint16_t)src_height, src_image.data());
    return img;
}


int Image::SaveAsBmp(const std::string& filename)
{
    std::vector<uint8_t> data((uint8_t*)pixels, ((uint8_t*)pixels) + width * height * nChannels);
    if (SaveBitmapImage(filename, &data, width, height))
        return 0;
    else
        return 1;
}

uint16_t Image::get_nChannels()
{
    return nChannels;
}

uint16_t Image::get_width() {
    return width;
}

uint16_t Image::get_height() {
    return height;
}

