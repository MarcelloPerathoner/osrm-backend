#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <boost/assert.hpp>

#include <random>

#include <cstdint>

namespace osrm::util
{

/**
 * Tabulation Hashing, see:
 * https://opendatastructures.org/ods-cpp/5_2_Linear_Probing.html#SECTION00923000000000000000
 */

class XORFastHash
{
    // 2KB which should comfortably fit into L1 cache
    std::uint16_t tab[4][0x100];

  public:
    XORFastHash()
    {
        union
        {
            std::uint64_t u64;
            std::uint16_t u16[4];
        } tmp;

        std::mt19937_64 generator(69); // impl. defined but deterministic default seed

        for (size_t i = 0; i < 0x100; ++i)
        {
            tmp.u64 = generator();
            tab[0][i] = tmp.u16[0];
            tab[1][i] = tmp.u16[1];
            tab[2][i] = tmp.u16[2];
            tab[3][i] = tmp.u16[3];
        }
    }

    inline std::uint16_t operator()(const std::uint32_t input) const
    {
        union
        {
            std::uint32_t u32;
            std::uint8_t u8[4];
        } in;

        in.u32 = input;
        return tab[0][in.u8[0]] ^ tab[1][in.u8[1]] ^ tab[2][in.u8[2]] ^ tab[3][in.u8[3]];
    }
};
} // namespace osrm::util

#endif // XOR_FAST_HASH_HPP
