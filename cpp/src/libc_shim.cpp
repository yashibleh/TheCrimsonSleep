// Minimal freestanding memory primitives. clang may lower struct copies and
// zero-fills to these symbols; supplying them keeps the wasm build library-free.
#ifdef __wasm__

extern "C" {

void* memset(void* dest, int value, unsigned long count) {
  unsigned char* d = (unsigned char*)dest;
  unsigned char v = (unsigned char)value;
  for (unsigned long i = 0; i < count; ++i) d[i] = v;
  return dest;
}

void* memcpy(void* dest, const void* src, unsigned long count) {
  unsigned char* d = (unsigned char*)dest;
  const unsigned char* s = (const unsigned char*)src;
  for (unsigned long i = 0; i < count; ++i) d[i] = s[i];
  return dest;
}

void* memmove(void* dest, const void* src, unsigned long count) {
  unsigned char* d = (unsigned char*)dest;
  const unsigned char* s = (const unsigned char*)src;
  if (d == s || count == 0) return dest;
  if (d < s) {
    for (unsigned long i = 0; i < count; ++i) d[i] = s[i];
  } else {
    for (unsigned long i = count; i > 0; --i) d[i - 1] = s[i - 1];
  }
  return dest;
}

int memcmp(const void* a, const void* b, unsigned long count) {
  const unsigned char* pa = (const unsigned char*)a;
  const unsigned char* pb = (const unsigned char*)b;
  for (unsigned long i = 0; i < count; ++i) {
    if (pa[i] != pb[i]) return (int)pa[i] - (int)pb[i];
  }
  return 0;
}

} // extern "C"

#endif
