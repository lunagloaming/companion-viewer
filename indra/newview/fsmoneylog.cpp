/**
 * @file fsmoneylog.cpp
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

#include "llviewerprecompiledheaders.h"

#include "fsmoneylog.h"

#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "lldate.h"
#include "lldir.h"
#include "llfile.h"
#include "lltransactiontypes.h"
#include "message.h"
#include "tea.h"

#include <algorithm>
#include <deque>

namespace
{
    // One line of the log, filled in when the message arrives and written once the name is known.
    struct Entry
    {
        std::string file;     // the account's money_log.txt, fixed at arrival
        std::string time;     // UTC, ISO 8601
        S32 change = 0;
        S32 balance = 0;
        LLUUID other_id;      // who the money came from or went to (null: the system)
        std::string object;   // object or item name, or the payment's description
        std::string kind;
    };

    bool sHaveBalance = false;  // a balance has been seen this session
    S32 sLastBalance = 0;
    std::deque<LLUUID> sRecentTransactions;  // the server can repeat a reply; log each transaction once

    std::string clean(std::string text)
    {
        for (char& c : text)
        {
            if (c == '\t' || c == '\n' || c == '\r')
            {
                c = ' ';
            }
        }
        return text;
    }

    void write_line(const Entry& entry, const std::string& name)
    {
        bool is_new = !LLFile::isfile(entry.file);
        llofstream out(entry.file.c_str(), std::ios_base::app);
        if (!out.is_open())
        {
            LL_WARNS("Money") << "Cannot open " << entry.file << LL_ENDL;
            return;
        }
        if (is_new)
        {
            out << "# Every change of the " << Tea::wrapCurrency("L$") << " balance, written by the viewer. Tab-separated:\n"
                << "# time (UTC)\tchange\tbalance after\tfrom or to\tobject or item\tkind\tkey of from or to\n";
        }
        out << entry.time << '\t'
            << (entry.change > 0 ? "+" : "") << entry.change << '\t'
            << entry.balance << '\t'
            << clean(name) << '\t'
            << clean(entry.object) << '\t'
            << entry.kind << '\t'
            << (entry.other_id.notNull() ? entry.other_id.asString() : std::string()) << '\n';
    }

    std::string kind_of(S32 transaction_type)
    {
        switch (transaction_type)
        {
            case TRANS_GIFT:               return "payment";
            case TRANS_PAY_OBJECT:         return "paid an object";
            case TRANS_OBJECT_PAYS:        return "paid by an object";
            case TRANS_OBJECT_SALE:        return "object sale";
            case TRANS_LAND_SALE:          return "land sale";
            case TRANS_LAND_PASS_SALE:     return "land pass";
            case TRANS_GROUP_CREATE:       return "group creation";
            case TRANS_GROUP_JOIN:         return "group join";
            case TRANS_GROUP_LAND_DEED:    return "land deed";
            case TRANS_UPLOAD_CHARGE:      return "upload";
            case TRANS_CLASSIFIED_CHARGE:  return "classified ad";
            case TRANS_PARCEL_DIR_FEE:     return "parcel directory fee";
            case TRANS_LAND_RELEASE:       return "land release";
            case TRANS_STIPEND_BASIC:
            case TRANS_STIPEND_DEVELOPER:
            case TRANS_STIPEND_ALWAYS:
            case TRANS_STIPEND_DAILY:
            case TRANS_STIPEND_RATING:
            case TRANS_STIPEND_DELTA:      return "stipend";
            default:                       return llformat("transaction type %d", transaction_type);
        }
    }
}

void FSMoneyLog::record(LLMessageSystem* msg)
{
    if (!msg || gDirUtilp->getLindenUserDir().empty())
    {
        return;
    }

    S32 balance = 0;
    LLUUID transaction_id;
    std::string description;
    msg->getS32("MoneyData", "MoneyBalance", balance);
    msg->getUUID("MoneyData", "TransactionID", transaction_id);
    msg->getString("MoneyData", "Description", description);

    if (transaction_id.notNull())
    {
        if (std::find(sRecentTransactions.begin(), sRecentTransactions.end(), transaction_id) != sRecentTransactions.end())
        {
            return;
        }
        sRecentTransactions.push_back(transaction_id);
        if (sRecentTransactions.size() > 50)
        {
            sRecentTransactions.pop_front();
        }
    }

    Entry entry;
    entry.file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "money_log.txt");
    entry.time = LLDate::now().asString();
    entry.balance = balance;
    entry.change = sHaveBalance ? balance - sLastBalance : 0;
    bool first = !sHaveBalance;
    sHaveBalance = true;
    sLastBalance = balance;

    bool is_group = false;
    bool has_transaction = false;
    if (msg->has("TransactionInfo"))
    {
        S32 type = 0;
        LLUUID source_id;
        LLUUID dest_id;
        bool source_is_group = false;
        bool dest_is_group = false;
        S32 amount = 0;
        bool success = false;
        std::string item;
        msg->getS32("TransactionInfo", "TransactionType", type);
        msg->getUUID("TransactionInfo", "SourceID", source_id);
        msg->getBOOL("TransactionInfo", "IsSourceGroup", source_is_group);
        msg->getUUID("TransactionInfo", "DestID", dest_id);
        msg->getBOOL("TransactionInfo", "IsDestGroup", dest_is_group);
        msg->getS32("TransactionInfo", "Amount", amount);
        msg->getString("TransactionInfo", "ItemDescription", item);
        msg->getBOOL("MoneyData", "TransactionSuccess", success);

        if (success && (source_id.notNull() || dest_id.notNull()))
        {
            has_transaction = true;
            bool you_paid = (source_id == gAgentID);
            entry.other_id = you_paid ? dest_id : source_id;
            is_group = you_paid ? dest_is_group : source_is_group;
            entry.kind = kind_of(type);
            // A payment's description is "Payment" unless the payer typed one
            entry.object = (type == TRANS_GIFT && item == "Payment") ? std::string() : item;
            // The transaction's own amount; the balance difference would merge transactions that
            // arrive together
            entry.change = you_paid ? -amount : amount;
        }
    }

    if (!has_transaction)
    {
        if (first)
        {
            entry.kind = "balance at login";
        }
        else if (entry.change == 0)
        {
            return;  // a balance refresh with nothing new
        }
        else
        {
            entry.kind = "balance changed";
            entry.object = description;
        }
    }

    if (entry.other_id.isNull())
    {
        write_line(entry, std::string());
    }
    else if (is_group)
    {
        if (!gCacheName)
        {
            write_line(entry, std::string());
            return;
        }
        gCacheName->getGroup(entry.other_id,
            [entry](const LLUUID&, const std::string& name, bool) { write_line(entry, name); });
    }
    else
    {
        LLAvatarNameCache::get(entry.other_id,
            [entry](const LLUUID&, const LLAvatarName& av_name) { write_line(entry, av_name.getCompleteName(true, true)); });
    }
}
