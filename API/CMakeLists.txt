include(TestBigEndian)

test_big_endian(BIGENDIAN)
if(${BIGENDIAN})
     add_definitions(-DBIG_ENDIAN)
endif(${BIGENDIAN})


#build OpenSCB API
find_path(LIBUSB_INCLUDE_DIR usb.h
          HINTS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS}
          PATH_SUFFIXES libusb )

find_library(LIBUSB_LIBRARY NAMES usb libusb
             HINTS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS} )

add_definitions(-DSCB_USE_LIBUSB)

add_library (openscb_api SHARED src/openscb.c src/openscb_advanced.c src/openscb_dev.c src/openscb_usb.c src/openscb_utils.c)
target_link_libraries(openscb_api ${LIBUSB_LIBRARY})
include_directories (${LIBUSB_INCLUDE_DIR})


add_executable (dfu-programmer dfu-prog/arguments.c dfu-prog/commands.c dfu-prog/intel_hex.c dfu-prog/util.c dfu-prog/atmel.c dfu-prog/dfu.c dfu-prog/main.c)
target_link_libraries(dfu-programmer ${LIBUSB_LIBRARY})
include_directories (${LIBUSB_INCLUDE_DIR})


#build test program for API
add_executable (test_api test_api/test_api.c)
target_link_libraries(test_api openscb_api)
include_directories (src)
