#pragma once

#include <cuda_runtime.h>
#include <cstdint>
#include <chrono>
#include <iostream>
#include "ImageEditor.h"

#define TILE_WIDTH 16

__global__ void convolution2DKernel_2batch(const uint8_t* __restrict__ input, uint8_t* __restrict__ output, const float* __restrict__ mask, int width, int height, int channels, int maskSize)
{
    // buffer di shared memory condiviso da tutti i thread del blocco (extern perché la dimensione è nota solo a runtime)
    extern __shared__ uint8_t N_ds[];

    const int maskRadius = maskSize / 2;
    const int w = TILE_WIDTH + maskSize - 1;

    int tx = threadIdx.x;
    int ty = threadIdx.y;

    int outputCol = blockIdx.x * TILE_WIDTH + tx;
    int outputRow = blockIdx.y * TILE_WIDTH + ty;

    // first batch loading
    int dest = threadIdx.y * TILE_WIDTH + threadIdx.x;
    int destY = dest / w;
    int destX = dest % w;
    int srcY = blockIdx.y * TILE_WIDTH + destY - maskRadius;
    int srcX = blockIdx.x * TILE_WIDTH + destX - maskRadius;

    const int nIdx1 = (destY * w + destX) * channels;
    if (srcY >= 0 && srcY < height && srcX >= 0 && srcX < width) {
        const int srcBase = (srcY * width + srcX) * channels;
        for (int c = 0; c < channels; c++) {
            N_ds[nIdx1 + c] = input[srcBase + c];
        }
    }
    else {
        for (int c = 0; c < channels; c++) {
            N_ds[nIdx1 + c] = 0;
        }
    }

    // second batch loading
    dest = threadIdx.y * TILE_WIDTH + threadIdx.x + TILE_WIDTH * TILE_WIDTH;
    destY = dest / w;
    destX = dest % w;
    srcY = blockIdx.y * TILE_WIDTH + destY - maskRadius;
    srcX = blockIdx.x * TILE_WIDTH + destX - maskRadius;

    if (destY < w) {
        const int nIdx2 = (destY * w + destX) * channels;
        if (srcY >= 0 && srcY < height && srcX >= 0 && srcX < width) {
            const int srcBase = (srcY * width + srcX) * channels;
            for (int c = 0; c < channels; c++) {
                N_ds[nIdx2 + c] = input[srcBase + c];
            }
        }
        else {
            for (int c = 0; c < channels; c++) {
                N_ds[nIdx2 + c] = 0;
            }
        }
    }

    __syncthreads();

    // alculating output (tutti i canali) ----
    if (outputRow < height && outputCol < width) { // ty < TILE_WIDTH && tx < TILE_WIDTH sempre vero
        for (int c = 0; c < channels; c++) {
            float sum = 0.0f;
            for (int i = 0; i < maskSize; i++) {
                for (int j = 0; j < maskSize; j++) {
                    sum += mask[i * maskSize + j] * N_ds[((i + ty) * w + (j + tx)) * channels + c];
                }
            }
            // clamp risultato in [0, 255]
            sum = fminf(fmaxf(sum, 0.0f), 255.0f);
            // salva risultato nell'output (mem globale)
            output[(outputRow * width + outputCol) * channels + c] = (uint8_t)sum;
        }
    }
}

std::pair<Image*, long> ImageEditor::convolveCUDA(Image* image, Kernel* kernel) {
    // controllo se presente CUDA-capable GPU
    cudaError_t cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "No CUDA-capable GPU found!\n");
        return std::pair<Image*, long>(nullptr, -1);
    }
    //

    auto start = std::chrono::high_resolution_clock::now();

    const uint8_t chs = image->get_nChannels();
    const uint16_t w = image->get_width();
    const uint16_t h = image->get_height();
    const uint16_t maskSize = kernel->get_size();
    uint8_t* srcAoS;
    image->get_raw_AoS(srcAoS);
    const size_t imgBytes = w * h * chs;

    // allocazione e copia dati sulla memoria della GPU
    const float* hMask = kernel->get_raw_data();
    uint8_t* dIn = nullptr;
    uint8_t* dOut = nullptr;
    float* dMask = nullptr;
    cudaMalloc(&dIn, imgBytes);
    cudaMalloc(&dOut, imgBytes);
    cudaMalloc(&dMask, maskSize * maskSize * sizeof(float));

    cudaMemcpy(dIn, srcAoS, imgBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(dMask, hMask, maskSize * maskSize * sizeof(float), cudaMemcpyHostToDevice);
    //

    // definizione blocchi e lancio del kernel
    const int blockWidth = TILE_WIDTH + maskSize - 1;
    //dim3 dimBlock(blockWidth, blockWidth); // 1 batch
    dim3 dimBlock(TILE_WIDTH, TILE_WIDTH); // 2 batch
    dim3 dimGrid((w + TILE_WIDTH - 1) / TILE_WIDTH, (h + TILE_WIDTH - 1) / TILE_WIDTH);
    const size_t sharedBytes = blockWidth * blockWidth * chs;
    convolution2DKernel_2batch << <dimGrid, dimBlock, sharedBytes >> > (dIn, dOut, dMask, w, h, chs, maskSize);
    cudaDeviceSynchronize();
    //

    // retrieve dei dati dalla GPU e free
    uint8_t* dst = new uint8_t[w * h * chs];
    cudaMemcpy(dst, dOut, imgBytes, cudaMemcpyDeviceToHost);

    cudaFree(dIn);
    cudaFree(dOut);
    cudaFree(dMask);
    delete[] srcAoS;
    //

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "CUDA Elapsed: " << elapsed << std::endl;

    Image* resImage = new Image(w, h, dst);
    delete[] dst;

    return std::pair<Image*, long>(resImage, elapsed);
}

/*__global__ void convolution2DKernel_1batch(const uint8_t* __restrict__ input, uint8_t* __restrict__ output, const float* __restrict__ mask, int width, int height, int channels, int maskSize)
{
    // buffer di shared memory condiviso da tutti i thread del blocco (extern perché la dimensione è nota solo a runtime)
    extern __shared__ uint8_t sMem[];

    const int maskRadius = maskSize / 2;
    const int tx = threadIdx.x;
    const int ty = threadIdx.y;
    const int blockWidth = blockDim.x;

    const int outputCol = blockIdx.x * TILE_WIDTH + tx;
    const int outputRow = blockIdx.y * TILE_WIDTH + ty;

    const int inputCol = outputCol - maskRadius;
    const int inputRow = outputRow - maskRadius;

    const int sIdx = (ty * blockWidth + tx) * channels;

    // se gli indici sono fuori dall'immagine, prende i valori al bordo
    const int clampedX = max(0, min(inputCol, width - 1));
    const int clampedY = max(0, min(inputRow, height - 1));

    // caricamento nella shared memory
    for (int c = 0; c < channels; c++) {
        sMem[sIdx + c] = input[(clampedY * width + clampedX) * channels + c];
    }

    __syncthreads();

    // computazione classica, effettuata dai soli thread che ricadono dentro al tile
    if (tx < TILE_WIDTH && ty < TILE_WIDTH && outputCol < width && outputRow < height) {
        const int dstIndex = (outputRow * width + outputCol) * channels;

        for (int c = 0; c < channels; c++) {
            float sum = 0.0f;

            for (int ky = 0; ky < maskSize; ky++) {
                for (int kx = 0; kx < maskSize; kx++) {
                    sum += mask[ky * maskSize + kx] * sMem[((ty + ky) * blockWidth + (tx + kx)) * channels + c]; // coefficiente mask (costant memory), valore pixel (shared memory)
                }
            }

            // clamp risultato in [0, 255]
            sum = fminf(fmaxf(sum, 0.0f), 255.0f);
            // salva risultato nell'output (mem globale)
            output[dstIndex + c] = static_cast<uint8_t>(sum);
        }
    }
}*/