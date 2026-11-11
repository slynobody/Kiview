#SPDX-License-Identifier: BSD-2-Clause
#SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
add_executable(kiview
)



target_sources(kiview PRIVATE main.cpp
    contentmanager.h contentmanager.cpp
    textviewer.h textviewer.cpp
    imageviewer.h imageviewer.cpp
    documentviewer.h documentviewer.cpp
    videoviewer.h videoviewer.cpp
    containerviewer.h containerviewer.cpp
    dbusinterface.h dbusinterface.cpp
    kiviewapplication.h kiviewapplication.cpp
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(kiview
    URI org.kde.kiview
    QML_FILES
        contents/ui/ContainerViewerUI.qml
        contents/ui/DocumentViewerUi.qml
        contents/ui/FallbackViewerUi.qml
        contents/ui/ImageViewerUi.qml
        contents/ui/InfoDialog.qml
        contents/ui/InfoDialogContent.qml
        contents/ui/Main.qml
        contents/ui/NavButtons.qml
        contents/ui/OpenAppButton.qml
        contents/ui/PdfDocumentView.qml
        contents/ui/TextViewerUi.qml
        contents/ui/VideoViewerUi.qml
        contents/ui/ViewersLoader.qml
)

ecm_qt_declare_logging_category(kiview
    HEADER kiview_debug.h
    IDENTIFIER KIVIEW_LOG
    CATEGORY_NAME org.kde.kiview
    DESCRIPTION "Kiview"
    EXPORT KIVIEW
)
ecm_qt_declare_logging_category(kiview
    HEADER kiview_document_debug.h
    IDENTIFIER KIVIEW_DOCUMENT_LOG
    CATEGORY_NAME org.kde.kiview.document
    DESCRIPTION "Kiview document conversion with LibreOffice"
    EXPORT KIVIEW
)
ecm_qt_install_logging_categories(EXPORT KIVIEW FILE kiview.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})


target_link_libraries(kiview PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::I18n
    KF6::I18nQml
    KF6::CoreAddons
    KF6::Archive
    KF6::DBusAddons
    KF6::KIOGui
    KF6::KIOWidgets
    KirigamiAddonsStatefulApp
    Qt6::DBus
    Qt6::Pdf
    Qt6::Multimedia

)

install(PROGRAMS io.github.nyre221.kiview.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES io.github.nyre221.kiview.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(TARGETS kiview ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

include(ECMGenerateDBusServiceFile)
ecm_generate_dbus_service_file(
    NAME org.kde.kiview
    EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/kiview --daemon"
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)
