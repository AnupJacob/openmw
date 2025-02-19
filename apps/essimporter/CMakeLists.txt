set(ESSIMPORTER_FILES
    main.cpp
    importer.cpp
    importplayer.cpp
    importnpcc.cpp
    importcrec.cpp
    importcellref.cpp
    importinventory.cpp
    importklst.cpp
    importcntc.cpp
    importgame.cpp
    importinfo.cpp
    importdial.cpp
    importques.cpp
    importjour.cpp
    importscri.cpp
    importscpt.cpp
    importproj.cpp
    importsplm.cpp
    importercontext.cpp
    converter.cpp
    convertacdt.cpp
    convertnpcc.cpp
    convertinventory.cpp
    convertcrec.cpp
    convertcntc.cpp
    convertscri.cpp
    convertscpt.cpp
    convertplayer.cpp
)

openmw_add_executable(openmw-essimporter
    ${ESSIMPORTER_FILES}
)

target_link_libraries(openmw-essimporter
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-essimporter gcov)
endif()

if (WIN32)
  INSTALL(TARGETS openmw-essimporter RUNTIME DESTINATION ".")
endif(WIN32)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(openmw-essimporter PRIVATE
        <algorithm>
        <filesystem>
        <fstream>
        <string>
        <vector>
    )
endif()
