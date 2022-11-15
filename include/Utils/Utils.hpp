#include "main.hpp"

#include <vector>
#include <functional>

using namespace std;

namespace Utils
{
    string GetUserId();

    std::vector<string> GetCulprits(string stacktrace, bool createModal = false);

    vector<string> GetCrashesFromUser();
}