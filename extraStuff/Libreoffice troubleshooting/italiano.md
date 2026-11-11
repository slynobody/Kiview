<!--
    SPDX-License-Identifier: CC-BY-SA-4.0
    SPDX-FileCopyrightText: 2024 Danilo Agostini <kyre334@gmail.com>
-->
# Requisiti:

Per vedere l'anterprima dei file doc,docx,xls,xlsx,ods,odp,ppt,pptx e odt è necessario avere libreoffice installato, tramite il gestore dei pacchetti o come flatpak.  
Con la versione flatpak, Kiview dà a libreoffice l'accesso alla cartella in cui converte il documento, quindi non è necessario cambiare i permessi di libreoffice.  
La versione flatpak di Kiview usa la versione flatpak di libreoffice installata nel sistema.

Libreoffice: https://flathub.org/apps/org.libreoffice.LibreOffice

# Errori:

## Error: source file could not be loaded.

Questo può accadere se stai usando la versione di libreoffice installata tramite gestore dei pacchetti e non hai installato tutti i componenti.
Per esempio, su ubuntu è necessario installare `libreoffice-writer` (docx,doc,odt), `libreoffice-calc` (xlsx,xls,csv,ods) e `libreoffice-impress` (odp,ppt,pptx).  
Se stai usando la versione flatpak, assicurati di non aver installato anche la versione del gestore dei pacchetti.

## Errore: Libreoffice non è installato.

Devi installare libreoffice per vedere l'anteprima.

## Errore: Il documento ha impiegato troppo tempo a caricare.

Viene mostrato se il file ha troppe pagine e impiega troppo tempo a caricare.

## Errore: File non trovato: Flatpak potrebbe non essere configurato correttamente.

Questo accade quando libreoffice ha convertito il documento ma ha scritto il PDF in una cartella che Kiview non può leggere.  
Se hai dato alla versione flatpak di libreoffice l'accesso alla cartella `/tmp` per una versione precedente di Kiview, quel permesso non è più necessario.






