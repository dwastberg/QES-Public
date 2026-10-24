# needs to be relative to the current project when we
# hierarchically integrate projects together.

#IF (HAS_CUDA_SUPPORT)
#  CUDA_ADD_EXECUTABLE( qesPlume
#          handlePlumeArgs.cpp
#          qesPlumeMain.cpp
#  )
#ELSE (HAS_CUDA_SUPPORT)
  ADD_EXECUTABLE( qesPlume
          handlePlumeArgs.cpp
          qesPlumeMain.cpp
  )
#ENDIF (HAS_CUDA_SUPPORT)

target_link_libraries(qesPlume PRIVATE qesplumecore)

IF (HAS_CUDA_SUPPORT)
  target_link_libraries(qesPlume PRIVATE qeswindsgpu)
ENDIF()
target_link_libraries(qesPlume PRIVATE qeswindscore)

target_link_libraries(qesPlume PRIVATE qesutil)

IF (HAS_OPTIX_SUPPORT)
  target_link_libraries(qesPlume PRIVATE qesOptix)
ENDIF()

link_external_libraries(qesPlume)
