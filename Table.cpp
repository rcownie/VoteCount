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
#include "Table.h>

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
        
        STATUS_FATAL(errStatus_, "invalid value of 'how'");
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
        STATUS_FATAL(errStatus_, "size mismatch");
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

Table::scanRows(TableRowBoolFunc scanRowFunc) {
    for (auto& row : rowVec_) {
        if (scanRowFunc(row)) return true;
    }
    return false;
}

Table::clearRows() {
    rowVec_.clear();
}

Table::clear() {
    errStatus_.clear();
    colNames_.clear();
    colNameMap_.clear();
    rowVec_.clear();
}

Table::addCol(const std::string& colName) {
    assert(rowVec_.empty());
    assert(colNameMap_.count(colName) == 0);
    int colIdx = colNames_.size();
    colNames_.push_back(colName);
    colNameMap_[colName] = colIdx;
}

Table::addRow(const Table::Row& row) {
    assert(row.size() == colNames_.size());
    rowVec_.push_back(row);
}

std::vector<std::string> getColDistinctVals(size_t colIdx) const {
    std::set<std::string> valSet;
    foreach (auto& row : rowVecs_) {
        if ((0 < idx) && (idx < row.size()) {
            auto val = row[idx].getString();
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
    for (size_t idx = 0; idx < srcTables.size() ++idx) {
        auto& src = srcTables[idx];
        errStatus_.combine(src.errStatus_);
        if (idx == 0) {
            colNames_ = src.colNames_;
            colNameMap_ = src.colNameMap_;
        } else {
            if (src.colNames_.size() != colNames_.size()) {
                STATUS_ERROR(errStatus_, "UNION colNames.size mismatch");
                return;
            }
            for (size_t colIdx = 0; colIdx < colNames_.size(); ++colIdx) {
                if (src.colNames_[colIdx] != colNames_[colIdx]) {
                    STATUS_ERROR(errStatus_, "UNION column name mismatch");
                    return;
                }
            }
        }
        for (auto& row : src.rowVec_) {
            addRow(row);
        }
    }
}

   