/*
 * coloropt.cpp - Psi color options class
 * Copyright (C) 2011  Sergey Ilinykh
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "coloropt.h"

#include "psioptions.h"

#include <QApplication>

ColorOpt::ColorOpt() : QObject(nullptr)
{
    connect(PsiOptions::instance(), SIGNAL(optionChanged(const QString &)), SLOT(optionChanged(const QString &)));
    connect(PsiOptions::instance(), SIGNAL(destroyed()), SLOT(reset()));

    typedef struct {
        const char         *opt;
        QPalette::ColorRole role;
    } SourceType;
    auto source = std::to_array<SourceType>({ { "contactlist.status.online", QPalette::Text },
                                              { "contactlist.status.offline", QPalette::Text },
                                              { "contactlist.status.away", QPalette::Text },
                                              { "contactlist.status.do-not-disturb", QPalette::Text },
                                              { "contactlist.profile.header-foreground", QPalette::Text },
                                              { "contactlist.profile.header-background", QPalette::Dark },
                                              { "contactlist.grouping.header-foreground", QPalette::Text },
                                              { "contactlist.grouping.header-background", QPalette::Base },
                                              { "contactlist.background", QPalette::Base },
                                              { "contactlist.status-change-animation1", QPalette::Text },
                                              { "contactlist.status-change-animation2", QPalette::Base },
                                              { "contactlist.status-messages", QPalette::Text },
                                              { "muc.role-moderator", QPalette::Text },
                                              { "muc.role-participant", QPalette::Text },
                                              { "muc.role-visitor", QPalette::Text },
                                              { "muc.role-norole", QPalette::Text },
                                              { "tooltip.background", QPalette::ToolTipBase },
                                              { "tooltip.text", QPalette::ToolTipText },
                                              { "messages.received", QPalette::Text },
                                              { "messages.sent", QPalette::Text },
                                              { "messages.informational", QPalette::Text },
                                              { "messages.usertext", QPalette::Text },
                                              { "messages.highlighting", QPalette::Text },
                                              { "messages.link", QPalette::Link },
                                              { "messages.link-visited", QPalette::Link },
                                              { "passive-popup.border", QPalette::Window } });
    for (const auto &item : source) {
        QString opt = QString("options.ui.look.colors.%1").arg(item.opt);
        colors.insert(opt, ColorData(PsiOptions::instance()->getOption(opt).value<QColor>(), item.role));
    }
}

QColor ColorOpt::color(const QString &opt, const QColor &defaultColor) const
{
    ColorData cd = colors.value(opt);
    // qDebug("get option: %s from data %s", qPrintable(opt), qPrintable(cd.color.isValid()? cd.color.name() : "Invalid
    // " + cd.color.name()));
    if (!cd.valid) {
        return PsiOptions::instance()->getOption(opt, defaultColor).value<QColor>();
    }
    if (cd.color.isValid()) {
        return cd.color;
    }
    return QApplication::palette().color(cd.role);
}

QPalette::ColorRole ColorOpt::colorRole(const QString &opt) const { return colors.value(opt).role; }

void ColorOpt::optionChanged(const QString &opt)
{
    if (opt.startsWith(QLatin1String("options.ui.look.colors")) && colors.contains(opt)) {
        colors[opt].color = PsiOptions::instance()->getOption(opt).value<QColor>();
        // qDebug("%s changed to %s", qPrintable(opt), qPrintable(colors[opt].color.isValid()? colors[opt].color.name()
        // : "Invalid " + colors[opt].color.name()));
        emit changed(opt);
    }
}

/**
 * Returns the singleton instance of this class
 * \return Instance of PsiOptions
 */
ColorOpt *ColorOpt::instance()
{
    if (!instance_)
        instance_.reset(new ColorOpt());
    return instance_.get();
}

void ColorOpt::reset() { instance_.reset(nullptr); }

std::unique_ptr<ColorOpt> ColorOpt::instance_;
