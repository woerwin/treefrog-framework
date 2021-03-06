#ifndef TMULTIPARTFORMDATA_H
#define TMULTIPARTFORMDATA_H

#include <QStringList>
#include <QHash>
#include <QPair>
#include <TGlobal>

class QIODevice;


class T_CORE_EXPORT TMimeHeader
{
public:
    TMimeHeader() { }
    TMimeHeader(const TMimeHeader &other);
    bool isEmpty() const { return headers.isEmpty(); }
    QByteArray header(const QByteArray &headerName) const;
    void setHeader(const QByteArray &headerName, const QByteArray &value);
    QByteArray contentDispositionParameter(const QByteArray &name) const;
    QByteArray dataName() const;
    QByteArray originalFileName() const;

protected:
    static int skipWhitespace(const QByteArray &text, int pos);
    static QHash<QByteArray, QByteArray> parseHeaderParameter(const QByteArray &header);

private:
    QList<QPair<QByteArray, QByteArray> >  headers;
};


class T_CORE_EXPORT TMimeEntity : protected QPair<TMimeHeader, QString>
{
public:
    TMimeEntity() { }
    TMimeEntity(const TMimeEntity &other);

    const TMimeHeader &header() const { return first; }
    TMimeHeader &header() { return first; }    
    QByteArray header(const QByteArray &headerName) const { return first.header(headerName); }
    QByteArray dataName() const { return first.dataName(); }
    QString contentType() const;
    qint64 fileSize() const;
    QByteArray originalFileName() const { return first.originalFileName(); }
    bool renameUploadedFile(const QString &newName, bool overwrite = false);
    QString uploadedFilePath() const;

private:
    TMimeEntity(const TMimeHeader &header, const QString &body);

    friend class TMultipartFormData;
};


class T_CORE_EXPORT TMultipartFormData
{
public:
    TMultipartFormData(const QByteArray &boundary = QByteArray());
    TMultipartFormData(const QByteArray &formData, const QByteArray &boundary);
    TMultipartFormData(const QString &bodyFilePath, const QByteArray &boundary);
    ~TMultipartFormData() { }

    bool isEmpty() const;
    bool hasFormItem(const QString &name) const { return postParameters.contains(name); }
    QString formItemValue(const QString &name) const { return postParameters.value(name).toString(); }
    QStringList allFormItemValues(const QString &name) const;
    const QVariantHash &formItems() const { return postParameters; }

    QString contentType(const QByteArray &dataName) const;
    QString originalFileName(const QByteArray &dataName) const;
    qint64 size(const QByteArray &dataName) const;
    bool renameUploadedFile(const QByteArray &dataName, const QString &newName, bool overwrite = false);

    TMimeEntity entity(const QByteArray &dataName) const;
    QList<TMimeEntity> entityList(const QByteArray &dataName) const;

protected:
    void parse(QIODevice *data);
    TMimeHeader parseMimeHeader(QIODevice *data) const;
    QByteArray parseContent(QIODevice *data) const;
    QString writeContent(QIODevice *data) const;

private:
    QByteArray dataBoundary;
    QVariantHash postParameters;
    QList<TMimeEntity> uploadedFiles;
};


Q_DECLARE_METATYPE(TMimeHeader)
Q_DECLARE_METATYPE(TMimeEntity)
Q_DECLARE_METATYPE(TMultipartFormData)

#endif // TMULTIPARTFORMDATA_H
