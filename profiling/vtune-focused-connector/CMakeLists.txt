find_package(ITT REQUIRED)

kp_add_library(kp_vtune_focused_connector kp_vtune_focused_connector.cpp)

target_link_libraries(kp_vtune_focused_connector ittapi)