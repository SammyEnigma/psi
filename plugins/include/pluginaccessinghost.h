#ifndef PLUGINACCESSINGHOST_H
#define PLUGINACCESSINGHOST_H

#include <QtPlugin>

class PsiPlugin;

class PluginAccessingHost {
public:
    virtual ~PluginAccessingHost() { }

    virtual QObject    *getPlugin(const QString &name) = 0;
    virtual QVariantMap selfMetadata() const           = 0;
};

Q_DECLARE_INTERFACE(PluginAccessingHost, "org.psi-im.PluginAccessingHost/0.2");

#endif // PLUGINACCESSINGHOST_H
