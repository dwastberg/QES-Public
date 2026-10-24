#pragma once
#include <stdexcept>

namespace qes {
// I/O-dependent configurations cannot run in the in-memory core.
class UnsupportedFeature : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};
}
