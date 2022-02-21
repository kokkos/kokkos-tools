find_package(ITT REQUIRED)

kp_add_library(kp_vtune_connector kp_vtune_connector.cpp)

target_link_libraries(kp_vtune_connector ittapi)
