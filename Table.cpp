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

#if 0
# define D(fmt, ...) { printf("DEBUG[%d] ", __LINE__); printf(fmt, __VA_ARGS__); fflush(stdout); }
#else 
# define D(fmt, ...) { }
#endif

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
        populateWithTransform("transform", src, acceptRowFunc, newColNames, newColFuncs);
    }
}

Table::Table(
    const char* how,
    const std::vector<Table>& srcVec
) {
    populateWithUnion(srcVec);
}

std::vector<std::string> Table::getColNames() const {
    return(colNames_);
}

bool isSameStr(const std::string& a, const std::string& b) {
    if (strcmp(a.c_str(), b.c_str()) == 0) return true;
    return false;
}

int Table::findColIdx(const std::string& colName) const {
#if 1
    auto iter = colNameMap_.find(colName);
    auto colIdx = ((iter == colNameMap_.end()) ? -1 : iter->second);
#else
    int colIdx;
    for (colIdx = 0;; ++colIdx) {
        if (colIdx >= (int)colNames_.size()) { colIdx = -1; break; }
        printf("-- findColIdx compare \"%s\" and \"%s\"\n", colName.c_str(), colNames_[colIdx].c_str());
        if (isSameStr(colNames_[colIdx], colName)) break;
    }
#endif
    //printf("-- findColIdx(\"%s\") -> %d\n", colName.c_str(), colIdx);
    return(colIdx);
}

bool Table::scanRows(TableRowBoolFunc scanRowFunc) const {
    for (auto& row : rowVec_) {
        if (scanRowFunc(row)) return true;
    }
    std::set<std::string> allColNames;
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
    int c = colName.c_str()[0];
    if ((c != 0) &&
        ((c <= ' ') || (c & 0x80))) {
        assert(0);
    }
    //assert(colNameMap_.count(colName) == 0);
    int colIdx = colNames_.size();
    std::string modColName = colName;
    if ((colName == "") || (colNameMap_.count(modColName) > 0)) {
        char buf[512];
        sprintf(buf, "%s_col%d", colName.c_str(), colIdx);
        modColName = std::string(buf);
    }
    if (colName[0] & 0x80) {
        assert(0);
    }
    colNames_.push_back(modColName);
    colNameMap_[modColName] = colIdx;
}

void Table::addRow(const TableRow& row) {
    if (row.size() != colNames_.size()) {
        fprintf(stderr, "ERROR: addRow row size %lu num cols %lu\n",
            row.size(), colNames_.size());
    }
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

void Table::populateWithTransform(
    const char* how, /* "transform" */
    const Table& src,
    TableRowBoolFunc acceptRowFunc,
    const std::vector<std::string>& newColNames,
    const std::vector<TableRowStringFunc>& newColFuncs
) {
    clear();
    errStatus_ = src.errStatus_;
    if (newColNames.size() != newColFuncs.size()) {
        STATUS_FATAL(errStatus_, 0, "num cols mismatch");
        return;
    }
    for (auto& colName : newColNames) {
        addCol(colName);
    }
    for (auto& srcRow : src.rowVec_) {
        if (!acceptRowFunc(srcRow)) continue;
        TableRow newRow;
        for (auto& colFunc : newColFuncs) {
            newRow.push_back(TableCell(colFunc(srcRow)));
        }
        rowVec_.push_back(newRow);
    }
}


void Table::populateWithUnion(const std::vector<Table>& srcTables) {
    assert(0);
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
    clear();
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
        if (c == EOF) goto finishTable;
        if (c == '<') {
            char tagBuf[512];
            char* tagPos = tagBuf;
            *tagPos++ = c;
            for (;;) {
                c = fgetc(f);
                if (c == EOF) goto finishTable;
                if (tagPos-tagPos < 511) { *tagPos++ = c; *tagPos = 0; }
                if (c == '>') {
                    break;
                }
            }
            int tag = htmlWhichTag(tagBuf);
#if 0
            {   char buf[16];
                strncpy(buf, tagBuf, 8);
                tagBuf[8] = 0;
                D("state %d htmlTag(\"%s\") -> %d\n", state, tagBuf, tag);
            }
#endif
            tagPos = tagBuf;
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
                        if (!curRow.empty()) addRow(curRow);
                    } else {
                        haveHeader = true;
                        for (auto& cell : curRow) {
                            const char* p = cell.getString().c_str();
                            while (*p == ' ') ++p;
                            addCol(std::string(p));
                        }
                    }
                    curRow.clear();
                    state = 0; // not in a row
                default:
                    break;
            }
            continue;
        }
        if (state == 3) {
            if (bufPos-buf < 511) { *bufPos++ = c; *bufPos = 0; }
        }
    }
finishTable:
    fclose(f);
}

#define DOUBLEQUOTE '\"'

void Table::populateWithCSV(const std::string& fileName) {
    clear();
    FILE* f = fopen(fileName.c_str(), "r");
    if (!f) {
        STATUS_ERROR(errStatus_, 0, "populateWithCSV can't open file");
        return;
    }
    bool addAnonymousColumns = true;
    bool skipFirst3 = false;
    //
    // Read first line to guess whether separator is comma or tab
    //
    int fieldSep = '\t';
    char lineBuf[4*1024];
    char* p = fgets(lineBuf, sizeof(lineBuf)-1, f);
    //
    // Some Windows CSV files start with 3 binary bytes 0xef 0xbb 0xef
    //
    if (p &&
        ((p[0] & 0xff) == 0xef) &&
        ((p[1] & 0xff) == 0xbb) &&
        ((p[2] & 0xff) == 0xef)) {
        skipFirst3 = true;
        p += 3;
    }
    int numComma = 0;
    int numTab = 0;
    if (p) {
        for (; *p; ++p) {
            if (*p == ',') ++numComma;
            if (*p == '\t') ++numTab;
        }
        if (numComma > numTab) fieldSep = ',';
    }
    rewind(f);
    if (skipFirst3) {
        for (int j = 0; j < 3; ++j) { (void)fgetc(f); }
    }
    //
    // Simple state machine copes with different line-ending
    //
    int lineNo = 1;
    bool isQuoted = false;
    bool haveHeader = false;
    char buf[512];
    char* bufPos = buf;
    TableRow curRow;
    int nextChar = fgetc(f); // one-byte lookahead
    for (;;) {
        int c = nextChar;
        if (c != EOF) {
            nextChar = fgetc(f);
            if (c == '\r') {
                c = '\n';
                if (nextChar == '\n') {
                    // treat CR or CR-LF as '\n'
                    nextChar = fgetc(f);
                }
            }
        }
        if (c == EOF) {
            break;
        }
        if (!isQuoted) {
            if (c == DOUBLEQUOTE) {
                isQuoted = true;
            } else if ((c == fieldSep) || (c == '\n')) {
                curRow.push_back(TableCell(buf));
                bufPos = buf;
                *bufPos = 0;
                if (c == '\n') { // a partial line doesn't count
                    if (haveHeader) {
                        int numExtraCols = (curRow.size() - colNames_.size());
                        if (rowVec_.empty() && (numExtraCols > 0)) {
                            STATUS_WARNING(errStatus_, 0, "adding anonymous columns");
                            for (int j = 0; j < numExtraCols; ++j) {
                                addCol("");
                            }
                        }
                        if (curRow.size() < colNames_.size()) {
                            fprintf(stderr, "ERROR: numRows %lu numCols %lu rowCols %lu\n",
                                    rowVec_.size(), curRow.size(), colNames_.size());
                        }
                        for (int idx = curRow.size(); --idx >= (int)colNames_.size();) {
                            //
                            // Ignore any trailing empty or "0" values
                            //
                            auto val = curRow[idx].getString();
                            if ((val == "") || (val == "0")) {
                                curRow.resize(idx);
                            } else {
                                break;
                            }
                        }
                        addRow(curRow);
                    } else {
                        haveHeader = true;
                        //printf("DEBUG: header row size %lu\n", curRow.size());
                        for (auto& cell : curRow) {
                            const char* p = cell.getString().c_str();
                            //
                            // Skip whitespace and unprintable characters at start of column name
                            //
                            while ((*p <= ' ') || (*p & 0x80)) ++p;
                            addCol(std::string(p));
                        }
                    }
                    curRow.clear();
                    if (c == EOF) break;
                    ++lineNo;
                }
            } else {
                if (bufPos < buf+511) { *bufPos++ = c; *bufPos = 0; }
            }
        } else {
            if (c == DOUBLEQUOTE) {
                if (nextChar == DOUBLEQUOTE) {
                    nextChar = fgetc(f);
                    if (bufPos < buf+511) { *bufPos++ = c; *bufPos = 0; }
                } else {
                    isQuoted = false;
                }
            } else {
                if (bufPos < buf+511) { *bufPos++ = c; *bufPos = 0; }
            }
        }
    }
    fclose(f);
}
