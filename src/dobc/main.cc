﻿
#include "vm.h"
#include "dobc.hh"

#define PSPEC_FILE          "../../../Processors/ARM/data/languages/ARMCortex.pspec"
#define CSPEC_FILE          "../../../Processors/ARM/data/languages/ARM.cspec"
#define TEST_SO             "../../../data/vmp/360_1/libjiagu.so"

static char help[] = {
    "dobc [-st (360free|ollvm)] [-i filename] \r\n" 
    "       -o                  output filename\r\n"
    "       -sd                 dump new so to so directory or current directory\r\n"
    "       -d[0-6]             debug info level\r\n"
    "       -da [hex address]   decode address\r\n"
    "       -dcfg               dump static trace cfg\r\n"
    "       --ghidra            ghidra config directory\r\n"
};

int main(int argc, char **argv)
{
    int i;
    char *filename = NULL, *st = NULL;
    intb sd = 0, dcfg = 0;
    char *out_filename = NULL, *ghidra = NULL;
    vector<intb>    addr_list;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-st")) {
            st = argv[++i];
        }
        else if (!strcmp(argv[i], "-i")) {
            filename = argv[++i];
        }
        else if (!strcmp(argv[i], "-o")) {
            out_filename = argv[++i];
        }
        else if (!strcmp(argv[i], "-sd")) {
            sd = 1;
        }
        else if (!strcmp(argv[i], "-c")) {
            ghidra = argv[++i];
        }
        else if (!strcmp(argv[i], "-da")) {
            while (((i+ 1) < argc) && argv[i + 1][0] != '-') {
                addr_list.push_back(strtoul(argv[i+1], NULL, 16));
                i++;
            }
        }
        else if (!strcmp(argv[i], "-dcfg")) {
            dcfg = 1;
        }
    }

    if (!ghidra || !st || !filename || addr_list.empty()) {
        puts(help);
        return -1;
    }

    dobc d(ghidra, filename);

    d.set_shelltype(st);
    d.debug.dump_cfg = dcfg;

    d.decode_address_list = addr_list;

    if (out_filename)
        d.out_filename.assign(out_filename, strlen(out_filename));

    if (sd)
        d.out_dir.assign(filename, basename(filename) - filename);

    DocumentStorage docstorage;
    Element *sleighroot = docstorage.openDocument(d.slafilename)->getRoot();
    docstorage.registerTag(sleighroot);

    d.init(docstorage);

    d.run();

    return 0;
}
