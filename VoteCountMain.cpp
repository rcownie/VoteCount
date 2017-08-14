//
// VoteCountMain.cpp - main() program for votecount
//
// Copyright (c) Richard Cownie, 2017.
//
// Author email: richard.cownie@pobox.com
//

//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
#include "Table.h"
#include <set>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage() {
    fprintf(stderr, "usage: votecount -s <stateAbbrev> -o <outName> <file1> [ <file2> ... ]\n");
    exit(1);
}

const char* optOutFile = "stdout";
const char* optStateId = "ks";
int optVerbose = 0;

std::string toLower(const char* s) {
    char buf[4096];
    char* bufPos = buf;
    for (const char* p = s; *p != 0; ++p) {
        int c = *p;
        if (('A' <= c) && (c <= 'Z')) c += 'a'-'A';
        *bufPos++ = c;
    }
    *bufPos = 0;
    return std::string(buf);
    
}

void transformStateTable(
    FILE* out,
    const std::string& stateId,
    const std::vector<std::string>& srcFiles
) {
    if (srcFiles.size() != 1) {
        fprintf(stderr, "ERROR: stateId \"%s\" needs exactly one file", stateId.c_str());
        exit(1);
    }
    Table tab0("html", srcFiles[0]);
    int colIdx = tab0.findColIdx("Candidate");
    auto vals = tab0.getColDistinctVals(colIdx);
    for (auto& val : vals) {
        printf("Candidate \"%s\"\n", val.c_str());
    }
}

void transformCountyTables(
    FILE* out,
    const std::string& stateId,
    const std::vector<std::string>& srcFiles
) {
    std::vector<Table> countyTab;
    //std::set<std::string> allCandidates;
    for (auto& srcFile : srcFiles) {
        printf("reading %s ...\n", srcFile.c_str());
        Table rawCounty("csv", srcFile);
        auto colNames = rawCounty.getColNames();
        //printf("colNames.size %lu\n", colNames.size());
        for (auto& colName : colNames) {
            printf("XX %s %s\n", colName.c_str(), srcFile.c_str());
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> fileNames;
    for (int j = 1; j < argc; ++j) {
        const char* a = argv[j];
        const char* nextArg = ((j+1 < argc) ? argv[j+1] : "MissingArg");
        if (a[0] == '-') {
            const char* optStr = ((strlen(a) > 2) ? a+2 : nullptr);
            switch (a[1]) {
                case 'o':
                    if (!optStr) { optStr = nextArg; ++j; }
                    optOutFile = optStr;
                    break;
                case 's':
                    if (!optStr) { optStr = nextArg; ++j; }
                    optStateId = optStr;
                    break;
                case 'v':
                    optVerbose = 1;
                    break;
                default:
                    usage();
            }
        } else {
            fileNames.push_back(a);
        }
    }
    FILE* out = stdout;
    if (strcmp(optOutFile, "stdout") != 0) {
        out = fopen(optOutFile, "w");
        if (!out) {
            fprintf(stderr, "ERROR: can't write output to \"%s\"\n", optOutFile);
            exit(1);
        }
    }
    std::string stateId = toLower(optStateId);
    if        ((stateId == "ks") || (stateId == "kansas")) {
        transformStateTable(out, "ks", fileNames);
    } else if ((stateId == "ny") || (stateId == "newyork")) {
        transformCountyTables(out, "ny", fileNames);
    }
    return(0);
}

 