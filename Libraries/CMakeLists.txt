set(MeshValmetLib_SRC 
MeshValmet.cxx
MeshValmet/compute_volume_overlap.cxx
MeshValmet/compute_error.cxx
MeshValmet/geomutils.cxx
MeshValmet/mesh_run.cxx
MeshValmet/model_analysis.cxx
MeshValmet/model_in.cxx
MeshValmet/reporting.cxx
MeshValmet/xalloc.cxx
)


include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

if( Build_Static )
  set( MeshValmetLib_LIBRARY_TYPE STATIC )
else()
  set( MeshValmetLib_LIBRARY_TYPE SHARED )
endif()

add_library(MeshValmetLib ${MeshValmetLib_LIBRARY_TYPE} ${MeshValmetLib_SRC} )

target_link_libraries(MeshValmetLib ${VTK_LIBRARIES} )
#message( ${vtkGUISupportQt_LIBRARIES} )

set_target_properties(MeshValmetLib  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    )

install(TARGETS MeshValmetLib
    LIBRARY DESTINATION "${INSTALL_LIBRARY_DESTINATION}" COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION "${INSTALL_ARCHIVE_DESTINATION}" COMPONENT RuntimeLibraries
    )

