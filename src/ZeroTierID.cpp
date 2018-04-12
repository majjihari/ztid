#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "generate.hpp"

// fixed seed
static const char *seed = "2f727c7f12896af73cc89c48ca0a00d3"
                          "66b728eb42ae8bcdd293538fddfdc05b"
                          "0d0a52d2278fa092d74f3524053e44a1"
                          "fae905c8ee9af42d567e789f819e7094";

char *file_read(const char *filename) {
    char buffer[512];
    ssize_t readval;
    int fd;

    if((fd = open(filename, O_RDONLY)) < 0) {
        perror(filename);
        return NULL;
    }

    if((readval = read(fd, buffer, sizeof(buffer))) < 0) {
        perror(filename);
        close(fd);
        return NULL;
    }

    buffer[readval - 1] = '\0';

    close(fd);
    return strdup(buffer);
}

int file_write(const char *filename, char *payload, int mode) {
    int fd;

    if((fd = open(filename, O_CREAT | O_RDWR, mode)) < 0) {
        perror(filename);
        return 1;
    }

    if(write(fd, payload, strlen(payload)) != strlen(payload)) {
        perror(filename);
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}


int dir_exists(char *path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

int dir_create(char *path) {
    char tmp[PATH_MAX], *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    }

    return mkdir(tmp, S_IRWXU);
}

// read macaddress from interface
// update provided hash context with the macaddress
int hash_update_interface(SHA512_CTX *ctx, char *interface) {
    char filename[PATH_MAX];
    char *buffer;

    sprintf(filename, "/sys/class/net/%s/address", interface);

    if(!(buffer = file_read(filename)))
        return 1;

    printf("[+] adding macaddress: %s\n", buffer);

    SHA512_Update(ctx, buffer, strlen(buffer));
    free(buffer);

    return 0;
}

// walk over all the network interface
// only keep physical connected interface
// update hash using macaddress
int hash_system_mac_address(SHA512_CTX *ctx) {
    struct ifaddrs *addrs, *tmp;
    char devlink[PATH_MAX], temp[8];
    int count = 0;

    getifaddrs(&addrs);

    for(tmp = addrs; tmp; tmp = tmp->ifa_next) {
        if(!tmp->ifa_addr || tmp->ifa_addr->sa_family != AF_PACKET)
            continue;

        // point to symlink which exists only for physical interface
        sprintf(devlink, "/sys/class/net/%s/device", tmp->ifa_name);

        // not a physical interface
        if(readlink(devlink, temp, sizeof(temp)) < 0)
            continue;

        printf("[+] eligible interface: %s\n", tmp->ifa_name);
        hash_update_interface(ctx, tmp->ifa_name);

        count += 1;
    }

    freeifaddrs(addrs);

    return count;
}

int hash_system_boardid(SHA512_CTX *ctx) {
    char *buffer;

    if(!(buffer = file_read("/sys/devices/virtual/dmi/id/board_serial")))
        return 1;

    SHA512_Update(ctx, buffer, strlen(buffer));

    return 0;
}

// generate a seed related to this machine
unsigned char *get_complete_seed() {
    unsigned char *hash = (unsigned char *) calloc(SHA512_DIGEST_LENGTH, 1);
    SHA512_CTX sh;

    // initialize the hash
    SHA512_Init(&sh);

    // add fixed seed
    SHA512_Update(&sh, seed, strlen(seed));

    // add motherboard id if available
    hash_system_boardid(&sh);

    // add all physical interface mac-address
    hash_system_mac_address(&sh);

    // generate the hash
    SHA512_Final(hash, &sh);

    printf("[+] final seed: ");

    for(int i = 0; i < SHA512_DIGEST_LENGTH; i++)
        printf("%02x", hash[i] & 0xff);

    printf("\n");

    return hash;
}

int main(int argc, char *argv[]) {
    unsigned char *realseed;
    char *pubident, *needle, *buffer;
    char *output, filename[PATH_MAX];

    output = (argc < 2) ? (char *) "./" : argv[1];
    printf("[+] output directory: %s\n", output);

    // get seed for this computer
    realseed = get_complete_seed();

    // generate id from seed
    generate(realseed, &buffer);
    free(realseed);

    // remove private part from buffer
    // to keep punlic data
    if(!(needle = strrchr(buffer, ':'))) {
        fprintf(stderr, "invalid buffer from generator\n");
        exit(EXIT_FAILURE);
    }

    if(!(pubident = strndup(buffer, needle - buffer))) {
        perror("strndup");
        exit(EXIT_FAILURE);
    }

    // dump data
    printf("[+] public: <%s>\n", pubident);
    printf("[+] private: <%s>\n", buffer);

    // writing files
    if(!dir_exists(output)) {
        if(dir_create(output) < 0) {
            perror(output);
            exit(EXIT_FAILURE);
        }
    }

    sprintf(filename, "%s/identity.public", output);

    printf("[+] writing public file: %s\n", filename);
    file_write(filename, pubident, 0644);

    sprintf(filename, "%s/identity.secret", output);

    printf("[+] writing private file: %s\n", filename);
    file_write(filename, buffer, 0600);

    return 0;
}
