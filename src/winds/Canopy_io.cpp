/****************************************************************************
 * Copyright (c) 2025 University of Utah
 * Copyright (c) 2025 University of Minnesota Duluth
 *
 * Copyright (c) 2025 Behnam Bozorgmehr
 * Copyright (c) 2025 Jeremy A. Gibbs
 * Copyright (c) 2025 Fabien Margairaz
 * Copyright (c) 2025 Eric R. Pardyjak
 * Copyright (c) 2025 Zachary Patterson
 * Copyright (c) 2025 Rob Stoll
 * Copyright (c) 2025 Lucas Ulmer
 * Copyright (c) 2025 Pete Willemsen
 *
 * This file is part of QES-Winds
 *
 * GPL-3.0 License
 *
 * QES-Winds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * QES-Winds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QES-Winds. If not, see <https://www.gnu.org/licenses/>.
 ****************************************************************************/


#include "util/QESLog.h"
#include "Canopy.h"
#include "WINDSGeneralData.h"
#include "util/ESRIShapefile.h"

void Canopy::initializeShapefileCanopies(const WINDSInputData *WID, WINDSGeneralData *WGD)
{

    qes::log::info() << "Creating canopies from shapefile..." << std::flush;

    std::vector<Building *> poly_buildings;
    // FM CLEANUP - NOT USED
    // float corner_height, min_height;
    std::vector<float> shpDomainSize(2), minExtent(2);
    WID->vegetationParams->SHPData->getLocalDomain(shpDomainSize);
    WID->vegetationParams->SHPData->getMinExtent(minExtent);

    // qes::log::writef(qes::log::Level::info, "\tShapefile Origin = (%.6f,%.6f)\n", minExtent[0], minExtent[1]);

    // If the shapefile is not covering the whole domain or the UTM coordinates
    // of the QES domain is different than shapefile origin
    if (WID->simParams->UTMx != 0.0 && WID->simParams->UTMy != 0.0) {
      minExtent[0] -= (minExtent[0] - WID->simParams->UTMx);
      minExtent[1] -= (minExtent[1] - WID->simParams->UTMy);
    }

    for (auto pIdx = 0u; pIdx < WID->vegetationParams->SHPData->m_polygons.size(); pIdx++) {

      // convert the global polys to local domain coordinates
      for (auto lIdx = 0u; lIdx < WID->vegetationParams->SHPData->m_polygons[pIdx].size(); lIdx++) {
        WID->vegetationParams->SHPData->m_polygons[pIdx][lIdx].x_poly -= minExtent[0];
        WID->vegetationParams->SHPData->m_polygons[pIdx][lIdx].y_poly -= minExtent[1];
      }

      // Setting base height for tree if there is a DEM file (TODO)
      if (WID->simParams->DTE_heightField && WID->simParams->DTE_mesh) {
        qes::log::error() << "Isolated tree from shapefile and DEM not implemented...\n";
      } else {
        // base_height.push_back(0.0);
      }

      for (auto lIdx = 0u; lIdx < WID->vegetationParams->SHPData->m_polygons[pIdx].size(); lIdx++) {
        WID->vegetationParams->SHPData->m_polygons[pIdx][lIdx].x_poly += WID->simParams->halo_x;
        WID->vegetationParams->SHPData->m_polygons[pIdx][lIdx].y_poly += WID->simParams->halo_y;
      }

      // Loop to create each of the polygon buildings read in from the shapefile
      int cId = allCanopiesV.size();
      // allCanopiesV.push_back(new CanopyIsolatedTree(WID, WGD, pIdx));
      allCanopiesV.push_back(new CanopyIsolatedTree(WID->vegetationParams->SHPData->m_polygons[pIdx],
                                                    WID->vegetationParams->SHPData->m_features["H"][pIdx],
                                                    WID->vegetationParams->SHPData->m_features["D"][pIdx],
                                                    0.0,
                                                    WID->vegetationParams->SHPData->m_features["LAI"][pIdx],
                                                    cId));
      canopy_id.push_back(cId);
      allCanopiesV[cId]->setPolyBuilding(WGD);
      allCanopiesV[cId]->setCellFlags(WID, WGD, cId);
      effective_height.push_back(allCanopiesV[cId]->height_eff);
    }
    qes::log::info() << "[done]" << std::endl;
  }
