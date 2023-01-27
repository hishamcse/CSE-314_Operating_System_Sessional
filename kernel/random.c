// Copied From : https://www.cs.virginia.edu/~cr4bd/4414/S2021/files/lcg_parkmiller_c.txt

static unsigned random_seed = 7;

#define RANDOM_MAX ((1u << 31u) - 1u)

__UINT64_TYPE__ lcg_parkmiller(unsigned *state)
{
    const __UINT64_TYPE__ N = 0x7fffffff;
    const __UINT64_TYPE__ G = 48271u;

    __UINT64_TYPE__ div = *state / (N / G);  /* max : 2,147,483,646 / 44,488 = 48,271 */
    __UINT64_TYPE__ rem = *state % (N / G);  /* max : 2,147,483,646 % 44,488 = 44,487 */

    __UINT64_TYPE__ a = rem * G;        /* max : 44,487 * 48,271 = 2,147,431,977 */
    __UINT64_TYPE__ b = div * (N % G);  /* max : 48,271 * 3,399 = 164,073,129 */

    return *state = (a > b) ? (a - b) : (a + (N - b));
}

__UINT64_TYPE__ next_random() {
    return lcg_parkmiller(&random_seed);
}
