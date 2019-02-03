project(SqliteOverlay)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

set(BUILD_TESTS 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmakeModules/")
find_package(SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS filesystem system log log_setup)
include_directories("." ${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
add_definitions(-DBOOST_LOG_DYN_LINK=1)  # linking fails if this is not set

# A path prefix for mapping MSYS2-directories to "raw" directories
set(WIN_LOCAL_PREFIX "D:/PortablePrograms/msys64/usr/local")

#
# My lib of sloppyly implemented helper functions
#
find_library(Sloppy_LIB NAME Sloppy PATHS /usr/local/lib /usr/lib /usr/local/lib64 /usr/lib64)
set(LIBS ${LIBS} ${Sloppy_LIB})
message("Using Sloppy lib at " ${Sloppy_LIB})

if (WIN32)
  include_directories(${WIN_LOCAL_PREFIX}/include)
ENDIF()

# detect if we are on windows, because for windows
# we have to use special code for timegm()
#
# see DateAndTime.cpp
#
IF (WIN32)
  add_definitions(-DIS_WINDOWS_BUILD)
ENDIF()

set(LIB_SOURCES
    SqliteDatabase.cpp
    SqliteDatabase.h
    SqlStatement.cpp
    SqlStatement.h
    CommonTabularClass.cpp
    CommonTabularClass.h
    #DbTab.cpp
    #DbTab.h
    TabRow.cpp
    TabRow.h
    ClausesAndQueries.cpp
    ClausesAndQueries.h
    #GenericDatabaseObject.cpp
    #GenericObjectManager.cpp
    #KeyValueTab.cpp
    #TableCreator.cpp
    Transaction.cpp
    Transaction.h
    #UserMngr.cpp
    #ActionMngr.cpp
    SqliteExceptions.h
    Defs.h
    Defs.cpp
)

add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wextra")
#target_compile_options(${PROJECT_NAME} PRIVATE "-Weffc++")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

set(LIB_VERSION_MAJOR 0)
set(LIB_VERSION_MINOR 3)
set(LIB_VERSION_PATCH 4)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${LIB_VERSION_STRING}
                                          SOVERSION ${LIB_VERSION_MAJOR})

#
# Installation data for the lib
#
# CMake's default destination path is /usr/local which is just fine
#
IF (WIN32)
  set(CMAKE_INSTALL_PREFIX ${WIN_LOCAL_PREFIX} )
  install(TARGETS ${PROJECT_NAME}
          RUNTIME DESTINATION lib
         )
ELSE()
  install(TARGETS ${PROJECT_NAME}
          LIBRARY DESTINATION lib
         )
ENDIF()

set(INSTALLATION_HEADERS
    SqliteDatabase.h
    SqlStatement.h
    CommonTabularClass.h
    DbTab.h
    TabRow.h
    ClausesAndQueries.h
    GenericDatabaseObject.h
    GenericObjectManager.h
    KeyValueTab.h
    TableCreator.h
    Transaction.h
    UserMngr.h
    ActionMngr.h
    )
install(FILES ${INSTALLATION_HEADERS} DESTINATION include/SqliteOverlay)


#
# Unit tests
#
set(LIB_SOURCES_TST
    tests/BasicTestClass.cpp
    tests/unitTestMain.cpp
    tests/DatabaseTestScenario.h
    tests/DatabaseTestScenario.cpp
    tests/SampleDB.h
    tests/SampleDB.cpp
    tests/tstStatement.cpp
    tests/tstDatabaseInit.cpp
    tests/tstDatabaseQueries.cpp
    tests/tstDatabaseMisc.cpp
    tests/tstCommonTabularClass.cpp
    tests/tstClausesAndQueries.cpp
    #tests/tstDbTab.cpp
    tests/tstTabRow.cpp
    #tests/tstTimestamps.cpp
    #tests/tstDateTime.cpp
    #tests/tstKeyValueTab.cpp
    tests/tstTransaction.cpp
    #tests/tstCopyAndBackup.cpp
    #tests/tstUserManager.cpp
    #tests/tstActionManager.cpp
)

find_package(GTest)

if (GTEST_FOUND AND Boost_FOUND AND BUILD_TESTS)
    message("   !!! Tests will be build as well !!!")
    include_directories("." ${GTEST_INCLUDE_DIRS})

    add_executable(${PROJECT_NAME}_Tests ${LIB_SOURCES_TST})
    set_property(TARGET ${PROJECT_NAME}_Tests PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${PROJECT_NAME}_Tests PROPERTY CXX_STANDARD_REQUIRED ON)

    set_target_properties(${PROJECT_NAME}_Tests PROPERTIES
      IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.so"
    )

    set(LIBS ${LIBS} ${PROJECT_NAME})
    message("Using these libs: ${LIBS}")
    message("Writing unit test binary to: ${CMAKE_BINARY_DIR}")

    target_link_libraries(${PROJECT_NAME}_Tests ${LIBS} ${GTEST_BOTH_LIBRARIES})
endif (GTEST_FOUND AND Boost_FOUND AND BUILD_TESTS)
