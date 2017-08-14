#ifndef TABLE_H
#define TABLE_H 1
//
// Table.h - simple but inefficient table-of-strings
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
#include "ErrStatus.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdlib.h>

//
// To allow table-expressions to be written concisely without compromising
// error-reporting, each table conatins an ErrStatus, and all table operators
// must combine and propagate the ErrStatus correctly.
//
class TableCell {
private:
    bool isNull_;
    std::string strVal_;

public:
    TableCell() : isNull_(true), strVal_() { }
    
    TableCell(const std::string& val) :
        isNull_(val == ""),
        strVal_(val) {
    }
    
    TableCell(int intVal) {
        char buf[64];
        sprintf(buf, "%d", intVal);
        isNull_ = false;
        strVal_ = buf;
    }
    
    TableCell(const TableCell& b) : isNull_(b.isNull_), strVal_(b.strVal_) { }

    bool isNull() {
        return(isNull_);
    }
    
    std::string getString() const {
        return(strVal_);
    }
    
    int64_t getInt() const {
        if (strVal_.size() == 0) return(0);
        const char* p = strVal_.c_str();
        while ((*p == ' ') || (*p == '\t')) ++p;
        char c0 = *p;
        if ((c0 == '+') || (c0 == '-') || ('0' <= c0) && (c0 <= '9')) {
            return atoi(p);
        }
        return(0);
    }
    
    TableCell& operator=(const TableCell& b) {
        isNull_ = b.isNull_;
        strVal_ = b.strVal_;
        return(*this);
    }
    
    TableCell& operator=(const std::string& strVal) {
        isNull_ = (strVal.size() == 0);
        strVal_ = strVal;
        return(*this);
    }
    
    TableCell& operator=(int64_t intVal) {
        char buf[64];
        sprintf(buf, "%ld", intVal);
        isNull_ = false;
        strVal_ = buf;
    }
    
};

class TableRow :
    public std::vector<TableCell> {
public:
    
};

typedef std::function<bool(const TableRow&)> TableRowBoolFunc;

typedef std::function<int64_t(const TableRow&)> TableRowIntFunc;

typedef std::function<std::string(const TableRow&)> TableRowStringFunc;

class Table {
private:
    ErrStatus errStatus_;
    std::vector<std::string> colNames_;
    std::unordered_map<std::string, int> colNameMap_;
    std::vector<TableRow> rowVec_;
    
public:
    Table();

    Table(const Table& b) = default;
    
    Table& operator=(const Table& b) = default;

public:    
    Table(const char* how, /* "html" or "csv" */
          const std::string& fileName);
    
    Table(const char* how, /* "transform" */
          const Table& src,
          TableRowBoolFunc acceptRowFunc,
          const std::vector<std::string>& newColNames,
          const std::vector<TableRowStringFunc>& newColFuncs);

    Table(const char* how, /* "union" */
          const std::vector<Table>& srcVec);
    
    std::vector<std::string> getColNames() const;
    
    int findColIdx(const std::string& colName) const;

    size_t getNumRows() const {
        return(rowVec_.size());
    }
    
    bool scanRows(TableRowBoolFunc scanRowFunc) const;
          
    std::vector<std::string> getColDistinctVals(size_t colIdx) const;
    
private:
    void clearRows();
    
    void clear();

    void populateWithTransform(
        const char* how,
        const Table& src, 
        TableRowBoolFunc acceptRowFunc,
        const std::vector<std::string>& newColNames,
        const std::vector<TableRowStringFunc>& newColFuncs
    );
    
    void populateWithUnion(const std::vector<Table>& srcTables);
    
    void populateWithHTML(const std::string& fileName);
    
    void populateWithCSV(const std::string& fileName);
    
    void addCol(const std::string& colName);
    
    void addRow(const TableRow& row);
    
};

typedef std::shared_ptr<Table> TablePtr;

#endif
