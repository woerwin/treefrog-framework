/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QDateTime>
#include <THtmlParser>
#include "otamaconverter.h"
#include "otmparser.h"
#include "erbconverter.h"
#include "erbparser.h"
#include "viewconverter.h"

#define TF_ATTRIBUTE_NAME  QLatin1String("data-tf")
#define LEFT_DELIM   QString("<% ")
#define RIGHT_DELIM  QString(" %>")
#define DUMMY_LABEL1  QLatin1String("#dummy")
#define DUMMY_LABEL2  QLatin1String("@dummy")

QString devIni;
static QString replaceMarker;

QString generateErbPhrase(const QString &str, int echoOption)
{
    QString s = str;
    s.remove(QRegExp(";+$"));
    QString res = LEFT_DELIM;
    
    if (echoOption == OtmParser::None) {
        res += s;
    } else {
        switch (echoOption) {
        case OtmParser::NormalEcho:
            res += QLatin1String("echo(");
            break;
            
        case OtmParser::EscapeEcho:
            res += QLatin1String("eh(");
            break;
            
        case OtmParser::ExportVarEcho:
            res += QLatin1String("techoex(");
            break;
            
        case OtmParser::ExportVarEscapeEcho:
            res += QLatin1String("tehex(");
            break;
            
        default:
            qCritical("Internal error, bad option: %d", echoOption);
            return QString();
            break;
        }

        res += s;
        res += QLatin1String(");");
    }

    res += RIGHT_DELIM;
    return res;
}


OtamaConverter::OtamaConverter(const QDir &output, const QDir &helpers)
    : erbConverter(output, helpers)
{ }


OtamaConverter::~OtamaConverter()
{ }


bool OtamaConverter::convert(const QString &filePath) const
{
    QFile htmlFile(filePath);
    QFile otmFile(ViewConverter::changeFileExtension(filePath, logicFileSuffix()));
    QString className = ViewConverter::getViewClassName(filePath);
    QFile outFile(erbConverter.outputDir().filePath(className + ".cpp"));

    // Checks file's timestamp
    QFileInfo htmlFileInfo(htmlFile);
    QFileInfo otmFileInfo(otmFile);
    QFileInfo outFileInfo(outFile);
    if (htmlFileInfo.exists() && outFileInfo.exists()) {
        if (outFileInfo.lastModified() > htmlFileInfo.lastModified() 
             && (!otmFileInfo.exists() || outFileInfo.lastModified() > otmFileInfo.lastModified())) {
            //printf("done    %s\n", qPrintable(outFile.fileName()));
            return true;
        } else {
            if (outFile.remove()) {
                printf("  removed  %s\n", qPrintable(outFile.fileName()));
            }
        }
    }

    if (!htmlFile.open(QIODevice::ReadOnly)) {
        qCritical("failed to read phtm file : %s", qPrintable(htmlFile.fileName()));
        return false;
    }

    // Otama logic reading
    QString otm;
    if (otmFile.open(QIODevice::ReadOnly)) {
        otm = QTextStream(&otmFile).readAll();
    }
    
    QString erb = OtamaConverter::convertToErb(QTextStream(&htmlFile).readAll(), otm);
    return erbConverter.convert(className, erb);
}


QString OtamaConverter::convertToErb(const QString &html, const QString &otm)
{
    if (replaceMarker.isEmpty()) {
        // Sets a replace-marker
        QSettings devSetting(devIni, QSettings::IniFormat);
        replaceMarker = devSetting.value("Otama.ReplaceMarker", "%%").toString();
    }
    
    // Parses HTML and Otama files
    THtmlParser htmlParser;
    htmlParser.parse(html);

    OtmParser otmParser(replaceMarker);
    otmParser.parse(otm);

    // Inserts include-header
    QStringList inc = otmParser.includeStrings();
    for (QListIterator<QString> it(inc); it.hasNext(); ) {
        THtmlElement &e = htmlParser.insertNewElement(0, 0);
        e.text  = LEFT_DELIM.trimmed();
        e.text += it.next();
        e.text += RIGHT_DELIM;
    }

    // Inserts init-code
    QString init = otmParser.getInitSrcCode();
    if (!init.isEmpty()) {
        THtmlElement &e = htmlParser.at(0);
        e.text = LEFT_DELIM + init + RIGHT_DELIM + e.text;
    }

    for (int i = htmlParser.elementCount() - 1; i > 0; --i) {
        THtmlElement &e = htmlParser.at(i);
        QString label = e.attribute(TF_ATTRIBUTE_NAME);

        if (e.hasAttribute(TF_ATTRIBUTE_NAME)) {
            e.removeAttribute(TF_ATTRIBUTE_NAME);
        }

        if (label == DUMMY_LABEL1 || label == DUMMY_LABEL2) {
            htmlParser.removeElementTree(i);
            continue;
        }

        QString val;
        OtmParser::EchoOption ech;
        
        // Content assignment
        val = otmParser.getSrcCode(label, OtmParser::ContentAssignment, &ech); // ~ operator
        if (!val.isEmpty()) {
            htmlParser.removeChildElements(i);
            e.text = generateErbPhrase(val, ech);
        } else {
            QStringList vals = otmParser.getWrapSrcCode(label, OtmParser::ContentAssignment);
            if (!vals.isEmpty()) {
                // Adds block codes
                QString bak = e.text;
                e.text  = LEFT_DELIM;
                e.text += vals[0].trimmed();
                e.text += RIGHT_DELIM;
                e.text += bak;
                QString s = vals.value(1).trimmed();
                if (!s.isEmpty()) {
                    THtmlElement &child = htmlParser.appendNewElement(i);
                    child.text  = LEFT_DELIM;
                    child.text += s;
                    child.text += RIGHT_DELIM;
                }
            }
        }

        // Tag merging
        val = otmParser.getSrcCode(label, OtmParser::TagMerging, &ech); // |== operator
        if (!val.isEmpty()) {
            val.remove(QRegExp(";+$"));

            QString attr;
            attr  = LEFT_DELIM;
            attr += QLatin1String("do { THtmlParser ___pr = THtmlParser::mergeElements(tr(\"");
            attr += ErbConverter::escapeNewline(e.toString());
            attr += QLatin1String("\"), (");

            if (ech == OtmParser::ExportVarEcho) {  // if |==$ then
                attr += QLatin1String("T_VARIANT(");
                attr += val;
                attr += QLatin1String(")");
            } else {
                attr += val;
            }
            attr += QLatin1String(")); ");
            attr += QLatin1String("echo(___pr.at(1).attributesString()); ");
            attr += RIGHT_DELIM;
            e.attributes.clear();
            e.attributes << qMakePair(attr, QString());
            e.text  = LEFT_DELIM;
            e.text += QLatin1String("eh(___pr.at(1).text); ");
            e.text += QLatin1String("echo(___pr.childElementsToString(1)); } while (0);");
            e.text += RIGHT_DELIM;
        }

        // Sets attributes
        val = otmParser.getSrcCode(label, OtmParser::AttributeSet, &ech); // + operator
        if (!val.isEmpty()) {
            QString s = generateErbPhrase(val, ech);
            e.setAttribute(s, QString());
        }

        // Replaces the element
        val = otmParser.getSrcCode(label, OtmParser::TagReplacement, &ech); // : operator
        if (!val.isEmpty()) {
            // Sets the variable
            htmlParser.removeElementTree(i);
            e.text = generateErbPhrase(val, ech);
        } else {
            QStringList vals = otmParser.getWrapSrcCode(label, OtmParser::TagReplacement);
            if (!vals.isEmpty()) {
                // Adds block codes
                int idx = htmlParser.at(e.parent).children.indexOf(i);
                THtmlElement &he1 = htmlParser.insertNewElement(e.parent, idx);
                he1.text  = LEFT_DELIM;
                he1.text += vals[0].trimmed();
                he1.text += RIGHT_DELIM;

                QString s = vals.value(1).trimmed();
                if (!s.isEmpty()) {
                    THtmlElement &he2 = htmlParser.insertNewElement(e.parent, idx + 2);
                    he2.text  = LEFT_DELIM;
                    he2.text += s;
                    he2.text += RIGHT_DELIM;
                }
            }
        }
    }
 
    return htmlParser.toString();
}
