<!--
    SPDX-License-Identifier: CC-BY-SA-4.0
    SPDX-FileCopyrightText: 2024 Danilo Agostini <kyre334@gmail.com>
-->
# Requirements:

To preview doc, docx, xls, xlsx, ods, odp, ppt, pptx and odt files you need to have libreoffice installed, from your package manager or as a flatpak.  
With the flatpak version, Kiview gives libreoffice access to the folder that it converts the document into, so you do not need to change the permissions of libreoffice.  
The flatpak version of Kiview uses the flatpak version of libreoffice that is installed on the system.

Libreoffice: https://flathub.org/apps/org.libreoffice.LibreOffice


# Errors:

## Error: source file could not be loaded.

This can happen if you are using the version of libreoffice installed via package manager and you have not installed all components.  
For example, on ubuntu you need to install `libreoffice-writer` (docx,doc,odt), `libreoffice-calc` (xlsx,xls,csv,ods) and `libreoffice-impress` (odp,ppt,pptx).  
If you are using the flatpak version, make sure you don't also have the package manager version installed.

## Error: Libreoffice is not installed.

You need to install libreoffice to see the preview.

## Error: The document took too long to load.

This is shown if the file has too many pages and is taking too long to load.

## Error: File not found: Flatpak may not be configured correctly.

This happens when libreoffice converted the document but wrote the PDF to a folder that Kiview cannot read.  
If you gave the flatpak version of libreoffice access to the `/tmp` folder for an older version of Kiview, that permission is no longer needed.
