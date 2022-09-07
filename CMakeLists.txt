cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(sc3ntist LANGUAGES C CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(
  parser STATIC
  "src/parser/SC3Argument.cpp"
  "src/parser/SC3Expression.cpp"
  "src/parser/SGHDDisassembler.cpp"
  "src/parser/SC3BaseDisassembler.cpp"
  "src/parser/CCCharset.cpp"
  "src/parser/SC3StringDecoder.cpp"
  "src/parser/SCXFile.cpp"
  "src/parser/SC3CodeBlock.cpp"
  "src/parser/ZeroDisassembler.cpp"
  "src/parser/CCDisassembler.cpp"
  "src/parser/RNEDisassembler.cpp"
  "src/parser/RNECharset.cpp"
  "src/parser/SGHDCharset.cpp"
  "src/parser/SupportedGame.cpp")
target_compile_features(parser INTERFACE cxx_std_17)

add_executable(SCXParser src/SCXParser.cpp)
target_link_libraries(SCXParser PRIVATE parser)
target_compile_features(SCXParser PRIVATE cxx_std_17)

find_package(
  Qt6
  COMPONENTS Core Gui Widgets Sql Core5Compat
  REQUIRED)

add_executable(
  sc3ntist
  "src/gui/analysis.cpp"
  "src/gui/debuggerapplication.cpp"
  "src/gui/disassemblyitemdelegate.cpp"
  "src/gui/disassemblymodel.cpp"
  "src/gui/disassemblyview.cpp"
  "src/gui/main.cpp"
  "src/gui/mainwindow.cpp"
  "src/gui/memorymodel.cpp"
  "src/gui/memoryview.cpp"
  "src/gui/newprojectdialog.cpp"
  "src/gui/project.cpp"
  "src/gui/projectcontextprovider.cpp"
  "src/gui/textdump.cpp"
  "src/gui/viewhelper.cpp"
  "src/gui/worklistdialog.cpp"
  "src/gui/xrefdialog.cpp"
  "src/gui/main.qrc"
  "src/gui/mainwindow.ui"
  # Headers must be listed or automoc won't find them
  "src/gui/enums.h"
  "src/gui/debuggerapplication.h"
  "src/gui/disassemblyitemdelegate.h"
  "src/gui/disassemblymodel.h"
  "src/gui/disassemblyview.h"
  "src/gui/mainwindow.h"
  "src/gui/memorymodel.h"
  "src/gui/memoryview.h"
  "src/gui/newprojectdialog.h"
  "src/gui/project.h"
  "src/gui/worklistdialog.h"
  "src/gui/xrefdialog.h")

# There's some very naughty type casting going on
target_compile_options(sc3ntist PRIVATE "-fpermissive")

target_compile_features(sc3ntist PRIVATE cxx_std_17)
target_include_directories(sc3ntist PRIVATE "src/")

find_package(
  Qt5
  COMPONENTS Core Gui Widgets Sql
  REQUIRED)
target_link_libraries(sc3ntist PRIVATE parser Qt5::Core Qt5::Widgets Qt5::Gui
                                       Qt5::Sql)
