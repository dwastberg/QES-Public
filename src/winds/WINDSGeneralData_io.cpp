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
#include "WINDSGeneralData.h"
#include "DTEHeightField.h"
#include "WRFInput.h"
#include "WindProfilerWRF.h"
#include "WindProfilerHRRR.h"
#include "HRRRData.h"
#include "util/ESRIShapefile.h"
#include "util/NetCDFInput.h"

void WINDSGeneralData::initializeWRFSensors(const WINDSInputData *WID)
{

    WRFInput *wrf_ptr = WID->simParams->wrfInputData;

    // Use WID->simParams->m_domIType == WRFOnly -- to indicate wrf
    // interp data usage
    if (WID->simParams->m_domIType == SimulationParameters::DomainInputType::WRFOnly) {

      // WRFOnly is the PREEVENTS Fire usage cases...

      wrf_nx = wrf_ptr->fm_nx;
      wrf_ny = wrf_ptr->fm_ny;

      windProfiler = std::make_unique<WindProfilerWRF>();

      // FM -> this should be added on wrf-input (read time form wrf)
      // initialize our time info...
      QEStime tmp("2022-01-01T00:00");
      sensortime.push_back(tmp);// that's roughly 01/19/22 at 1:00pm Central
      sensortime_id.push_back(0);
      timestamp.push_back(sensortime[0]);

      totalTimeIncrements = WID->simParams->totalTimeIncrements;


      // FM -> CODE REMOVED TO WITH WINDPROFILER CLASSES // TO CLEAN
#if 0
      WID->metParams->sensors.resize(wrf_ptr->fm_nx * wrf_ptr->fm_ny);

      for (auto i = 0; i < wrf_ptr->fm_nx * wrf_ptr->fm_ny; i++) {
        // create a new sensor element
        if (!WID->metParams->sensors[i]) {
          WID->metParams->sensors[i] = new Sensor();
        }

        // create one time series for each sensor, for now
        WID->metParams->sensors[i]->TS.resize(1);
        if (!WID->metParams->sensors[i]->TS[0]) {
          WID->metParams->sensors[i]->TS[0] = new TimeSeries();
        }

        // WRF profile data -- sensor blayer flag is 4
        WID->metParams->sensors[i]->TS[0]->site_blayer_flag = 4;
      }

      // Also need to correctly add time stamps
      // example from below:
      //   for (size_t t = 0; t < sensortime_id.size(); t++) {
      //      timestamp.push_back(bt::from_time_t(sensortime[t]));
      //   }

      // initialize our time info...
      //QEStime tmp("2022-01-01T00:00");
      //sensortime.push_back(tmp);// that's roughly 01/19/22 at 1:00pm Central
      //sensortime_id.push_back(0);
      //timestamp.push_back(sensortime[0]);

      //totalTimeIncrements = WID->simParams->totalTimeIncrements;

      // Here to take care of the first time this is built
      for (auto i = 0; i < wrf_ptr->fm_nx; i++) {
        for (auto j = 0; j < wrf_ptr->fm_ny; j++) {
          int index = i + j * wrf_ptr->fm_nx;
          WID->metParams->sensors[index]->site_coord_flag = 1;
          WID->metParams->sensors[index]->site_xcoord = (i + halo_index_x + 0.5) * dx;
          WID->metParams->sensors[index]->site_ycoord = (j + halo_index_y + 0.5) * dy;

          WID->metParams->sensors[index]->TS[0]->site_wind_dir.resize(wrf_ptr->ht_fmw.size());
          WID->metParams->sensors[index]->TS[0]->site_z_ref.resize(wrf_ptr->ht_fmw.size());
          WID->metParams->sensors[index]->TS[0]->site_U_ref.resize(wrf_ptr->ht_fmw.size());

          WID->metParams->sensors[index]->TS[0]->site_z0 = 0.1;// should get per cell from WRF data...  we do load per atm cell...
          WID->metParams->sensors[index]->TS[0]->site_one_overL = 0.0;

          // for each height in the WRF profile
          for (auto p = 0; p < wrf_ptr->ht_fmw.size(); p++) {
            int id = index + p * wrf_ptr->fm_nx * wrf_ptr->fm_ny;
            WID->metParams->sensors[index]->TS[0]->site_z_ref[p] = wrf_ptr->ht_fmw[p];
            WID->metParams->sensors[index]->TS[0]->site_U_ref[p] = sqrt((wrf_ptr->u0_fmw[id] * wrf_ptr->u0_fmw[id]) + (wrf_ptr->v0_fmw[id] * wrf_ptr->v0_fmw[id]));
            WID->metParams->sensors[index]->TS[0]->site_wind_dir[p] = 180 + (180 / pi) * atan2(wrf_ptr->v0_fmw[id], wrf_ptr->u0_fmw[id]);
          }
        }
      }
#endif
      // FM -> CODE REMOVED TO WITH WINDPROFILER CLASSES // TO CLEAN


      // u0 and v0 are wrf_ptr->fm_nx * wrf_ptr->fm_ny *
      // wrf_ptr->ht_fmw.size()
      //
      // The heights themselves are in the wrf_ptr->ht_fmw array

    }

    else if (wrf_ptr->statData.size() > 0) {
      qes::log::info() << "Size of WRF station/sensor profile data: " << wrf_ptr->statData.size() << std::endl;
      WID->metParams->sensors.resize(wrf_ptr->statData.size());

      for (auto i = 0u; i < wrf_ptr->statData.size(); i++) {
        qes::log::info() << "Station " << i << " ("
                  << wrf_ptr->statData[i].xCoord << ", "
                  << wrf_ptr->statData[i].yCoord << ")" << std::endl;

        if (!WID->metParams->sensors[i])
          WID->metParams->sensors[i] = new Sensor();

        WID->metParams->sensors[i]->site_xcoord = wrf_ptr->statData[i].xCoord;
        WID->metParams->sensors[i]->site_ycoord = wrf_ptr->statData[i].yCoord;

        // Need to allocate time series... not fully pulling all WRF
        // time series yet... just first
        WID->metParams->sensors[i]->TS.resize(1);
        // Also need to allocate the space...
        if (!WID->metParams->sensors[i]->TS[0])
          WID->metParams->sensors[i]->TS[0] = new TimeSeries;

        // WRF profile data -- sensor blayer flag is 4
        WID->metParams->sensors[i]->TS[0]->site_blayer_flag = 4;

        // Make sure to set size_z0 to be z0 from WRF cell
        WID->metParams->sensors[i]->TS[0]->site_z0 = wrf_ptr->statData[i].z0;

        //
        // 1 time series for now - how do we deal with this for
        // new time steps???  Need to figure out ASAP.
        //
        // Also need to correctly add time stamps
        // example from below:
        //   for (size_t t = 0; t < sensortime_id.size(); t++) {
        //      timestamp.push_back(bt::from_time_t(sensortime[t]));
        //   }

        for (int t = 0; t < 1; t++) {
          qes::log::info() << "\tTime Series: " << t << std::endl;

          time_t tmp = t;
          QEStime tmp2(tmp);
          sensortime.push_back(tmp2);
          sensortime_id.push_back(t);
          timestamp.push_back(sensortime[t]);

          int profDataSz = wrf_ptr->statData[i].profiles[t].size();
          WID->metParams->sensors[i]->TS[0]->site_wind_dir.resize(profDataSz);
          WID->metParams->sensors[i]->TS[0]->site_z_ref.resize(profDataSz);
          WID->metParams->sensors[i]->TS[0]->site_U_ref.resize(profDataSz);


          for (size_t p = 0; p < wrf_ptr->statData[i].profiles[t].size(); p++) {
            qes::log::info() << "\t" << wrf_ptr->statData[i].profiles[t][p].zCoord
                      << ", " << wrf_ptr->statData[i].profiles[t][p].ws
                      << ", " << wrf_ptr->statData[i].profiles[t][p].wd << std::endl;

            WID->metParams->sensors[i]->TS[0]->site_z_ref[p] = wrf_ptr->statData[i].profiles[t][p].zCoord;
            WID->metParams->sensors[i]->TS[0]->site_U_ref[p] = wrf_ptr->statData[i].profiles[t][p].ws;
            WID->metParams->sensors[i]->TS[0]->site_wind_dir[p] = wrf_ptr->statData[i].profiles[t][p].wd;
          }
        }
      }
    }
  }

void WINDSGeneralData::initializeTerrain(const WINDSInputData *WID)
{

    mesh = WID->simParams->DTE_mesh;

    int ii, jj, idx;
    // ////////////////////////////////
    // Retrieve terrain height field //
    // ////////////////////////////////
    for (int i = 0; i < domain.nx() - 2 * halo_index_x - 1; i++) {
      for (int j = 0; j < domain.ny() - 2 * halo_index_y - 1; j++) {
        // Gets height of the terrain for each cell
        ii = i + halo_index_x;
        jj = j + halo_index_y;
        idx = ii + jj * (domain.nx() - 1);
        terrain[idx] = WID->simParams->DTE_mesh->getHeight(i * domain.dx() + domain.dx() * 0.5f, j * domain.dy() + domain.dy() * 0.5f);
        if (terrain[idx] < 0.0) {
          terrain[idx] = 0.0;
        }
        id = ii + jj * domain.nx();
        for (size_t k = 0; k < domain.z.size() - 1; k++) {
          terrain_face_id[id] = k;
          if (terrain[idx] <= domain.z_face[k + 1]) {
            break;
          }
        }
      }
    }

    for (int i = halo_index_x; i < domain.nx() - halo_index_x - 1; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = i + halo_index_y * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny(); j++) {
        id = i + (domain.ny() - halo_index_y - 1) * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }
    }

    for (int j = halo_index_y; j < domain.ny() - halo_index_y - 1; j++) {
      for (int i = 0; i < halo_index_x; i++) {
        id = halo_index_x + j * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }

      for (int i = domain.nx() - halo_index_x - 1; i < domain.nx(); i++) {
        id = (domain.nx() - halo_index_x - 1) + j * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }
    }

    for (int i = 0; i < halo_index_x; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = halo_index_x + halo_index_y * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny(); j++) {
        id = halo_index_x + (domain.ny() - halo_index_y - 1) * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }
    }

    for (int i = domain.nx() - halo_index_x - 1; i < domain.nx() - 1; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = (domain.nx() - halo_index_x - 1) + halo_index_y * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_face_id[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny() - 1; j++) {
        id = (domain.nx() - halo_index_x - 1) + (domain.ny() - halo_index_y - 2) * domain.nx();
        long icell_face = i + j * domain.nx();
        terrain_face_id[icell_face] = terrain_id[id];
      }
    }

    for (int i = 0; i < domain.nx() - 2 * halo_index_x - 1; i++) {
      for (int j = 0; j < domain.ny() - 2 * halo_index_y - 1; j++) {
        // Gets height of the terrain for each cell
        ii = i + halo_index_x;
        jj = j + halo_index_y;
        idx = ii + jj * (domain.nx() - 1);
        for (size_t k = 0; k < domain.z.size() - 1; k++) {
          terrain_id[idx] = k;
          if (terrain[idx] < domain.z[k]) {
            break;
          }
        }
      }
    }

    for (int i = halo_index_x; i < domain.nx() - halo_index_x - 1; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = i + halo_index_y * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny() - 1; j++) {
        id = i + (domain.ny() - halo_index_y - 2) * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }
    }

    for (int j = halo_index_y; j < domain.ny() - halo_index_y - 1; j++) {
      for (int i = 0; i < halo_index_x; i++) {
        id = halo_index_x + j * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }

      for (int i = domain.nx() - halo_index_x - 1; i < domain.nx() - 1; i++) {
        id = (domain.nx() - halo_index_x - 2) + j * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }
    }

    for (int i = 0; i < halo_index_x; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = halo_index_x + halo_index_y * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny() - 1; j++) {
        id = halo_index_x + (domain.ny() - halo_index_y - 2) * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }
    }

    for (int i = domain.nx() - halo_index_x - 1; i < domain.nx() - 1; i++) {
      for (int j = 0; j < halo_index_y; j++) {
        id = (domain.nx() - halo_index_x - 2) + halo_index_y * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }

      for (int j = domain.ny() - halo_index_y - 1; j < domain.ny() - 1; j++) {
        id = (domain.nx() - halo_index_x - 2) + (domain.ny() - halo_index_y - 2) * (domain.nx() - 1);
        long icell_cent = i + j * (domain.nx() - 1);
        terrain_id[icell_cent] = terrain_id[id];
        terrain[icell_cent] = terrain[id];
      }
    }

    for (int i = 0; i < domain.nx() - 1; i++) {
      for (int j = 0; j < domain.ny() - 1; j++) {
        // Gets height of the terrain for each cell
        int idx = i + j * (domain.nx() - 1);
        for (size_t k = 0; k < domain.z.size() - 1; k++) {
          if (terrain[idx] < domain.z[k + 1]) {
            break;
          }
          // icell_cent = i + j * (nx - 1) + (k + 1) * (nx - 1) * (ny - 1);
          long icell_cent = domain.cell(i, j, k + 1);
          center_id[icell_cent] = 0;// Marks the cell center as inside solid
        }
      }
    }

    if (WID->simParams->meshTypeFlag == 0 && WID->simParams->readCoefficientsFlag == 0) {
      ///////////////////////////////////
      // Stair-step (original QUIC)    //
      ///////////////////////////////////

      auto start_stair = std::chrono::high_resolution_clock::now();
      qes::log::info() << "[QES-WINDS]\t Stair-step method for terrain..." << std::endl;

      for (int i = 0; i < domain.nx() - 1; i++) {
        for (int j = 0; j < domain.ny() - 1; j++) {
          // Gets height of the terrain for each cell
          int idx = i + j * (domain.nx() - 1);
          for (size_t k = 0; k < domain.z.size() - 1; k++) {
            if (terrain[idx] < domain.z[k + 1]) {
              break;
            }

            // icell_cent = i + j * (nx - 1) + (k + 1) * (nx - 1) * (ny - 1);
            long icell_cent = domain.cell(i, j, k + 1);
            icellflag[icell_cent] = 2;
          }
        }
      }

      auto finish_stair = std::chrono::high_resolution_clock::now();// Finish recording execution time

      std::chrono::duration<float> elapsed_stair = finish_stair - start_stair;
      qes::log::info() << "\t\t elapsed time: " << elapsed_stair.count() << " s\n";
    }

    if (WID->simParams->meshTypeFlag == 1 && WID->simParams->readCoefficientsFlag == 0) {
      //////////////////////////////////
      //        Cut-cell method       //
      //////////////////////////////////

      auto start_cut = std::chrono::high_resolution_clock::now();
      qes::log::info() << "[QES-WINDS]\t Cut-cell method for terrain..." << std::endl;

      // Calling calculateCoefficient function to calculate area fraction coefficients for cut-cells
      // WID->simParams->DTE_heightField->setCells(cells, this, WID);
      WID->simParams->DTE_heightField->setCells(this, WID);

      auto finish_cut = std::chrono::high_resolution_clock::now();// Finish recording execution time

      std::chrono::duration<float> elapsed_cut = finish_cut - start_cut;
      qes::log::info() << "\t\t elapsed time: " << elapsed_cut.count() << " s\n";
    }
  }

void WINDSGeneralData::initializeShapefileBuildings(const WINDSInputData *WID)
{
      qes::log::info() << "[QES-WINDS]\t Creating buildings from shapefile..." << std::flush;


      float corner_height, min_height;

      std::vector<float> shpDomainSize(2), minExtent(2);
      WID->buildingsParams->SHPData->getLocalDomain(shpDomainSize);
      WID->buildingsParams->SHPData->getMinExtent(minExtent);

      // qes::log::writef(qes::log::Level::info, "\tShapefile Origin = (%.6f,%.6f)\n", minExtent[0], minExtent[1]);

      // If the shapefile is not covering the whole domain or the UTM coordinates
      // of the QES domain is different than shapefile origin
      if (WID->simParams->UTMx != 0.0 && WID->simParams->UTMy != 0.0) {
        minExtent[0] -= (minExtent[0] - WID->simParams->UTMx);
        minExtent[1] -= (minExtent[1] - WID->simParams->UTMy);
      }

      for (size_t pIdx = 0u; pIdx < WID->buildingsParams->SHPData->m_polygons.size(); pIdx++) {

        // convert the global polys to local domain coordinates
        for (auto lIdx = 0u; lIdx < WID->buildingsParams->SHPData->m_polygons[pIdx].size(); lIdx++) {
          WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].x_poly -= minExtent[0];
          WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].y_poly -= minExtent[1];
        }

        // Setting base height for buildings if there is a DEM file
        if (WID->simParams->DTE_heightField && WID->simParams->DTE_mesh) {
          // Get base height of every corner of building from terrain height
          min_height = WID->simParams->DTE_mesh->getHeight(WID->buildingsParams->SHPData->m_polygons[pIdx][0].x_poly,
                                                           WID->buildingsParams->SHPData->m_polygons[pIdx][0].y_poly);
          if (min_height < 0) {
            min_height = 0.0;
          }
          for (size_t lIdx = 1u; lIdx < WID->buildingsParams->SHPData->m_polygons[pIdx].size(); lIdx++) {
            corner_height = WID->simParams->DTE_mesh->getHeight(WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].x_poly,
                                                                WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].y_poly);

            if (corner_height < min_height && corner_height >= 0.0) {
              min_height = corner_height;
            }
          }
          base_height.push_back(min_height);
        } else {
          base_height.push_back(0.0);
        }

        for (size_t lIdx = 0u; lIdx < WID->buildingsParams->SHPData->m_polygons[pIdx].size(); lIdx++) {
          WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].x_poly += WID->simParams->halo_x;
          WID->buildingsParams->SHPData->m_polygons[pIdx][lIdx].y_poly += WID->simParams->halo_y;
        }

        // Loop to create each of the polygon buildings read in from the shapefile
        int bId = allBuildingsV.size();

        auto building = std::make_unique<PolyBuilding>(
          WID->buildingsParams->SHPData->m_polygons[pIdx],
          WID->buildingsParams->SHPData->m_features[WID->buildingsParams->shpHeightField][pIdx]
            * WID->buildingsParams->heightFactor,
          base_height[bId], bId);
        allBuildingsV.push_back(building.get());
        ownedBuildings_.push_back(std::move(building));
        building_id.push_back(bId);
        allBuildingsV[bId]->setPolyBuilding(this);
        allBuildingsV[bId]->setCellFlags(WID, this, bId);
        effective_height.push_back(allBuildingsV[bId]->height_eff);
      }
      qes::log::info() << "\r[QES-WINDS]\t Creating buildings from shapefile... [DONE]" << std::endl;
    }

void WINDSGeneralData::readCoefficients(const WINDSInputData *WID)
{

    NetCDFInput *NCDFInput = new NetCDFInput(WID->simParams->coeffFile);

    int ncnx, ncny, ncnz, ncnt;
    std::vector<size_t> start;
    std::vector<size_t> count;

    start = { 0, 0, 0, 0 };
    NCDFInput->getDimensionSize("x", ncnx);
    NCDFInput->getDimensionSize("y", ncny);
    NCDFInput->getDimensionSize("z", ncnz);
    NCDFInput->getDimensionSize("t", ncnt);

    count = { static_cast<unsigned long>(1),
              static_cast<unsigned long>(ncnz - 1),
              static_cast<unsigned long>(ncny - 1),
              static_cast<unsigned long>(ncnx - 1) };


    NCDFInput->getVariableData("icellflag", start, count, icellflag);

    resetICellFlag();

    // Read in solver coefficients
    NCDFInput->getVariableData("e", start, count, e);
    NCDFInput->getVariableData("f", start, count, f);
    NCDFInput->getVariableData("g", start, count, g);
    NCDFInput->getVariableData("h", start, count, h);
    NCDFInput->getVariableData("m", start, count, m);
    NCDFInput->getVariableData("n", start, count, n);
  }

WINDSGeneralData::WINDSGeneralData(const std::string &inputFile)
  : domain(inputFile)
{
  qes::log::info() << "-------------------------------------------------------------------" << std::endl;
  qes::log::info() << "[QES-WINDS]\t Initialization of wind model...\n";
  qes::log::info() << "[QES-WINDS]\t Loading QES-winds fields " << std::endl;

  // fullname passed to WINDSGeneralData
  input = new NetCDFInput(inputFile);

  // create wall instance for BC
  wall = std::make_unique<Wall>();

  // nx,ny - face centered value (consistant with QES-Winds)
  int nx, ny, nz;

  input->getDimensionSize("x_face", nx);
  input->getDimensionSize("y_face", ny);
  // nz - face centered value + bottom ghost (consistant with QES-Winds)
  input->getDimensionSize("z_face", nz);

  if ((nx != domain.nx()) || (ny != domain.ny()) || (nz != domain.nz())) {
    { std::ostringstream message; message << "[ERROR] \t data size incompatible " << std::endl; throw qes::InputError(message.str()); }
  }

  // nt - number of time instance in data
  input->getDimensionSize("t", nt);


  // This is what winds gd really needs to do...  let domain handle stuff above
  // Allocate memory
  allocateMemory();

  // get time variables
  std::vector<float> t;
  t.resize(nt);
  input->getVariableData("t", t);

  // check if times is in the NetCDF file
  NcVar NcVar_timestamp;
  input->getVariable("timestamp", NcVar_timestamp);

  if (NcVar_timestamp.isNull()) {
    QESout::warning("No timestamp in NetCDF file");
    QEStime tmp("2022-01-01T00:00");
    for (int k = 0; k < nt; k++) {
      // ptime test= from_iso_extended_string(WID->metParams->sensors[i]->TS[t]->timeStamp)
      timestamp.push_back(tmp + t[k]);
    }
  } else {
    qes::log::info() << "\t\t Loading " << nt << " time steps" << std::endl;
    for (int k = 0; k < nt; k++) {
      std::vector<size_t> start_time;
      std::vector<size_t> count_time;
      start_time = { static_cast<unsigned long>(k), 0 };
      count_time = { 1, 19 };

      char timestamp_tmp[19];
      NcVar_timestamp.getVar(start_time, count_time, &timestamp_tmp[0]);

      std::string tmp = "";
      for (int i = 0; i < 19; ++i) {
        tmp += timestamp_tmp[i];
      }

      QEStime time(tmp);
      qes::log::info() << "\t\t " << time << std::endl;

      timestamp.push_back(time);
    }
  }
  totalTimeIncrements = nt;

  // netCDF variables
  std::vector<size_t> start;
  std::vector<size_t> count_2d;

  start = { 0, 0 };
  count_2d = { static_cast<unsigned long>(domain.ny() - 1),
               static_cast<unsigned long>(domain.nx() - 1) };

  // terrain (cell-center)
  terrain.resize((domain.ny() - 1) * (domain.nx() - 1), 0.0);
  NcVar NcVar_terrain;
  input->getVariable("terrain", NcVar_terrain);
  if (!NcVar_terrain.isNull()) {// => terrain data in QES-Winds file
    input->getVariableData("terrain", start, count_2d, terrain);
  } else {// => no external terrain data provided
    qes::log::info() << "[WINDS Data] \t no terrain data found -> assumed flat" << std::endl;
  }
}

void WINDSGeneralData::loadNetCDFData(int stepin)
{

  qes::log::info() << "[QES-WINDS] \t Loading data at step " << stepin
            << " (" << timestamp[stepin] << ")" << std::endl;
#if 0
  std::vector<size_t> start_time;
  std::vector<size_t> count_time;
  start_time = { static_cast<unsigned long>(stepin), 0 };
  count_time = { 1, 19 };

  NcVar NcVar_timestamp;
  input->getVariable("timestamp", NcVar_timestamp);

  std::vector<char> timestamp_tmp;
  NcVar_timestamp.getVar(start_time, count_time, &timestamp_tmp[0]);
  std::string tmp;
  for (int i = 0; i < 19; ++i) {
    tmp[i] = timestamp_tmp[i];
  }
  QEStime time(tmp);
  qes::log::info() << "read at time " << time << std::endl;

  timestamp.push_back(time);
#endif
  // netCDF variables
  std::vector<size_t> start;
  std::vector<size_t> count_cc;
  std::vector<size_t> count_fc;

  start = { static_cast<unsigned long>(stepin), 0, 0, 0 };
  count_cc = { 1,
               static_cast<unsigned long>(domain.nz() - 1),
               static_cast<unsigned long>(domain.ny() - 1),
               static_cast<unsigned long>(domain.tnx - 1) };
  count_fc = { 1,
               static_cast<unsigned long>(domain.nz()),
               static_cast<unsigned long>(domain.ny()),
               static_cast<unsigned long>(domain.nx()) };

  // cell-center variables
  // icellflag (see .h for velues)
  input->getVariableData("icellflag", start, count_cc, icellflag);
  /// coefficients for SOR solver
  NcVar NcVar_SORcoeff;
  input->getVariable("e", NcVar_SORcoeff);

  if (!NcVar_SORcoeff.isNull()) {
    input->getVariableData("e", start, count_cc, e);
    input->getVariableData("f", start, count_cc, f);
    input->getVariableData("g", start, count_cc, g);
    input->getVariableData("h", start, count_cc, h);
    input->getVariableData("m", start, count_cc, m);
    input->getVariableData("n", start, count_cc, n);
  } else {
    qes::log::info() << "[WINDS Data] \t no SORcoeff data found -> assumed e,f,g,h,m,n=1" << std::endl;
  }

  // face-center variables
  input->getVariableData("u", start, count_fc, u);
  input->getVariableData("v", start, count_fc, v);
  input->getVariableData("w", start, count_fc, w);

  // clear wall indices container (guarantee entry vector)
  wall_right_indices.clear();
  wall_left_indices.clear();
  wall_above_indices.clear();
  wall_below_indices.clear();
  wall_front_indices.clear();
  wall_back_indices.clear();

  // define new wall indices container for new data
  wall->defineWalls(this);


  return;
}

void WINDSGeneralData::downscaleHRRR(const WINDSInputData *WID)
{
  qes::log::info() << "Processing HRRR data..." << std::flush;
  hrrrInputData = new HRRRData(WID->hrrrInput->HRRRFile);
  hrrrInputData->findHRRRSensors(WID, this);

  std::vector<int> site_i(hrrrInputData->hrrrSensorID.size(), 0);
  std::vector<int> site_j(hrrrInputData->hrrrSensorID.size(), 0);

  for (size_t i = 0; i < hrrrInputData->hrrrSensorID.size(); i++) {
    // Create new sensor object
    WID->metParams->sensors.push_back(new Sensor());
    WID->metParams->sensors[i]->site_coord_flag = 1;
    if (WID->simParams->UTMZone == hrrrInputData->hrrrSensorUTMzone[i]) {
      WID->metParams->sensors[i]->site_xcoord = hrrrInputData->hrrrSensorUTMx[i] - WID->simParams->UTMx;
    } else {
      int end_zone = 729400;
      int start_zone = 270570;
      int zone_diff = hrrrInputData->hrrrSensorUTMzone[i] - WID->simParams->UTMZone;
      WID->metParams->sensors[i]->site_xcoord = (hrrrInputData->hrrrSensorUTMx[i] - start_zone) + (zone_diff - 1) * (end_zone - start_zone) + (end_zone - WID->simParams->UTMx);
    }
    WID->metParams->sensors[i]->site_ycoord = hrrrInputData->hrrrSensorUTMy[i] - WID->simParams->UTMy;
    site_i[i] = WID->metParams->sensors[i]->site_xcoord / domain.dx();
    site_j[i] = WID->metParams->sensors[i]->site_ycoord / domain.dy();
  }

  float site_distance;
  float min_distance;
  if (WID->hrrrInput->interpolationScheme == 1) {// Nearest site interpolation scheme
    nearest_site_id.resize(domain.nx() * domain.ny(), 0);
    for (size_t j = 0; j < domain.ny(); j++) {
      for (size_t i = 0; i < domain.nx(); i++) {
        int id = i + j * domain.nx();// Index in horizontal surface
        min_distance = 100000.0;
        for (size_t ii = 0; ii < hrrrInputData->hrrrSensorID.size(); ii++) {
          if (site_i[ii] >= 0 && site_j[ii] >= 0 && site_i[ii] < domain.nx() - 1 && site_j[ii] < domain.ny() - 1) {
            site_distance = sqrt(pow(i * domain.dx() - WID->metParams->sensors[ii]->site_xcoord, 2.0) + pow(j * domain.dy() - WID->metParams->sensors[ii]->site_ycoord, 2.0));
            if (site_distance < min_distance) {
              min_distance = site_distance;
              nearest_site_id[id] = ii;
            }
          } else {
            continue;
          }
        }
      }
    }
  }

  if (WID->hrrrInput->interpolationScheme == 2) {// Bilinear interpolation scheme
    int site1_id_temp, site2_id_temp, site3_id_temp, site4_id_temp;
    int k, l, m, n;
    closest_site_ids.resize((domain.nx() - 1) * (domain.ny() - 1));

    for (size_t j = 1; j < domain.ny() - 1; j++) {
      for (size_t i = 1; i < domain.nx() - 1; i++) {
        int id = i + j * (domain.nx() - 1);// Index in horizontal surface
        for (size_t ii = 0; ii < hrrrInputData->hrrrSensorID.size(); ii++) {
          site2_id_temp = -1;
          site3_id_temp = -1;
          site4_id_temp = -1;
          site1_id_temp = ii;
          k = hrrrInputData->hrrrSensorID[ii] / hrrrInputData->xSize;
          l = hrrrInputData->hrrrSensorID[ii] - k * hrrrInputData->xSize;
          for (size_t jj = ii; jj < hrrrInputData->hrrrSensorID.size(); jj++) {
            m = hrrrInputData->hrrrSensorID[jj] / hrrrInputData->xSize;
            n = hrrrInputData->hrrrSensorID[jj] - m * hrrrInputData->xSize;
            if (m == k && n == l + 1) {
              site2_id_temp = jj;
              break;
            }
          }

          for (size_t jj = ii; jj < hrrrInputData->hrrrSensorID.size(); jj++) {
            m = hrrrInputData->hrrrSensorID[jj] / hrrrInputData->xSize;
            n = hrrrInputData->hrrrSensorID[jj] - m * hrrrInputData->xSize;
            if (m == k + 1 && n == l + 1) {
              site3_id_temp = jj;
              break;
            }
          }

          for (size_t jj = ii; jj < hrrrInputData->hrrrSensorID.size(); jj++) {
            m = hrrrInputData->hrrrSensorID[jj] / hrrrInputData->xSize;
            n = hrrrInputData->hrrrSensorID[jj] - m * hrrrInputData->xSize;
            if (m == k + 1 && n == l) {
              site4_id_temp = jj;
              break;
            }
          }

          if (site1_id_temp != -1 && site2_id_temp != -1 && site3_id_temp != -1 && site4_id_temp != -1) {
            if (j * domain.dy() >= WID->metParams->sensors[site1_id_temp]->site_ycoord && j * domain.dy() <= WID->metParams->sensors[site3_id_temp]->site_ycoord && i * domain.dx() >= WID->metParams->sensors[site4_id_temp]->site_xcoord && i * domain.dx() <= WID->metParams->sensors[site2_id_temp]->site_xcoord) {
              closest_site_ids[id].push_back(site1_id_temp);
              closest_site_ids[id].push_back(site2_id_temp);
              closest_site_ids[id].push_back(site3_id_temp);
              closest_site_ids[id].push_back(site4_id_temp);
              break;
            } else {
              continue;
            }
          } else {
            continue;
          }
        }
      }
    }
  }


  QEStime *hrrrTime;
  for (size_t t = 0; t < hrrrInputData->hrrrTime.size(); t++) {
    hrrrTime = new QEStime(hrrrInputData->hrrrTimeTrans[t]);
    QEStime tmp(hrrrTime->getTimestamp());
    sensortime.push_back(tmp);
    sensortime_id.push_back(t);
  }

  for (size_t t = 0; t < hrrrInputData->hrrrTime.size(); t++) {
    hrrrInputData->readSensorData(t);
    for (size_t i = 0; i < hrrrInputData->hrrrSensorID.size(); i++) {
      WID->metParams->sensors[i]->TS.push_back(new TimeSeries);
      WID->metParams->sensors[i]->TS[t]->time = sensortime[t];
      WID->metParams->sensors[i]->TS[t]->site_blayer_flag = 5;
      WID->metParams->sensors[i]->TS[t]->site_z_ref.push_back(10.0);
      WID->metParams->sensors[i]->TS[t]->site_U_ref.push_back(hrrrInputData->hrrrSpeed[i]);
      WID->metParams->sensors[i]->TS[t]->site_wind_dir.push_back(hrrrInputData->hrrrDir[i]);
      WID->metParams->sensors[i]->TS[t]->site_z0 = hrrrInputData->hrrrZ0[hrrrInputData->hrrrSensorID[i]];
      if (WID->hrrrInput->stabilityClasses == 0) {// No stability
        WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.0;
      } else if (WID->hrrrInput->stabilityClasses == 1) {// Pasquill-Gifford stability classes
        if (hrrrInputData->hrrrShortRadiation[hrrrInputData->hrrrSensorID[i]] > 0) {// If during day

          if (hrrrInputData->hrrrShortRadiation[hrrrInputData->hrrrSensorID[i]] > 700) {// If strong solar insolation
            if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 3.0) {// If wind is less than 2m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.4;// Class A stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 3.0 && WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 5.0) {// If wind is greater than 3m/s and less than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.22;// Class B stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 5.0) {// If wind is greater than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.074;// Class C stability
            }
          }


          if (hrrrInputData->hrrrShortRadiation[hrrrInputData->hrrrSensorID[i]] >= 350 && hrrrInputData->hrrrShortRadiation[hrrrInputData->hrrrSensorID[i]] <= 700) {// If moderate solar insolation
            if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 2.0) {// If wind is less than 2m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.4;// Class A stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 2.0 && WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 5.0) {// If wind is greater than 2m/s and less than 3m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.22;// Class B stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 5.0 && WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 6.0) {// If wind is greater than 5m/s and less than 6m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.074;// Class C stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 6.0) {// If wind is greater than 6m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.0;// Class D stability
            }
          }


          if (hrrrInputData->hrrrShortRadiation[hrrrInputData->hrrrSensorID[i]] < 350) {// If slight solar insolation
            if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 2.0) {// If wind is less than 2m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.22;// Class B stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 2.0 && WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 5.0) {// If wind is greater than 2m/s and less than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = -0.074;// Class C stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 5.0) {// If wind is greater than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.0;// Class D stability
            }
          }
        } else {// If during night
          if (hrrrInputData->hrrrCloudCover[hrrrInputData->hrrrSensorID[i]] > 50.0) {// High cloud cover

            if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 3.0) {// If wind is less than 3m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.018;// Class E stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 3.0) {// If wind is greater than 3m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.0;// Class D stability
            }
          } else {// Low cloud cover

            if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 3.0) {// If wind is less than 3m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.047;// Class F stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 3.0 && WID->metParams->sensors[i]->TS[t]->site_U_ref[0] < 5.0) {// If wind is greater than 3m/s and less than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.018;// Class E stability
            } else if (WID->metParams->sensors[i]->TS[t]->site_U_ref[0] >= 5.0) {// If wind is greater than 5m/s
              WID->metParams->sensors[i]->TS[t]->site_one_overL = 0.0;// Class D stability
            }
          }
        }
      } else if (WID->hrrrInput->stabilityClasses == 2) {// Monin-Obukhov lenght (based on surface fluxes)
        float cp = 1006;// Specific heat capacity of air at constant pressure (J/kg.K)
        float g = 9.81;// Gravitational acceleration (m/s^2)
        float rho = 1.2;// Air density (kg/m^3)
        float vk = 0.4;// Von Karman constant
        WID->metParams->sensors[i]->TS[t]->site_one_overL = -(vk * g * hrrrInputData->hrrrSenHeatFlux[hrrrInputData->hrrrSensorID[i]]) / (cp * rho * hrrrInputData->hrrrPotTemp[hrrrInputData->hrrrSensorID[i]] * pow(hrrrInputData->hrrrUStar[hrrrInputData->hrrrSensorID[i]], 3.0));
      }
    }
  }
  qes::log::info() << "[done]" << std::endl;
}
