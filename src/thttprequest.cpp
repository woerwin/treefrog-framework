/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THttpRequest>
#include <TMultipartFormData>
#include <THttpUtility>
#include "tsystemglobal.h"

typedef QHash<QString, Tf::HttpMethod> MethodHash;

Q_GLOBAL_STATIC_WITH_INITIALIZER(MethodHash, methodHash,
{
    x->insert("get",     Tf::Get);
    x->insert("head",    Tf::Head);
    x->insert("post",    Tf::Post);
    x->insert("options", Tf::Options);
    x->insert("put",     Tf::Put);
    x->insert("delete",  Tf::Delete);
    x->insert("trace",   Tf::Trace);
})

/*!
  \class THttpRequest
  \brief The THttpRequest class contains request information for HTTP.
*/


THttpRequest::THttpRequest(const THttpRequest &other)
    : reqHeader(other.reqHeader),
      queryParams(other.queryParams),
      formParams(other.formParams),
      multiFormData(other.multiFormData)
{ }


THttpRequest::THttpRequest(const THttpRequestHeader &header, const QByteArray &body)
    : reqHeader(header)
{
    parseBody(body);
}


THttpRequest::THttpRequest(const QByteArray &header, const QByteArray &body)
    : reqHeader(header)
{
    parseBody(body);
}


THttpRequest::THttpRequest(const QByteArray &header, const QString &filePath)
    : reqHeader(header), multiFormData(filePath, boundary())
{ }


THttpRequest::~THttpRequest()
{ }


void THttpRequest::setRequest(const THttpRequestHeader &header, const QByteArray &body)
{
    reqHeader = header;
    parseBody(body);
}


void THttpRequest::setRequest(const QByteArray &header, const QByteArray &body)
{
    reqHeader = THttpRequestHeader(header);
    parseBody(body);
}


void THttpRequest::setRequest(const QByteArray &header, const QString &filePath)
{
    reqHeader = THttpRequestHeader(header);
    multiFormData = TMultipartFormData(filePath, boundary());
    formParams.unite(multiFormData.formItems());
}

/*!
  Returns the method.
 */
Tf::HttpMethod THttpRequest::method() const
{
    QString s = reqHeader.method().toLower();
    if (!methodHash()->contains(s)) {
        return Tf::Invalid;
    }
    return methodHash()->value(s);
}

/*!
  Returns the string value whose name is equal to \a name from the URL or the
  form data.
 */
QString THttpRequest::parameter(const QString &name) const
{
    return allParameters()[name].toString();
}

/*!
  \fn bool THttpRequest::hasQuery() const

  Returns true if the URL contains a Query.
 */

/*!
  Returns true if there is a query string pair whose name is equal to \a name
  from the URL.
 */
bool THttpRequest::hasQueryItem(const QString &name) const
{
    return queryParams.contains(name);
}

/*!
  Returns the query string value whose name is equal to \a name from the URL.
 */
QString THttpRequest::queryItemValue(const QString &name) const
{
    return queryParams.value(name).toString();
}

/*!
  This is an overloaded function.
  Returns the query string value whose name is equal to \a name from the URL.
  If the query string contains no item with the given \a name, the function
  returns \a defaultValue.
 */
QString THttpRequest::queryItemValue(const QString &name, const QString &defaultValue) const
{
    return queryParams.value(name, QVariant(defaultValue)).toString();
}

/*!
  Returns the list of query string values whose name is equal to \a name from
  the URL.
 */
QStringList THttpRequest::allQueryItemValues(const QString &name) const
{
    QStringList ret;
    QVariantList values = queryParams.values(name);
    for (QListIterator<QVariant> it(values); it.hasNext(); ) {
        ret << it.next().toString();
    }
    return ret;
}

/*!
  \fn QVariantHash THttpRequest::queryItems() const
  Returns the query string of the URL, as a hash of keys and values.
 */


/*!
  \fn bool THttpRequest::hasForm() const

  Returns true if the request contains form data.
 */


/*!
  Returns true if there is a string pair whose name is equal to \a name from
  the form data.
 */
bool THttpRequest::hasFormItem(const QString &name) const
{
    return formParams.contains(name);
}

/*!
  Returns the string value whose name is equal to \a name from the form data.
 */
QString THttpRequest::formItemValue(const QString &name) const
{
    return formParams.value(name).toString();
}

/*!
  This is an overloaded function.
  Returns the string value whose name is equal to \a name from the form data.
  If the form data contains no item with the given \a name, the function
  returns \a defaultValue.
 */
QString THttpRequest::formItemValue(const QString &name, const QString &defaultValue) const
{
    return formParams.value(name, QVariant(defaultValue)).toString();
}

/*!
  Returns the list of string value whose name is equal to \a name from the
  form data.
 */
QStringList THttpRequest::allFormItemValues(const QString &name) const
{
    QStringList ret;
    QVariantList values = formParams.values(name);
    for (QListIterator<QVariant> it(values); it.hasNext(); ) {
        ret << it.next().toString();
    }
    return ret;
}

/*!
  Returns the list of string value whose key is equal to \a key, such as
  "hoge[]", from the form data.
 */
QStringList THttpRequest::formItemList(const QString &key) const
{
    QString k = key;
    if (!k.endsWith("[]")) {
        k += QLatin1String("[]");
    }
    return allFormItemValues(k);
}

/*!
  Returns the hash of string value whose key is equal to \a key from
  the form data. Obsolete function.
 */
QHash<QString, QString> THttpRequest::formItemHash(const QString &key) const
{
    QHash<QString, QString> hash;
    QRegExp rx(QLatin1Char('^') + key + "\\[(.+)\\]$");
    for (QHashIterator<QString, QVariant> i(formParams); i.hasNext(); ) {
        i.next();
        if (rx.exactMatch(i.key())) {
            hash.insert(rx.cap(1), i.value().toString());
        }
    }
    return hash;
}

/*!
  Returns the hash of variant value whose key is equal to \a key from
  the form data.
 */
QVariantHash THttpRequest::formItems(const QString &key) const
{
    QVariantHash hash;
    QRegExp rx(QLatin1Char('^') + key + "\\[(.+)\\]$");
    for (QHashIterator<QString, QVariant> i(formParams); i.hasNext(); ) {
        i.next();
        if (rx.exactMatch(i.key())) {
            hash.insert(rx.cap(1), i.value());
        }
    }
    return hash;
}

/*!
  \fn QVariantHash THttpRequest::formItems() const
  
  Returns the hash of all form data.
 */

/*!

 */
void THttpRequest::parseBody(const QByteArray &body)
{
    switch (method()) {
    case Tf::Post:
        // form parameter
        if (!body.isEmpty()) {
            QList<QByteArray> formdata = body.split('&');
            for (QListIterator<QByteArray> i(formdata); i.hasNext(); ) {
                QList<QByteArray> nameval = i.next().split('=');
                if (!nameval.value(0).isEmpty()) {
                    // URL decode
                    QString key = THttpUtility::fromUrlEncoding(nameval.value(0));
                    QString val = THttpUtility::fromUrlEncoding(nameval.value(1));
		    formParams.insertMulti(key, val);
                    tSystemDebug("POST Hash << %s : %s", qPrintable(key), qPrintable(val));
                }
            }
        }
        // fallthrough

    case Tf::Get: {
        // query parameter
        QList<QByteArray> data = reqHeader.path().split('?');
        QString getdata = data.value(1);
        if (!getdata.isEmpty()) {
            QStringList pairs = getdata.split('&', QString::SkipEmptyParts);
            for (QStringListIterator i(pairs); i.hasNext(); ) {
                QStringList s = i.next().split('=');
                if (!s.value(0).isEmpty()) {
                    QString key = THttpUtility::fromUrlEncoding(s.value(0).toLatin1());
                    QString val = THttpUtility::fromUrlEncoding(s.value(1).toLatin1());
		    queryParams.insertMulti(key, val);
                    tSystemDebug("GET Hash << %s : %s", qPrintable(key), qPrintable(val));
                }
            }
        }
        break; }
                
    default:
        // do nothing
        break;
    }
}


QByteArray THttpRequest::boundary() const
{
    QByteArray boundary;
    QString contentType = reqHeader.rawHeader("content-type").trimmed();

    if (contentType.startsWith("multipart/form-data", Qt::CaseInsensitive)) {
        QStringList lst = contentType.split(QChar(';'), QString::SkipEmptyParts, Qt::CaseSensitive);
        for (QStringListIterator it(lst); it.hasNext(); ) {
            QString string = it.next().trimmed();
            if (string.startsWith("boundary=", Qt::CaseInsensitive)) {
                boundary  = "--";
                boundary += string.mid(9).toLatin1();
                break;
            }
        }
    }
    return boundary;
}

/*!
  Returns the cookie associated with the name.
 */
QByteArray THttpRequest::cookie(const QString &name) const
{
    QList<TCookie> list = cookies();
    for (QListIterator<TCookie> i(list); i.hasNext(); ) {
        const TCookie &c = i.next();
        if (c.name() == name) {
            return c.value();
        }
    }
    return QByteArray();
}

/*!
  Returns the all cookies.
 */
QList<TCookie> THttpRequest::cookies() const
{
    QList<TCookie> result;
    QList<QByteArray> cookieStrings = reqHeader.rawHeader("Cookie").split(';');
    for (QListIterator<QByteArray> i(cookieStrings); i.hasNext(); ) {
        QByteArray ba = i.next().trimmed();
        if (!ba.isEmpty())
            result += TCookie::parseCookies(ba);
    }
    return result;
}


/*!
  \fn QVariantHash THttpRequest::allParameters() const
  
  Returns a hash of all form data.
 */
QVariantHash THttpRequest::allParameters() const
{
    QVariantHash params = queryParams;
    return params.unite(formParams);
}


/*!
  \fn TMultipartFormData &THttpRequest::multipartFormData()
  
  Returns a object of multipart/form-data.
 */
