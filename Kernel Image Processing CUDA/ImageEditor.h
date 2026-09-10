#pragma once

#include "Image.h"
#include "Kernel.h"

namespace ImageEditor {
    enum class ComputeMode {
        AoS,
        SoA,
        CUDA
    };

    std::string modeToString(ComputeMode mode);

    //kernel image processing
    std::pair<Image*, long> convolveAoS(Image* image, Kernel* kernel);
    std::pair<Image*, long> convolveSoA(Image* image, Kernel* kernel);
    std::pair<Image*, long> convolveCUDA(Image* image, Kernel* kernel);
    std::pair<Image*, long> convolve(Image* image, Kernel* kernel, ComputeMode mode);

    //effects
    std::pair<Image*, long> sharpen(Image* image, ComputeMode mode);
    std::pair<Image*, long> gaussian_blur(Image* image, ComputeMode mode);
    std::pair<Image*, long> edge_detection_effect(Image* image, ComputeMode mode);
    std::pair<Image*, long> emboss(Image* image, ComputeMode mode);
}

