#include "Domain.h"
#include "util/NetCDFInput.h"
#include <algorithm>
using namespace qes;

Domain::Domain(const std::string &inputFile)
{
  auto *input = new NetCDFInput(inputFile);

  input->getDimensionSize("x_face", domainData.nx);
  input->getDimensionSize("y_face", domainData.ny);
  // nz - face centered value + bottom ghost (consistant with QES-Winds)
  input->getDimensionSize("z_face", domainData.nz);

  x.resize(domainData.nx - 1);
  y.resize(domainData.ny - 1);
  z.resize(domainData.nz - 1);
  z_face.resize(domainData.nz);
  dz_array.resize(domainData.nz - 1, 0.0);

  input->getVariableData("x", x);
  domainData.dx = x[1] - x[0]; /**< Grid resolution in x-direction */

  input->getVariableData("y", y);
  domainData.dy = y[1] - y[0]; /**< Grid resolution in x-direction */
  // dxy = MIN_S(dx, dy);

  domainData.dxy = std::min(domainData.dx, domainData.dy);

  input->getVariableData("z", z);
  // check if dz_array is in the NetCDF file
  NcVar NcVar_dz;
  input->getVariable("dz_array", NcVar_dz);
  if (!NcVar_dz.isNull()) {
    input->getVariableData("dz_array", dz_array);
    domainData.dz = *std::min_element(dz_array.begin(), dz_array.end());
  } else {
    domainData.dz = z[1] - z[0];
    for (size_t k = 0; k < z.size(); k++) {
      dz_array[k] = domainData.dz;
    }
  }

  // check if z_face is in the NetCDF file
  NcVar NcVar_zface;
  input->getVariable("z_face", NcVar_zface);
  if (!NcVar_zface.isNull()) {
    input->getVariableData("z_face", z_face);
  } else {
    z_face[0] = -dz_array[0];
    z_face[1] = 0.0;
    for (size_t k = 2; k < z_face.size() - 1; ++k) {
      z_face[k] = z_face[k - 1] + dz_array[k - 1];
    }
  }
}
