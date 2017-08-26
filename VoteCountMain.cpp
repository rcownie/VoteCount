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
#include <map>
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
    
    const std::string& getCounty() const {
        return county_;
    }
    
    const std::string& getPrecinct() const {
        return precinct_;
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
    
    int colGet(int idx) const {
        return((idx < vec_.size()) ? vec_[idx] : 0);
    }
    
    int getTotal() const {
        int total = 0;
        for (auto val : vec_) total += val;
        return(total);
    }
};

class PrecinctMap {
public:
    std::map<CountyPrecinct, PrecinctVotes> map_;

public:
    PrecinctVotes& findPrecinct(const std::string& county, const std::string& precinct) {
        return map_[CountyPrecinct(county, precinct)];
    }
    
    void genOutput(FILE* out, const std::unordered_map<std::string, int>& candidateMap) {
        std::vector<std::string> colNames;
        colNames.push_back("County");
        colNames.push_back("Precinct");
        colNames.push_back("Race");
        colNames.push_back("TotalVotes");
        size_t baseIdx = colNames.size();
        for (auto& pairA : candidateMap) {
            size_t idx = (baseIdx + pairA.second);
            while (colNames.size() <= idx) colNames.push_back("");
            colNames[idx] = pairA.first;
        }
        for (size_t j = 0; j < colNames.size(); ++j) {
            if (j > 0) fprintf(out, "\t");
            fprintf(out, "%s", colNames[j].c_str());
        }
        fprintf(out, "\n");
        std::map<std::string, PrecinctVotes> countyMap;
        int stateTotal = 0;
        for (auto& pairB : map_) {
            fprintf(out, "%s\t", pairB.first.getCounty().c_str());
            fprintf(out, "%s\t", pairB.first.getPrecinct().c_str());
            fprintf(out, "%s\t", "President");
            auto& countyVotes = countyMap[pairB.first.getCounty()];
            auto& precinctVotes = pairB.second;
            fprintf(out, "%d\t", precinctVotes.getTotal());
            stateTotal += precinctVotes.getTotal();
            for (size_t idx = baseIdx; idx < colNames.size(); ++idx) {
                int votes = precinctVotes.colGet(idx-baseIdx);
                countyVotes.colAdd(idx-baseIdx, votes);
                fprintf(out, "%d", precinctVotes.colGet(idx-baseIdx));
                if (idx+1 < colNames.size()) fprintf(out, "\t");
            }
            fprintf(out, "\n");
        }
        fclose(out);
        FILE* f = fopen("county.csv", "w");
        fprintf(f, "County\t");
        fprintf(f, "TotalVotes\t");
        baseIdx = 2;
        for (size_t j = 0; j < colNames.size(); ++j) {
            if (j > 0) fprintf(f, "\t");
            fprintf(f, "%s", colNames[j].c_str());
        }
        fprintf(f, "\n");
        for (auto& pairC : countyMap) {
            fprintf(f, "%s\t", pairC.first.c_str());
            auto& countyVotes = pairC.second;
            fprintf(f, "%d\t", countyVotes.getTotal());
            for (size_t idx = baseIdx; idx < colNames.size()-2; ++idx) {
                int votes = countyVotes.colGet(idx-baseIdx);
                fprintf(f, "%d", votes);
                if (idx+1 < colNames.size()) fprintf(f, "\t");
            }
            fprintf(f, "\n");
        }
        fclose(f);
        fprintf(stderr, "StateTotalVotes %d\n", stateTotal);
    }
};

class StringMap {
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
    StringMap(const Table& confTab, const std::string& stateId, const std::string& actionOrColumn) {
        auto colState  = confTab.findColIdx("State");
        auto colCounty = confTab.findColIdx("County");
        auto colActionOrColumn = confTab.findColIdx("ActionOrColumn");
        auto colMatchCase = confTab.findColIdx("MatchCase");
        auto colMapTo = confTab.findColIdx("MapTo");
        auto colMapFrom = confTab.findColIdx("MapFrom");
        for (int phase = 0; phase < 3; ++phase) {
            confTab.scanRows(
                [=](const TableRow& row)->bool {
                    if (row[colActionOrColumn].getString() == actionOrColumn) {
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
    
    std::string mapString(const std::string& val) const {
        // FIXME: need county name
        for (auto& rule : rules_) {
            if (isMatchingStr(rule.matchCase_, rule.oldName_, val)) {
                // The rules are in priority order, so the first rule that matches
                // is the correct value.  Hmm, should we also allow further remapping
                // by lower-priority rules ?  Keep it simple for now ...
                return(rule.newName_);
            }
        }
        return(val);
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
        [=,&candidateMap](const TableRow& row)->bool {
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
    );
    int newIdx = candidateMap.size();
    if (candidateMap.find("Other") == candidateMap.end()) {
        candidateMap["Other"] = newIdx++;
    }
    if (candidateMap.find("Invalid") == candidateMap.end()) {
        candidateMap["Invalid"] = newIdx++;
    }
    return(candidateMap);
}

bool stringEndsWithTotal(const std::string& str) {
    size_t len = str.length();
    if (len < 5) return false;
    const char* last5 = str.c_str() + (len-5);
    const char* total = "total";
    for (size_t j = 0; j < 5; ++j) {
        int c = last5[j];
        if (('A' <= c) && (c <= 'Z')) c += 'a'-'A';
        if (c != total[j]) return false;
    }
    return true;
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
    
    StringMap colNameMap(confTab, stateId, "!MapColumn");
    auto candidateMap = getConfCandidates(confTab, stateId);
    PrecinctMap precinctMap;
    for (auto& srcFile : srcFiles) {
        if (optVerbose) printf("reading %s ...\n", srcFile.c_str());
        const char* htmlOrCsv = (strstr(srcFile.c_str(), ".htm") ? "html" : "csv");
        Table countyA(htmlOrCsv, srcFile);
        const char* fname = srcFile.c_str();
        const char* endCounty = strstr(fname, "precinct");
        while ((endCounty > fname) && (endCounty[-1] == '_')) --endCounty;
        const char* beginCounty = endCounty;
        while ((beginCounty > fname) && (beginCounty[-1] != '_')) --beginCounty;
        char countyBuf[512];
        strncmp(countyBuf, beginCounty, endCounty-beginCounty);
        countyBuf[beginCounty-endCounty] = 0;
        std::string guessCounty(countyBuf);
        //
        // First transformation is to apply colNameMap
        //
        auto newColNamesA = countyA.getColNames();
        for (size_t idx = 0; idx < newColNamesA.size(); ++idx) {
            newColNamesA[idx] = colNameMap.mapString(newColNamesA[idx]);
            //printf("-- newColNames[%ld] = %s\n", idx, newColNamesA[idx].c_str());
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
                        }
                        return false;
                    }
                );
            }
        } else {
            //
            // Accumulating the real results
            //
            auto colCounty    = countyB.findColIdx("County");
            auto colPrecinct  = countyB.findColIdx("PrecinctName");
            auto colRace      = countyB.findColIdx("Race");
            auto colCandidate = countyB.findColIdx("Candidate");
            std::vector<int> validCols;
            std::vector<int> invalidCols;
            std::vector<const char*> validColNames({
                "ElectionDayVotes",
                "Votes",
                "absentee",
                "absentee_affidavit",
                "absentee_hc",
                "absentee_military_votes",
                "absentee_votes",
                "advance_votes",
                "affidavit",
                "affidavit_votes",
                "election_day",
                "emergency_votes",
                "federal_votes",
                "machine_votes",
                "manually_counted_emergency",
                "military_votes",
                "paper_votes",
                "poll_votes",
                "polling_votes",
                "provisional",
                "provisional_votes",
                "public_counter_votes",
                "special_presidential",
                "vote",
                "votes",
            });
            for (auto colName : validColNames) {
                auto colIdx = countyB.findColIdx(colName);
                if (colIdx >= 0) {
                    validCols.push_back(colIdx);
                }
            }
            StringMap candidateValueMap(confTab, stateId, "Candidate");
            StringMap raceValueMap(confTab, stateId, "Race");
            countyB.scanRows(
                [=,&raceValueMap,&candidateValueMap,&candidateMap,&precinctMap](const TableRow& row)->bool {
                    std::string valCounty;
                    //printf("-- colCounty %d\n", colCounty);
                    if (colCounty >= 0) {
                        valCounty = row[colCounty].getString();
                        //printf("-- row.getString() %s\n", valCounty.c_str());
                    }
                    if (valCounty == "") {
                        valCounty = guessCounty;
                        //printf("-- use guessCounty %s\n", guessCounty.c_str());
                    }
                    auto valPrecinct = row[colPrecinct].getString();
                    if ((valPrecinct == "") ||
                        (valPrecinct == valCounty) || // "COWLEY KS" uses this as county-total
                        stringEndsWithTotal(valPrecinct)) {
                        //fprintf(stderr, "DEBUG: skip precinct '%s'\n", valPrecinct.c_str());
                        return false;
                    }
                    auto valRace = ((colRace >= 0) ? row[colRace].getString() : "President");
                    valRace = raceValueMap.mapString(valRace);
                    if (!isMatchingStr(false, valRace, "president") &&
                        !isMatchingStr(false, valRace, "electors for president & vice president")) {
                        return false;
                    }
                    auto valCandidate = row[colCandidate].getString();
                    //
                    // Apply the right mapping rules here, before looking up the name
                    // in the table of canonical candidates.
                    //
                    valCandidate = candidateValueMap.mapString(valCandidate);                    
                    //
                    // Check for rows with candidate-names which have been mapped to "Ignore",
                    // this is used to skip rows with partial sums or totals, rather than
                    // votes for individual candidates.
                    //
                    if (isMatchingStr(false, valCandidate, "Ignore")) {
                        // Some files contain precinct partial totals, which should be ignored,
                        return false;
                    }
                    auto iter = candidateMap.find(valCandidate);
                    if (iter == candidateMap.end()) {
                        if (valCandidate == "") {
                            valCandidate = "Invalid";
                        } else {
                            valCandidate = "Other";
                        }
                        iter = candidateMap.find(valCandidate);
                    }
                    int candidateIdx = iter->second;
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
                    return false;
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
        precinctMap.genOutput(out, candidateMap);
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

 