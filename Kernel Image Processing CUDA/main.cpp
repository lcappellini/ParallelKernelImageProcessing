
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <sstream>
#include "Image.h"
#include "ImageEditor.h"
#include <filesystem>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


using namespace std;

std::vector<std::string> get_input_bmps() {
    std::vector<std::string> files;
    for (auto& entry : std::filesystem::directory_iterator(".")) {
        if (!entry.is_regular_file())
            continue;

        auto filename = entry.path().filename().string();
        if (filename.starts_with("in_") && entry.path().extension() == ".bmp")
            files.push_back(entry.path().string());
    }
    // ordinamento secondo explorer.exe, quindi con risoluzione crescente
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        std::wstring wa(a.begin(), a.end());
        std::wstring wb(b.begin(), b.end());
        return StrCmpLogicalW(wa.c_str(), wb.c_str()) < 0;
    });

    return files;
}

int main(int argc, char* argv[]) {

    int ATTEMPTS = 50;

    std::string config = "";

#ifdef CUDA_CONFIG
    config = "CUDA";
    const ImageEditor::ComputeMode modes[1] = { ImageEditor::ComputeMode::CUDA };
#elif defined(BASE_CONFIG)
    config = "BASE";
    ATTEMPTS = 5;
    const ImageEditor::ComputeMode modes[1] = { ImageEditor::ComputeMode::AoS };
#elif defined(OPENMP_CONFIG)
    config = "OpenMP";
    const ImageEditor::ComputeMode modes[1] = { ImageEditor::ComputeMode::AoS };
#elif defined(AVX2_CONFIG)
    config = "AVX2";
    const ImageEditor::ComputeMode modes[1] = { ImageEditor::ComputeMode::AoS };
#elif defined(OPENMP_AVX2_CONFIG)
    config = "OpenMP_AVX2";
    const ImageEditor::ComputeMode modes[1] = { ImageEditor::ComputeMode::AoS };
#elif defined(AOSVSSOA_CONFIG)
    config = "AoSvsSoA";
    const ImageEditor::ComputeMode modes[2] = { ImageEditor::ComputeMode::AoS, ImageEditor::ComputeMode::SoA };
#endif


    auto inputFiles = get_input_bmps();

    std::ofstream file("timings_" + config + ".csv");

    file << "size,mode,microseconds\n";

    for (std::string& inputFile : inputFiles) {

        cout << "Loading file: " << inputFile << endl;

        Image* img = Image::LoadFromBmp(inputFile);
        if (img == nullptr) {
            cerr << "ERROR: There was an error while loading the input image" << endl;
            exit(1);
        }

        for (ImageEditor::ComputeMode mode : modes) {

            std::string outputFile = "out_" + std::to_string(img->get_width()) + "x" + std::to_string(img->get_height()) + "_" + ImageEditor::modeToString(mode) + ".bmp";

            Image* new_img = nullptr;

            bool saved = false;
            for (int n = 0; n < ATTEMPTS; n++) {
                std::pair<Image*, long> result = ImageEditor::edge_detection_effect(img, mode);
                new_img = result.first;
                long elapsedMs = result.second;

                if (new_img == nullptr || elapsedMs == -1) {
                    cout << "[" << ImageEditor::modeToString(mode) << "] Error: result is null!" << endl;
                    continue;
                }

                if (new_img->get_width() != img->get_width() || new_img->get_height() != img->get_height()) {
                    cout << "[" << ImageEditor::modeToString(mode) << "] Error: size doesn't match!" << endl;
                    continue;
                }

                if (!saved && new_img) {
                    int res = new_img->SaveAsBmp(outputFile);
                    if (res > 0) {
                        cout << "[" << ImageEditor::modeToString(mode) << "] Error: Unable to save the image!" << endl;
                    }
                    saved = true;
                }

                cout << "[" << ImageEditor::modeToString(mode) << "] Elapsed: " << elapsedMs << "ms" << endl;

                file << new_img->get_width() << "x" << new_img->get_height() << "," << ImageEditor::modeToString(mode) << "," << elapsedMs << "\n";

                free(new_img);
            }
        }
    }

    file.close();

    return 0;
}