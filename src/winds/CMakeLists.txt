add_library(qeswindscore STATIC
  ../qes/Domain.cpp
  Canopy.cpp CanopyElement.cpp CanopyHomogeneous.cpp CanopyIsolatedTree.cpp
  CanopyWindbreak.cpp CanopyROC.cpp Cell.cpp CutCell.cpp CutBuilding.cpp
  LocalMixingDefault.cpp LocalMixingSerial.cpp LocalMixingOptix.cpp
  PolyBuilding.cpp Sensor.cpp Solver.cpp Solver_CPU.cpp Solver_CPU_RB.cpp SolverFactory.cpp
  TURBGeneralData.cpp TURBWall.cpp TURBWallBuilding.cpp TURBWallTerrain.cpp
  WindProfilerSensorType.cpp WindProfilerBarnCPU.cpp
  WINDSGeneralData.cpp Wall.cpp UpwindCavity.cpp NonLocalMixing.cpp PolygonWake.cpp
  StreetCanyon.cpp StreetCanyonModified.cpp Sidewall.cpp Rooftop.cpp StreetIntersection.cpp)
target_link_libraries(qeswindscore PUBLIC qesutil)
target_include_directories(qeswindscore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(qeswindscore PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(QES_ENABLE_IO)
  add_library(qeswindsio STATIC
    ../qes/Domain_io.cpp InputData_io.cpp WINDSGeneralData_io.cpp TURBGeneralData_io.cpp
    Canopy_io.cpp PolyBuilding_io.cpp LocalMixing.cpp LocalMixingNetCDF.cpp DTEHeightField.cpp
    WindProfilerWRF.cpp WindProfilerHRRR.cpp WRFInput.cpp HRRRData.cpp
    WINDSOutputVisualization.cpp WINDSOutputWorkspace.cpp WINDSOutputWRF.cpp TURBOutput.cpp)
  # Mixed initialisation calls into I/O only in this build. CMake repeats this
  # static-library component on ELF linkers to resolve both directions.
  target_link_libraries(qeswindsio PUBLIC qeswindscore qesutilio)
  target_link_libraries(qeswindscore PUBLIC qeswindsio)
else()
  target_sources(qeswindscore PRIVATE
    ../qes/Domain_noio.cpp InputData_noio.cpp WINDSGeneralData_noio.cpp
    TURBGeneralData_noio.cpp PolyBuilding_noio.cpp)
endif()
if(HAS_CUDA_SUPPORT)
  target_sources(qeswindscore PRIVATE WindProfilerBarnGPU.cpp)
  add_library(qeswindsgpu STATIC
    Solver_GPU_GlobalMemory.cu Solver_GPU_SharedMemory.cu WINDSGeneralData.cu
    TURBGeneralData.cu WindProfilerBarnGPU.cu)
  target_link_libraries(qeswindsgpu PUBLIC qeswindscore)
  set_target_properties(qeswindsgpu PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()
if(HAS_OPTIX_SUPPORT)
  cuda_compile_and_embed(EMBEDDED_OPTIX "${CMAKE_CURRENT_SOURCE_DIR}/OptixRayTrace.cu" OptixRayTrace)
  add_library(qesOptix STATIC OptixRayTrace.cpp ${EMBEDDED_OPTIX})
  target_link_libraries(qesOptix PUBLIC qeswindscore)
endif()
