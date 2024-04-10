/*
 * iconselect.cpp - class that allows user to select an PsiIcon from an Iconset
 * Copyright (C) 2003-2005  Michail Pishchagin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "iconselect.h"

#include "actionlineedit.h"
#include "emojiregistry.h"
#include "iconaction.h"
#include "iconset.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>
#include <QWidgetAction>

#include <cmath>

//----------------------------------------------------------------------------
// IconSelectButton
//----------------------------------------------------------------------------

//! \if _hide_doc_
/**
    \class IconSelectButton
    \brief This button is used by IconSelect and displays one PsiIcon
*/
class IconSelectButton : public QAbstractButton {
    Q_OBJECT

private:
    PsiIcon *ic;
    QSize    s;
    QSize    maxIconSize;
    bool     animated;

public:
    IconSelectButton(QWidget *parent) : QAbstractButton(parent)
    {
        ic       = nullptr;
        animated = false;
        connect(this, SIGNAL(clicked()), SLOT(iconClicked()));
    }

    ~IconSelectButton()
    {
        iconStop();

        if (ic) {
            delete ic;
            ic = nullptr;
        }
    }

    void setIcon(const PsiIcon *i, const QSize &maxSize = QSize())
    {
        iconStop();

        if (ic) {
            delete ic;
            ic = nullptr;
        }

        maxIconSize = maxSize;
        if (i)
            ic = new PsiIcon(*(const_cast<PsiIcon *>(i)));
        else
            ic = nullptr;
    }

    const PsiIcon *icon() const { return ic; }

    QSize sizeHint() const { return s; }
    void  setSizeHint(QSize sh) { s = sh; }

signals:
    void iconSelected(const PsiIcon *);
    void textSelected(QString);

private:
    void iconStart()
    {
        if (ic) {
            connect(ic, SIGNAL(pixmapChanged()), SLOT(iconUpdated()));
            if (!animated) {
                ic->activated(false);
                animated = true;
            }

            if (!ic->text().isEmpty()) {
                // and list of possible variants in the ToolTip
                QStringList toolTip;
                for (const PsiIcon::IconText &t : ic->text()) {
                    toolTip += t.text;
                }

                QString toolTipText = toolTip.join(", ");
                if (toolTipText.length() > 30)
                    toolTipText = toolTipText.left(30) + "...";

                setToolTip(toolTipText);
            }
        }
    }

    void iconStop()
    {
        if (ic) {
            disconnect(ic, nullptr, this, nullptr);
            if (animated) {
                ic->stop();
                animated = false;
            }
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *)
#else
    void enterEvent(QEnterEvent *)
#endif
    {
        iconStart();
        setFocus();
        update();
    } // focus follows mouse mode
    void leaveEvent(QEvent *)
    {
        iconStop();
        clearFocus();
        update();
    }

private slots:
    void iconUpdated() { update(); }

    void iconClicked()
    {
        clearFocus();
        if (ic) {
            emit iconSelected(ic);
            emit textSelected(ic->defaultText());
        } else {
            emit textSelected(text());
        }
    }

private:
    // reimplemented
    void paintEvent(QPaintEvent *)
    {
        QPainter                  p(this);
        QFlags<QStyle::StateFlag> flags = QStyle::State_Active | QStyle::State_Enabled;

        if (hasFocus())
            flags |= QStyle::State_Selected;

        QStyleOptionMenuItem opt;
        opt.palette = palette();
        opt.state   = flags;
        opt.font    = font();
        opt.rect    = rect();
        style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);

        if (ic) {
            QPixmap pix = ic->pixmap(maxIconSize);
            if (pix.width() > maxIconSize.width() || pix.height() > maxIconSize.height()) {
                pix = pix.scaled(maxIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            p.drawPixmap((width() - pix.width()) / 2, (height() - pix.height()) / 2, pix);
        } else {
            p.drawText(opt.rect, Qt::AlignVCenter | Qt::AlignCenter, text());
        }
    }
};
//! \endif

//----------------------------------------------------------------------------
// IconSelect -- the widget that does all dirty work
//----------------------------------------------------------------------------

class IconSelect : public QWidget {
    Q_OBJECT

private:
    IconSelectPopup *menu;
    Iconset          is;
    QGridLayout     *grid;
    QString          titleFilter;
    bool             shown;
    bool             emojiSorting;

signals:
    void updatedGeometry();

public:
    IconSelect(IconSelectPopup *parentMenu);
    ~IconSelect();

    void           setIconset(const Iconset &);
    const Iconset &iconset() const;

    void setEmojiSortingEnabled(bool enabled);
    void setTitleFilter(const QString &title);

protected:
    QList<PsiIcon *> sortEmojis() const;
    void             noIcons();
    void             createLayout();
    void             updateGrid();

    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);

        QStyleOptionMenuItem opt;
        opt.palette = palette();
        opt.rect    = rect();
        style()->drawControl(QStyle::CE_MenuEmptyArea, &opt, &p, this);
    }

protected slots:
    void closeMenu();
};

IconSelect::IconSelect(IconSelectPopup *parentMenu) : QWidget(parentMenu)
{
    menu = parentMenu;
    connect(menu, SIGNAL(textSelected(QString)), SLOT(closeMenu()));

    grid = nullptr;
    noIcons();

    Q_UNUSED(shown)
}

IconSelect::~IconSelect() { }

void IconSelect::closeMenu()
{
    // this way all parent menus (if any) would be closed too
    QMouseEvent me(QEvent::MouseButtonPress, menu->pos() - QPoint(5, 5), QCursor::pos(), Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    menu->mousePressEvent(&me);

    // just in case
    menu->close();
}

void IconSelect::createLayout()
{
    Q_ASSERT(!grid);
    grid        = new QGridLayout(this);
    auto margin = style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, this);
    grid->setContentsMargins(margin, margin, margin, margin);
    grid->setSpacing(1);
}

void IconSelect::noIcons()
{
    createLayout();
    QLabel *lbl = new QLabel(this);
    grid->addWidget(lbl, 0, 0);
    lbl->setText(tr("No icons available"));
    emit updatedGeometry();
}

void IconSelect::setIconset(const Iconset &iconset)
{
    is    = iconset;
    shown = false; // we need to recompute geometry
    updateGrid();
}

void IconSelect::updateGrid()
{
    blockSignals(true);
    // delete all children
    if (grid) {
        delete grid;
        grid = nullptr;

        QObjectList list = children();
        qDeleteAll(list);
    }

    bool fontEmojiMode = is.count() == 0;

    // first we need to find optimal size for elements and don't forget about
    // taking too much screen space
    float w = 0, h = 0;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto fontSz = qApp->fontMetrics().height();
#else
    const auto fontSz = QFontMetrics(qApp->font()).height();
#endif
    int  maxPrefTileHeight = fontSz * 3;
    auto maxPrefSize       = QSize(maxPrefTileHeight, maxPrefTileHeight);

    double                              count; // the 'double' type is somewhat important for MSVC.NET here
    QList<const EmojiRegistry::Emoji *> emojis;
    if (fontEmojiMode) {
        for (auto const &emoji : EmojiRegistry::instance()) {
            if (titleFilter.isEmpty() || emoji.name.contains(titleFilter)) {
                emojis.append(&emoji);
                if (!titleFilter.isEmpty() && emojis.size() == 40) {
                    break;
                }
            }
        }

        count = emojis.count();
        w     = fontSz * 2.5;
        h     = fontSz * 2.5;
    } else {
        QListIterator<PsiIcon *> it = is.iterator();
        for (count = 0; it.hasNext(); count++) {
            PsiIcon *icon    = it.next();
            auto     pix     = icon->pixmap(maxPrefSize);
            auto     pixSize = pix.size();
            if (pix.width() > maxPrefSize.width() || pix.height() > maxPrefSize.height()) {
                pixSize.scale(maxPrefSize, Qt::KeepAspectRatio);
            }
            w += pixSize.width();
            h += pixSize.height();
        }
        w /= float(count);
        h /= float(count);
    }

    const int margin   = 2;
    int       tileSize = int(qMax(w, h)) + 2 * margin;

    QRect r       = menu->screen()->availableGeometry();
    int   maxSize = qMin(r.width(), r.height()) / 3;

    int size       = int(ceil(std::sqrt(count)));
    int maxColumns = int(maxSize / tileSize);
    size           = size > maxColumns ? maxColumns : size;

    // now, fill grid with elements
    createLayout();

    int row    = 0;
    int column = 0;

    if (fontEmojiMode) {
        auto font = qApp->font();
        if (font.pointSize() == -1)
            font.setPixelSize(font.pixelSize() * 2.5);
        else
            font.setPointSize(font.pointSize() * 2.5);
#if defined(Q_OS_WIN)
        font.setFamily("Segoe UI Emoji");
#elif defined(Q_OS_MAC)
        font.setFamily("Apple Color Emoji");
#else
        font.setFamily("Noto Color Emoji");
#endif
        for (auto const &emoji : emojis) {
            IconSelectButton *b = new IconSelectButton(this);
            b->setFont(font);
            grid->addWidget(b, row, column);
            b->setText(emoji->code);
            b->setToolTip(emoji->name);
            b->setSizeHint(QSize(tileSize, tileSize));
            connect(b, qOverload<const PsiIcon *>(&IconSelectButton::iconSelected), menu,
                    &IconSelectPopup::iconSelected);
            connect(b, &IconSelectButton::textSelected, menu, &IconSelectPopup::textSelected);

            if (++column >= size) {
                ++row;
                column = 0;
            }
        }
    } else {
        QListIterator<PsiIcon *> it = is.iterator();
        QList<PsiIcon *>         sortIcons;
        if (emojiSorting) {
            sortIcons = sortEmojis();
            it        = QListIterator<PsiIcon *>(sortIcons);
        }
        while (it.hasNext()) {
            IconSelectButton *b = new IconSelectButton(this);
            grid->addWidget(b, row, column);
            b->setIcon(it.next(), maxPrefSize);
            b->setSizeHint(QSize(tileSize, tileSize));
            connect(b, qOverload<const PsiIcon *>(&IconSelectButton::iconSelected), menu,
                    &IconSelectPopup::iconSelected);
            connect(b, &IconSelectButton::textSelected, menu, &IconSelectPopup::textSelected);

            if (++column >= size) {
                ++row;
                column = 0;
            }
        }
    }
    blockSignals(false);
    if (!shown) {
        shown = true;
        emit updatedGeometry();
    }
}

const Iconset &IconSelect::iconset() const { return is; }

void IconSelect::setEmojiSortingEnabled(bool enabled) { emojiSorting = enabled; }

void IconSelect::setTitleFilter(const QString &title)
{
    titleFilter = title;
    updateGrid();
}

QList<PsiIcon *> IconSelect::sortEmojis() const
{
    QList<PsiIcon *> ret;
    QList<PsiIcon *> notEmoji;
    ret.reserve(is.count());
    QHash<QString, PsiIcon *> cp2icon; // codepoint to icon map

    auto &er = EmojiRegistry::instance();

    for (const auto &icon : is) {
        bool found = false;
        for (const auto &text : icon->text()) {
            if (er.isEmoji(text.text)) {
                cp2icon.insert(text.text, icon);
                found = true;
                break;
            }
        }
        if (!found)
            notEmoji.append(icon);
    }

    for (auto const &group : EmojiRegistry::instance().groups) {
        for (auto const &subgroup : group.subGroups) {
            for (auto const &emoji : subgroup.emojis) {
                auto icon = cp2icon.value(emoji.code);
                if (icon) {
                    ret.append(icon);
                }
            }
        }
    }

    ret += notEmoji;
    return ret;
}

//----------------------------------------------------------------------------
// IconSelectPopup
//----------------------------------------------------------------------------

class IconSelectPopup::Private : public QObject {
    Q_OBJECT
public:
    Private(IconSelectPopup *parent) : QObject(parent), parent_(parent), icsel_(nullptr), emotsAction_(nullptr) { }

    IconSelectPopup *parent_;
    IconSelect      *icsel_;
    QWidgetAction   *emotsAction_;
    QScrollArea     *scrollArea_;
    ActionLineEdit  *findBar_;
    IconAction      *findAct_;
    QWidgetAction   *findAction_;

public slots:
    void updatedGeometry()
    {
        emotsAction_->setDefaultWidget(scrollArea_);
        QRect r       = scrollArea_->screen()->availableGeometry();
        int   maxSize = qMin(r.width(), r.height()) / 3;
        int   vBarWidth
            = scrollArea_->verticalScrollBar()->isEnabled() ? scrollArea_->verticalScrollBar()->sizeHint().rwidth() : 0;
        scrollArea_->setMinimumWidth(icsel_->sizeHint().rwidth() + vBarWidth);
        scrollArea_->setMinimumHeight(qMin(icsel_->sizeHint().rheight(), maxSize));
        scrollArea_->setFrameStyle(QFrame::Plain);
        parent_->removeAction(emotsAction_);
        parent_->addAction(emotsAction_); // add menu item
        findAct_->setPsiIcon("psi/search");
    }

    void setTitleFilter(const QString &filter) { icsel_->setTitleFilter(filter); }
};

IconSelectPopup::IconSelectPopup(QWidget *parent) : QMenu(parent)
{
    d         = new Private(this);
    d->icsel_ = new IconSelect(this);

    d->findAction_ = new QWidgetAction(this);
    d->findBar_    = new ActionLineEdit(nullptr);
    d->findAct_    = new IconAction(d->findBar_);
    d->findBar_->addAction(d->findAct_);
    d->findAction_->setDefaultWidget(d->findBar_);
    addAction(d->findAction_);
    connect(d->findBar_, &QLineEdit::textChanged, d, &Private::setTitleFilter);

    d->emotsAction_ = new QWidgetAction(this);
    d->scrollArea_  = new QScrollArea(this);
    d->scrollArea_->setWidget(d->icsel_);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->scrollArea_->setWidgetResizable(true);
    connect(d->icsel_, &IconSelect::updatedGeometry, d, &IconSelectPopup::Private::updatedGeometry);
    d->updatedGeometry();
}

IconSelectPopup::~IconSelectPopup()
{
    d->findAction_->setDefaultWidget(nullptr);
    delete d->findBar_;
}

void IconSelectPopup::setIconset(const Iconset &i) { d->icsel_->setIconset(i); }

const Iconset &IconSelectPopup::iconset() const { return d->icsel_->iconset(); }

void IconSelectPopup::setEmojiSortingEnabled(bool enabled) { d->icsel_->setEmojiSortingEnabled(enabled); }

/**
    It's used by child widget to close the menu by simulating a
    click slightly outside of menu. This seems to be the best way
    to achieve this.
*/
void IconSelectPopup::mousePressEvent(QMouseEvent *e) { QMenu::mousePressEvent(e); }

#include "iconselect.moc"
