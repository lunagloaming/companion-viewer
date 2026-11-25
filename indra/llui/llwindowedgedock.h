/**
 * @file llwindowedgedock.h
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

#ifndef LL_LLWINDOWEDGEDOCK_H
#define LL_LLWINDOWEDGEDOCK_H

#include "llhandle.h"
#include "llrect.h"
#include "llsd.h"
#include "llsingleton.h"

#include <string>
#include <vector>

class LLContextMenu;
class LLFloater;
class LLView;

// Floaters docked to an edge of the viewer window stay ordinary children of
// gFloaterView. This class lays them out in an area along each edge and
// shrinks the "inset views" (the world view placeholder and the toolbar view
// holder) so the world view and the toolbars sit between the areas instead of
// under them.
//
// The edge docked to first runs the full length of the window; the next one
// runs the length that is left, and so on. Several floaters in one area share it: stacked top to bottom
// at the sides, left to right at the top and bottom. Every floater can dock
// unless it is on the exclusion list (FSWindowEdgeDockExcluded) or too big for
// the edge. The layout is saved in FSWindowEdgeDockLayout.
class LLWindowEdgeDock : public LLSingleton<LLWindowEdgeDock>
{
    LLSINGLETON(LLWindowEdgeDock);
    ~LLWindowEdgeDock();

public:
    // How far a docked floater's title bar must be dragged before it comes loose
    static constexpr S32 UNDOCK_SLOP = 12;

    enum EEdge
    {
        EDGE_NONE = -1,
        EDGE_TOP = 0,
        EDGE_LEFT,
        EDGE_RIGHT,
        EDGE_BOTTOM,
        EDGE_COUNT
    };

    static EEdge edgeFromName(const std::string& name);
    static std::string edgeName(EEdge edge);

    // The views that leave the docked areas out. They must share gFloaterView's parent.
    void addInsetView(LLView* view);
    // The navigation bar along the top of the window: docked areas start below its location row, which keeps the
    // full width, and its favorites row narrows to the space between the side areas (an area docked to the top
    // sits below it). Any of them may be null; rows that are hidden take no space.
    void setNavigationBar(LLView* location_row, LLView* favorites_row, LLView* favorites);

    bool canDock(LLFloater* floater, EEdge edge);
    void dock(LLFloater* floater, EEdge edge, S32 position = -1);
    void undock(LLFloater* floater);
    EEdge getEdge(const LLFloater* floater) const;

    // Hooks from LLFloater and LLFloaterView
    void onFloaterVisibilityChanged(LLFloater* floater, bool visible);
    void onFloaterMinimized(LLFloater* floater);
    void onFloaterClosed(LLFloater* floater, bool app_quitting);
    void onFloaterAttachedElsewhere(LLFloater* floater);   // hosted in a multi-floater, or attached to a button
    void onFloaterReshapedByUser(LLFloater* floater, const LLRect& old_rect);
    void onFloaterViewReshaped() { mLayoutDirty = true; }
    void drawBackgrounds();
    void drawDropPreview();

    // Dragging a floater's title bar. Screen coordinates.
    // undockForDrag returns true if the floater was docked and is now free under the mouse.
    bool undockForDrag(LLFloater* floater, S32 screen_x, S32 screen_y);
    void updateDrag(LLFloater* floater, S32 screen_x, S32 screen_y);
    void endDrag(LLFloater* floater);

    // The title bar's right-click menu. x and y are local to spawning_view. False if it has nothing to offer.
    bool showMenu(LLFloater* floater, LLView* spawning_view, S32 x, S32 y);

    // Once a frame, before the world view rect is read
    void update();

    // The space the docked areas take from each side of the floater view
    S32 getInset(EEdge edge) const { return mAppliedInsets[edge]; }

private:
    struct Entry
    {
        std::string         mName;
        LLSD                mKey;
        F32                 mLength = 0.f;  // along the edge, in pixels; a preference, scaled to fit
        LLRect              mFreeRect;      // where it was before docking
        LLHandle<LLFloater> mFloater;
    };

    struct Area
    {
        std::vector<Entry>  mEntries;       // in order: top to bottom at the sides, left to right at the top and bottom
        S32                 mDepth = 0;     // across the edge: the preferred width (sides) or height (top, bottom)
    };

    void loadLayout();
    void saveLayout();

    Entry* findEntry(const LLFloater* floater, EEdge* edge_out = nullptr, size_t* index_out = nullptr);
    Entry* findSavedEntry(LLFloater* floater, EEdge* edge_out);
    void removeEntry(EEdge edge, size_t index, bool restore_free_rect);

    bool isActive(const Entry& entry) const;
    static bool isHorizontal(EEdge edge) { return edge == EDGE_TOP || edge == EDGE_BOTTOM; }

    LLRect getFrame() const;
    S32 acrossSize(LLFloater* floater, EEdge edge) const;     // the floater's minimum across the edge
    S32 alongMinimum(LLFloater* floater, EEdge edge) const;
    S32 maxDepth(EEdge edge, const LLRect& frame) const;

    void arrange();
    // world_out, if given: what is left for the world view (under the navigation bar, as ever, without a top area)
    void computeAreaRects(const LLRect& frame, const S32 depths[EDGE_COUNT], const std::vector<EEdge>& order,
                          LLRect rects[EDGE_COUNT], LLRect* world_out = nullptr) const;
    bool measureNavigationBar(); // true if the rows moved or were shown or hidden
    void fitFavorites(const LLRect rects[EDGE_COUNT]);
    void updateOrder();
    void applyInsets();
    bool resizeInProgress() const;
    void setDockedState(LLFloater* floater, bool docked);
    void restoreFloater(LLFloater* floater, const LLRect& free_rect);

    void onMenuDock(const LLSD& param);
    void onMenuUndock();
    void onMenuExclude();
    bool onMenuCanDock(const LLSD& param);
    bool onMenuIsDocked();

    Area                    mAreas[EDGE_COUNT];
    std::vector<EEdge>      mOrder;     // edges with docked floaters, in the order they were first docked to
    S32                     mDepths[EDGE_COUNT];        // the depths in use after the last arrange
    S32                     mAppliedInsets[EDGE_COUNT]; // what the inset views leave out now
    std::vector<LLHandle<LLView> > mInsetViews;
    LLHandle<LLView>        mLocationRow;
    LLHandle<LLView>        mFavoritesRow;
    LLHandle<LLView>        mFavorites;
    S32                     mFavoritesMarginLeft = 0;   // its own margins in its row, without docked areas
    S32                     mFavoritesMarginRight = 0;
    S32                     mDockTop = S32_MAX;         // floater view coordinates: below the location row
    S32                     mFavoritesBottom = S32_MAX; // and the bottom of the favorites row, where a top area starts

    bool                    mLoaded = false;
    bool                    mArranging = false;
    bool                    mLayoutDirty = false;
    bool                    mInsetsDirty = false;

    EEdge                   mDropEdge = EDGE_NONE;      // where a dragged floater would dock
    S32                     mDropPosition = -1;
    LLRect                  mDropRect;                  // in floater view coordinates

    LLHandle<LLContextMenu> mMenuHandle;
    LLHandle<LLFloater>     mMenuFloater;
};

#endif // LL_LLWINDOWEDGEDOCK_H
