/**
 * @file fsmoneylog.h
 * @brief Writes every change of the L$ balance to a per-account text file.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2026 Luna Witch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#ifndef FS_MONEYLOG_H
#define FS_MONEYLOG_H

class LLMessageSystem;

// Appends one line to money_log.txt in the account's folder for every change of the balance:
// time (UTC), change, balance after it, who it came from or went to, the object or item, and the
// kind of transaction. The first balance of each session is logged too, as a starting point.
// It runs for every MoneyBalanceReply, whatever the notification settings are. Nothing reads the
// file; it's a record for the user.
namespace FSMoneyLog
{
    void record(LLMessageSystem* msg);
}

#endif // FS_MONEYLOG_H
