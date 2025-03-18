# Qt Widgets for Technical Applications
# available at http://www.http://qwt.sourceforge.net/
#
# The module defines the following variables:
#  Qwt_FOUND - the system has Qwt
#  QWT_INCLUDE_DIR - where to find qwt_plot.h
#  QWT_INCLUDE_DIRS - qwt includes
#  QWT_LIBRARY - where to find the Qwt library
#  QWT_LIBRARIES - aditional libraries
#  QWT_MAJOR_VERSION - major version
#  QWT_MINOR_VERSION - minor version
#  QWT_PATCH_VERSION - patch version
#  QWT_VERSION_STRING - version (ex. 5.2.1)
#  QWT_ROOT_DIR - root dir (ex. /usr/local)
#
# It also defines this imported target:
#  Qwt::Qwt

#=============================================================================
# Copyright 2010-2013, Julien Schueller
# Copyright 2018-2020, Rolf Eike Beer
# Copyright 2022, Olaf Mandel
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# The views and conclusions contained in the software and documentation are those
# of the authors and should not be interpreted as representing official policies,
# either expressed or implied, of the FreeBSD Project.
#=============================================================================

# Match our searches below to the used Qt::Gui version
foreach (v IN ITEMS 6 5)
  set(lib "Qt${v}::Gui")
  if (NOT TARGET "${lib}")
    continue ()
  endif ()

  get_target_property(incdir "${lib}" INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(libdir "${lib}" INTERFACE_LINK_DIRECTORIES)
  if (NOT libdir)
    set(libdir "${incdir}")
    list(FILTER libdir INCLUDE REGEX "/include/?$")
    list(TRANSFORM libdir REPLACE "include/?$" "lib")
  endif ()

  get_target_property(confs "${lib}" IMPORTED_CONFIGURATIONS)
  foreach (c IN LISTS confs)
    get_target_property(loc "${lib}" IMPORTED_LOCATION_${c})
    if (NOT libdir)
      string(REGEX REPLACE "/[^/]*$" "" libdir "${loc}")
      string(REGEX REPLACE "/bin$" "/lib" libdir "${libdir}")
    endif ()
    string(TOLOWER "${loc}" loc)
    if (loc MATCHES "qt[56]gui")
      break ()
    endif ()
  endforeach ()
  unset(confs)
  unset(loc)

  if (incdir AND libdir)
    set(_QWT_QT_INCLUDE_DIR "${incdir}")
    set(_QWT_QT_LINK_DIR "${libdir}")
    set(_QWT_QT_VERSION "${v}")
    break ()
  endif ()
endforeach ()
unset(lib)
unset(incdir)
unset(libdir)

find_path ( QWT_INCLUDE_DIR
  NAMES qwt_plot.h
  HINTS ${_QWT_QT_INCLUDE_DIR}
  PATH_SUFFIXES qwt-qt${_QWT_QT_VERSION} qwt qwt6
  NO_DEFAULT_PATH
)
if (NOT QWT_INCLUDE_DIR)
  find_path ( QWT_INCLUDE_DIR
    NAMES qwt_plot.h
    HINTS ${_QWT_QT_INCLUDE_DIR}
    PATH_SUFFIXES qwt-qt${_QWT_QT_VERSION} qwt qwt6
  )
endif ()
unset(_QWT_QT_INCLUDE_DIR)

set ( QWT_INCLUDE_DIRS ${QWT_INCLUDE_DIR} )

# version
set ( _VERSION_FILE ${QWT_INCLUDE_DIR}/qwt_global.h )
if ( EXISTS ${_VERSION_FILE} )
  file ( STRINGS ${_VERSION_FILE} _VERSION_LINE REGEX "define[ ]+QWT_VERSION_STR" )
  if ( _VERSION_LINE )
    string ( REGEX REPLACE ".*define[ ]+QWT_VERSION_STR[ ]+\"([^\"]*)\".*" "\\1" QWT_VERSION_STRING "${_VERSION_LINE}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" QWT_MAJOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" QWT_MINOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" QWT_PATCH_VERSION "${QWT_VERSION_STRING}" )
  endif ()
  unset ( _VERSION_LINE )
endif ()
unset ( _VERSION_FILE )

find_library ( QWT_LIBRARY
  NAMES qwt-qt${_QWT_QT_VERSION} qwt
  HINTS ${_QWT_QT_LINK_DIR}
)
unset(_QWT_QT_LINK_DIR)
unset(_QWT_QT_VERSION)

set ( QWT_LIBRARIES ${QWT_LIBRARY} )

# try to guess root dir from include dir
if ( QWT_INCLUDE_DIR )
  string ( REGEX REPLACE "(.*)/include.*" "\\1" QWT_ROOT_DIR ${QWT_INCLUDE_DIR} )
# try to guess root dir from library dir
elseif ( QWT_LIBRARY )
  string ( REGEX REPLACE "(.*)/lib[/|32|64].*" "\\1" QWT_ROOT_DIR ${QWT_LIBRARY} )
endif ()

include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Qwt REQUIRED_VARS QWT_LIBRARY QWT_INCLUDE_DIR VERSION_VAR QWT_VERSION_STRING )

if (Qwt_FOUND AND NOT TARGET Qwt::Qwt)
  add_library(Qwt::Qwt UNKNOWN IMPORTED)
  set_target_properties(Qwt::Qwt PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${QWT_LIBRARIES}")
  if (QWT_LIBRARY MATCHES [[\.(dll(\.a)?|lib|so)$]])  # shared library?
    set_target_properties(Qwt::Qwt PROPERTIES
                          INTERFACE_COMPILE_DEFINITIONS QWT_DLL)
  endif ()
endif ()

mark_as_advanced (
  QWT_LIBRARY
  QWT_LIBRARIES
  QWT_INCLUDE_DIR
  QWT_INCLUDE_DIRS
  QWT_MAJOR_VERSION
  QWT_MINOR_VERSION
  QWT_PATCH_VERSION
  QWT_VERSION_STRING
  QWT_ROOT_DIR
)
