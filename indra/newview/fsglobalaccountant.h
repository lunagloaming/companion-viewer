/**
 * @file fsglobalaccountant.h
 * @brief Starts Global Accountant (tools/slmoney), the app that shows the money logs.
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

#ifndef FS_GLOBALACCOUNTANT_H
#define FS_GLOBALACCOUNTANT_H

// Help > Global Accountant: starts the Global Accountant app (tools/slmoney), a separate program
// that shows every account's balance, income and expenses from the money logs (fsmoneylog).
// It is packaged with the viewer; the viewer also looks where its own installers put it, or at FSGlobalAccountantPath if that is set, and says so if it isn't there.
namespace FSGlobalAccountant
{
    void launch();
}

#endif // FS_GLOBALACCOUNTANT_H
