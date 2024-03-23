#include <ctime>
#include <random>

std::mt19937 randomDevice(time(NULL));

int GenerateInt(int minValue, int maxValue)
{
    std::uniform_int_distribution<int> udist(minValue, maxValue);
    return udist(randomDevice);
}

float GenerateFloat(float minValue, float maxValue)
{
    std::uniform_real_distribution<float> udist(minValue, maxValue);
    return udist(randomDevice);
}
