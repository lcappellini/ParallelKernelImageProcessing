#include <stdexcept>
#include "Kernel.h"

using namespace std;

Kernel::Kernel(uint16_t size, float* matrix) : size(size), matrix(matrix) {

}

uint16_t Kernel::get_size() const {
    return size;
}

const float* Kernel::get_raw_data()
{
    return matrix;
}

Kernel::~Kernel() {
    delete[] matrix;
}
