# needs to be relative to the current project when we
# hierarchically integrate projects together.

#IF (HAS_CUDA_SUPPORT)
#	CUDA_ADD_EXECUTABLE( qesWinds
#			handleWINDSArgs.cpp
#			qesWindsMain.cpp
#	)
#ELSE (HAS_CUDA_SUPPORT)
	ADD_EXECUTABLE( qesWinds
			handleWINDSArgs.cpp
			qesWindsMain.cpp
	)
#ENDIF (HAS_CUDA_SUPPORT)

# target_link_libraries(qesWinds curand)
target_link_libraries(qesWinds PRIVATE qeswindscore)
IF (HAS_CUDA_SUPPORT)
  target_link_libraries(qesWinds PRIVATE qeswindsgpu)
ENDIF()
target_link_libraries(qesWinds PRIVATE qesutil)

IF (HAS_OPTIX_SUPPORT)
  target_link_libraries(qesWinds PRIVATE qesOptix)
ENDIF()

link_external_libraries(qesWinds)

