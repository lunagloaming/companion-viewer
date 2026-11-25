/**
 * @file llwindowedgedock.cpp
 * @brief Docks floaters to the edges of the viewer window, taking the space from the world view.
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

#include "linden_common.h"

#include "llwindowedgedock.h"

#include "llcontrol.h"
#include "lldraghandle.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llrender2dutils.h"
#include "llresizebar.h"
#include "llui.h"
#include "lluicolortable.h"
#include "lluictrlfactory.h"

#include <algorithm>

namespace
{
    // An area may take at most this share of the window across its edge, and a floater
    // whose minimum size across the edge is larger can't dock there.
    constexpr F32 MAX_AREA_SHARE = 0.4f;

    // How close to the window edge the mouse must be while dragging a title bar to dock there.
    constexpr S32 DROP_ZONE = 16;

    const char* const EDGE_NAMES[LLWindowEdgeDock::EDGE_COUNT] = { "top", "left", "right", "bottom" };

    LLControlGroup* settings()
    {
        return LLUI::getInstance()->mSettingGroups["config"];
    }
}

LLWindowEdgeDock::LLWindowEdgeDock()
{
    for (S32 edge = 0; edge < EDGE_COUNT; ++edge)
    {
        mDepths[edge] = 0;
        mAppliedInsets[edge] = 0;
    }
}

LLWindowEdgeDock::~LLWindowEdgeDock()
{
}

// static
LLWindowEdgeDock::EEdge LLWindowEdgeDock::edgeFromName(const std::string& name)
{
    for (S32 edge = 0; edge < EDGE_COUNT; ++edge)
    {
        if (name == EDGE_NAMES[edge])
        {
            return (EEdge)edge;
        }
    }
    return EDGE_NONE;
}

// static
std::string LLWindowEdgeDock::edgeName(EEdge edge)
{
    return (edge > EDGE_NONE && edge < EDGE_COUNT) ? EDGE_NAMES[edge] : "";
}

void LLWindowEdgeDock::addInsetView(LLView* view)
{
    if (view)
    {
        mInsetViews.push_back(view->getHandle());
        mInsetsDirty = true;
    }
}

void LLWindowEdgeDock::addTopInsetView(LLView* view)
{
    if (view)
    {
        mTopInsetViews.push_back(view->getHandle());
        mInsetsDirty = true;
    }
}

//-----------------------------------------------------------------------------
// Saved layout
//-----------------------------------------------------------------------------

void LLWindowEdgeDock::loadLayout()
{
    mLoaded = true;

    LLControlGroup* group = settings();
    if (!group || !group->controlExists("FSWindowEdgeDockLayout"))
    {
        return;
    }

    const LLSD layout = group->getLLSD("FSWindowEdgeDockLayout");
    if (!layout.isMap())
    {
        return;
    }

    const LLSD& depths = layout["depths"];
    for (LLSD::map_const_iterator it = depths.beginMap(); it != depths.endMap(); ++it)
    {
        EEdge edge = edgeFromName(it->first);
        if (edge != EDGE_NONE)
        {
            mAreas[edge].mDepth = it->second.asInteger();
        }
    }

    const LLSD& order = layout["order"];
    for (LLSD::array_const_iterator it = order.beginArray(); it != order.endArray(); ++it)
    {
        EEdge edge = edgeFromName(it->asString());
        if (edge != EDGE_NONE && std::find(mOrder.begin(), mOrder.end(), edge) == mOrder.end())
        {
            mOrder.push_back(edge);
        }
    }

    const LLSD& floaters = layout["floaters"];
    for (LLSD::array_const_iterator it = floaters.beginArray(); it != floaters.endArray(); ++it)
    {
        const LLSD& saved = *it;
        EEdge edge = edgeFromName(saved["edge"].asString());
        if (edge == EDGE_NONE || saved["name"].asString().empty())
        {
            continue;
        }

        Entry entry;
        entry.mName = saved["name"].asString();
        entry.mKey = saved["key"];
        entry.mLength = (F32)saved["length"].asReal();
        if (saved.has("free_rect"))
        {
            entry.mFreeRect.setValue(saved["free_rect"]);
        }
        mAreas[edge].mEntries.push_back(entry);
    }
    updateOrder();
}

void LLWindowEdgeDock::saveLayout()
{
    LLControlGroup* group = settings();
    if (!group || !group->controlExists("FSWindowEdgeDockLayout"))
    {
        return;
    }

    LLSD layout = LLSD::emptyMap();
    LLSD depths = LLSD::emptyMap();
    LLSD floaters = LLSD::emptyArray();
    for (S32 edge = 0; edge < EDGE_COUNT; ++edge)
    {
        depths[EDGE_NAMES[edge]] = mAreas[edge].mDepth;
        for (const Entry& entry : mAreas[edge].mEntries)
        {
            LLSD saved;
            saved["name"] = entry.mName;
            saved["key"] = entry.mKey;
            saved["edge"] = EDGE_NAMES[edge];
            saved["length"] = entry.mLength;
            saved["free_rect"] = entry.mFreeRect.getValue();
            floaters.append(saved);
        }
    }
    LLSD order = LLSD::emptyArray();
    for (EEdge edge : mOrder)
    {
        order.append(EDGE_NAMES[edge]);
    }
    layout["depths"] = depths;
    layout["order"] = order;
    layout["floaters"] = floaters;
    group->setLLSD("FSWindowEdgeDockLayout", layout);
}

//-----------------------------------------------------------------------------
// Entries
//-----------------------------------------------------------------------------

LLWindowEdgeDock::Entry* LLWindowEdgeDock::findEntry(const LLFloater* floater, EEdge* edge_out, size_t* index_out)
{
    if (!floater)
    {
        return nullptr;
    }

    for (S32 edge = 0; edge < EDGE_COUNT; ++edge)
    {
        std::vector<Entry>& entries = mAreas[edge].mEntries;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (entries[i].mFloater.get() == floater)
            {
                if (edge_out)
                {
                    *edge_out = (EEdge)edge;
                }
                if (index_out)
                {
                    *index_out = i;
                }
                return &entries[i];
            }
        }
    }
    return nullptr;
}

// A saved entry with no live floater that this floater should take over
LLWindowEdgeDock::Entry* LLWindowEdgeDock::findSavedEntry(LLFloater* floater, EEdge* edge_out)
{
    const std::string& name = floater->getInstanceName();
    if (name.empty())
    {
        return nullptr;
    }

    for (S32 edge = 0; edge < EDGE_COUNT; ++edge)
    {
        for (Entry& entry : mAreas[edge].mEntries)
        {
            if (entry.mName == name
                && !entry.mFloater.get()
                && LLFloater::KeyCompare::equate(entry.mKey, floater->getKey()))
            {
                *edge_out = (EEdge)edge;
                return &entry;
            }
        }
    }
    return nullptr;
}

void LLWindowEdgeDock::removeEntry(EEdge edge, size_t index, bool restore_free_rect)
{
    std::vector<Entry>& entries = mAreas[edge].mEntries;
    if (index >= entries.size())
    {
        return;
    }

    LLFloater* floater = entries[index].mFloater.get();
    LLRect free_rect = entries[index].mFreeRect;
    entries.erase(entries.begin() + index);
    updateOrder();

    if (floater)
    {
        setDockedState(floater, false);
        if (restore_free_rect)
        {
            restoreFloater(floater, free_rect);
        }
    }
    arrange();
}

bool LLWindowEdgeDock::isActive(const Entry& entry) const
{
    LLFloater* floater = entry.mFloater.get();
    return floater
        && !floater->isDead()
        && floater->isEdgeDocked()
        && floater->getVisible()
        && !floater->isMinimized()
        && floater->getParent() == gFloaterView;
}

void LLWindowEdgeDock::setDockedState(LLFloater* floater, bool docked)
{
    EEdge edge = EDGE_NONE;
    if (docked)
    {
        findEntry(floater, &edge);
    }
    floater->setEdgeDocked(docked, edge);
}

void LLWindowEdgeDock::restoreFloater(LLFloater* floater, const LLRect& free_rect)
{
    LLRect rect = free_rect;
    if (rect.isEmpty())
    {
        rect = floater->getRect();
    }
    // As the user's own move: the floater is kept on screen and remembers where it is.
    floater->setShape(rect, true);
    if (gFloaterView)
    {
        gFloaterView->adjustToFitScreen(floater, false);
    }
}

//-----------------------------------------------------------------------------
// Docking
//-----------------------------------------------------------------------------

LLRect LLWindowEdgeDock::getFrame() const
{
    return gFloaterView ? gFloaterView->getLocalRect() : LLRect();
}

S32 LLWindowEdgeDock::acrossSize(LLFloater* floater, EEdge edge) const
{
    if (isHorizontal(edge))
    {
        return floater->isResizable() ? floater->getMinHeight() : floater->getRect().getHeight();
    }
    return floater->isResizable() ? floater->getMinWidth() : floater->getRect().getWidth();
}

S32 LLWindowEdgeDock::alongMinimum(LLFloater* floater, EEdge edge) const
{
    if (!floater->isResizable())
    {
        return isHorizontal(edge) ? floater->getRect().getWidth() : floater->getRect().getHeight();
    }
    return isHorizontal(edge) ? floater->getMinWidth() : floater->getMinHeight();
}

S32 LLWindowEdgeDock::maxDepth(EEdge edge, const LLRect& frame) const
{
    return (S32)((isHorizontal(edge) ? frame.getHeight() : frame.getWidth()) * MAX_AREA_SHARE);
}

bool LLWindowEdgeDock::canDock(LLFloater* floater, EEdge edge)
{
    if (!floater || edge <= EDGE_NONE || edge >= EDGE_COUNT || !gFloaterView)
    {
        return false;
    }

    // Only free, movable floaters that can be found again by name
    if (floater->getParent() != gFloaterView
        || floater->getHost()
        || floater->isDocked()          // attached to a button (LLDockableFloater), a different thing
        || !floater->getCanDrag()
        || floater->getInstanceName().empty())
    {
        return false;
    }

    LLControlGroup* group = settings();
    if (group && group->controlExists("FSWindowEdgeDockExcluded"))
    {
        // By the name it is registered under or the name in its XUI file
        const LLSD excluded = group->getLLSD("FSWindowEdgeDockExcluded");
        const std::string& name = floater->getInstanceName();
        const std::string& xui_name = floater->getName();
        for (LLSD::array_const_iterator it = excluded.beginArray(); it != excluded.endArray(); ++it)
        {
            const std::string excluded_name = it->asString();
            if (excluded_name == name || excluded_name == xui_name)
            {
                return false;
            }
        }
    }

    // Too big for this edge
    const LLRect frame = getFrame();
    if (acrossSize(floater, edge) > maxDepth(edge, frame))
    {
        return false;
    }
    if (alongMinimum(floater, edge) > (isHorizontal(edge) ? frame.getWidth() : frame.getHeight()))
    {
        return false;
    }

    return true;
}

LLWindowEdgeDock::EEdge LLWindowEdgeDock::getEdge(const LLFloater* floater) const
{
    EEdge edge = EDGE_NONE;
    if (floater && floater->isEdgeDocked())
    {
        const_cast<LLWindowEdgeDock*>(this)->findEntry(floater, &edge);
    }
    return edge;
}

void LLWindowEdgeDock::dock(LLFloater* floater, EEdge edge, S32 position)
{
    if (!mLoaded)
    {
        loadLayout();
    }

    if (floater && floater->isMinimized())
    {
        floater->setMinimized(false);
    }
    if (!canDock(floater, edge))
    {
        return;
    }

    Entry entry;
    entry.mName = floater->getInstanceName();
    entry.mKey = floater->getKey();
    entry.mFloater = floater->getHandle();
    entry.mFreeRect = floater->getRect();
    entry.mLength = (F32)(isHorizontal(edge) ? floater->getRect().getWidth() : floater->getRect().getHeight());

    // Already docked somewhere: keep where it was before that
    EEdge old_edge = EDGE_NONE;
    size_t old_index = 0;
    if (Entry* old_entry = findEntry(floater, &old_edge, &old_index))
    {
        entry.mFreeRect = old_entry->mFreeRect;
        if (old_edge == edge)
        {
            entry.mLength = old_entry->mLength;
            if (position > (S32)old_index)
            {
                --position;     // the entries after it move up once it is taken out
            }
        }
        mAreas[old_edge].mEntries.erase(mAreas[old_edge].mEntries.begin() + old_index);
    }

    // The first floater in an area keeps its size across the edge
    Area& area = mAreas[edge];
    bool area_in_use = false;
    for (const Entry& other : area.mEntries)
    {
        area_in_use = area_in_use || isActive(other);
    }
    if (!area_in_use)
    {
        area.mDepth = isHorizontal(edge) ? floater->getRect().getHeight() : floater->getRect().getWidth();
    }

    if (position < 0 || position > (S32)area.mEntries.size())
    {
        position = (S32)area.mEntries.size();
    }
    area.mEntries.insert(area.mEntries.begin() + position, entry);
    updateOrder();

    LL_INFOS("WindowEdgeDock") << "Docked floater " << entry.mName << " (" << floater->getName() << ") to the "
                               << edgeName(edge) << " edge" << LL_ENDL;

    setDockedState(floater, true);
    arrange();
    saveLayout();
    floater->setFrontmost(false);
}

void LLWindowEdgeDock::undock(LLFloater* floater)
{
    EEdge edge = EDGE_NONE;
    size_t index = 0;
    if (findEntry(floater, &edge, &index))
    {
        removeEntry(edge, index, true);
        saveLayout();
    }
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

void LLWindowEdgeDock::onFloaterVisibilityChanged(LLFloater* floater, bool visible)
{
    if (!floater || (!visible && !floater->isEdgeDocked()) || floater->getInstanceName().empty())
    {
        return;
    }

    if (!mLoaded)
    {
        loadLayout();
    }

    if (visible && !floater->isEdgeDocked())
    {
        // Docked when it was last open: dock it again
        EEdge edge = EDGE_NONE;
        Entry* entry = findSavedEntry(floater, &edge);
        if (!entry)
        {
            return;
        }

        if (!canDock(floater, edge))
        {
            // Excluded since, or too big for the window now
            mAreas[edge].mEntries.erase(mAreas[edge].mEntries.begin() + (entry - &mAreas[edge].mEntries[0]));
            updateOrder();
            saveLayout();
            return;
        }

        entry->mFloater = floater->getHandle();
        if (!floater->getRect().isEmpty())
        {
            entry->mFreeRect = floater->getRect();
        }
        setDockedState(floater, true);
    }

    arrange();
}

void LLWindowEdgeDock::onFloaterMinimized(LLFloater* floater)
{
    if (floater && floater->isEdgeDocked())
    {
        arrange();
    }
}

void LLWindowEdgeDock::onFloaterClosed(LLFloater* floater, bool app_quitting)
{
    // A closed single-instance floater docks again when it is next opened. Other floaters
    // (several inventories, profiles and so on) come and go, so they forget it.
    if (!app_quitting && floater && floater->isEdgeDocked() && !floater->getIsSingleInstance())
    {
        undock(floater);
    }
}

void LLWindowEdgeDock::onFloaterAttachedElsewhere(LLFloater* floater)
{
    EEdge edge = EDGE_NONE;
    size_t index = 0;
    if (floater && floater->isEdgeDocked() && findEntry(floater, &edge, &index))
    {
        removeEntry(edge, index, false);
        saveLayout();
    }
}

void LLWindowEdgeDock::onFloaterReshapedByUser(LLFloater* floater, const LLRect& old_rect)
{
    if (mArranging)
    {
        return;
    }

    EEdge edge = EDGE_NONE;
    size_t index = 0;
    if (!findEntry(floater, &edge, &index))
    {
        return;
    }

    Area& area = mAreas[edge];
    const LLRect frame = getFrame();
    const LLRect new_rect = floater->getRect();

    // The inner edge sets the area's depth
    S32 depth = -1;
    switch (edge)
    {
    case EDGE_LEFT:
        if (new_rect.mRight != old_rect.mRight)
            depth = new_rect.mRight - frame.mLeft;
        break;
    case EDGE_RIGHT:
        if (new_rect.mLeft != old_rect.mLeft)
            depth = frame.mRight - new_rect.mLeft;
        break;
    case EDGE_TOP:
        if (new_rect.mBottom != old_rect.mBottom)
            depth = frame.mTop - new_rect.mBottom;
        break;
    case EDGE_BOTTOM:
        if (new_rect.mTop != old_rect.mTop)
            depth = new_rect.mTop - frame.mBottom;
        break;
    default:
        break;
    }
    if (depth >= 0)
    {
        S32 min_depth = 0;
        for (const Entry& entry : area.mEntries)
        {
            if (isActive(entry))
            {
                min_depth = llmax(min_depth, acrossSize(entry.mFloater.get(), edge));
            }
        }
        area.mDepth = llclamp(depth, min_depth, llmax(min_depth, maxDepth(edge, frame)));
    }

    // The edges between floaters move the divider with the neighbour
    if (floater->isResizable())
    {
        const bool horizontal = isHorizontal(edge);
        auto along = [horizontal](const LLRect& rect) { return horizontal ? rect.getWidth() : rect.getHeight(); };

        // Neighbours: the one before (above, or to the left) and after
        S32 before = -1;
        S32 after = -1;
        for (S32 i = (S32)index - 1; i >= 0 && before < 0; --i)
        {
            if (isActive(area.mEntries[i]))
                before = i;
        }
        for (S32 i = (S32)index + 1; i < (S32)area.mEntries.size() && after < 0; ++i)
        {
            if (isActive(area.mEntries[i]))
                after = i;
        }

        // How much this floater grew at its start (top, or left) and its end
        S32 grow_start = horizontal ? old_rect.mLeft - new_rect.mLeft : new_rect.mTop - old_rect.mTop;
        S32 grow_end = horizontal ? new_rect.mRight - old_rect.mRight : old_rect.mBottom - new_rect.mBottom;

        // Every flexible floater's length becomes its actual size, so the shares are exact
        for (Entry& entry : area.mEntries)
        {
            if (isActive(entry) && entry.mFloater.get()->isResizable())
            {
                entry.mLength = (F32)along(entry.mFloater.get()->getRect());
            }
        }
        Entry& self = area.mEntries[index];
        self.mLength = (F32)along(old_rect);

        auto move_divider = [&](S32 neighbour, S32 grow)
        {
            if (neighbour < 0 || grow == 0)
                return;
            Entry& other = area.mEntries[neighbour];
            LLFloater* other_floater = other.mFloater.get();
            if (!other_floater->isResizable())
                return;
            const S32 self_min = alongMinimum(floater, edge);
            const S32 other_min = alongMinimum(other_floater, edge);
            grow = llclamp(grow, self_min - (S32)self.mLength, (S32)other.mLength - other_min);
            self.mLength += grow;
            other.mLength -= grow;
        };
        move_divider(before, grow_start);
        move_divider(after, grow_end);
    }

    arrange();
    saveLayout();
}

//-----------------------------------------------------------------------------
// Layout
//-----------------------------------------------------------------------------

void LLWindowEdgeDock::updateOrder()
{
    // Edges no longer docked to drop out; newly docked ones go last
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        const EEdge edge = (EEdge)e;
        auto it = std::find(mOrder.begin(), mOrder.end(), edge);
        if (mAreas[edge].mEntries.empty() && it != mOrder.end())
        {
            mOrder.erase(it);
        }
    }
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        const EEdge edge = (EEdge)e;
        if (!mAreas[edge].mEntries.empty() && std::find(mOrder.begin(), mOrder.end(), edge) == mOrder.end())
        {
            mOrder.push_back(edge);
        }
    }
}

void LLWindowEdgeDock::computeAreaRects(const LLRect& frame, const S32 depths[EDGE_COUNT], const std::vector<EEdge>& order,
                                        LLRect rects[EDGE_COUNT]) const
{
    // Each area in turn takes a strip along its edge of what the areas before it left
    LLRect remaining = frame;
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        rects[e] = LLRect();
    }
    for (EEdge edge : order)
    {
        const S32 depth = depths[edge];
        if (depth <= 0)
        {
            continue;
        }
        switch (edge)
        {
        case EDGE_LEFT:
            rects[edge] = LLRect(remaining.mLeft, remaining.mTop, remaining.mLeft + depth, remaining.mBottom);
            remaining.mLeft += depth;
            break;
        case EDGE_RIGHT:
            rects[edge] = LLRect(remaining.mRight - depth, remaining.mTop, remaining.mRight, remaining.mBottom);
            remaining.mRight -= depth;
            break;
        case EDGE_TOP:
            rects[edge] = LLRect(remaining.mLeft, remaining.mTop, remaining.mRight, remaining.mTop - depth);
            remaining.mTop -= depth;
            break;
        case EDGE_BOTTOM:
            rects[edge] = LLRect(remaining.mLeft, remaining.mBottom + depth, remaining.mRight, remaining.mBottom);
            remaining.mBottom += depth;
            break;
        default:
            break;
        }
    }
}

void LLWindowEdgeDock::arrange()
{
    if (mArranging || !gFloaterView)
    {
        return;
    }
    mArranging = true;
    mLayoutDirty = false;

    const LLRect frame = getFrame();

    // Depths first: the areas docked to earlier decide where the later ones start
    S32 depths[EDGE_COUNT];
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        const EEdge edge = (EEdge)e;
        Area& area = mAreas[edge];
        depths[edge] = 0;

        S32 min_depth = 0;
        bool in_use = false;
        for (const Entry& entry : area.mEntries)
        {
            if (isActive(entry))
            {
                in_use = true;
                min_depth = llmax(min_depth, acrossSize(entry.mFloater.get(), edge));
            }
        }
        if (in_use)
        {
            S32 depth = area.mDepth > 0 ? area.mDepth : min_depth;
            depths[edge] = llclamp(depth, min_depth, llmax(min_depth, maxDepth(edge, frame)));
        }
    }

    LLRect rects[EDGE_COUNT];
    computeAreaRects(frame, depths, mOrder, rects);

    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        const EEdge edge = (EEdge)e;
        if (depths[edge] <= 0)
        {
            continue;
        }

        const bool horizontal = isHorizontal(edge);
        const LLRect& area_rect = rects[edge];
        const S32 length = horizontal ? area_rect.getWidth() : area_rect.getHeight();

        // Floaters that can't be resized keep their size; the others share what is left
        struct Slot
        {
            LLFloater*  mFloater;
            bool        mFixed;
            F32         mPreference;
            S32         mMinimum;
            S32         mSize;
        };
        std::vector<Slot> slots;
        S32 fixed_total = 0;
        for (Entry& entry : mAreas[edge].mEntries)
        {
            if (!isActive(entry))
            {
                continue;
            }
            LLFloater* floater = entry.mFloater.get();
            Slot slot;
            slot.mFloater = floater;
            slot.mFixed = !floater->isResizable();
            slot.mMinimum = alongMinimum(floater, edge);
            if (slot.mFixed)
            {
                slot.mSize = slot.mMinimum;
                fixed_total += slot.mSize;
            }
            else
            {
                if (entry.mLength <= 0.f)
                {
                    entry.mLength = (F32)(horizontal ? floater->getRect().getWidth() : floater->getRect().getHeight());
                }
                slot.mPreference = llmax(entry.mLength, 1.f);
                slot.mSize = 0;
            }
            slots.push_back(slot);
        }

        // Share the space by preference, giving any floater that would fall below its minimum
        // the minimum and sharing the rest again
        S32 flexible_space = llmax(length - fixed_total, 0);
        std::vector<bool> settled(slots.size(), false);
        for (size_t i = 0; i < slots.size(); ++i)
        {
            settled[i] = slots[i].mFixed;
        }
        bool changed = true;
        while (changed)
        {
            changed = false;
            F32 preference_total = 0.f;
            S32 space = flexible_space;
            for (size_t i = 0; i < slots.size(); ++i)
            {
                if (!slots[i].mFixed && settled[i])
                    space -= slots[i].mSize;
                else if (!settled[i])
                    preference_total += slots[i].mPreference;
            }
            for (size_t i = 0; i < slots.size(); ++i)
            {
                if (settled[i])
                    continue;
                S32 size = preference_total > 0.f ? (S32)(space * slots[i].mPreference / preference_total) : 0;
                if (size < slots[i].mMinimum)
                {
                    slots[i].mSize = slots[i].mMinimum;
                    settled[i] = true;
                    changed = true;
                    break;
                }
                slots[i].mSize = size;
            }
        }
        // The last flexible floater takes what rounding left over
        S32 used = 0;
        S32 last_flexible = -1;
        for (size_t i = 0; i < slots.size(); ++i)
        {
            used += slots[i].mSize;
            if (!slots[i].mFixed)
                last_flexible = (S32)i;
        }
        if (last_flexible >= 0 && used < length)
        {
            slots[last_flexible].mSize += length - used;
        }

        // Place them: from the top at the sides, from the left at the top and bottom
        S32 cursor = horizontal ? area_rect.mLeft : area_rect.mTop;
        for (const Slot& slot : slots)
        {
            LLFloater* floater = slot.mFloater;
            const S32 across = slot.mFixed
                ? (horizontal ? floater->getRect().getHeight() : floater->getRect().getWidth())
                : depths[edge];

            LLRect rect;
            if (horizontal)
            {
                const S32 bottom = edge == EDGE_TOP ? area_rect.mTop - across : area_rect.mBottom;
                rect.setLeftTopAndSize(cursor, bottom + across, slot.mSize, across);
                cursor += slot.mSize;
            }
            else
            {
                const S32 left = edge == EDGE_LEFT ? area_rect.mLeft : area_rect.mRight - across;
                rect.setLeftTopAndSize(left, cursor, across, slot.mSize);
                cursor -= slot.mSize;
            }

            if (rect != floater->getRect())
            {
                floater->setShape(rect, false);
            }
        }
    }

    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        mDepths[e] = depths[e];
    }
    mInsetsDirty = true;
    mArranging = false;
}

bool LLWindowEdgeDock::resizeInProgress() const
{
    // While an area or a divider is being dragged, the world view keeps its size: changing it
    // rebuilds the render targets. It catches up when the mouse is let go.
    LLView* captor = dynamic_cast<LLView*>(gFocusMgr.getMouseCapture());
    if (captor && dynamic_cast<LLResizeBar*>(captor))
    {
        LLFloater* floater = dynamic_cast<LLFloater*>(captor->getParent());
        return floater && floater->isEdgeDocked();
    }
    return false;
}

void LLWindowEdgeDock::applyInsets()
{
    mInsetsDirty = false;

    for (const LLHandle<LLView>& handle : mInsetViews)
    {
        LLView* view = handle.get();
        if (!view || !view->getParent())
        {
            continue;
        }
        LLRect rect = view->getParent()->getLocalRect();
        rect.mLeft += mDepths[EDGE_LEFT];
        rect.mRight -= mDepths[EDGE_RIGHT];
        rect.mTop -= mDepths[EDGE_TOP];
        rect.mBottom += mDepths[EDGE_BOTTOM];
        if (rect != view->getRect())
        {
            view->setShape(rect);
        }
    }

    // The space left over, in the floater view, then in each top view's parent: the view keeps its height
    if (gFloaterView)
    {
        LLRect space = getFrame();
        space.mLeft += mDepths[EDGE_LEFT];
        space.mRight -= mDepths[EDGE_RIGHT];
        space.mTop -= mDepths[EDGE_TOP];
        space.mBottom += mDepths[EDGE_BOTTOM];
        for (const LLHandle<LLView>& handle : mTopInsetViews)
        {
            LLView* view = handle.get();
            if (!view || !view->getParent())
            {
                continue;
            }
            LLRect in_parent;
            gFloaterView->localRectToOtherView(space, &in_parent, view->getParent());
            LLRect rect(in_parent.mLeft, in_parent.mTop, in_parent.mRight, in_parent.mTop - view->getRect().getHeight());
            if (rect != view->getRect())
            {
                view->setShape(rect);
            }
        }
    }

    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        mAppliedInsets[e] = mDepths[e];
    }
}

void LLWindowEdgeDock::update()
{
    if (mLayoutDirty)
    {
        arrange();
    }
    if (mInsetsDirty && !resizeInProgress())
    {
        applyInsets();
    }
}

void LLWindowEdgeDock::drawBackgrounds()
{
    LLRect rects[EDGE_COUNT];
    computeAreaRects(getFrame(), mDepths, mOrder, rects);

    static LLUIColor background = LLUIColorTable::instance().getColor("WindowEdgeDockBackgroundColor", LLColor4(0.1f, 0.1f, 0.1f, 1.f));
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        if (mDepths[e] > 0)
        {
            gl_rect_2d(rects[e], background.get(), true);
        }
    }
}

//-----------------------------------------------------------------------------
// Dragging
//-----------------------------------------------------------------------------

bool LLWindowEdgeDock::undockForDrag(LLFloater* floater, S32 screen_x, S32 screen_y)
{
    EEdge edge = EDGE_NONE;
    size_t index = 0;
    Entry* entry = findEntry(floater, &edge, &index);
    if (!entry || !gFloaterView)
    {
        return false;
    }

    // Keep the grabbed point of the title bar under the mouse, at the same share of the width
    const LLRect docked = floater->getRect();
    LLRect free_rect = entry->mFreeRect;
    if (free_rect.isEmpty())
    {
        free_rect = docked;
    }

    S32 x = 0;
    S32 y = 0;
    gFloaterView->screenPointToLocal(screen_x, screen_y, &x, &y);
    const F32 share = docked.getWidth() > 0 ? llclamp((F32)(x - docked.mLeft) / (F32)docked.getWidth(), 0.f, 1.f) : 0.5f;
    const S32 below_top = llmax(docked.mTop - y, 0);
    const S32 width = free_rect.getWidth();
    const S32 height = free_rect.getHeight();
    free_rect.setLeftTopAndSize(x - (S32)(share * width), y + below_top, width, height);
    entry->mFreeRect = free_rect;

    removeEntry(edge, index, true);
    saveLayout();
    return true;
}

void LLWindowEdgeDock::updateDrag(LLFloater* floater, S32 screen_x, S32 screen_y)
{
    mDropEdge = EDGE_NONE;
    mDropPosition = -1;

    LLControlGroup* group = settings();
    if (!gFloaterView || !floater || floater->isEdgeDocked()
        || (group && group->controlExists("FSWindowEdgeDockByDragging") && !group->getBOOL("FSWindowEdgeDockByDragging")))
    {
        return;
    }

    if (!mLoaded)
    {
        loadLayout();
    }

    S32 x = 0;
    S32 y = 0;
    gFloaterView->screenPointToLocal(screen_x, screen_y, &x, &y);
    const LLRect frame = getFrame();

    // Over an area already in use: join it, beside the docked floater under the mouse
    LLRect areas[EDGE_COUNT];
    computeAreaRects(frame, mDepths, mOrder, areas);
    EEdge edge = EDGE_NONE;
    for (S32 e = 0; e < EDGE_COUNT && edge == EDGE_NONE; ++e)
    {
        if (mDepths[e] > 0 && areas[e].pointInRect(x, y))
        {
            edge = (EEdge)e;
        }
    }

    // Otherwise the nearest edge, if the mouse is close enough to it
    if (edge == EDGE_NONE)
    {
        const S32 distances[EDGE_COUNT] = { frame.mTop - y, x - frame.mLeft, frame.mRight - x, y - frame.mBottom };
        for (S32 e = 0; e < EDGE_COUNT; ++e)
        {
            if (distances[e] < DROP_ZONE && (edge == EDGE_NONE || distances[e] < distances[edge]))
            {
                edge = (EEdge)e;
            }
        }
    }
    if (edge == EDGE_NONE || !canDock(floater, edge))
    {
        return;
    }

    const std::vector<Entry>& entries = mAreas[edge].mEntries;
    const bool horizontal = isHorizontal(edge);

    if (mDepths[edge] > 0)
    {
        // The docked floater under the mouse (or the nearest one along the edge): the new one takes
        // the half of it nearer the mouse, before it (above, or to the left) or after it
        S32 target = -1;
        S32 best_distance = S32_MAX;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (!isActive(entries[i]))
                continue;
            const LLRect rect = entries[i].mFloater.get()->getRect();
            const S32 along = horizontal ? x : y;
            const S32 start = horizontal ? rect.mLeft : rect.mBottom;
            const S32 end = horizontal ? rect.mRight : rect.mTop;
            const S32 distance = along < start ? start - along : (along > end ? along - end : 0);
            if (distance < best_distance)
            {
                best_distance = distance;
                target = (S32)i;
            }
        }
        if (target < 0)
        {
            return;
        }

        const LLRect rect = entries[target].mFloater.get()->getRect();
        LLRect half = areas[edge];
        bool before;
        if (horizontal)
        {
            before = x < rect.getCenterX();
            half.mLeft = before ? rect.mLeft : rect.getCenterX();
            half.mRight = before ? rect.getCenterX() : rect.mRight;
        }
        else
        {
            before = y > rect.getCenterY();
            half.mTop = before ? rect.mTop : rect.getCenterY();
            half.mBottom = before ? rect.getCenterY() : rect.mBottom;
        }
        mDropRect = half;
        mDropPosition = before ? target : target + 1;
        mDropEdge = edge;
        return;
    }

    // An empty edge: the whole area it would get
    S32 depths[EDGE_COUNT];
    for (S32 e = 0; e < EDGE_COUNT; ++e)
    {
        depths[e] = mDepths[e];
    }
    const S32 size = horizontal ? floater->getRect().getHeight() : floater->getRect().getWidth();
    depths[edge] = llclamp(size, acrossSize(floater, edge), llmax(acrossSize(floater, edge), maxDepth(edge, frame)));
    // It would be docked to after the edges in use
    std::vector<EEdge> order;
    for (EEdge other : mOrder)
    {
        if (other != edge)
            order.push_back(other);
    }
    order.push_back(edge);
    LLRect rects[EDGE_COUNT];
    computeAreaRects(frame, depths, order, rects);
    mDropRect = rects[edge];
    mDropPosition = (S32)entries.size();
    mDropEdge = edge;
}

void LLWindowEdgeDock::endDrag(LLFloater* floater)
{
    const EEdge edge = mDropEdge;
    const S32 position = mDropPosition;
    mDropEdge = EDGE_NONE;
    mDropPosition = -1;

    if (edge != EDGE_NONE && floater && !floater->isDead())
    {
        dock(floater, edge, position);
    }
}

void LLWindowEdgeDock::drawDropPreview()
{
    // The drag ended some other way (the mouse capture was taken away)
    if (mDropEdge != EDGE_NONE && !dynamic_cast<LLDragHandle*>(gFocusMgr.getMouseCapture()))
    {
        mDropEdge = EDGE_NONE;
    }
    if (mDropEdge == EDGE_NONE)
    {
        return;
    }

    static LLUIColor preview = LLUIColorTable::instance().getColor("WindowEdgeDockPreviewColor", LLColor4(0.3f, 0.6f, 1.f, 1.f));
    LLColor4 fill = preview.get();
    fill.mV[VALPHA] *= 0.3f;
    gl_rect_2d(mDropRect, fill, true);
    gl_rect_2d(mDropRect, preview.get(), false);
}

//-----------------------------------------------------------------------------
// Title bar menu
//-----------------------------------------------------------------------------

bool LLWindowEdgeDock::showMenu(LLFloater* floater, LLView* spawning_view, S32 x, S32 y)
{
    if (!floater || !spawning_view || !LLMenuGL::sMenuContainer)
    {
        return false;
    }

    bool offer = floater->isEdgeDocked();
    for (S32 e = 0; e < EDGE_COUNT && !offer; ++e)
    {
        offer = canDock(floater, (EEdge)e);
    }
    if (!offer)
    {
        return false;
    }

    LLContextMenu* menu = mMenuHandle.get();
    if (!menu)
    {
        LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit_registrar;
        commit_registrar.add("WindowEdgeDock.Dock", boost::bind(&LLWindowEdgeDock::onMenuDock, this, _2));
        commit_registrar.add("WindowEdgeDock.Undock", boost::bind(&LLWindowEdgeDock::onMenuUndock, this));

        LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
        enable_registrar.add("WindowEdgeDock.CanDock", boost::bind(&LLWindowEdgeDock::onMenuCanDock, this, _2));
        enable_registrar.add("WindowEdgeDock.IsDocked", boost::bind(&LLWindowEdgeDock::onMenuIsDocked, this));

        menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
            "menu_window_edge_dock.xml", LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
        if (!menu)
        {
            LL_WARNS() << "Unable to load the window edge dock menu" << LL_ENDL;
            return false;
        }
        mMenuHandle = menu->getHandle();
    }

    mMenuFloater = floater->getHandle();
    menu->show(x, y);
    LLMenuGL::showPopup(spawning_view, menu, x, y);
    return true;
}

void LLWindowEdgeDock::onMenuDock(const LLSD& param)
{
    LLFloater* floater = mMenuFloater.get();
    EEdge edge = edgeFromName(param.asString());
    if (floater && edge != EDGE_NONE && getEdge(floater) != edge)
    {
        dock(floater, edge);
    }
}

void LLWindowEdgeDock::onMenuUndock()
{
    if (LLFloater* floater = mMenuFloater.get())
    {
        undock(floater);
    }
}

bool LLWindowEdgeDock::onMenuCanDock(const LLSD& param)
{
    LLFloater* floater = mMenuFloater.get();
    EEdge edge = edgeFromName(param.asString());
    return floater && edge != EDGE_NONE && getEdge(floater) != edge && canDock(floater, edge);
}

bool LLWindowEdgeDock::onMenuIsDocked()
{
    LLFloater* floater = mMenuFloater.get();
    return floater && floater->isEdgeDocked();
}
