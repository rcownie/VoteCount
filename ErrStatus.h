#ifndef ERRSTATUS_H
#define ERRSTATUS_H 1
/*
 * ErrStatus.h - utility class for propagating errors through functional code
 *
 * Copyright (c) Richard Cownie, 2017.
 *
 * Author email: richard.cownie@pobox.com
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <memory>
#include <string>
#include <vector>

//
// Error severity values
//
#define SEV_FATAL   1
#define SEV_ERROR   2
#define SEV_WARNING 3
#define SEV_INFO    4
#define SEV_DEBUG   5
#define SEV_UNKNOWN 99

//
// A single message
//

class ErrSingle {
public:
    size_t seqId_;
    const char* file_;
    int line_;
    int severity_;
    int code_;
    std::string str_;

private:
    static size_t seqIdCounter;

public:
    ErrSingle(const char* file, int line, int sev, int code, const std::string& str) :
        seqId_(++seqIdCounter),
        file_(file),
        line_(line),
        severity_(sev),
        code_(code),
        str_(str) {
    }

    ErrSingle(const ErrSingle& b) = default;

    ErrSingle& operator=(const ErrSingle& b) = default;

    bool operator<(const ErrSingle& b) const {
        return(seqId_ < b.seqId_);
    }
};

class ErrStatus {
public:
    bool isFail_;
    int  mostSevere_;
    std::vector<ErrSingle> errVec_;

public:
    ErrStatus() :
        isFail_(false),
        mostSevere_(SEV_UNKNOWN),
        errVec_() {
    }
    
    ErrStatus(const ErrStatus& b) = default;
    
    void clear() {
        isFail_ = false;
        mostSevere_ = SEV_UNKNOWN;
        errVec_.clear();
    }
        
    bool isFail() {
        return(isFail_);
    }

    void push(const char* file, int line, int sev, int code, const char* msg);
    
    void combine(const ErrStatus& b);

    std::string toString() const;
};

#define STATUS_FATAL(s, code, msg) { \
    (s).push(__FILE__, __LINE__, SEV_FATAL, (code), (msg)); \
}

#define STATUS_ERROR(s, code, msg) { \
    (s).push(__FILE__, __LINE__, SEV_ERROR, (code), (msg)); \
}

#define STATUS_WARNING(s, code, msg) { \
    (s).push(__FILE__, __LINE__, SEV_WARNING, (code), (msg)); \
}

#define STATUS_INFO(s, msg) { \
    (s).push(__FILE__, __LINE__, SEV_INFO, 0, (msg)); \
}

#define STATUS_DEBUG(s, code, msg) { \
    (s).push(__FILE__, __LINE__, SEV_DEBUG, 0, (msg)); \
}

#endif

