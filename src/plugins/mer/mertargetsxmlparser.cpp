/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "mertargetsxmlparser.h"

#include <utils/fileutils.h>

#include <QAbstractMessageHandler>
#include <QAbstractXmlReceiver>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QXmlQuery>
#include <QXmlStreamWriter>
#include <QStack>

const char TARGET[] = "target";
const char TARGETS[] = "targets";
const char OUTPUT[] = "output";
const char NAME[] = "name";
const char VERSION[] = "version";
const char GCCDUMPMACHINE[] = "GccDumpMachine";
const char QMAKEQUERY[] = "QmakeQuery";

namespace Mer {

namespace {

class MessageHandler : public QAbstractMessageHandler
{
public:
    QString errorString() const
    {
        return QObject::tr("Error %1:%2:%3 %4").arg(m_identifier.toString())
                .arg(m_sourceLocation.line())
                .arg(m_sourceLocation.column())
                .arg(m_description);
    }

protected:
    virtual void handleMessage(QtMsgType /*type*/, const QString &description,
                               const QUrl &identifier, const QSourceLocation &sourceLocation)
    {
        m_description = description;
        m_identifier = identifier;
        m_sourceLocation = sourceLocation;
    }

private:
    QString m_description;
    QSourceLocation m_sourceLocation;
    QUrl m_identifier;
};

class XmlReceiver : public QAbstractXmlReceiver
{
public:
    XmlReceiver(const QXmlNamePool &namePool) : m_namePool(namePool), m_version(-1) {}

    int version() const
    {
        return m_version;
    }

    QList<MerTargetData> targetData() const
    {
        return m_targets;
    }

protected:
    virtual void startElement(const QXmlName &name)
    {
        const QString element = name.localName(m_namePool);
        if (element == QLatin1String(TARGET))
            m_currentTarget.clear();
        m_currentElementStack.push(element);
    }

    virtual void endElement()
    {
        if (m_currentElementStack.pop() == QLatin1String(TARGET))
            m_targets.append(m_currentTarget);
    }

    virtual void attribute(const QXmlName &name,
                           const QStringRef &value)
    {
        const QString attributeName = name.localName(m_namePool);
        const QString attributeValue = value.toString();
        const QString element = m_currentElementStack.top();
        if (element == QLatin1String(TARGETS) && attributeName == QLatin1String(VERSION))
            m_version = attributeValue.toInt();
        else if (element == QLatin1String(TARGET) && attributeName == QLatin1String(NAME))
            m_currentTarget.name = attributeValue;
        else if (element == QLatin1String(OUTPUT) && attributeName == QLatin1String(NAME))
            m_attributeValue = attributeValue;
        else
            m_attributeValue.clear();
    }

    virtual void characters(const QStringRef &value)
    {
        if (m_currentElementStack.top() == QLatin1String(OUTPUT)) {
            if (m_attributeValue == QLatin1String(GCCDUMPMACHINE))
                m_currentTarget.gccDumpMachine = value.toString();
            else if (m_attributeValue == QLatin1String(QMAKEQUERY))
                m_currentTarget.qmakeQuery = value.toString();
        }
    }

    virtual void comment(const QString &/*value*/)
    {
    }

    virtual void startDocument()
    {
    }

    virtual void endDocument()
    {
    }

    virtual void processingInstruction(const QXmlName &/*target*/, const QString &/*value*/)
    {
    }

    virtual void atomicValue(const QVariant &/*value*/)
    {
    }
    virtual void namespaceBinding(const QXmlName &/*name*/)
    {
    }
    virtual void startOfSequence()
    {
    }
    virtual void endOfSequence()
    {
    }

private:
    QXmlNamePool m_namePool;
    QStack<QString> m_currentElementStack;
    QList<MerTargetData> m_targets;
    MerTargetData m_currentTarget;
    QString m_attributeValue;
    int m_version;
};

#ifdef Q_OS_MAC
#  define SHARE_PATH "/../Resources"
#else
#  define SHARE_PATH "/../share/qtcreator"
#endif

static QString applicationDirPath()
{
    return QCoreApplication::applicationDirPath();
}

static inline QString sharedDirPath()
{
    QString appPath = applicationDirPath();

    return QFileInfo(appPath + QLatin1String(SHARE_PATH)).absoluteFilePath();
}

} // Anonymous

class MerTargetsXmlReaderPrivate
{
public:
    MerTargetsXmlReaderPrivate()
        : receiver(new XmlReceiver(query.namePool())),
          error(false)
    {}

    ~MerTargetsXmlReaderPrivate()
    {
        delete receiver;
    }

    QXmlQuery query;
    XmlReceiver *receiver;
    MessageHandler messageHandler;
    bool error;
    QString errorString;
};

MerTargetsXmlReader::MerTargetsXmlReader(const QString &fileName, QObject *parent)
    : QObject(parent),
      d(new MerTargetsXmlReaderPrivate)
{
    Utils::FileReader reader;
    d->error = !reader.fetch(fileName, QIODevice::ReadOnly);
    if (d->error) {
        d->errorString = reader.errorString();
        return;
    }

    QXmlSchema schema;
    schema.setMessageHandler(&d->messageHandler);

    Utils::FileReader schemeReader;
    d->error = !schemeReader.fetch(QString::fromLatin1("%1/mer/targets.xsd").arg(sharedDirPath()),
            QIODevice::ReadOnly);
    if (d->error) {
        d->errorString = schemeReader.errorString();
        return;
    }
    schema.load(schemeReader.data());
    d->error = !schema.isValid();
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    QXmlSchemaValidator validator(schema);
    validator.setMessageHandler(&d->messageHandler);
    d->error = !validator.validate(reader.data());
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    d->query.setQuery(QString::fromLatin1("doc('%1')").arg(fileName));
    d->query.setMessageHandler(&d->messageHandler);
    d->error = !d->query.isValid();
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    d->error = !d->query.evaluateTo(d->receiver);
    if (d->error)
        d->errorString = d->messageHandler.errorString();
}

MerTargetsXmlReader::~MerTargetsXmlReader()
{
    delete d;
}

bool MerTargetsXmlReader::hasError() const
{
    return d->error;
}

QString MerTargetsXmlReader::errorString() const
{
    return d->errorString;
}

QList<MerTargetData> MerTargetsXmlReader::targetData() const
{
    return d->receiver->targetData();
}

int MerTargetsXmlReader::version() const
{
    return d->receiver->version();
}

void MerTargetData::clear()
{
    name.clear();
    gccDumpMachine.clear();
    qmakeQuery.clear();
}

class MerTargetsXmlWriterPrivate
{
public:
    MerTargetsXmlWriterPrivate(const QString &fileName) : fileSaver(fileName, QIODevice::WriteOnly) {}

    Utils::FileSaver fileSaver;
};

MerTargetsXmlWriter::MerTargetsXmlWriter(const QString &fileName, int version,
                                         const QList<MerTargetData> &targetData, QObject *parent)
    : QObject(parent),
      d(new MerTargetsXmlWriterPrivate(fileName))
{
    QByteArray data;
    QXmlStreamWriter writer(&data);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String(TARGETS));
    writer.writeAttribute(QLatin1String(VERSION), QString::number(version));
    foreach (const MerTargetData &data, targetData) {
        writer.writeStartElement(QLatin1String(TARGET));
        writer.writeAttribute(QLatin1String(NAME), data.name);
        writer.writeStartElement(QLatin1String(OUTPUT));
        writer.writeAttribute(QLatin1String(NAME), QLatin1String(GCCDUMPMACHINE));
        writer.writeCharacters(data.gccDumpMachine);
        writer.writeEndElement(); // output
        writer.writeStartElement(QLatin1String(OUTPUT));
        writer.writeAttribute(QLatin1String(NAME), QLatin1String(QMAKEQUERY));
        writer.writeCharacters(data.qmakeQuery);
        writer.writeEndElement(); // output
        writer.writeEndElement(); // target
    }
    writer.writeEndElement(); // targets
    writer.writeEndDocument();
    d->fileSaver.write(data);
    d->fileSaver.finalize();
}

MerTargetsXmlWriter::~MerTargetsXmlWriter()
{
    delete d;
}

bool MerTargetsXmlWriter::hasError() const
{
    return d->fileSaver.hasError();
}

QString MerTargetsXmlWriter::errorString() const
{
    return d->fileSaver.errorString();
}

} // Mer
