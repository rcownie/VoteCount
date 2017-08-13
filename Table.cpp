//
// Table.cpp - simple but inefficient table-of-strings
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#include "Table.h"
#include <set>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

Table::Table() {
}

Table::Table(
    const char* how,
    const std::string& fileName
) {
    if        (strcmp(how, "html") == 0) {
        populateWithHTML(fileName);
    } else if (strcmp(how, "csv") == 0) {
        populateWithCSV(fileName);
    } else {
        STATUS_FATAL(errStatus_, 0, "invalid value of 'how'");
    }
}

Table::Table(
    const char* how,
    const Table& src,
    TableRowBoolFunc acceptRowFunc,
    const std::vector<std::string>& newColNames,
    const std::vector<TableRowStringFunc>& newColFuncs
) {
    if (newColNames.size() != newColFuncs.size()) {
        STATUS_FATAL(errStatus_, 0, "size mismatch");
    } else {
        populateWithTransform(src, acceptRowFunc, newColNames, newColFuncs);
    }
}

Table::Table(
    const char* how,
    const std::vector<Table>& srcVec
) {
    populateWithUnion(srcVec);
}

bool Table::scanRows(TableRowBoolFunc scanRowFunc) {
    for (auto& row : rowVec_) {
        if (scanRowFunc(row)) return true;
    }
    return false;
}

void Table::clearRows() {
    rowVec_.clear();
}

void Table::clear() {
    errStatus_.clear();
    colNames_.clear();
    colNameMap_.clear();
    rowVec_.clear();
}

void Table::addCol(const std::string& colName) {
    assert(rowVec_.empty());
    assert(colNameMap_.count(colName) == 0);
    int colIdx = colNames_.size();
    colNames_.push_back(colName);
    colNameMap_[colName] = colIdx;
}

void Table::addRow(const TableRow& row) {
    assert(row.size() == colNames_.size());
    rowVec_.push_back(row);
}

std::vector<std::string> Table::getColDistinctVals(size_t colIdx) const {
    std::set<std::string> valSet;
    for (auto& row : rowVec_) {
        if ((0 < colIdx) && (colIdx < row.size())) {
            auto val = row[colIdx].getString();
            valSet.insert(val);
        }
    }
    std::vector<std::string> result;
    for (auto& val : valSet) {
        result.push_back(val);
    }
    return(result);
}

void Table::populateWithUnion(const std::vector<Table> srcTables) {
    clear();
    for (size_t idx = 0; idx < srcTables.size(); ++idx) {
        auto& src = srcTables[idx];
        errStatus_.combine(src.errStatus_);
        if (idx == 0) {
            colNames_ = src.colNames_;
            colNameMap_ = src.colNameMap_;
        } else {
            if (src.colNames_.size() != colNames_.size()) {
                STATUS_ERROR(errStatus_, 0, "UNION num cols mismatch");
                return;
            }
            for (size_t colIdx = 0; colIdx < colNames_.size(); ++colIdx) {
                if (src.colNames_[colIdx] != colNames_[colIdx]) {
                    STATUS_ERROR(errStatus_, 0, "UNION column name mismatch");
                    return;
                }
            }
        }
        for (auto& row : src.rowVec_) {
            addRow(row);
        }
    }
}

#define HTML_OTHER    0
#define HTML_TR       1
#define HTML_SLASHTR  2
#define HTML_TD       3
#define HTML_SLASHTD  4
#define HTML_P        5
#define HTML_SLASHP   6

static int htmlWhichTag(const char* s) {
    if ((s[0] == '<') && (s[1] == '/')) {
        if (!strcmp(s+2, "tr>")) return HTML_SLASHTR;
        if (!strcmp(s+2, "td>")) return HTML_SLASHTD;
        if (!strcmp(s+2, "p>"))  return HTML_SLASHP;
    } else {
        if (!strcmp(s, "<tr>") || !strncmp(s, "<tr ", 4)) return HTML_TR;
        if (!strcmp(s, "<td>") || !strncmp(s, "<td ", 4)) return HTML_TD;
        if (!strcmp(s, "<p>")  || !strncmp(s, "<p ", 4))  return HTML_P;
    }
    return HTML_OTHER;
}

void Table::populateWithHTML(const std::string& fileName) {
    FILE* f = fopen(fileName.c_str(), "r");
    if (!f) {
        STATUS_ERROR(errStatus_, 0, "populateWithHTML can't open file");
        return;
    }
    //
    // Simple state machine detects <tr>, </tr>, <td>, </td>, <p>, </p>
    //
    bool haveHeader = false;
    char buf[512];
    char* bufPos = nullptr;
    TableRow curRow;
    int state = 0;
    for (;;) {
        int c = fgetc(f);
        if (c == '<') {
            char tagBuf[512];
            char* tagPos = tagBuf;
            *tagPos++ = c;
            for (;;) {
                c = fgetc(f);
                if (c == EOF) break;
                if (tagPos-tagPos < 511) { *tagPos++ = c; *tagPos = 0; }
                if (c == '>') {
                    int tag = htmlWhichTag(tagBuf);
                    switch (tag) {
                        case HTML_TR:
                            curRow.clear();
                            state = 1; // in a row
                            break;
                        case HTML_TD:
                            if (state == 1) state = 2; // in a data cell
                            break;
                        case HTML_P:
                            if (state == 2) {
                                bufPos = buf;
                                state = 3; // in a <p>
                            }
                            break;
                        case HTML_SLASHP:
                            if (state == 3) {
                                curRow.push_back(TableCell(buf));
                                state = 2;
                            }
                            break;
                        case HTML_SLASHTD:
                            if (state == 2) state = 1;
                            break;
                        case HTML_SLASHTR:
                            if (haveHeader) {
                                addRow(curRow);
                            } else {
                                haveHeader = true;
                                for (auto& cell : curRow) {
                                    addCol(cell.getString());
                                }
                            }
                            curRow.clear();
                            state = 0; // not in a row
                        default:
                            break;
                    }
                }
            }
            continue;
        }
        if (state == 3) {
            if (bufPos-buf < 511) { *bufPos++ = c; *bufPos = 0; }
        }
    }
    fclose(f);
}

void Table::populateWithCSV(const std::string& fileName) {
    FILE* f = fopen(fileName.c_str(), "r");
    if (!f) {
        STATUS_ERROR(errStatus_, 0, "populateWithCSV can't open file");
        return;
    }
    //
    // Simple state machine detects <tr>, </tr>, <td>, </td>, <p>, </p>
    //
    bool haveHeader = false;
    char buf[512];
    char* bufPos = nullptr;
    TableRow curRow;
    for (;;) {
        int c = fgetc(f);
        if ((c == '\t') || (c == '\n') || (c == EOF)) {
            if ((c == '\n') && (bufPos > buf) && (bufPos[-1] == '\r')) {
                // kludge for a CR-LF sequence
                *--bufPos = 0;
            }
            curRow.push_back(TableCell(buf));
            if ((c == '\n') || (c == EOF)) {
                if (haveHeader) {
                    addRow(curRow);
                } else {
                    haveHeader = true;
                    for (auto& cell : curRow) {
                        addCol(cell.getString());
                    }
                }
                curRow.clear();
                if (c == EOF) break;
            }
        } else {
            if (bufPos < buf+511) { *bufPos++ = c; *bufPos = 0; }
        }
    }
    fclose(f);
}
