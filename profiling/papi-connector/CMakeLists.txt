kp_add_library(kp_papi_connector kp_papi_connector.cpp)

target_link_libraries(kp_papi_connector PRIVATE PAPI::PAPI)
