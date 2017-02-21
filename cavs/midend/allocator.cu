#include "cavs/midend/allocator.h"
#include "cavs/midend/devices.h"
#include "cavs/util/macros_gpu.h"

namespace midend {

class GPUAllocator : public Allocator {
 public:
  GPUAllocator() 
      : Allocator(DeviceTypeToString(GPU), GPU) {}    
  void* AllocateRaw(size_t nbytes) override {
    LOG(INFO) << "allocating " << nbytes << " bytes";
    void* ptr = NULL;
    checkCudaError(cudaMalloc(&ptr, nbytes)); 
    return ptr;
  }
  void DeallocateRaw(void* buf) override {
    checkCudaError(cudaFree(buf));
  }
};

Allocator* gpu_allocator() {
  static GPUAllocator gpu_alloc;
  return &gpu_alloc;
}
REGISTER_STATIC_ALLOCATOR("GPU", gpu_allocator());

} //namespace midend
