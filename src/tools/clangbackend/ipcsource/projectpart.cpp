/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectpart.h"

#include <projectpartcontainer.h>

#include <utf8stringvector.h>

#include <cstring>

namespace ClangBackEnd {

class ProjectPartData {
public:
    ProjectPartData(const Utf8String &projectPartId);
    ~ProjectPartData();

public:
    void clearArguments();

public:
    time_point lastChangeTimePoint;
    std::vector<const char*> arguments;
    Utf8String projectPartId;
};

void ProjectPartData::clearArguments()
{
    for (auto argument : arguments)
        delete [] argument;

    arguments.clear();
}

ProjectPartData::ProjectPartData(const Utf8String &projectPartId)
    : lastChangeTimePoint(std::chrono::steady_clock::now()),
      projectPartId(projectPartId)
{
}

ProjectPartData::~ProjectPartData()
{
    clearArguments();
}

ProjectPart::ProjectPart(const Utf8String &projectPartId)
    : d(std::make_shared<ProjectPartData>(projectPartId))
{
}

ProjectPart::ProjectPart(const Utf8String &projectPartId,
                         std::initializer_list<Utf8String> arguments)
    : d(std::make_shared<ProjectPartData>(projectPartId))
{
    setArguments(arguments);
}

ProjectPart::ProjectPart(const ProjectPartContainer &projectContainer)
    : d(std::make_shared<ProjectPartData>(projectContainer.projectPartId()))
{
    setArguments(projectContainer.arguments());
}

ProjectPart::~ProjectPart() = default;
ProjectPart::ProjectPart(const ProjectPart &) = default;
ProjectPart &ProjectPart::operator=(const ProjectPart &) = default;

ProjectPart::ProjectPart(ProjectPart &&other)
    : d(std::move(other.d))
{
}

ProjectPart &ProjectPart::operator=(ProjectPart &&other)
{
    d = std::move(other.d);

    return *this;
}

void ProjectPart::clear()
{
    d->projectPartId.clear();
    d->clearArguments();
    updateLastChangeTimePoint();
}

const Utf8String &ProjectPart::projectPartId() const
{
    return d->projectPartId;
}

static const char *strdup(const Utf8String &utf8String)
{
    char *cxArgument = new char[utf8String.byteSize() + 1];
    std::memcpy(cxArgument, utf8String.constData(), utf8String.byteSize() + 1);

    return cxArgument;
}

void ProjectPart::setArguments(const Utf8StringVector &arguments)
{
    d->clearArguments();
    d->arguments.resize(arguments.size());
    std::transform(arguments.cbegin(), arguments.cend(), d->arguments.begin(), strdup);
    updateLastChangeTimePoint();
}

const std::vector<const char*> &ProjectPart::arguments() const
{
    return d->arguments;
}

int ProjectPart::argumentCount() const
{
    return int(d->arguments.size());
}

const char * const *ProjectPart::cxArguments() const
{
    return arguments().data();
}

const time_point &ProjectPart::lastChangeTimePoint() const
{
    return d->lastChangeTimePoint;
}

void ProjectPart::updateLastChangeTimePoint()
{
    d->lastChangeTimePoint = std::chrono::steady_clock::now();
}

bool operator==(const ProjectPart &first, const ProjectPart &second)
{
    return first.projectPartId() == second.projectPartId();
}

} // namespace ClangBackEnd

