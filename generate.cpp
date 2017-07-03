#include "Identity.hpp"

using namespace std;
using namespace ZeroTier;

//g++ --shared -fPIC -o libgenerate.so generate.cpp Identity.cpp SHA512.cpp Salsa20.cpp Utils.cpp C25519.cp

extern "C" {
    void generate(char **v) {
        Identity* id = new Identity();
        id->generate();
        string str = id->toString(true);

        *v = new char[str.length() + 1];
        strcpy(*v, str.c_str());
        return;
    }
}