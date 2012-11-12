/************************************************************************
 *                                                                      *
 *  FreeSynd - a remake of the classic Bullfrog game "Syndicate".       *
 *                                                                      *
 *   Copyright (C) 2012  Benoit Blancard <benblan@users.sourceforge.net>*
 *                                                                      *
 *    This program is free software;  you can redistribute it and / or  *
 *  modify it  under the  terms of the  GNU General  Public License as  *
 *  published by the Free Software Foundation; either version 2 of the  *
 *  License, or (at your option) any later version.                     *
 *                                                                      *
 *    This program is  distributed in the hope that it will be useful,  *
 *  but WITHOUT  ANY WARRANTY;without even  the impliedwarranty of      *
 *  MERCHANTABILITY  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 *  General Public License for more details.                            *
 *                                                                      *
 *    You can view the GNU  General Public License, online, at the GNU  *
 *  project'sweb  site;  see <http://www.gnu.org/licenses/gpl.html>.  *
 *  The full text of the license is also included in the file COPYING.  *
 *                                                                      *
 ************************************************************************/

#include "menus/minimaprenderer.h"
#include "mission.h"
#include "gfx/screen.h"
#include "app.h"

const int MinimapRenderer::kMiniMapSizePx = 128;

void MinimapRenderer::setZoom(EZoom zoom) {
    zoom_ = zoom;
    pixpertile_ = 10 - zoom_;
    updateRenderingInfos();
}

BriefMinimapRenderer::BriefMinimapRenderer() : mm_timer(500) {
    scroll_step_ = 0;
}

/*!
 * Init the renderer with a new mission, zoom level and draw_enemies params.
 */
void BriefMinimapRenderer::init(Mission *pMission, EZoom zoom, bool draw_enemies) {
    p_mission_ = pMission;
    setZoom(zoom);
    b_draw_enemies_ = draw_enemies;
    mm_timer.reset();
    minimap_blink_ = 0;

    // Initialize minimap origin by looking for the position
    // of the first found agent on the map
    initMinimapLocation();
}

/*!
 * Centers the minimap on the starting position of agents
 */
void BriefMinimapRenderer::initMinimapLocation() {
    bool found = false;
    int maxx = p_mission_->mmax_x_;
    int maxy = p_mission_->mmax_y_;

    for (int x = 0; x < maxx && (!found); x++) {
        for (int y = 0; y < maxy && (!found); y++) {
            if (p_mission_->getMinimapOverlay(x, y) == 1) {
                // We found a tile with an agent on it
                // stop searching and memorize position
                mm_tx_ = x;
                mm_ty_ = y;
                found = true;
            }
        }
    }

    uint16 halftiles = mm_maxtile_ / 2;
    mm_tx_ = (mm_tx_ < halftiles) ? 0 : (mm_tx_ - halftiles + 1);
    mm_ty_ = (mm_ty_ < halftiles) ? 0 : (mm_ty_ - halftiles + 1);

    clipMinimapToRightAndDown();
}

/*!
 *
 */
void BriefMinimapRenderer::clipMinimapToRightAndDown() {
    if ((mm_tx_ + mm_maxtile_) >= p_mission_->mmax_x_) {
        // We assume that map size in tiles (p_mission_->mmax_x_)
        // is bigger than the minimap size (mm_maxtile_)
        mm_tx_ = p_mission_->mmax_x_ - mm_maxtile_;
    }

    if ((mm_ty_ + mm_maxtile_) >= p_mission_->mmax_y_) {
        // We assume that map size in tiles (p_mission_->mmax_y_)
        // is bigger than the minimap size (mm_maxtile_)
        mm_ty_ = p_mission_->mmax_y_ - mm_maxtile_;
    }
}

void BriefMinimapRenderer::updateRenderingInfos() {
    scroll_step_ = 30 / pixpertile_;
    mm_maxtile_ = 120 / pixpertile_;
}

void BriefMinimapRenderer::zoomOut() {
    switch (zoom_) {
    case ZOOM_X2:
        setZoom(ZOOM_X1);
        break;
    case ZOOM_X3:
        setZoom(ZOOM_X2);
        break;
    case ZOOM_X4:
        setZoom(ZOOM_X3);
        break;
    default:
        break;
    }

    // check if map should be aligned with right and bottom border
    // as when zooming out only displays more tiles but does not
    // move the minimap origin
    clipMinimapToRightAndDown();
}

bool BriefMinimapRenderer::handleTick(int elapsed) {
    if (mm_timer.update(elapsed)) {
        minimap_blink_ ^= 1;
        return true;
    }

    return false;
}

/*!
 * Scrolls right using current scroll step. If scroll is too far,
 * clips scrolling to the map's right border.
 */
void BriefMinimapRenderer::scrollRight() {
    mm_tx_ += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Scrolls left using current scroll step. If scroll is too far,
 * clips scrolling to the map's left border.
 */
void BriefMinimapRenderer::scrollLeft() {
    // if scroll_step is bigger than mm_tx_
    // then mm_tx_ -= scroll_step_ would be negative
    // but mm_tx_ is usigned so it would be an error
    if (mm_tx_ < scroll_step_) {
        mm_tx_ = 0;
    } else {
        // we know that mm_tx_ >= scroll_step_
        mm_tx_ -= scroll_step_;
    }
}

/*!
 * Scrolls up using current scroll step. If scroll is too far,
 * clips scrolling to the map's top border.
 */
void BriefMinimapRenderer::scrollUp() {
    if (mm_ty_ < scroll_step_) {
        mm_ty_ = 0;
    } else {
        // we know that mm_ty_ >= scroll_step_
        mm_ty_ -= scroll_step_;
    }
}

/*!
 * Scrolls down using current scroll step. If scroll is too far,
 * clips scrolling to the map's bottom border.
 */
void BriefMinimapRenderer::scrollDown() {
    mm_ty_ += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Renders the minimap at the given position on the screen.
 * \param mm_x X coord in absolute pixels.
 * \param mm_y Y coord in absolute pixels.
 */
void BriefMinimapRenderer::render(uint16 mm_x, uint16 mm_y) {
    for (uint16 tx = mm_tx_; tx < (mm_tx_ + mm_maxtile_); tx++) {
        uint16 xc = mm_x + (tx - mm_tx_) * pixpertile_;
        for (uint16 ty = mm_ty_; ty < (mm_ty_ + mm_maxtile_); ty++) {
            unsigned char c = p_mission_->getMinimapOverlay(tx, ty);
            switch (c) {
                case 0:
                    c = p_mission_->getMiniMap()->getColourAt(tx, ty);
                    break;
                case 1:
                    c = minimap_blink_ ? 14 : 12;
                    break;
                case 2:
                    if (b_draw_enemies_)
                        c = minimap_blink_ ? 14 : 5;
                    else
                        c = p_mission_->getMiniMap()->getColourAt(tx, ty);
            }
            g_Screen.drawRect(xc, mm_y + (ty - mm_ty_) * pixpertile_, pixpertile_,
                pixpertile_, c);
        }
    }
}

/*!
 * Default constructor.
 */
GamePlayMinimapRenderer::GamePlayMinimapRenderer() : 
    mm_timer_weap(300, false), mm_timer_ped(260, false) {
    p_mission_ = NULL;
}

/*!
 * Sets a new mission for rendering the minimap.
 * \param pMission A mission.
 * \param b_scannerEnabled True if scanner is enabled -> changes zoom level.
 */
void GamePlayMinimapRenderer::init(Mission *pMission, bool b_scannerEnabled) {
    p_mission_ = pMission;
    setScannerEnabled(b_scannerEnabled);
    mm_tx_ = 0;
    mm_ty_ = 0;
    offset_x_ = 0;
    offset_y_ = 0;
    cross_x_ = 64;
    cross_y_ = 64;
    mm_timer_weap.reset();
}

void GamePlayMinimapRenderer::updateRenderingInfos() {
    // mm_maxtile_ can be 17 or 33
    mm_maxtile_ = 128 / pixpertile_ + 1;
}

/*!
 * Setting the scanner on or off will play on the zooming level.
 * \param b_enabled True will set a zoom of X1, else X3.
 */
void GamePlayMinimapRenderer::setScannerEnabled(bool b_enabled) {
    setZoom(b_enabled ? ZOOM_X1 : ZOOM_X3);
}

/*!
 * Centers the minimap on the given tile. Usually, the minimap is centered
 * on the selected agent. If the agent is too close from the border, the minimap
 * does not move anymore.
 * \param tileX The X coord of the tile.
 * \param tileX The Y coord of the tile.
 * \param offX The offset of the agent on the tile.
 * \param offY The offset of the agent on the tile.
 */
void GamePlayMinimapRenderer::centerOn(uint16 tileX, uint16 tileY, int offX, int offY) {
    uint16 halfSize = mm_maxtile_ / 2;

    if (tileX < halfSize) {
        // we're too close of the top border -> stop moving along X axis
        mm_tx_ = 0;
        offset_x_ = 0;
    } else if ((tileX + halfSize) >= p_mission_->mmax_x_) {
        // we're too close of the bottom border -> stop moving along X axis
        mm_tx_ = p_mission_->mmax_x_ - mm_maxtile_;
        offset_x_ = 0;
    } else {
        mm_tx_ = tileX - halfSize;
        offset_x_ = offX / (256 / pixpertile_);
    }

    if (tileY < halfSize) {
        mm_ty_ = 0;
        offset_y_ = 0;
    } else if ((tileY + halfSize) >= p_mission_->mmax_y_) {
        mm_ty_ = p_mission_->mmax_y_ - mm_maxtile_;
        offset_y_ = 0;
    } else {
        mm_ty_ = tileY - halfSize;
        offset_y_ = offY / (256 / pixpertile_);
    }

    // get the cross coordinate
    // 
    // TODO : see if we can remove + 1
    cross_x_ = mapToMiniMapX(tileX + 1, offX);
    cross_y_ = mapToMiniMapY(tileY + 1, offY);
}

bool GamePlayMinimapRenderer::handleTick(int elapsed) {
    mm_timer_ped.update(elapsed);
    mm_timer_weap.update(elapsed);

    return true;
}

/*!
 * Renders the minimap at the given position on the screen.
 * \param mm_x X coord in absolute pixels.
 * \param mm_y Y coord in absolute pixels.
 */
void GamePlayMinimapRenderer::render(uint16 mm_x, uint16 mm_y) {
    // A temporary buffer composed of mm_maxtile + 1 columns and rows.
    // we use a slightly larger rendering buffer not to have
    // to check borders. At the end we only display  the mm_maxtile x mm_maxtile tiles.
    // On top of this we use one size for both resolutions as 18*18*8*8 > 34*34*4*4
    uint8 minimap_layer[18*18*8*8];
    uint8 mm_layer_size = mm_maxtile_ + 1;
    // The final minimap that will be displayed : the minimap is 128*128 pixels
    uint8 minimap_final_layer[kMiniMapSizePx*kMiniMapSizePx];

    // In this loop, we fill the buffer with floor colour. the first row and column
    // is not filled
    memset(minimap_layer, 0, 18*18*8*8);
    for (int j = 0; j < mm_maxtile_; j++) {
        for (int i = 0; i < mm_maxtile_; i++) {
            uint8 gcolour = p_mission_->getMiniMap()->getColourAt(mm_tx_ + i, mm_ty_ + j);
            for (char inc = 0; inc < pixpertile_; inc ++) {
                memset(minimap_layer + (j + 1) * pixpertile_ * pixpertile_ * mm_layer_size + 
                    (i + 1) * pixpertile_ + inc * pixpertile_ * mm_layer_size,
                    gcolour, pixpertile_);
            }
        }
    }

    // Draw the minimap cross
    drawFillRect(minimap_layer, cross_x_, 0, 1, (mm_maxtile_+1) * 8, fs_cmn::kColorBlack);
    drawFillRect(minimap_layer, 0, cross_y_, (mm_maxtile_+1) * 8, 1, fs_cmn::kColorBlack);
    
    // draw all visible elements on the minimap
    drawPedestrians(minimap_layer);
    drawWeapons(minimap_layer);
    drawCars(minimap_layer);
    

    // Copy the temp buffer in the final minimap using the tile offset so the minimap movement
    // is smoother
    for (int j = 0; j < kMiniMapSizePx; j++) {
        memcpy(minimap_final_layer + (kMiniMapSizePx * j), 
            minimap_layer + (pixpertile_ * pixpertile_ * mm_layer_size) +
            (j + offset_y_) * pixpertile_ * mm_layer_size + pixpertile_ + offset_x_, kMiniMapSizePx);
    }

    // Draw the minimap on the screen
    g_Screen.blit(mm_x, mm_y, kMiniMapSizePx, kMiniMapSizePx, minimap_final_layer);
}

void GamePlayMinimapRenderer::drawCars(uint8 *a_minimap) {
    for (int i = 0; i < p_mission_->numVehicles(); i++) {
        VehicleInstance *p_vehicle = p_mission_->vehicle(i);
        int tx = p_vehicle->tileX();
        int ty = p_vehicle->tileY();

        if (isVisible(tx, ty)) {
            // vehicle is on minimap and is not driven by one of our agent.
            // if a car is driven by our agent we only draw the yellow
            // circle for the driver
            PedInstance *p_ped = p_vehicle->getDriver();
            if (p_ped == NULL || !p_ped->isOurAgent()) {
                size_t vehicle_size = (zoom_ == ZOOM_X1) ? 2 : 4;
                int px = mapToMiniMapX(tx + 1, p_vehicle->offX()) - vehicle_size / 2;
                int py = mapToMiniMapY(ty + 1, p_vehicle->offY()) - vehicle_size / 2;

                drawFillRect(a_minimap, px, py, vehicle_size, vehicle_size, fs_cmn::kColorWhite);
            } else {
                int px = mapToMiniMapX(tx + 1, p_vehicle->offX());
                int py = mapToMiniMapY(ty + 1, p_vehicle->offY());
                uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorBlack : fs_cmn::kColorLightGreen;
                drawPedCircle(a_minimap, px, py, fs_cmn::kColorYellow, borderColor);
            }
        }
    }
}

void GamePlayMinimapRenderer::drawWeapons(uint8 * a_minimap) {
    const size_t weapon_size = 2;
    for (int i = 0; i < p_mission_->numWeapons(); i++)
	{
		WeaponInstance *p = p_mission_->weapon(i);
		int tx = p->tileX();
		int ty = p->tileY();
		int ox = p->offX();
		int oy = p->offY();

        // we draw weapons that have no owner ie that are on the ground
		if (!p->hasOwner() && isVisible(tx, ty)) {
            if (mm_timer_weap.state()) {
				int px = mapToMiniMapX(tx + 1, ox) - 1;
                int py = mapToMiniMapY(ty + 1, oy) - 1;

                drawFillRect(a_minimap, px, py, weapon_size, weapon_size, fs_cmn::kColorLightGrey);
            }
        }
	}
}

void GamePlayMinimapRenderer::drawPedestrians(uint8 * a_minimap) {
    for (int i = 0; i < p_mission_->numPeds(); i++)
	{
		PedInstance *p_ped = p_mission_->ped(i);
		int tx = p_ped->tileX();
		int ty = p_ped->tileY();
		int ox = p_ped->offX();
		int oy = p_ped->offY();

		if (p_ped->health() > 0 && isVisible(tx, ty))
		{
            int px = mapToMiniMapX(tx + 1, ox);
            int py = mapToMiniMapY(ty + 1, oy);
			if (p_ped->isPersuaded())
			{
				// col_Yellow circle with a black or lightgreen border (blinking)
                uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorLightGreen : fs_cmn::kColorBlack;
                drawPedCircle(a_minimap, px, py, fs_cmn::kColorYellow, borderColor);
			} else {
                switch (p_ped->getMainType())
				{
				case PedInstance::m_tpPedestrian:
				case PedInstance::m_tpCriminal:
                    {
					    // white rect 2x2 (opaque and transparent blinking)
                        size_t ped_width = 2;
                        size_t ped_height = 2;
                        if (mm_timer_ped.state()) {
					        px -= 1;
                            py -= 1;
                            
                            // draw the square
                            drawFillRect(a_minimap, px, py, ped_width, ped_height, fs_cmn::kColorWhite);
                        }
					break;
                    }
                case PedInstance::m_tpAgent:
                {
                    if (p_ped->inVehicle() == NULL)
                    {
                        if (p_ped->isOurAgent())
                        {
                            // col_Yellow circle with a black or lightgreen border (blinking)
                            uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorBlack : fs_cmn::kColorLightGreen;
                            drawPedCircle(a_minimap, px, py, fs_cmn::kColorYellow, borderColor);
                        } else {
                            // col_LightRed circle with a black or dark red border (blinking)
                            uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorBlack : fs_cmn::kColorDarkRed;
                            drawPedCircle(a_minimap, px, py, fs_cmn::kColorLightRed, borderColor);
                        }
                    }
                }
                break;
				case PedInstance::m_tpPolice:
                    {
					// blue circle with a black or col_BlueGrey (blinking)
                    uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorBlack : fs_cmn::kColorBlueGrey;
                    drawPedCircle(a_minimap, px, py, fs_cmn::kColorBlue, borderColor);
                    }
					break;
				case PedInstance::m_tpGuard:
                    {
					// col_LightGrey circle with a black or white border (blinking) 
                    uint8 borderColor = (mm_timer_ped.state()) ? fs_cmn::kColorWhite : fs_cmn::kColorBlack;
                    drawPedCircle(a_minimap, px, py, fs_cmn::kColorLightGrey, borderColor);
                    }
					break;
                }
            }
        }
    }
}

/*!
 * This array is used as a mask to draw circle for peds.
 * Values of the mask are :
 * - 0 : use the color of the buffer
 * - 1 : use border color if buffer color is not the fillColor
 * - 2 : use the fillColor
 */
uint8 g_ped_circle_mask_[] = {
    0,  0,  1,  1,  1,  0,  0,
    0,  1,  2,  2,  2,  1,  0,
    1,  2,  2,  2,  2,  2,  1,
    1,  2,  2,  2,  2,  2,  1,
    1,  2,  2,  2,  2,  2,  1,
    0,  1,  2,  2,  2,  1,  0,
    0,  0,  1,  1,  1,  0,  0
};

/*!
    * Draw a circle with the given colors for fill and border. This is used to represent agents, police and guards.
    * \param a_buffer destination buffer
    * \param mm_x X coord in the destination buffer
    * \param mm_y Y coord in the destination buffer
    * \param fillColor the color to fill the rect
    * \param borderColor the color to draw the circle border
    */
void GamePlayMinimapRenderer::drawPedCircle(uint8 * a_buffer, int mm_x, int mm_y, uint8 fillColor, uint8 borderColor) {
    // Size of the mask : it's a square of 4x4 pixels.
    const uint8 kCircleMaskSize = 7;
    // centers the circle on the ped position and add pixels to skip the first row and column
    mm_x -= 3;
    mm_y -= 3;
    
    for (uint8 j = 0; j < kCircleMaskSize; j++) {
        for (uint8 i = 0; i < kCircleMaskSize; i++) {
            // get the color at the current point
            int i_index = (mm_y + j) * pixpertile_ * (mm_maxtile_ + 1) + mm_x + i;
            uint8 bufferColor = a_buffer[i_index];
            switch(g_ped_circle_mask_[j*kCircleMaskSize + i]) {
            case 2:
                a_buffer[i_index] = fillColor;
                break;
            case 1:
                if (bufferColor != fillColor) {
                    a_buffer[i_index] = borderColor;
                }
                break;
            default:
                break;
            }
        }
    }
}
