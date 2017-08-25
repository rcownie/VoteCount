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
#include <unordered_map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage() {
    fprintf(stderr, "usage: votecount [-a <analyzeCol>] [-c <confFile>] -s <stateAbbrev> -o <outName> <file1> [ <file2> ... ]\n");
    exit(1);
}

const char* optAnalyzeCol;
const char* optConfFile = "votecount_2016.conf";
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

bool isMatchingStr(bool matchCase, const std::string& a, const std::string& b) {
    if (matchCase) {
        return(a == b);
    } else {
        return(toLower(a.c_str()) == toLower(b.c_str()));
    }
}

bool isTrueOrYes(const std::string& s) {
    auto val = toLower(s.c_str());
    if ((val == "yes") ||
        (val == "y") ||
        (val == "true") ||
        (val == "t")) {
        return true;
    }
    return false;
}

class CountyPrecinct {
private:
    std::string county_;
    std::string precinct_;

public:
    CountyPrecinct(const std::string& county, const std::string& precinct) :
        county_(county),
        precinct_(precinct) {
    }
    
    CountyPrecinct(const CountyPrecinct& b) = default;
    
    CountyPrecinct& operator=(const CountyPrecinct& b) {
        county_ = b.county_;
        precinct_ = b.precinct_;
    }
    
    bool operator==(const CountyPrecinct& b) const {
        return((county_ == b.county_) && (precinct_ == b.precinct_));
    }
    
    bool operator<(const CountyPrecinct& b) const {
        return(
            (county_ < b.county_) ||
            ((county_ == b.county_) && (precinct_ < b.precinct_))
        );
    }
};

class PrecinctVotes {
private:
    std::vector<int> vec_;

public:
    void colAdd(int idx, int val) {
        while (vec_.size() < idx+1) {
            vec_.push_back(0);
        }
        vec_[idx] += val;
    }
    
    int colGet(int idx) {
        return((idx < vec_.size()) ? vec_[idx] : 0);
    }
};

class PrecinctMap {
public:
    std::map<CountyPrecinct, PrecinctVotes> map_;

public:
    PrecinctVotes& findPrecinct(const std::string& county, const std::string& precinct) {
        return map_[CountyPrecinct(county, precinct)];
    }
    
};

class ColumnNameMap {
private:
    class Rule {
    public:
        std::string county_;
        std::string oldName_;
        bool        matchCase_;
        std::string newName_;
    
    public:
        Rule(std::string county, std::string oldName, bool matchCase, std::string newName) :
            county_(county),
            oldName_(oldName),
            matchCase_(matchCase),
            newName_(newName) {
        }
        
        Rule(const Rule& b) = default;
    };

    std::vector<Rule> rules_;
public:
    ColumnNameMap(const Table& confTab, const std::string& stateId) {
        auto colState  = confTab.findColIdx("State");
        auto colCounty = confTab.findColIdx("County");
        auto colActionOrColumn = confTab.findColIdx("ActionOrColumn");
        auto colMatchCase = confTab.findColIdx("MatchCase");
        auto colMapTo = confTab.findColIdx("MapTo");
        auto colMapFrom = confTab.findColIdx("MapFrom");
        for (int phase = 0; phase < 3; ++phase) {
            confTab.scanRows(
                [=](const TableRow& row)->bool {
                    if (row[colActionOrColumn].getString() == "!MapColumn") {
                        auto valState = row[colState].getString();
                        if ((valState != "ANY") && !isMatchingStr(false, valState, stateId)) {
                            // ignore rules specific to other states
                            return false;
                        }
                        auto valCounty = row[colCounty].getString();
                        Rule newRule(
                            valCounty,
                            row[colMapFrom].getString(),
                            isTrueOrYes(row[colMatchCase].getString()),
                            row[colMapTo].getString()
                        );
                        if       (valCounty != "ANY") {
                            if (phase == 0) this->rules_.push_back(newRule);
                        } else if (valState != "ANY") {
                            if (phase == 1) this->rules_.push_back(newRule);
                        } else {
                            if (phase == 2) this->rules_.push_back(newRule);
                        }
                    }
                    return false;
                }
            );
        }
    }
    
    std::string mapColumnName(const std::string& colName) {
        // FIXME: need county name
        for (auto& rule : rules_) {
            if (isMatchingStr(rule.matchCase_, rule.oldName_, colName)) {
                // The rules are in priority order, so the first rule that matches
                // is the correct value.  Hmm, should we also allow further remapping
                // by lower-priority rules ?  Keep it simple for now ...
                return(rule.newName_);
            }
        }
        return(colName);
    }
};

class ColumnValueSummary {
private:
    std::set<std::string> set_;
    bool hasMinAndMax_;
    int64_t min_;
    int64_t max_;

public:
    ColumnValueSummary() :
        set_(),
        hasMinAndMax_(false),
        min_(0),
        max_(0) {
    }
    
    ColumnValueSummary(const ColumnValueSummary& b) = default;
    
    void insert(const std::string& str) {
        std::string tmpS(str);
        const char* s = tmpS.c_str();
        const char* p = s;
        if ((*p == '+') || (*p == '-')) ++p;
        for (; *p != 0; ++p) {
            int c = *p;
            if (('0' <= c) && (c <= '9')) { 
                int64_t val = atol(s);
                if (!hasMinAndMax_) {
                    hasMinAndMax_ = true;
                    min_ = val;
                    max_ = val;
                } else {
                    if (min_ > val) min_ = val;
                    if (max_ < val) max_ = val;
                }
            } else {
                set_.insert(str);
            }
        }
    }
    
    std::string toString() const {
        std::stringstream ss;
        bool haveStrings = !set_.empty();
        ss << "{";
        if (hasMinAndMax_) {
            ss << " /* min=" << min_ << ", max=" << max_ << "*/ ";
        }
        if (!set_.empty()) {
            ss << "\n";
            for (auto& val : set_) {
                ss << "    \"" << val.c_str() << "\",\n";
            }
        }
        ss << "}";
        return ss.str();
    }
                                            
};

void checkConfTable(const Table& confTab) {
    bool fail = false;
    std::vector<const char*> wantColNames(
        { "State", "County", "ActionOrColumn", "MatchCase", "MapTo", "MapFrom" }
    );
    auto colNames = confTab.getColNames();
    for (size_t j = 0; j < wantColNames.size(); ++j) {
        if (j >= colNames.size()) {
            fprintf(stderr, "ERROR: missing column \"%s\"\n", wantColNames[j]);
            fail = true;
        } else if (colNames[j] != std::string(wantColNames[j])) {
            fprintf(stderr, "ERROR: column mismatch want \"%s\", got \"%s\"\n",
                wantColNames[j], colNames[j].c_str());
            fail = true;
        }
    }
    if (fail) {
        exit(1);
    }
}

std::unordered_map<std::string, int> getConfCandidates(const Table& confTab, std::string stateId) {
    std::unordered_map<std::string, int> candidateMap;
    auto colState          = confTab.findColIdx("State");
    auto colActionOrColumn = confTab.findColIdx("ActionOrColumn");
    auto colMapTo          = confTab.findColIdx("MapTo");
    confTab.scanRows(
        [=,&candidateSet](const TableRow& row)->bool {
            if (row[colActionOrColumn].getString() != "!Candidate") {
                return false;
            }
            auto valState = row[colState].getString();
            if ((valState == "ANY") || (valState == stateId)) {
                auto iter = candidateMap.find(row[colMapTo].getString());
                if (iter == candidateMap.end()) {
                    int newIdx = candidateMap.size();
                    candidateMap[row[colMapTo].getString()] = newIdx;
                }
            }
            return false;
        }
    )
    candidateMap
    return(candidateSet);
}

void transformTables(
    FILE* out,
    const Table& confTab,
    const std::string& stateId,
    const std::vector<std::string>& srcFiles
) {
    std::vector<Table> countyTab;
    std::set<std::string> allColNames;
    ColumnValueSummary oneColSummary;
    
    ColumnNameMap colNameMap(confTab, stateId);
    auto candidateMap = getConfCandidates(confTab, stateId);
    PrecinctMap precinctMap;
    for (auto& srcFile : srcFiles) {
        if (optVerbose) printf("reading %s ...\n", srcFile.c_str());
        const char* htmlOrCsv = (strstr(srcFile.c_str(), ".htm") ? "html" : "csv");
        Table countyA(htmlOrCsv, srcFile);
        //
        // First transformation is to apply colNameMap
        //
        auto newColNamesA = countyA.getColNames();
        for (size_t idx = 0; idx < newColNamesA.size(); ++idx) {
            newColNamesA[idx] = colNameMap.mapColumnName(newColNamesA[idx]);
        }
        std::vector<TableRowStringFunc> newColFuncsA;
        for (size_t idxA = 0; idxA < newColNamesA.size(); ++idxA) {
            newColFuncsA.push_back(
                [idxA](const TableRow& row)->std::string {
                    return row[idxA].getString();
                }
            );
        }
        Table countyB(
            "transform",
            countyA,
            [](const TableRow&)->bool { return true; },
            newColNamesA,
            newColFuncsA
        );
        //
        // *If* there is an "Office" or "Race" column, filter out rows with
        // non-president values.
        //
        int filterCol = -1;
        
        if (filterCol < 0) filterCol = countyB.findColIdx("Office");
        if (filterCol < 0) filterCol = countyB.findColIdx("office");
        if (filterCol < 0) filterCol = countyB.findColIdx("Race");
        if (filterCol < 0) filterCol = countyB.findColIdx("race");
        if (optAnalyzeCol != nullptr) {
            //
            // Accumulate a ColumnValueSummary for the interesting column
            //
            auto colNames = countyB.getColNames();
            for (auto colName : colNames) {
                allColNames.insert(colName);
            }
            auto analyzeCol = countyB.findColIdx(optAnalyzeCol);
            if (analyzeCol >= 0) {
                countyB.scanRows(
                    [=,&oneColSummary](const TableRow& row)->bool {
                        if (filterCol != analyzeCol) {
                            if (filterCol >= 0) {
                                auto filterVal = row[filterCol].getString();
                                if (!isMatchingStr(false, filterVal, "President") &&
                                    !isMatchingStr(false, filterVal, "Electors for President & Vice President")) {
                                    return false;
                                }    
                            }
                            if (analyzeCol >= 0) {
                                auto analyzeVal = row[analyzeCol].getString();
                                oneColSummary.insert(analyzeVal);
                            }
                            return false;
                        }
                    }
                );
            }
        } else {
            //
            // Accumulating the real results
            //
            auto colCounty    = countyB.findColIdx("County");
            auto colPrecinct  = countyB.findColIdx("PrecinctName");
            auto colCandidate = countyB.findColIdx("Candidate");
            std::vector<int> validCols;
            std::vector<int invalidCols>
            countyB.scanRows(
                [=,&candidateMap,&precinctMap](const TableRow& row)->bool {
                    auto valCounty = row[colCounty].getString();
                    auto valPrecinct = row[colPrecinct].getString();
                    if ((valPrecinct == "") || isMatchingStr(false, valPrecinct, "total")) {
                        return false;
                    }
                    auto valCandidate = row[colCandidate].getString();
                    auto iter = candidateMap.find(valCandidate);
                    if (iter == candidateMap.end()) {
                        if (isMatchingStr(false, valCandidate, "total")) return false;
                        if (valCandidate == "") {
                            valCandidate = "Invalid";
                        } else {
                            valCandidate = "Other";
                        }
                        iter = candidateMap.find(valCandidate);
                    }
                    int candidateIdx = iter->second();
                    int64_t numValid = 0;
                    for (auto col : validCols) {
                        numValid += row[col].getInt();
                    }
                    int64_t numInvalid = 0;
                    for (auto col : invalidCols) {
                        numInvalid += row[col].getInt();
                    }
                    if (valCandidate == "Invalid") {
                        numInvalid += numValid;
                        numValid = 0;
                    }
                    auto& precinctVotes = precinctMap.findPrecinct(valCounty, valPrecinct);
                    if (numValid > 0) {
                        precinctVotes.colAdd(candidateIdx, numValid);
                    }
                    if (numInvalid > 0) {
                        auto iterB = candidateMap.find("Invalid");
                        if (iterB != candidateMap.end()) {
                            precinctVotes.colAdd(iterB->second, numInvalid);
                        }
                    }
                }
            );
        }
    }
    if (optAnalyzeCol != nullptr) {
        //
        // Print all column names
        //
        printf("Analyze: -a %s\n", optAnalyzeCol);
        for (auto& colName : allColNames) {
            printf("Analyze.ColName: \"%s\"\n", colName.c_str());
        }
        printf("Analyze: summary of column \"%s\" values\n", optAnalyzeCol);
        printf("%s\n", oneColSummary.toString().c_str());
    } else {
        //
        // Output the real accumulated results
        //
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
                case 'a':
                    if (!optStr) { optStr = nextArg; ++j; }
                    optAnalyzeCol = optStr;
                    break;
                case 'c':
                    if (!optStr) { optStr = nextArg; ++j; }
                    optConfFile = optStr;
                    break;
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
    Table confTab("csv", optConfFile);
    checkConfTable(confTab);
    std::string stateId = toLower(optStateId);
    transformTables(out, confTab, stateId, fileNames);
    return(0);
}

 