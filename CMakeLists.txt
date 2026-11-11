#SPDX-License-Identifier: BSD-2-Clause
#SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>

cmake_minimum_required(VERSION 3.16)
project(kiview VERSION 1.1)

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.28.0")


find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMFindQmlModule)
include(KDEClangFormat)
include(KDEGitCommitHooks)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Quick Test Gui QuickControls2 Widgets Pdf DBus Multimedia)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n Kirigami Archive Config CoreAddons DBusAddons KIO)
find_package(KF6KirigamiAddons 1.4.0 REQUIRED)
ecm_find_qmlmodule(org.kde.syntaxhighlighting 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(icons)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ki18n_install(po)
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

