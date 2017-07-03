#include "Identity.hpp"

using namespace std;
using namespace ZeroTier;

//g++ --shared -fPIC -o libgenerate.so generate.cpp Identity.cpp SHA512.cpp Salsa20.cpp Utils.cpp C25519.cp

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