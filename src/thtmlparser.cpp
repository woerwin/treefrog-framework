/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THtmlParser>
#include <THttpUtility>


THtmlElement::THtmlElement()
    :  tagClosed(false), parent(0)
{ }


bool THtmlElement::isEmpty() const
{
    return tag.isEmpty() && text.isEmpty() && attributes.isEmpty();
}


bool THtmlElement::isEndElement() const
{
    return children.isEmpty();
}


bool THtmlElement::hasAttribute(const QString &name) const
{
    for (int i = 0; i < attributes.size(); ++i) {
        if (attributes.at(i).first == name) {
            return true;
        }
    }
    return false;
}


QString THtmlElement::attribute(const QString &name, const QString &defaultValue) const
{
    for (int i = 0; i < attributes.size(); ++i) {
        if (attributes.at(i).first == name) {
            return THttpUtility::trimmedQuotes(attributes.at(i).second);
        }
    }
    return defaultValue;
}


void THtmlElement::setAttribute(const QString &name, const QString &value)
{
    QString v = (!value.isNull()) ? QLatin1Char('"') + value + QLatin1Char('"') : value;
    for (int i = 0; i < attributes.size(); ++i) {
        if (attributes[i].first == name) {
            attributes[i].second = v;
            return;
        }
    }

    attributes << qMakePair(name, v);
}


void THtmlElement::removeAttribute(const QString &name)
{
    for (QMutableListIterator<QPair<QString, QString> > i(attributes); i.hasNext(); ) {
        if (i.next().first == name) {
            i.remove();
        }
    }
}


void THtmlElement::clear()
{
    tag.clear();
    attributes.clear();
    text.clear();
    selfCloseMark.clear();
    tagClosed = false;
    children.clear();
}


QString THtmlElement::attributesString() const
{
    QString string;   
    if (!tag.isEmpty()) {
        for (int i = 0; i < attributes.count(); ++i) {
            const QPair<QString, QString> &attr = attributes.at(i);
            if (!attr.first[0].isSpace() && !string.isEmpty() &&
                !string[string.length() - 1].isSpace()) {
                string += QLatin1Char(' ');
            }
            string += attr.first;
            if (!attr.second.isEmpty()) {
                string += QLatin1Char('=');
                string += attr.second;
            }
        }
    }
    return string;
}


QString THtmlElement::toString() const
{
    QString string;

    if (!tag.isEmpty()) {
        string += QLatin1Char('<');
        string += tag;
        
        if (!attributes.isEmpty()) {
            string += QLatin1Char(' ');
            string += attributesString();
        }
    
        // tag self-closing
        string += selfCloseMark;
        string += QLatin1Char('>');
    }
    string += text;
    return string;
}


THtmlParser::THtmlParser()
    : pos(0)
{
    elements.resize(1);
}


bool THtmlParser::hasPrefix(const QString &str, int offset) const
{
    return (pos + offset >= 0 && pos + offset + str.length() - 1 < txt.length()
            && txt.midRef(pos + offset, str.length()) == str);
}


bool THtmlParser::isElementClosed(int i) const
{
    if (i == 0)
        return true;

    const THtmlElement &n = at(i);
    if (n.tagClosed || !n.selfCloseMark.isEmpty()) {
        return true;
    }

    QString tag = n.tag.toLower();
    if (tag == QLatin1String("img") || tag == QLatin1String("hr")
        || tag == QLatin1String("br") || tag == QLatin1String("meta")) {
        return  true;
    }

    return false;
}


void THtmlParser::parse(const QString &text)
{
    elements.clear();
    elements.resize(1);
    txt = text;
    pos = 0;
    
    parse();
    //dumpHtml();
}


void THtmlParser::parse()
{
    while (pos < txt.length()) {
        QChar c = txt.at(pos++);
        if (c == QLatin1Char('<')) {
            parseTag();
        } else {
            last().text += c;
        }
    }
}


void THtmlParser::skipWhiteSpace(int *crCount, int *lfCount)
{
    if (crCount)
        *crCount = 0;

    if (lfCount)
        *lfCount = 0;

    for ( ; pos < txt.length(); ++pos) {
        QChar c = txt.at(pos);
        if (!c.isSpace()) {
            break;
        }

        if (c == QLatin1Char('\r') && crCount) {
            ++(*crCount);
        } else if (c == QLatin1Char('\n') && lfCount) {
            ++(*lfCount);
        }
    }
}


void THtmlParser::skipUpTo(const QString &str)
{
    int i = txt.indexOf(str, pos);
    pos = (i >= 0) ? i + str.length() : txt.length();
}


void THtmlParser::parseTag()
{
    skipWhiteSpace();

    // Exclamation mark declarations
    if (txt.at(pos) == QLatin1Char('!')) {
        parseExclamationTag();
        return;
    }

    // Question mark declarations
    if (txt.at(pos) == QLatin1Char('?')) {
        parseQuestionTag();
        return;
    }

    // Close-tag
    if (txt.at(pos) == QLatin1Char('/')) {
        parseCloseTag();
        return;
    }

    // p : parent index
    int p = lastIndex();
    while (p > 0) {
        if (!at(p).tag.isEmpty() && !isElementClosed(p)) {
            break;
        }
        p = at(p).parent;
    }

    THtmlElement &he = appendNewElement(p);
    he.tag = parseWord();

    // Parses the attributes
    he.attributes.clear();
    if (pos < txt.length() && txt.at(pos).isSpace()) {
        he.attributes = parseAttributes();
    }

    // Tag closed?
    for ( ; pos < txt.length(); ++pos) {
        if (txt.at(pos) == QLatin1Char('/')) {
            he.selfCloseMark = (hasPrefix(" /", -1)) ? QLatin1String(" /") : QLatin1String("/");
        }
        
        if (txt.at(pos) == QLatin1Char('>')) {
            ++pos;
            break;
        }
    }

    if (isElementClosed(lastIndex())) {
        appendNewElement(he.parent);
    }
}


void THtmlParser::parseCloseTag()
{
    ++pos;
    skipWhiteSpace();
    QString tag = parseWord();
    skipUpTo(">");
    
    // Finds corresponding open element
    int i = lastIndex();
    while (i > 0) {
        if (!tag.isEmpty() && at(i).tag.toLower() == tag.toLower() && !isElementClosed(i)) {
            break;
        }
        i = at(i).parent;
    }

    if (i > 0) {
        at(i).tagClosed = true;
    } else {
        // Can't find a corresponding open element
        last().text += QLatin1String("</");
        last().text += tag;
        last().text += QLatin1Char('>');
        return;
    }

    // Append a empty element for next entry
    int p = at(i).parent;
    appendNewElement(p);
}


QList<QPair<QString, QString> > THtmlParser::parseAttributes()
{
    QList<QPair<QString, QString> > attrs;

    while (pos < txt.length()) {
        int cr = 0, lf = 0;
        skipWhiteSpace(&cr, &lf);
        if (txt.at(pos) == QLatin1Char('>') || txt.at(pos) == QLatin1Char('/')) {
            break;
        }

        // Newline
        if (lf > 0) {
            QString newline = (lf == cr) ? QLatin1String("\r\n") : QLatin1String("\n");
            attrs << qMakePair(newline, QString());  // Appends the newline as a attribute
        }
        
        // Appends the key-value
        QString key = parseWord();
        if (key.isEmpty()) {
            break;
        }

        skipWhiteSpace();
        QString value;
        if (pos < txt.length() && txt.at(pos) == QLatin1Char('=')) {
            pos++;
            skipWhiteSpace();
            value = parseWord();
        }

        attrs << qMakePair(key, value);
    }

    return attrs;
}


void THtmlParser::appendTextToLastElement(const QString &upto)
{
    int end = txt.indexOf(upto, pos);
    if (end >= 0) {
        last().text += txt.midRef(pos, end + upto.length() - pos);
        pos = end + upto.length();
    } else {
        last().text += txt.midRef(pos);
        pos = txt.length();
    }
}


void THtmlParser::parseExclamationTag()
{
    last().text += QLatin1String("<!");
    ++pos;
    
    if (hasPrefix("--")) {
        appendTextToLastElement("-->");
    } else {
        appendTextToLastElement(">");
    }
}


void THtmlParser::parseQuestionTag()
{
    last().text += QLatin1String("<?");
    ++pos;
    appendTextToLastElement("?>");
}


THtmlElement &THtmlParser::appendNewElement(int parent)
{
    return insertNewElement(parent, -1);
}


THtmlElement &THtmlParser::insertNewElement(int parent, int index)
{
    if (elementCount() > 1 && last().isEmpty()) {
        // Re-use element
        changeParent(lastIndex(), parent, index);
    } else {
        elements.resize(elements.size() + 1);
        last().parent = parent;
        if (index >= 0 && index < elements[parent].children.count()) {
            elements[parent].children.insert(index, lastIndex());
        } else {
            elements[parent].children.append(lastIndex());
        }
    }
    return last();
}


void THtmlParser::removeElementTree(int index)
{
    removeChildElements(index);
    at(index).clear();
}


void THtmlParser::removeChildElements(int index)
{
    for (int i = 0; i < at(index).children.count(); ++i) {
        removeElementTree(at(index).children[i]);
    }
}


void THtmlParser::changeParent(int index, int newParent, int newIndex)
{
    THtmlElement &e = at(index);
    int i = elements[e.parent].children.indexOf(index);
    if (i >= 0) {
        elements[e.parent].children.remove(i);
    }
    e.parent = newParent;

    if (newIndex >= 0 && newIndex < elements[newParent].children.count()) {
        elements[newParent].children.insert(newIndex, index);
    } else {
        elements[newParent].children.append(index);
    }
}


// Parses one word
QString THtmlParser::parseWord()
{
    QString word;
    QChar c = txt.at(pos);
    if (c == QLatin1Char('\"')) { // double quotes
        word += c;
        ++pos;
        for ( ; pos < txt.length(); ++pos) {
            word += txt.at(pos);
            if (txt.at(pos) == QLatin1Char('\"') && txt.at(pos - 1) != QLatin1Char('\\')) {
                ++pos;
                break;
            }
        }
    } else if (c == QLatin1Char('\'')) { // single quotes
        word += c;
        ++pos;
        for ( ; pos < txt.length(); ++pos) {
            word += txt.at(pos);
            if (txt.at(pos) == QLatin1Char('\'') && txt.at(pos - 1) != QLatin1Char('\\')) {
                ++pos;
                break;
            }
        }
    } else {
        for ( ; pos < txt.length(); ++pos) {
            c = txt.at(pos);
            if (hasPrefix("/>") || c == QLatin1Char('>') || c == QLatin1Char('<')
                || c == QLatin1Char('=') || c.isSpace()) {
                break;
            }
            word += c;
        }
    }
    return word;
}


int THtmlParser::depth(int i) const
{
    int depth = 0;
    while (i > 0) {
        i = at(i).parent;
        ++depth;
    }
    return depth;
}


QString THtmlParser::toString() const
{
    return elementsToString(0); 
}


QString THtmlParser::elementsToString(int index) const
{
    QString string;
    const THtmlElement &e = at(index);
    string = e.toString();
    string += childElementsToString(index);

    if (!e.tag.isEmpty() && e.tagClosed) {
        string += QLatin1String("</");
        string += e.tag;
        string += QLatin1Char('>');
    }

    return string; 
}


QString THtmlParser::childElementsToString(int index) const
{
    QString string;
    const THtmlElement &e = at(index);
    for (int i = 0; i < e.children.count(); ++i) {
        string += elementsToString(e.children[i]);
    }
    return string;    
}


// void THtmlParser::dumpHtml() const
// {
//     for (int i = 0; i < elements.count(); ++i) {
//         tSystemDebug("%s:%s:%d:%s", qPrintable(QString(depth(i) * 4, QLatin1Char(' '))), qPrintable(at(i).tag), at(i).children.count(), qPrintable(at(i).text));
//     }
// }


THtmlParser THtmlParser::mid(int index) const
{
    THtmlParser res;
    if (at(index).isEndElement()) {
        res.elements << elements[index];
        res.at(0).children.append(1);
        res.at(1).parent = 0;
    } else {
        res.elements += elements.mid(index);
        res.at(0).children.append(1);
        int d = index - 1;
        for (int i = 1; i < res.elementCount(); ++i) {
            res.at(i).parent -= d;
            for (int j = 0; j < res.at(i).children.count(); ++j) {
                res.at(i).children[j] -= d;
            }
        }
    }
    return res;
}


void THtmlParser::append(int parent, const THtmlParser &parser)
{
    if (parser.elementCount() <= 1)
        return;

    THtmlElement &e = appendNewElement(parent);
    e.tag = parser.at(1).tag;
    e.attributes = parser.at(1).attributes;
    e.text = parser.at(1).text;
    e.selfCloseMark = parser.at(1).selfCloseMark;
    e.tagClosed = parser.at(1).tagClosed;
    int idx = lastIndex();
    for (int i = 0; i < parser.at(1).children.count(); ++i) {
        append(idx, parser.mid(parser.at(1).children[i]));
    }
}


void THtmlParser::prepend(int parent, const THtmlParser &parser)
{
    if (parser.elementCount() <= 1)
        return;

    THtmlElement &e = insertNewElement(parent, 0);
    e.tag = parser.at(1).tag;
    e.attributes = parser.at(1).attributes;
    e.text = parser.at(1).text;
    e.selfCloseMark = parser.at(1).selfCloseMark;
    e.tagClosed = parser.at(1).tagClosed;
    int idx = lastIndex();
    for (int i = 0; i < parser.at(1).children.count(); ++i) {
        prepend(idx, parser.mid(parser.at(1).children[i]));
    }
}


void THtmlParser::merge(const THtmlParser &other)
{
    if (elementCount() <= 1 || other.elementCount() <= 1 || at(1).tag != other.at(1).tag) {
        return;
    }
    
    // Adds attributes
    for (int i = 0; i < other.at(1).attributes.count(); ++i) {
        const QPair<QString, QString> &p = other.at(1).attributes[i];
        at(1).setAttribute(p.first, THttpUtility::trimmedQuotes(p.second));
    }

    if (!other.at(1).text.isEmpty()
        || (at(1).children.isEmpty() && !other.at(1).children.isEmpty())) {
        at(1).text = other.at(1).text;
    }

    // Merges the elements
    for (int i = 0; i < other.at(1).children.count(); ++i) {
        prepend(1, other.mid(other.at(1).children[i]));
    }
}


THtmlParser THtmlParser::mergeElements(const QString &s1, const QString &s2)
{
    THtmlParser p1, p2;
    p1.parse(s1);
    p2.parse(s2);
    p1.merge(p2);
    return p1;
}
