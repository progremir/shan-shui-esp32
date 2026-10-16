// Small-object allocator behind C++ new/delete.
//
// The painter makes millions of short-lived vectors and strings. ESP-IDF's
// malloc+free costs ~11 us on this board, which was a large share of the
// painting time. Blocks up to 512 bytes come from per-size free lists here
// (well under 1 us); larger ones go to the normal heap. Blocks are never
// returned to the heap, so the lists stay warm for the next chunk.

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>

#include <new>

namespace {

const uint16_t kSizes[] = {16, 32, 48, 64, 96, 128, 192, 256, 384, 512};
const int kClasses = sizeof(kSizes) / sizeof(kSizes[0]);
const uint32_t kBig = 0xffffffffu;
const size_t kSlab = 32 * 1024;
// Internal RAM is much faster than PSRAM; let small blocks use some of it.
const size_t kInternalBudget = 64 * 1024;

struct Header {
  uint32_t cls;
  uint32_t pad;  // keeps the payload 8-byte aligned
};

struct Node {
  Node* next;
};

Node* free_list[kClasses];
uint8_t* slab = nullptr;
size_t slab_left = 0;
size_t internal_used = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

int class_for(size_t n) {
  for (int i = 0; i < kClasses; i++)
    if (n <= kSizes[i]) return i;
  return -1;
}

void* big(size_t n) {
  Header* h = (Header*)malloc(n + sizeof(Header));
  if (!h) return nullptr;
  h->cls = kBig;
  return h + 1;
}

void* alloc(size_t n) {
  int c = class_for(n);
  if (c < 0) return big(n);
  size_t block = kSizes[c] + sizeof(Header);
  portENTER_CRITICAL(&mux);
  Node* node = free_list[c];
  if (node) {
    free_list[c] = node->next;
    portEXIT_CRITICAL(&mux);
    return node;  // the header in front of it still names class c
  }
  if (slab_left < block) {
    portEXIT_CRITICAL(&mux);
    // Refill outside the critical section; heap_caps_malloc may block.
    uint8_t* fresh = nullptr;
    bool internal = false;
    if (internal_used + kSlab <= kInternalBudget) {
      fresh = (uint8_t*)heap_caps_malloc(kSlab, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
      internal = fresh != nullptr;
    }
    if (!fresh) fresh = (uint8_t*)heap_caps_malloc(kSlab, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!fresh) return big(n);
    portENTER_CRITICAL(&mux);
    if (internal) internal_used += kSlab;
    // The old slab's tail is abandoned (< 520 bytes).
    slab = fresh;
    slab_left = kSlab;
  }
  Header* h = (Header*)slab;
  slab += block;
  slab_left -= block;
  portEXIT_CRITICAL(&mux);
  h->cls = (uint32_t)c;
  return h + 1;
}

void release(void* p) {
  if (!p) return;
  Header* h = (Header*)p - 1;
  if (h->cls == kBig) {
    free(h);
    return;
  }
  Node* node = (Node*)p;
  portENTER_CRITICAL(&mux);
  node->next = free_list[h->cls];
  free_list[h->cls] = node;
  portEXIT_CRITICAL(&mux);
}

}  // namespace

void* operator new(size_t n) {
  void* p = alloc(n);
  if (!p) abort();
  return p;
}
void* operator new[](size_t n) { return operator new(n); }
void* operator new(size_t n, const std::nothrow_t&) noexcept { return alloc(n); }
void* operator new[](size_t n, const std::nothrow_t&) noexcept { return alloc(n); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, size_t) noexcept { release(p); }
void operator delete[](void* p, size_t) noexcept { release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { release(p); }
