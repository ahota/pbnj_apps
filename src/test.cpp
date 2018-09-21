#include <Volume.h>

int main(int argc, char **argv)
{
    pbnj::Volume *v = new pbnj::Volume("/mnt/seenas1/data/ironProt.nc", "data", 64, 64, 64, false);
    return 0;
}
