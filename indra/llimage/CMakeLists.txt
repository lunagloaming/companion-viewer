# -*- cmake -*-

project(llimage)

include(00-Common)
include(LLCommon)
include(LLImage)
include(JPEG)
include(LLKDU)
include(ZLIBNG)
include(LLAddBuildTest)
include(bugsplat)
include(Tut)

set(llimage_SOURCE_FILES
    fsimagebc7.cpp
    bc7enc/bc7enc.cpp
    bc7enc/bc7decomp.cpp
    llimagebmp.cpp
    llimage.cpp
    llimagedimensionsinfo.cpp
    llimagedxt.cpp
    llimagefilter.cpp
    llimagej2c.cpp
    llimagejpeg.cpp
    llimagepng.cpp
    llimagetga.cpp
    llimageworker.cpp
    llpngwrapper.cpp
    )

set(llimage_HEADER_FILES
    CMakeLists.txt

    fsimagebc7.h
    bc7enc/bc7enc.h
    bc7enc/bc7decomp.h
    llimage.h
    llimagebmp.h
    llimagedimensionsinfo.h
    llimagedxt.h
    llimagefilter.h
    llimagej2c.h
    llimagejpeg.h
    llimagepng.h
    llimagetga.h
    llimageworker.h
    llmapimagetype.h
    llpngwrapper.h
    )

list(APPEND llimage_SOURCE_FILES ${llimage_HEADER_FILES})

# <FS:Luna Witch> Vendored BC7 encoder (bc7enc/README.md): built as it is, without our warnings as errors
if (NOT WINDOWS)
    set_source_files_properties(bc7enc/bc7enc.cpp bc7enc/bc7decomp.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif ()
# </FS:Luna Witch>

add_library (llimage ${llimage_SOURCE_FILES})
target_include_directories( llimage  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
if (USE_KDU)
    target_link_libraries(llimage llkdu)
else (USE_KDU)
    target_link_libraries(llimage llimagej2coj)
endif (USE_KDU)

target_link_libraries(llimage
        llfilesystem
        llmath
        llcommon
        ll::libpng
        ll::libjpeg
    )

# Add tests
if (LL_TESTS)
  SET(llimage_TEST_SOURCE_FILES
    llimageworker.cpp
    )
  LL_ADD_PROJECT_UNIT_TESTS(llimage "${llimage_TEST_SOURCE_FILES}")
endif (LL_TESTS)



