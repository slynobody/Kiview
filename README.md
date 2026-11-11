<!--
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2024 Danilo Agostini <kyre334@gmail.com>
-->

# Kiview  
## cloned from https://invent.kde.org/danagost/Kiview (poc: working in dolphin)
Kiview gives you ability to quickly preview different file types in dolphin through spacebar (without needing to launch default application).

### Supported file types:
##### Text Files:
txt, sh, json, md
##### Documents:
pdf, doc, docx, odt, ods, xlsx, xls, csv, odp, ppt, pptx
##### Images:
png, jpg, jpeg, kra, svgz, svg, webp, jxl
##### Audio/Video Files:
mp4, mp3, webm, mkv, m4a, m4v
##### Archives:
zip, gz, xz, 7z

## Installation (debian)
sudo apt install build-essential cmake extra-cmake-modules gettext qt6-declarative-dev qt6-base-dev  libkf6i18n-dev libkf6coreaddons-dev qml-module-org-kde-qqc2desktopstyle kf6-kirigami-dev qt6-webengine-dev gstreamer1.0-libav libva-dev kf6-karchive-dev libkf6dbusaddons-dev kirigami-addons-dev
./install.sh

## Installation (other)
dependencies: [Link](Y>>>-/blob/master/requirements(DEV%20ONLY).txt?ref_type=heads)  

## after installation:
restart dolphin
Dolphin >> Settings >> 'configure keyboard layouts' >> 'Kiview' >> link it with key (f.e. spacebar)

## How it works

press linked key (f.e. <space>) in dolphin on any supported file to show preview, <space> again to close preview
press linked key (f.e. <space>) in dolphin in any folder to get an overview, <space> again to close preview

### From the terminal:  
This shows the preview of the file:   
`kiview -s /home/user/some_file_or_folder`  
This shows an overview of the working directory:  
`kiview /home/user/some_folder`

`-s` means selection and will be used by dolphin when the user selects a file or folder. If the user has not selected anything, then `-s` will not be used and dolphin/kiview will simply show an overview of the currently open folder in dolphin.

**In simple words:**  
This is like when you invoke the shortcut after selecting a file:  
`kiview -s /home/user/some_file_or_folder`  
This is like when you invoke the shortcut when nothing has been selected:  
`kiview /home/user/some_folder`  

#### Examples:
![Screenshot-30-11-2023-CET-9](https://github.com/Nyre221/Kiview/assets/104171042/4e06a068-3d51-4b57-adcf-f42693b92e18)

![Screenshot-30-11-2023-CET-10](https://github.com/Nyre221/Kiview/assets/104171042/37b1fb2c-5e89-48fb-b5cf-25750734fd14)

![screenshot1](https://github.com/Nyre221/Kiview/assets/104171042/69f0806b-7c17-487d-a5dd-e4a6892b2b31)

