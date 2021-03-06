#ifndef TACCESSAUTHENTICATOR_H
#define TACCESSAUTHENTICATOR_H

#include <QString>
#include <QStringList>
#include <QList>
#include <TGlobal>

class TAbstractUser;


class T_CORE_EXPORT TAccessAuthenticator
{
public:
    TAccessAuthenticator();
    virtual ~TAccessAuthenticator() { }

    void setAllowAll(bool allow) { allowAll = allow; }
    void setDenyAll(bool deny) { allowAll = !deny; }
    void setAllowGroup(const QString &groupKey, const QString &action);
    void setAllowGroup(const QString &groupKey, const QStringList &actions);
    void setDenyGroup(const QString &groupKey, const QString &action);
    void setDenyGroup(const QString &groupKey, const QStringList &actions);
    void setAllowUser(const QString &identityKey, const QString &action);
    void setAllowUser(const QString &identityKey, const QStringList &actions);
    void setDenyUser(const QString &identityKey, const QString &action);
    void setDenyUser(const QString &identityKey, const QStringList &actions);
    virtual bool authenticate(const TAbstractUser *user) const;
    void clear();
    
protected:
    void addRules(int type, const QString &key, const QStringList &actions, bool allow);

    class AccessRule
    {
    public:
        enum Type {
            Group = 0,
            User,
        };

        AccessRule(int t, const QString &k, const QString &act, bool alw)
            : type(t), key(k), action(act), allow(alw) { }
        
        int type;
        QString key;
        QString action;
        bool allow;
    };

    bool allowAll;
    QList<AccessRule> accessRules;
};

#endif // TACCESSAUTHENTICATOR
