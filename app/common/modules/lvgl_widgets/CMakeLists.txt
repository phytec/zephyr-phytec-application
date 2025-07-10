target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_widgets.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)

target_sources(app PRIVATE
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_needle.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c
)
