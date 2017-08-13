*
* ErrStatus.cpp - utility class for propagating errors through functional code
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
#include "ErrStatus.h"

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
        mostSevere_ = b.mostSevere_
    }
}
