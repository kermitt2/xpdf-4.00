#========================================================================
#
# xpdf-qt/cmake-xpdf.txt
#
# CMake script for the xpdf application.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

if ((QT4_FOUND OR Qt5Widgets_FOUND)
    AND HAVE_SPLASH AND MULTITHREADED AND USE_EXCEPTIONS)
  include_directories("${PROJECT_SOURCE_DIR}")
  include_directories("${PROJECT_BINARY_DIR}")
  include_directories("${PROJECT_SOURCE_DIR}/goo")
  include_directories("${PROJECT_SOURCE_DIR}/fofi")
  include_directories("${PROJECT_SOURCE_DIR}/splash")
  include_directories("${PROJECT_SOURCE_DIR}/xpdf")
  if (QT4_FOUND)
    include(${QT_USE_FILE})
  else ()
    include_directories("${QT_INCLUDES}")
    add_definitions(${QT_DEFINITIONS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QT_CFLAGS}")
    if(POLICY CMP0020)
      # do not link to qtmain.lib
      cmake_policy(SET CMP0020 OLD)
    endif()
  endif ()

  if (XPDFWIDGET_PRINTING)
    if (WIN32)
      if (EXISTS ${PROJECT_SOURCE_DIR}/xpdf/WinPDFPrinter.cc)
        add_definitions(-DXPDFWIDGET_PRINTING=1)
        add_definitions(-DXPDFWIDGET_WIN32_PRINTING=1)
        set(XPDFWIDGET_PRINT_MODULES XpdfWidgetPrint.cc
                                     ${PROJECT_SOURCE_DIR}/xpdf/GDIOutputDev.cc
                                     ${PROJECT_SOURCE_DIR}/xpdf/WinPDFPrinter.cc)
      endif ()
    else ()
      add_definitions(-DXPDFWIDGET_PRINTING=1)
      set(XPDFWIDGET_PRINT_MODULES XpdfWidgetPrint.cc)
    endif ()
  else ()
    set(XPDFWIDGET_PRINT_MODULES "")
  endif ()

  if (QT4_FOUND)
    qt4_wrap_cpp(XPDFWIDGET_MOC XpdfWidget.h)
    qt4_wrap_cpp(XPDFVIEWER_MOC XpdfViewer.h)
    qt4_wrap_cpp(XPDFAPP_MOC XpdfApp.h)
    qt4_add_resources(XPDF_ICONS icons.qrc)
  else ()
    qt5_wrap_cpp(XPDFWIDGET_MOC XpdfWidget.h)
    qt5_wrap_cpp(XPDFVIEWER_MOC XpdfViewer.h)
    qt5_wrap_cpp(XPDFAPP_MOC XpdfApp.h)
    qt5_add_resources(XPDF_ICONS icons.qrc)
  endif ()

  # workaround for a conflict between the Qt5 QDateTime and windows.h
  if (WIN32)
    add_definitions(-DNOMINMAX)
  endif ()

  add_executable(xpdf
    $<TARGET_OBJECTS:goo_objs>
    $<TARGET_OBJECTS:fofi_objs>
    $<TARGET_OBJECTS:splash_objs>
    $<TARGET_OBJECTS:xpdf_objs>
    $<TARGET_OBJECTS:xpdf_widget_objs>
    ${XPDFWIDGET_PRINT_MODULES}
    QtPDFCore.cc
    XpdfApp.cc
    XpdfViewer.cc
    XpdfWidget.cc
    xpdf.cc
    xpdf.rc
    ${XPDFAPP_MOC}
    ${XPDFVIEWER_MOC}
    ${XPDFWIDGET_MOC}
    ${XPDF_ICONS}
  )
  target_link_libraries(xpdf ${QT_LIBRARIES} ${EXTRA_QT_LIBRARIES}
                        ${FREETYPE_LIBRARY} ${FREETYPE_OTHER_LIBS}
                        ${DTYPE_LIBRARY} ${LCMS_LIBRARY}
                        ${CMAKE_THREAD_LIBS_INIT})
  set_property(TARGET xpdf PROPERTY WIN32_EXECUTABLE 1)

  install(TARGETS xpdf RUNTIME DESTINATION bin)
  install(FILES ${PROJECT_SOURCE_DIR}/doc/xpdf.1 DESTINATION man/man1)
endif ()
