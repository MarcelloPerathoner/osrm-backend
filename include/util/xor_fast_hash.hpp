#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <random>

#include <cstdint>

namespace osrm::util
{

/*
    This is an implementation of Tabulation hashing, which has suprising properties like
   universality.
    The space requirement is 2*2^16 = 256 kb of memory, which fits into L2 cache.
    Evaluation boils down to 10 or less assembly instruction on any recent X86 CPU:

    1: movq    table2(%rip), %rdx
    2: movl    %edi, %eax
    3: movzwl  %di, %edi
    4: shrl    $16, %eax
    5: movzwl  %ax, %eax
    6: movzbl  (%rdx,%rax), %eax
    7: movq    table1(%rip), %rdx
    8: xorb    (%rdx,%rdi), %al
    9: movzbl  %al, %eax
    10: ret

*/

constexpr size_t SIZE = 0x10000;
class XORFastHash
{
    std::array<std::uint32_t, SIZE> table;

  public:
    XORFastHash()
    {
        std::mt19937 generator(69); // impl. defined but deterministic default seed

        for (size_t i = 0; i < SIZE; ++i)
        {
            table[i] = generator();
        }
    }

    inline std::uint16_t operator()(const std::uint32_t originalValue) const
    {
        union
        {
            std::uint32_t u32;
            std::uint16_t u16[2];
        } u;

        u.u32 = originalValue;

        return table[u.u16[0]] ^ table[u.u16[1]];
    }
};
} // namespace osrm::util

#endif // XOR_FAST_HASH_HPP
