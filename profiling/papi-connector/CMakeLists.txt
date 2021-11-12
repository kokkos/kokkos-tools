add_library(kp_papi_connector SHARED kp_papi_connector.cpp)

target_link_libraries(kp_papi_connector PRIVATE PAPI::PAPI)