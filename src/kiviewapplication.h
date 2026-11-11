// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
#ifndef KIVIEWAPPLICATION_H
#define KIVIEWAPPLICATION_H

#include <AbstractKirigamiApplication>
#include <QQmlEngine>

// The actions of Kiview, whose shortcuts the user can change in the shortcuts editor.
// The Shortcut items of QML read their keys from the shortcuts property.
class KiviewApplication : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_NAMED_ELEMENT(KiviewApp)
    QML_SINGLETON

    // The keys of each action, by action name, as a list of key sequences in portable text.
    Q_PROPERTY(QVariantMap shortcuts READ shortcuts NOTIFY shortcutsChanged FINAL)

public:
    explicit KiviewApplication(QObject *parent);
    // Returns the instance that main() creates, which stays owned by C++.
    static KiviewApplication *create(QQmlEngine *, QJSEngine *);

    QVariantMap shortcuts() const;

    // Returns the key sequences, in portable text, as a list for the user, such as "E and Space".
    Q_INVOKABLE QString keysText(const QStringList &sequences) const;

Q_SIGNALS:
    void shortcutsChanged();

protected:
    void setupActions() override;
};

#endif // KIVIEWAPPLICATION_H
