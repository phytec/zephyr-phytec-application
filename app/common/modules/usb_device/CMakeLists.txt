target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_device.c  ${CMAKE_CURRENT_SOURCE_DIR}/sample_usbd_init.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)
