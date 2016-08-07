project(SqliteOverlay)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmakeModules/")
find_package(SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS filesystem system)
include_directories("." ${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

#
# My lib of sloppyly implemented helper functions
#
set(Sloppy_HOME_DIR "../libSloppy")
set(Sloppy_INCLUDE_DIR ${Sloppy_HOME_DIR})
if (CMAKE_BUILD_TYPE MATCHES Debug)
  find_library(Sloppy_LIB NAME Sloppy HINTS "${Sloppy_HOME_DIR}/debug")
else (CMAKE_BUILD_TYPE MATCHES Debug)
  find_library(Sloppy_LIB NAME Sloppy HINTS "${Sloppy_HOME_DIR}/release")
endif (CMAKE_BUILD_TYPE MATCHES Debug)
set(LIBS ${LIBS} ${Sloppy_LIB})
include_directories(${Sloppy_INCLUDE_DIR})

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
    Logger.cpp
    SqlStatement.cpp
    HelperFunc.cpp
    CommonTabularClass.cpp
    DbTab.cpp
    TabRow.cpp
    ClausesAndQueries.cpp
    GenericDatabaseObject.cpp
    GenericObjectManager.cpp
    #DateAndTime.cpp
    KeyValueTab.cpp
    TableCreator.cpp
    Transaction.cpp
    StringHelper.cpp
)

add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

set(LIB_VERSION_MAJOR 0)
set(LIB_VERSION_MINOR 1)
set(LIB_VERSION_PATCH 1)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${LIB_VERSION_STRING}
                                          SOVERSION ${LIB_VERSION_MAJOR})

#
# Unit tests
#
set(LIB_SOURCES_TST
    tests/BasicTestClass.cpp
    tests/unitTestMain.cpp
    tests/tstDatabaseInit.cpp
    tests/tstDatabaseQueries.cpp
    tests/DatabaseTestScenario.cpp
    tests/SampleDB.cpp
    tests/tstCommonTabularClass.cpp
    tests/tstClausesAndQueries.cpp
    tests/tstDbTab.cpp
    tests/tstTabRow.cpp
    tests/tstTimestamps.cpp
    tests/tstDateTime.cpp
    tests/tstKeyValueTab.cpp
    tests/tstTransaction.cpp
    tests/tstCopyAndBackup.cpp
)

find_package(GTest)

if (GTEST_FOUND AND Boost_FOUND)
    include_directories("." ${GTEST_INCLUDE_DIRS})

    add_executable(${PROJECT_NAME}_Tests ${LIB_SOURCES} ${LIB_SOURCES_TST})
    set_property(TARGET ${PROJECT_NAME}_Tests PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${PROJECT_NAME}_Tests PROPERTY CXX_STANDARD_REQUIRED ON)

    target_link_libraries(${PROJECT_NAME}_Tests ${GTEST_BOTH_LIBRARIES} ${LIBS})
endif (GTEST_FOUND AND Boost_FOUND)
