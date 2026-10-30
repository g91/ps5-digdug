// Runtime pieces the game needs that the clean-room libc.prx shim does not
// promise to implement.  Defining them here binds every call (including the
// compiler's own memcpy/memset calls and libc++'s) to this executable.
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" {

void* memcpy(void* __restrict dst, const void* __restrict src, size_t n) {
    void* d = dst;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(src), "+c"(n) : : "memory");
    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    auto* d = static_cast<unsigned char*>(dst);
    auto* s = static_cast<const unsigned char*>(src);
    if (d <= s || d >= s + n) {
        __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
    } else {
        d += n - 1;
        s += n - 1;
        __asm__ volatile("std\n\trep movsb\n\tcld" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
    }
    return dst;
}

void* memset(void* dst, int value, size_t n) {
    void* d = dst;
    __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(value) : "memory");
    return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
    auto* p = static_cast<const unsigned char*>(a);
    auto* q = static_cast<const unsigned char*>(b);
    for (size_t i = 0; i < n; ++i) {
        __asm__ volatile("");   // keep the optimiser from turning this back into a call
        if (p[i] != q[i]) return p[i] < q[i] ? -1 : 1;
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) {
        __asm__ volatile("");
        ++n;
    }
    return n;
}

// xorshift32; the game only uses the low bits (rnd() & 0xFF, rand() & 1, ...)
static uint32_t randState = 0x2545F491u;

void srand(unsigned seed) { randState = seed ? seed : 0x2545F491u; }

int rand(void) {
    uint32_t x = randState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    randState = x;
    return (int)(x & 0x7FFFFFFF);
}

}  // extern "C"

// libc++ pieces normally provided by libc++.a, which this app does not link.
_LIBCPP_BEGIN_NAMESPACE_STD

void __libcpp_verbose_abort(const char*, ...) { __builtin_trap(); }

// <string> declares these basic_string<char> members extern; instantiate them
// here exactly as libc++'s own string.cpp does.
#define DD_INSTANTIATE(...) template __VA_ARGS__;
#ifdef _LIBCPP_ABI_STRING_OPTIMIZED_EXTERNAL_INSTANTIATION
_LIBCPP_STRING_UNSTABLE_EXTERN_TEMPLATE_LIST(DD_INSTANTIATE, char)
#else
_LIBCPP_STRING_V1_EXTERN_TEMPLATE_LIST(DD_INSTANTIATE, char)
#endif
#undef DD_INSTANTIATE
template string operator+ <char, char_traits<char>, allocator<char>>(char const*, string const&);

string to_string(int value) {
    char digits[12];
    int n = 0;
    unsigned v = value < 0 ? 0u - (unsigned)value : (unsigned)value;
    do {
        digits[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    string s;
    if (value < 0) s.push_back('-');
    while (n) s.push_back(digits[--n]);
    return s;
}

_LIBCPP_END_NAMESPACE_STD
