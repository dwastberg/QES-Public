
#IF (HAS_CUDA_SUPPORT)
#  cuda_add_executable(util_output util_output.cpp)
  
#  set(examples
#          util_output)

#ELSE (HAS_CUDA_SUPPORT)
  add_executable(util_output util_output.cpp)

  set(examples
          util_output)
      
#ENDIF (HAS_CUDA_SUPPORT)

foreach (example ${examples})
  
  target_link_libraries(${example} PRIVATE qesplumecore)
  IF (HAS_CUDA_SUPPORT)
    target_link_libraries(${example} PRIVATE qeswindsgpu)
  ENDIF()
  target_link_libraries(${example} PRIVATE qeswindscore)
  target_link_libraries(${example} PRIVATE qesutil)  
  IF(HAS_OPTIX_SUPPORT)
    target_link_libraries(${example} PRIVATE qesOptix)
  ENDIF()

  link_external_libraries(${example})
    
endforeach(example)
