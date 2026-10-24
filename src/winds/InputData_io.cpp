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
#include "WINDSInputData.h"
#include "DTEHeightField.h"
#include "WRFInput.h"
#include "util/ESRIShapefile.h"
#include "util/QESFileSystemHandler.h"
#include "fire/Fires.hpp"

void SimulationParameters::parseValues()
{
    ParseVector<int> *domain_in;
    parseElement<ParseVector<int>>(false, domain_in, "domain");
    if (domain_in) {
      if (domain_in->size() == 3) {
        domain = { (*domain_in)[0], (*domain_in)[1], (*domain_in)[2] };
      } else {
        { std::ostringstream message; message << "[ERROR] unvalid number of argument for domain parameter" << std::endl; throw qes::InputError(message.str()); }
      }
    }
    ParseVector<float> *grid_in;
    parseElement<ParseVector<float>>(false, grid_in, "cellSize");
    if (grid_in) {
      if (grid_in->size() == 3) {
        grid = { (*grid_in)[0], (*grid_in)[1], (*grid_in)[2] };
      } else {
        { std::ostringstream message; message << "[ERROR] unvalid number of argument for cellSize parameter" << std::endl; throw qes::InputError(message.str()); }
      }
    }
    parsePrimitive<int>(false, verticalStretching, "verticalStretching");
    parseMultiPrimitives<float>(false, dz_value, "dz_value");
    parsePrimitive<unsigned int>(false, totalTimeIncrements, "totalTimeIncrements");
    parsePrimitive<int>(false, logLawFlag, "logLawFlag");
    parsePrimitive<int>(false, maxIterations, "maxIterations");
    parsePrimitive<double>(false, tolerance, "tolerance");
    parsePrimitive<int>(false, meshTypeFlag, "meshTypeFlag");
    parsePrimitive<float>(false, domainRotation, "domainRotation");
    parsePrimitive<int>(false, originFlag, "originFlag");
    parsePrimitive<float>(false, UTMx, "UTMx");
    parsePrimitive<float>(false, UTMy, "UTMy");
    parsePrimitive<int>(false, UTMZone, "UTMZone");
    parsePrimitive<std::string>(false, UTMZoneLetter, "UTMZoneLetter");
    parsePrimitive<float>(false, DEMDistancex, "DEMDistancex");
    parsePrimitive<float>(false, DEMDistancey, "DEMDistancey");
    parsePrimitive<float>(false, halo_x, "halo_x");
    parsePrimitive<float>(false, halo_y, "halo_y");
    parsePrimitive<float>(false, heightFactor, "heightFactor");
    parsePrimitive<int>(false, readCoefficientsFlag, "readCoefficientsFlag");

    coeffFile = "";
    parsePrimitive<std::string>(false, coeffFile, "COEFF");

    demFile = "";
    parsePrimitive<std::string>(false, demFile, "DEM");

    wrfFile = "";
    wrfSensorSample = 1;
    parsePrimitive<std::string>(false, wrfFile, "WRF");
    parsePrimitive<int>(false, wrfSensorSample, "WRFSensorSample");

    //
    // WRF coupling
    //
    parsePrimitive<bool>(false, wrfCoupling, "WRFCoupling");

    // Determine which use case to use for WRF/DEM combinations
    if ((demFile != "") && (wrfFile != "")) {
      // Both specified
      // DEM - read in terrain
      // WRF - retrieve wind profiles only
      m_domIType = WRFDEM;
    } else if ((demFile == "") && (wrfFile != "")) {
      // Only WRF Specified
      // WRF - pull terrain and retrieve wind profiles
      m_domIType = WRFOnly;
    } else if (demFile != "") {
      // Only DEM Specified - nothing set for WRF Input
      m_domIType = DEMOnly;
    } else {
      m_domIType = UNKNOWN;
    }

    if (demFile != "")
      demFile = QESfs::get_absolute_path(demFile);
    if (wrfFile != "")
      wrfFile = QESfs::get_absolute_path(wrfFile);
    if (coeffFile != "")
      coeffFile = QESfs::get_absolute_path(coeffFile);

    //
    // Process the data files based on the state determined above
    //
    if (m_domIType == WRFOnly) {
      //
      // WRF File is specified
      // Read in height field
      //
      // This step parses the WRF data, attempting to extract
      // the Fire mesh if it exists (the atm mesh, otherwise)
      // into a height field.
      //
      // We extract the dimensions of the fire (or atm) mesh for
      // nx and ny domain cells.
      //
      qes::log::info() << "Processing WRF data for terrain and met param sensors from " << wrfFile << std::endl;
      wrfInputData = new WRFInput(wrfFile, UTMx, UTMy, UTMZone, UTMZoneLetter, 0, 0, wrfCoupling, wrfSensorSample);
      qes::log::info() << "WRF input data processing completed." << std::endl;

      // Apply halo to wind profile locations -- halo units are
      // meters so can add to other coordinates/positions, such
      // as station coordinates.
      wrfInputData->applyHalotoStationData(halo_x, halo_y);

      // Debug to show where the stations are...
      // wrfInputData->dumpStationData();

      // In the current setup, grid may NOT be set... be careful
      // may need to initialize it here if nullptr is true for grid

      // utilize the wrf information to construct a DTE_heightfield
      DTE_heightField = new DTEHeightField(wrfInputData->fmHeight,
                                           std::tuple<int, int, int>(wrfInputData->fm_nx, wrfInputData->fm_ny, wrfInputData->fm_nz),
                                           std::tuple<float, float, float>(wrfInputData->fm_dx, wrfInputData->fm_dy, wrfInputData->fm_dz),
                                           halo_x,
                                           halo_y);

      // Make sure to pull the dx, dy and dz back from the WRF calculations
      grid[0] = wrfInputData->fm_dx;
      grid[1] = wrfInputData->fm_dy;
      grid[2] = wrfInputData->fm_dz;

      qes::log::info() << "Dim: " << wrfInputData->fm_nx << " X " << wrfInputData->fm_ny << " X " << wrfInputData->fm_nz
                << " at " << grid[0] << " X " << grid[1] << " X " << grid[2] << std::endl;

      domain = { wrfInputData->fm_nx, wrfInputData->fm_ny, wrfInputData->fm_nz };
      DTE_heightField->setDomain(domain, grid);

      // need this to make sure domain sizes include halo
      int halo_x_WRFAddition = (int)floor(halo_x / wrfInputData->fm_dx);
      int halo_y_WRFAddition = (int)floor(halo_y / wrfInputData->fm_dy);

      qes::log::info() << "Halo Addition to dimensions: " << halo_x_WRFAddition << " cells, " << halo_y_WRFAddition << " cells" << std::endl;

      domain[0] += 2 * halo_x_WRFAddition;
      domain[1] += 2 * halo_y_WRFAddition;

      // let WRF class know about the dimensions that the halo adds...
      wrfInputData->setHaloAdditions(halo_x_WRFAddition, halo_y_WRFAddition);

      qes::log::info() << "domain size with halo borders: " << domain[0] << " x " << domain[1] << std::endl;

      DTE_mesh = new Mesh(DTE_heightField->getTris());
      qes::log::info() << "Meshing of DEM complete\n";
    } else if (m_domIType == WRFDEM) {

      qes::log::info() << "Reading DEM and processing WRF data for met param sensors from " << wrfFile << std::endl;

      // First read DEM as usual
      qes::log::info() << "Domain: " << domain[0] << ", " << domain[1] << ", " << domain[2] << std::endl;
      qes::log::info() << "Grid: " << grid[0] << ", " << grid[1] << ", " << grid[2] << std::endl;
      // Extracting Digital Elevation Data from: demFile
      DTE_heightField = new DTEHeightField(demFile,
                                           std::tuple<int, int, int>(domain[0], domain[1], domain[2]),
                                           std::tuple<float, float, float>(grid[0], grid[1], grid[2]),
                                           UTMx,
                                           UTMy,
                                           originFlag,
                                           DEMDistancex,
                                           DEMDistancey);
      assert(DTE_heightField);

      qes::log::info() << "Forming triangle mesh...\n";
      DTE_heightField->setDomain(domain, grid);
      DTE_mesh = new Mesh(DTE_heightField->getTris());
      qes::log::info() << "Mesh complete\n";

      // To proceed and cull sensors appropriately, we will need
      // the lower-left bounds from the DEM if UTMx and UTMy and
      // UTMZone are not all 0
      //
      // ??? Parse primitive should return true or false if a
      // value was parsed, rather than this
      // ??? can refactor that later.
      bool useUTM_for_DEMLocation = false;
      float uEps = 0.001;
      if (((UTMx > -uEps) && (UTMx < uEps)) && ((UTMy > -uEps) && (UTMy < uEps)) && (UTMZone == 0)) {
        useUTM_for_DEMLocation = true;
      }
      if (useUTM_for_DEMLocation) {
        qes::log::info() << "UTM (" << UTMx << ", " << UTMy << "), Zone: " << UTMZone << " will be used as lower-left location for DEM." << std::endl;
      }

      // Note from Pete:
      // Normally, will want to see if UTMx and UTMy are valid
      // in the DEM and if so, pass that UTMx and UTMy into the
      // WRFInput.  Passing 0s for these should not likely cause
      // issues since we're either adding or subtracting these
      // values.

      // Then, read WRF File extracting ONLY the sensor data
      bool onlySensorData = true;
      float dimX = domain[0] * grid[0];
      float dimY = domain[1] * grid[1];
      qes::log::info() << "dimX = " << dimX << ", dimY = " << dimY << std::endl;
      wrfInputData = new WRFInput(wrfFile, UTMx, UTMy, UTMZone, UTMZoneLetter, dimX, dimY, wrfSensorSample, wrfCoupling, onlySensorData);
      qes::log::info() << "WRF Wind Velocity Profile Data processing completed." << std::endl;
    }

    else if (m_domIType == DEMOnly) {
      // Extracting Digital Elevation Data from: demFile
      DTE_heightField = new DTEHeightField(demFile,
                                           std::tuple<int, int, int>(domain[0], domain[1], domain[2]),
                                           std::tuple<float, float, float>(grid[0], grid[1], grid[2]),
                                           UTMx,
                                           UTMy,
                                           originFlag,
                                           DEMDistancex,
                                           DEMDistancey);
      assert(DTE_heightField);

      // Forming triangle mesh...
      DTE_heightField->setDomain(domain, grid);
      DTE_mesh = new Mesh(DTE_heightField->getTris());

      // for (auto it = DTE_mesh->optixTris.begin(); it != DTE_mesh->optixTris.end(); ++it) {
      //   if ((*it)->n[2] < 0)
      //     qes::log::warning() << "[WARNING] triangle definition " << (*it)->n << std::endl;
      // }

    } else {
      // No DEM, so make sure these are null
      DTE_heightField = nullptr;
      DTE_mesh = nullptr;
      wrfInputData = nullptr;
    }
  }

SimulationParameters::~SimulationParameters()
{
    // close the scanner
    if (DTE_heightField)
      DTE_heightField->closeScanner();
  }

void BuildingsParams::parseValues()
{
    buildings.clear();

    parsePrimitive<float>(true, wallRoughness, "wallRoughness");
    parsePrimitive<int>(true, rooftopFlag, "rooftopFlag");
    parsePrimitive<int>(true, upwindCavityFlag, "upwindCavityFlag");
    parsePrimitive<int>(true, streetCanyonFlag, "streetCanyonFlag");
    parsePrimitive<int>(true, streetIntersectionFlag, "streetIntersectionFlag");
    parsePrimitive<int>(true, wakeFlag, "wakeFlag");
    parsePrimitive<int>(true, highRiseFlag, "highRiseFlag");
    parsePrimitive<int>(true, sidewallFlag, "sidewallFlag");

    parsePrimitive<int>(false, numBuildings, "numBuildings");
    parsePrimitive<int>(false, numPolygonNodes, "numPolygonNodes");
    parseMultiPolymorphs(false, buildings, Polymorph<Building, RectangularBuilding>("rectangularBuilding"));
    parseMultiPolymorphs(false, buildings, Polymorph<Building, PolygonQUICBuilding>("QUICBuilding"));
    parsePrimitive<float>(false, heightFactor, "heightFactor");

    shpFile = "";
    parsePrimitive<std::string>(false, shpFile, "SHPFile");
    shpBuildingLayerName = "buildings";// defaults
    parsePrimitive<std::string>(false, shpBuildingLayerName, "SHPBuildingLayer");
    shpHeightField = "H";// default;
    parsePrimitive<std::string>(false, shpHeightField, "SHPHeightField");

    SHPData = nullptr;
    if (shpFile != "") {
      shpFile = QESfs::get_absolute_path(shpFile);
      // Read polygon node coordinates and building height from shapefile
      // SHPData = new ESRIShapefile(shpFile, shpTreeLayerName, shpPolygons, shpFeatures);
      SHPData = new ESRIShapefile(shpFile, shpBuildingLayerName);
      // qes::log::info() << shpPolygons.size() << " " << shpTreeHeight.size() << std::endl;
    }
  }

void VegetationParams::parseValues()
{
    parsePrimitive<int>(false, wakeFlag, "wakeFlag");

    // read the input data for canopies
    parseMultiPolymorphs(false, canopies, Polymorph<Building, CanopyHomogeneous>("Homogeneous"));
    parseMultiPolymorphs(false, canopies, Polymorph<Building, CanopyIsolatedTree>("IsolatedTree"));
    parseMultiPolymorphs(false, canopies, Polymorph<Building, CanopyWindbreak>("Windbreak"));
    parseMultiPolymorphs(false, canopies, Polymorph<Building, CanopyROC>("ROC"));

    // add other type of canopy here

    shpFile = "";
    parsePrimitive<std::string>(false, shpFile, "SHPFile");

    shpTreeLayerName = "trees";// defaults
    parsePrimitive<std::string>(false, shpTreeLayerName, "SHPTreeLayer");

    //
    // Process ESRIShapeFile here, but leave extraction of poly
    // building for later in WINDSGeneralData
    //
    SHPData = nullptr;
    if (shpFile != "") {
      shpFile = QESfs::get_absolute_path(shpFile);
      // Read polygon node coordinates and building height from shapefile
      // SHPData = new ESRIShapefile(shpFile, shpTreeLayerName, shpPolygons, shpFeatures);
      SHPData = new ESRIShapefile(shpFile, shpTreeLayerName);
      // qes::log::info() << shpPolygons.size() << " " << shpTreeHeight.size() << std::endl;
    }
  }

void MetParams::parseValues()
{
    parsePrimitive<int>(false, z0_domain_flag, "z0_domain_flag");
    parseMultiElements<Sensor>(false, sensors, "sensor");

    parseMultiPrimitives<std::string>(false, sensorName, "sensorName");
    for (auto i = 0u; i < sensorName.size(); i++) {
      sensorName[i] = QESfs::get_absolute_path(sensorName[i]);
    }
  }

void HRRRInput::parseValues()
{
    // parseMultiPrimitives<std::string>(false, inputFields, "inputFields");
    parsePrimitive<int>(false, interpolationScheme, "interpolationScheme");
    parsePrimitive<int>(false, stabilityClasses, "stabilityClasses");
    HRRRFile = "";
    parsePrimitive<std::string>(false, HRRRFile, "HRRRFile");
    if (HRRRFile != "") {
      HRRRFile = QESfs::get_absolute_path(HRRRFile);
    }
  }

void TURBParams::parseValues()
{

    parsePrimitive<int>(true, methodLocalMixing, "method");
    parsePrimitive<bool>(false, save2file, "save");
    parsePrimitive<std::string>(false, filename, "LMfile");
    parsePrimitive<std::string>(false, varname, "varname");
    parsePrimitive<bool>(false, flagNonLocalMixing, "nonLocalMixing");
    parsePrimitive<int>(false, terrainWallFlag, "terrainWallFlag");
    parsePrimitive<int>(false, buildingWallFlag, "buildingWallFlag");
    parsePrimitive<float>(false, turbUpperBound, "turbUpperBound");
    parsePrimitive<float>(false, backgroundMixing, "backgroundMixing");

#ifdef HAS_OPTIX
    // defaults for local mixing sample rates -- used with ray
    // tracing methods
    if (methodLocalMixing == 3) {// OptiX
      mlSamplesPerAirCell = 2000;
    } else {
      mlSamplesPerAirCell = 500;// other ray-traced methods
    }
    parsePrimitive<int>(false, mlSamplesPerAirCell, "samples");
    if (methodLocalMixing == 3) {
      qes::log::info() << "Setting samples per air cell for ray-traced mixing length to "
                << mlSamplesPerAirCell << std::endl;
    }
#else
    if (methodLocalMixing == 3) {
      QESout::warning("Optix not supported -> set method to serial (methodLocalMixing = 1)");
      methodLocalMixing = 1;
    }
#endif

    if (methodLocalMixing < 0 || methodLocalMixing > 4) {
      std::string mess = "Unknown local mixing method -> ";
      mess += "set method to height above terrain (methodLocalMixing = 0)";
      QESout::warning(mess);
      methodLocalMixing = 0;
    }

    if ((methodLocalMixing == 4 || save2file) && (filename.empty())) {
      std::string mess = "No local mixing file provided -> ";
      mess += "set method to height above terrain(methodLocalMixing = 0) ";
      QESout::warning(mess);
      methodLocalMixing = 0;
    }
    if (methodLocalMixing == 0 || methodLocalMixing == 4) {
      save2file = "false";
    }

    if (!filename.empty()) {
      filename = QESfs::get_absolute_path(filename);
    }

    sigConst = nullptr;
    ParseVector<float> *sig_in = nullptr;
    parseElement<ParseVector<float>>(false, sig_in, "sigmaConst");
    if (sig_in) {
      if (sig_in->size() == 3) {
        sigConst = new Vector3Float((*(sig_in))[0], (*(sig_in))[1], (*(sig_in))[2]);
      } else {
        throw qes::InputError("sigmaConst must contain three values");
      }
    }
  }

void WINDSInputData::parseValues()
{
    parseElement<SimulationParameters>(true, simParams, "simulationParameters");
    parseElement<FileOptions>(false, fileOptions, "fileOptions");
    parseElement<MetParams>(false, metParams, "metParams");
    parseElement<TURBParams>(false, turbParams, "turbParams");
    parseElement<BuildingsParams>(false, buildingsParams, "buildingsParams");
    parseElement<VegetationParams>(false, vegetationParams, "vegetationParams");
    parseElement<HRRRInput>(false, hrrrInput, "hrrrInput");
    parseElement<Fires>(false, fires, "fires");
  }

WINDSInputData::WINDSInputData(const std::string fileName)
{

    fileOptions = 0;
    metParams = 0;
    turbParams = 0;
    buildingsParams = 0;
    vegetationParams = 0;
    hrrrInput = 0;
    fires = 0;

    QESfs::set_file_path(fileName);
    // read and parse the XML
    parseXML(fileName, "QESWindsParameters");
  }

Sensor::Sensor(const std::string fileName)
{
    // read and parse the XML
    parseXML(fileName, "sensor");
  }
