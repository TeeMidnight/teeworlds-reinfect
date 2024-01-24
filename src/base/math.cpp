#include <random>

#include "math.h"

static std::random_device RandomDevice;
static std::mt19937 RandomEngine(RandomDevice());

int random_int(int Min, int Max)
{
    std::uniform_int_distribution<> Distribution(Min, Max);

    return Distribution(RandomEngine);
} // infclassR