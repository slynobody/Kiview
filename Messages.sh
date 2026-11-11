#! /usr/bin/env bash
#SPDX-License-Identifier: GPL-3.0-or-later
#SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
$XGETTEXT $(find src -name '*.cpp' -o -name '*.h' -o -name '*.qml') -o $podir/kiview.pot
