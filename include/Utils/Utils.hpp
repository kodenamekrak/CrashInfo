#include "main.hpp"

#include <vector>

namespace Utils
{
    std::string GetUserId();

    std::vector<std::string> GetCulprits(std::string stacktrace);

    std::vector<std::string> GetCrashesFromUser();
}