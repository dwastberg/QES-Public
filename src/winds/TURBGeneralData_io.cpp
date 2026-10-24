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
#include "TURBGeneralData.h"
#include "util/NetCDFInput.h"

TURBGeneralData::TURBGeneralData(const std::string inputFile, WINDSGeneralData *WGD)
  : domain(WGD->domain)
{
  qes::log::info() << "-------------------------------------------------------------------" << std::endl;
  qes::log::info() << "[QES-TURB]\t Initialization of turbulence model...\n";
  qes::log::info() << "[TURB Data] \t Loading QES-turb fields " << std::endl;

  m_WGD = WGD;

  // fullname passed to WINDSGeneralData
  input = new NetCDFInput(inputFile);

  // nx,ny - face centered value (consistant with QES-Winds)
  int nx, ny, nz;
  input->getDimensionSize("x", nx);
  input->getDimensionSize("y", ny);
  // nz - face centered value + bottom ghost (consistant with QES-Winds)
  input->getDimensionSize("z", nz);
  // nt - number of time instance in data
  input->getDimensionSize("t", nt);

  if ((nx != domain.nx() - 1) || (ny != domain.ny() - 1) || (nz != domain.nz() - 1)) {
    { std::ostringstream message; message << "[ERROR] \t data size incompatible " << std::endl; throw qes::InputError(message.str()); }
  }

  allocateMemory();

  // get time variables
  t.resize(nt);
  input->getVariableData("t", t);

  // check if times is in the NetCDF file
  NcVar NcVar_timestamp;
  input->getVariable("timestamp", NcVar_timestamp);

  if (NcVar_timestamp.isNull()) {
    QESout::warning("No timestamp in NetCDF file");
  }

  /*
     vector containing cell id of fluid cell
     do not include 1 cell shell around the domain
     => i=1...nx-2 j=1...ny-2
     do not include 1 cell layer at the top of the domain
     => k=1...nz-2
  */
  for (int k = 1; k < domain.nz() - 2; k++) {
    for (int j = 1; j < domain.ny() - 2; j++) {
      for (int i = 1; i < domain.nx() - 2; i++) {
        // int id = i + j * (nx - 1) + k * (nx - 1) * (ny - 1);
        long id = domain.cell(i, j, k);
        if (m_WGD->icellflag[id] != 0 && m_WGD->icellflag[id] != 2) {
          icellfluid.push_back(id);
        }
      }
    }
  }
}

void TURBGeneralData::loadNetCDFData(int stepin)
{
  qes::log::info() << "[QES-TURB]\t Loading data at step " << stepin
            << " (" << m_WGD->timestamp[stepin] << ")" << std::endl;

  // netCDF variables
  std::vector<size_t> start;
  std::vector<size_t> count_cc;
  std::vector<size_t> count_fc;

  start = { static_cast<unsigned long>(stepin), 0, 0, 0 };
  count_cc = { 1,
               static_cast<unsigned long>(domain.nz() - 1),
               static_cast<unsigned long>(domain.ny() - 1),
               static_cast<unsigned long>(domain.nx() - 1) };

  // stress tensor
  input->getVariableData("txx", start, count_cc, txx);
  input->getVariableData("txy", start, count_cc, txy);
  input->getVariableData("txz", start, count_cc, txz);
  input->getVariableData("tyy", start, count_cc, tyy);
  input->getVariableData("tyz", start, count_cc, tyz);
  input->getVariableData("tzz", start, count_cc, tzz);

  input->getVariableData("tke", start, count_cc, tke);
  input->getVariableData("CoEps", start, count_cc, CoEps);

  NcVar NcVar_timestamp;
  input->getVariable("div_tau_x", NcVar_timestamp);

  if (!NcVar_timestamp.isNull()) {
    input->getVariableData("div_tau_x", start, count_cc, div_tau_x);
    input->getVariableData("div_tau_y", start, count_cc, div_tau_y);
    input->getVariableData("div_tau_z", start, count_cc, div_tau_z);
  } else {
    divergenceStress();
  }

  return;
  // qes::log::info() << "\t\t Memory allocation completed.\n";
  qes::log::info() << "[QES-TURB]\t Initialization of turbulence model completed.\n";
}
