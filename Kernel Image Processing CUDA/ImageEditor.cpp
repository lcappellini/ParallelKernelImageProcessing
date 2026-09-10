#include <iostream>
#include "ImageEditor.h"
#include <chrono>

using namespace std;

std::string ImageEditor::modeToString(ComputeMode mode) {
    switch (mode) {
    case ComputeMode::AoS:
        return "AoS";
    case ComputeMode::SoA:
        return "SoA";
    case ComputeMode::CUDA:
        return "CUDA";
    }
}

std::pair<Image*, long> ImageEditor::convolveAoS(Image* image, Kernel* kernel) {

    const uint8_t chs = image->get_nChannels();
    const uint16_t w = image->get_width();
    const uint16_t h = image->get_height();

    const uint16_t size = kernel->get_size();
    const uint16_t halfSize = size / 2;

    uint8_t* dst = new uint8_t[w * h * chs];

    uint8_t* srcAoS;
    image->get_raw_AoS(srcAoS);

    const float* kernelData = kernel->get_raw_data();

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint32_t dstIndex = (y * w + x) * chs;

            float sumR = 0;
            float sumG = 0;
            float sumB = 0;

            for (int ky = 0; ky < size; ky++) {
                int py = y + ky - halfSize;

                if (py < 0)
                    py = 0;
                else if (py >= h)
                    py = h - 1;

                for (int kx = 0; kx < size; kx++) {
                    int px = x + kx - halfSize;

                    if (px < 0)
                        px = 0;
                    else if (px >= w)
                        px = w - 1;

                    const uint32_t srcIndex = (py * w + px) * chs;

                    const float k = kernelData[ky * size + kx];
                    sumR += srcAoS[srcIndex] * k;
                    sumG += srcAoS[srcIndex + 1] * k;
                    sumB += srcAoS[srcIndex + 2] * k;
                }
            }

            if (sumR > 255)
                sumR = 255;
            else if (sumR < 0)
                sumR = 0;

            if (sumG > 255)
                sumG = 255;
            else if (sumG < 0)
                sumG = 0;

            if (sumB > 255)
                sumB = 255;
            else if (sumB < 0)
                sumB = 0;

            dst[dstIndex] = static_cast<uint8_t>(sumR);
            dst[dstIndex + 1] = static_cast<uint8_t>(sumG);
            dst[dstIndex + 2] = static_cast<uint8_t>(sumB);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    Image* resImage = new Image(w, h, dst);

    delete[] srcAoS;
    delete[] dst;

    return std::pair<Image*, long>(resImage, elapsed);
}

std::pair<Image*, long> ImageEditor::convolveSoA(Image* image, Kernel* kernel) {

    const uint8_t chs = image->get_nChannels();
    const uint16_t w = image->get_width();
    const uint16_t h = image->get_height();

    const uint16_t size = kernel->get_size();
    const uint16_t halfSize = size / 2;

    uint8_t* srcR;
    uint8_t* srcG;
    uint8_t* srcB;
    image->get_raw_SoA(srcR, srcG, srcB);

    uint8_t* dst = new uint8_t[w * h * chs];
    uint8_t* dstR = new uint8_t[w * h];
    uint8_t* dstG = new uint8_t[w * h];
    uint8_t* dstB = new uint8_t[w * h];

    const float* kernelData = kernel->get_raw_data();

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint32_t dstIndex = y * w + x;

            float sumR = 0;
            float sumG = 0;
            float sumB = 0;

            for (int ky = 0; ky < size; ky++) {
                int py = y + ky - halfSize;

                if (py < 0)
                    py = 0;
                else if (py >= h)
                    py = h - 1;

                for (int kx = 0; kx < size; kx++) {
                    int px = x + kx - halfSize;

                    if (px < 0)
                        px = 0;
                    else if (px >= w)
                        px = w - 1;

                    const uint32_t srcIndex = py * w + px;

                    const float k = kernelData[ky * size + kx];
                    sumR += srcR[srcIndex] * k;
                    sumG += srcG[srcIndex] * k;
                    sumB += srcB[srcIndex] * k;
                }
            }

            if (sumR > 255)
                sumR = 255;
            else if (sumR < 0)
                sumR = 0;

            if (sumG > 255)
                sumG = 255;
            else if (sumG < 0)
                sumG = 0;

            if (sumB > 255)
                sumB = 255;
            else if (sumB < 0)
                sumB = 0;

            dstR[dstIndex] = static_cast<uint8_t>(sumR);
            dstG[dstIndex] = static_cast<uint8_t>(sumG);
            dstB[dstIndex] = static_cast<uint8_t>(sumB);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    for (uint32_t i = 0; i < w * h; i++) {
        dst[i * chs] = dstR[i];
        dst[i * chs + 1] = dstG[i];
        dst[i * chs + 2] = dstB[i];
    }

    Image* resImage = new Image(w, h, dst);

    delete[] srcR;
    delete[] srcG;
    delete[] srcB;

    delete[] dstR;
    delete[] dstG;
    delete[] dstB;

    delete[] dst;

    return std::pair<Image*, long>(resImage, elapsed);
}

std::pair<Image*, long> ImageEditor::convolve(Image* image, Kernel* kernel, ComputeMode mode)
{
    switch (mode) {
    case ComputeMode::SoA:
        return ImageEditor::convolveSoA(image, kernel);
    case ComputeMode::AoS:
        return ImageEditor::convolveAoS(image, kernel);
    case ComputeMode::CUDA:
        return ImageEditor::convolveCUDA(image, kernel);
    }
}

std::pair<Image*, long> ImageEditor::sharpen(Image* image, ComputeMode mode) {
    Kernel* ker = new Kernel(3, new float[9] {0, -1, 0, -1, 5, -1, 0, -1, 0});
    return ImageEditor::convolve(image, ker, mode);
}

std::pair<Image*, long> ImageEditor::gaussian_blur(Image* image, ComputeMode mode) {
    Kernel* ker = new Kernel(3, new float[9] {1 / 16.0, 1 / 8.0, 1 / 16.0, 1 / 8.0, 1 / 4.0, 1 / 8.0, 1 / 16.0, 1 / 8.0, 1 / 16.0});
    return ImageEditor::convolve(image, ker, mode);;
}

std::pair<Image*, long> ImageEditor::edge_detection_effect(Image* image, ComputeMode mode) {
    Kernel* ker = new Kernel(3, new float[9] {-1, -1, -1, -1, 8, -1, -1, -1, -1});
    return ImageEditor::convolve(image, ker, mode);
}

std::pair<Image*, long> ImageEditor::emboss(Image* image, ComputeMode mode) {
    Kernel* ker = new Kernel(3, new float[9] {-2, -1, 0, -1, 1, 1, 0, 1, 2});
    return ImageEditor::convolve(image, ker, mode);
}
