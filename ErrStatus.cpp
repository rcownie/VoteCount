//
// ErrStatus.cpp - utility class for propagating errors through functional code
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
#include <algorithm>
#include <sstream>

size_t ErrSingle::seqIdCounter;

void ErrStatus::push(const char* file, int line, int sev, int code, const char* msg) {
    if (mostSevere_ > sev) {
        mostSevere_ = sev;
        if ((sev == SEV_ERROR) || (sev == SEV_FATAL)) isFail_ = true;
    }
    errVec_.emplace_back(file, line, sev, code, msg);
}

void ErrStatus::combine(const ErrStatus& b) {
    isFail_ = (isFail_ || b.isFail_);
    if (mostSevere_ > b.mostSevere_) {
        mostSevere_ = b.mostSevere_;
    }
    errVec_.reserve(errVec_.size()+b.errVec_.size());
    for (auto& e : b.errVec_) {
        errVec_.push_back(e);
    }
    std::sort(errVec_.begin(), errVec_.end());
}

std::string ErrStatus::toString() const {
    std::stringstream ss;
    for (auto& e : errVec_) {
        const char* sevStr;
        switch (e.severity_) {
            case SEV_FATAL:   sevStr = "FATAL"; break;
            case SEV_ERROR:   sevStr = "ERROR"; break;
            case SEV_WARNING: sevStr = "WARNING"; break;
            case SEV_INFO:    sevStr = "INFO"; break;
            case SEV_DEBUG:   sevStr = "DEBUG"; break;
            default:          sevStr = "SEV_UNKNOWN"; break;
        }
        ss << sevStr << ": " << e.file_ << "," << e.line_ << ": ";
        if (e.code_ != 0) ss << "[" << e.code_ << "]";
        ss << e.str_;
        
        if (e.str_[e.str_.size()-1] != '\n') {
            ss << "\n";
        }
    }
    return ss.str();
}
