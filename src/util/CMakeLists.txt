add_library(qesutil STATIC
  QESout.cpp calcTime.cpp BVH.cpp HitRecord.cpp Mesh.cpp Triangle.cpp
  Ray.cpp SphereDirections.cpp QEStime.cpp)
target_link_libraries(qesutil PUBLIC qesbuild)
target_include_directories(qesutil PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(qesutil PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(QES_ENABLE_IO)
  add_library(qesutilio STATIC
    ArgumentParsing.cpp ESRIShapefile.cpp GIStool.cpp
    NetCDFInput.cpp NetCDFOutput.cpp QESNetCDFOutput.cpp QESFileSystemHandler.cpp
    QESFileOutput_v2.cpp QESNetCDFOutput_v2.cpp DataSource.cpp)
  target_link_libraries(qesutilio PUBLIC qesutil qesiodeps)
endif()
if(HAS_CUDA_SUPPORT)
  add_library(qesutilgpu STATIC VectorMath_CUDA.cu)
  target_link_libraries(qesutilgpu PUBLIC qesutil)
  set_target_properties(qesutilgpu PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()
