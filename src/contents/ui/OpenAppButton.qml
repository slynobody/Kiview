// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kiview
import org.kde.kirigami as Kirigami

Kirigami.Action {
    icon.name: "document-open"
    onTriggered: Manager.openApp()
}
