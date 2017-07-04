#include "Identity.hpp"

using namespace std;
using namespace ZeroTier;

extern "C" {
    void generate(unsigned char *seed, char **out) {
        Identity* id = new Identity();
        id->generate(seed);
        string str = id->toString(true);

        *out = new char[str.length() + 1];
        strcpy(*out, str.c_str());
        return;
    }
}