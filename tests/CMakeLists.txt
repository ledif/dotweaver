find_package(Qt6 REQUIRED COMPONENTS Test)

enable_testing()

# Test for ChezmoiService
add_executable(test_chezmoiservice
    test_chezmoiservice.cpp
    ../src/chezmoiservice.cpp
)

target_link_libraries(test_chezmoiservice
    Qt6::Core
    Qt6::Test
)

target_include_directories(test_chezmoiservice PRIVATE
    ../src
    ${CMAKE_CURRENT_BINARY_DIR}/../
)

add_test(NAME ChezmoiServiceTest COMMAND test_chezmoiservice)

# Test for DotfileManager
add_executable(test_dotfilemanager
    test_dotfilemanager.cpp
    ../src/dotfilemanager.cpp
    ../src/chezmoiservice.cpp
)

target_link_libraries(test_dotfilemanager
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
)

target_include_directories(test_dotfilemanager PRIVATE
    ../src
    ${CMAKE_CURRENT_BINARY_DIR}/../
)

add_test(NAME DotfileManagerTest COMMAND test_dotfilemanager)
