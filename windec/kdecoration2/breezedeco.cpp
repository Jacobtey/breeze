/*
* Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "breezedeco.h"

#include "breeze.h"

#include "breezebuttons.h"
#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationButtonGroup>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationShadow>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KPluginFactory>

#include <QPainter>

K_PLUGIN_FACTORY_WITH_JSON(
    BreezeDecoFactory,
    "breeze.json",
    registerPlugin<Breeze::Decoration>();
    registerPlugin<Breeze::Button>(QStringLiteral("button"));
)

namespace Breeze
{


    //________________________________________________________________
    static int g_sDecoCount = 0;
    static QSharedPointer<KDecoration2::DecorationShadow> g_sShadow;

    Decoration::Decoration(QObject *parent, const QVariantList &args)
        : KDecoration2::Decoration(parent, args)
        , m_colorSettings(client().data()->palette())
        , m_leftButtons(nullptr)
        , m_rightButtons(nullptr)
    {
        g_sDecoCount++;
    }

    //________________________________________________________________
    Decoration::~Decoration()
    {
        g_sDecoCount--;
        if (g_sDecoCount == 0) {
            // last deco destroyed, clean up shadow
            g_sShadow.clear();
        }
    }

    //________________________________________________________________
    void Decoration::init()
    {
        recalculateBorders();
        updateTitleBar();
        auto s = settings();
        connect(s.data(), &KDecoration2::DecorationSettings::borderSizeChanged, this, &Decoration::recalculateBorders);
        // a change in font might cause the borders to change
        connect(s.data(), &KDecoration2::DecorationSettings::fontChanged, this, &Decoration::recalculateBorders);
        connect(s.data(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::recalculateBorders);
        connect(client().data(), &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Decoration::recalculateBorders);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedHorizontallyChanged, this, &Decoration::recalculateBorders);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedVerticallyChanged, this, &Decoration::recalculateBorders);
        connect(client().data(), &KDecoration2::DecoratedClient::captionChanged, this,
            [this]()
            {
                // update the caption area
                update(captionRect());
            }
        );

        connect(client().data(), &KDecoration2::DecoratedClient::activeChanged,    this, [this]() { update(); });
        connect(client().data(), &KDecoration2::DecoratedClient::paletteChanged,   this,
            [this]() {
                m_colorSettings.update(client().data()->palette());
                update();
            }
        );
        connect(client().data(), &KDecoration2::DecoratedClient::widthChanged,     this, &Decoration::updateTitleBar);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateTitleBar);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::setOpaque);

        connect(client().data(), &KDecoration2::DecoratedClient::widthChanged,     this, &Decoration::updateButtonPositions);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateButtonPositions);
        connect(client().data(), &KDecoration2::DecoratedClient::shadedChanged,    this, &Decoration::updateButtonPositions);

        createButtons();
        createShadow();
    }

    //________________________________________________________________
    void Decoration::updateTitleBar()
    {
        auto s = settings();
        const bool maximized = client().data()->isMaximized();
        const int width = client().data()->width();
        const int height = maximized ? borderTop() : borderTop() - s->smallSpacing();
        const int x = maximized ? 0 : s->largeSpacing() / 2;
        const int y = maximized ? 0 : s->smallSpacing();
        setTitleBar(QRect(x, y, width, height));
    }

    //________________________________________________________________
    static int borderSize(const QSharedPointer<KDecoration2::DecorationSettings> &settings, bool bottom)
    {
        const int baseSize = settings->largeSpacing() / 2;
        switch (settings->borderSize()) {
            case KDecoration2::BorderSize::None:
            return 0;
            case KDecoration2::BorderSize::NoSides:
            return bottom ? baseSize : 0;
            case KDecoration2::BorderSize::Tiny:
            return baseSize / 2;
            case KDecoration2::BorderSize::Normal:
            return baseSize;
            case KDecoration2::BorderSize::Large:
            return baseSize * 1.5;
            case KDecoration2::BorderSize::VeryLarge:
            return baseSize * 2;
            case KDecoration2::BorderSize::Huge:
            return baseSize * 2.5;
            case KDecoration2::BorderSize::VeryHuge:
            return baseSize * 3;
            case KDecoration2::BorderSize::Oversized:
            return baseSize * 5;
            default:
            return baseSize;
        }
    }

    //________________________________________________________________
    static int borderSize(const QSharedPointer<KDecoration2::DecorationSettings> &settings)
    {
        return borderSize(settings, false);
    }

    //________________________________________________________________
    void Decoration::recalculateBorders()
    {
        auto s = settings();
        const auto c = client().data();
        const Qt::Edges edges = c->adjacentScreenEdges();
        int left   = c->isMaximizedHorizontally() || edges.testFlag(Qt::LeftEdge) ? 0 : borderSize(s);
        int right  = c->isMaximizedHorizontally() || edges.testFlag(Qt::RightEdge) ? 0 : borderSize(s);

        QFontMetrics fm(s->font());
        int top = qMax(fm.boundingRect(c->caption()).height(), s->gridUnit() * 2);

        // padding below
        // extra pixel is used for the active window outline
        top += s->smallSpacing() + 1;

        // padding above
        if (!c->isMaximized()) top += s->smallSpacing();

        int bottom = c->isMaximizedVertically() || edges.testFlag(Qt::BottomEdge) ? 0 : borderSize(s, true);
        setBorders(QMargins(left, top, right, bottom));

        const int extSize = s->largeSpacing() / 2;
        int extSides = 0;
        int extBottom = 0;
        if (s->borderSize() == KDecoration2::BorderSize::None) {
            extSides = extSize;
            extBottom = extSize;
        } else if (s->borderSize() == KDecoration2::BorderSize::NoSides) {
            extSides = extSize;
        }
        setResizeOnlyBorders(QMargins(extSides, 0, extSides, extBottom));
    }

    //________________________________________________________________
    void Decoration::createButtons()
    {
        m_leftButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Left, this, &Button::create);
        m_rightButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Right, this, &Button::create);
        updateButtonPositions();
    }

    //________________________________________________________________
    void Decoration::updateButtonPositions()
    {
        auto s = settings();
        const int padding = client().data()->isMaximized() ? 0 : s->smallSpacing();
        m_rightButtons->setSpacing(s->smallSpacing());
        m_leftButtons->setSpacing(s->smallSpacing());
        m_leftButtons->setPos(QPointF(padding, padding));
        m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - padding, padding));
    }

    //________________________________________________________________
    void Decoration::paint(QPainter *painter, const QRect &repaintRegion)
    {
        // TODO: optimize based on repaintRegion

        // paint background
        painter->fillRect(rect(), Qt::transparent);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_colorSettings.frame(client().data()->isActive()));

        // clip away the top part
        painter->save();
        painter->setClipRect(0, borderTop(), size().width(), size().height() - borderTop(), Qt::IntersectClip);
        painter->drawRoundedRect(rect(), Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);
        painter->restore();

        paintTitleBar(painter, repaintRegion);

        painter->restore();
    }

    //________________________________________________________________
    void Decoration::paintTitleBar(QPainter *painter, const QRect &repaintRegion)
    {
        const auto c = client().data();
        const bool active = c->isActive();
        const QRect titleRect(QPoint(0, 0), QSize(size().width(), borderTop()));
        const QColor titleBarColor(m_colorSettings.titleBarColor(c->isActive()));

        // render a linear gradient on title area
        QLinearGradient gradient( 0, 0, 0, titleRect.height() );
        gradient.setColorAt(0.0, titleBarColor.lighter(c->isActive() ? 120.0 : 100.0));
        gradient.setColorAt(0.8, titleBarColor);

        painter->save();
        painter->setBrush(gradient);
        painter->setPen(Qt::NoPen);

        if (c->isMaximized())
        {

            painter->drawRect(titleRect);

        } else {

            // we make the rect a little bit larger to be able to clip away the rounded corners on bottom
            painter->setClipRect(titleRect, Qt::IntersectClip);
            painter->drawRoundedRect(titleRect.adjusted(0, 0, 0, Metrics::Frame_FrameRadius), Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);

        }

        auto s = settings();

        // TODO: should be config option
        if( active )
        {
            painter->setRenderHint( QPainter::Antialiasing, false );
            painter->setBrush( Qt::NoBrush );
            painter->setPen( c->palette().color( QPalette::Highlight ) );
            painter->drawLine( titleRect.bottomLeft(), titleRect.bottomRight() );
        }

        painter->restore();

        // draw caption
        painter->setFont(s->font());
        const QRect cR = captionRect();
        const QString caption = painter->fontMetrics().elidedText(c->caption(), Qt::ElideMiddle, cR.width());
        painter->setPen(m_colorSettings.font(c->isActive()));
        painter->drawText(cR, Qt::AlignCenter | Qt::TextSingleLine, caption);

        // draw all buttons
        m_leftButtons->paint(painter, repaintRegion);
        m_rightButtons->paint(painter, repaintRegion);
    }

    //________________________________________________________________
    int Decoration::captionHeight() const
    {
        return borderTop() - settings()->smallSpacing() * (client().data()->isMaximized() ? 2 : 3) - 1;
    }

    //________________________________________________________________
    QRect Decoration::captionRect() const
    {
        const int leftOffset = m_leftButtons->geometry().x() + m_leftButtons->geometry().width();
        const int rightOffset = size().width() - m_rightButtons->geometry().x();
        const int offset = qMax(leftOffset, rightOffset);
        const int yOffset = client().data()->isMaximized() ? 0 : settings()->smallSpacing();
        // below is the spacer
        return QRect(offset, yOffset, size().width() - offset * 2, captionHeight());
    }

    //________________________________________________________________
    void Decoration::createShadow()
    {
        if (g_sShadow) {
            setShadow(g_sShadow);
            return;
        }
        auto decorationShadow = QSharedPointer<KDecoration2::DecorationShadow>::create();
        decorationShadow->setPadding(QMargins(10, 10, 20, 20));
        decorationShadow->setInnerShadowRect(QRect(20, 20, 20, 20));

        QImage image(60, 60, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        auto gradientStopColor = [](qreal alpha) {
            QColor color(35, 38, 41);
            color.setAlphaF(alpha);
            return color;
        };
        QRadialGradient radialGradient(20, 20, 20);
        radialGradient.setColorAt(0.0,  gradientStopColor(0.35));
        radialGradient.setColorAt(0.25, gradientStopColor(0.25));
        radialGradient.setColorAt(0.5,  gradientStopColor(0.13));
        radialGradient.setColorAt(0.75, gradientStopColor(0.04));
        radialGradient.setColorAt(1.0,  gradientStopColor(0.0));

        QLinearGradient linearGradient;
        linearGradient.setColorAt(0.0,  gradientStopColor(0.35));
        linearGradient.setColorAt(0.25, gradientStopColor(0.25));
        linearGradient.setColorAt(0.5,  gradientStopColor(0.13));
        linearGradient.setColorAt(0.75, gradientStopColor(0.04));
        linearGradient.setColorAt(1.0,  gradientStopColor(0.0));

        QPainter p(&image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        // topLeft
        p.fillRect(QRect(0, 0, 20, 20), radialGradient);
        // top
        linearGradient.setStart(20, 20);
        linearGradient.setFinalStop(20, 0);
        p.fillRect(QRect(20, 0, 20, 20), linearGradient);
        // topRight
        radialGradient.setCenter(40.0, 20.0);
        radialGradient.setFocalPoint(40.0, 20.0);
        p.fillRect(QRect(40, 0, 20, 20), radialGradient);
        // left
        linearGradient.setStart(20, 20);
        linearGradient.setFinalStop(0, 20);
        p.fillRect(QRect(0, 20, 20, 20), linearGradient);
        // bottom left
        radialGradient.setCenter(20.0, 40.0);
        radialGradient.setFocalPoint(20.0, 40.0);
        p.fillRect(QRect(0, 40, 20, 20), radialGradient);
        // bottom
        linearGradient.setStart(20, 40);
        linearGradient.setFinalStop(20, 60);
        p.fillRect(QRect(20, 40, 20, 20), linearGradient);
        // bottom right
        radialGradient.setCenter(40.0, 40.0);
        radialGradient.setFocalPoint(40.0, 40.0);
        p.fillRect(QRect(40, 40, 20, 20), radialGradient);
        // right
        linearGradient.setStart(40, 20);
        linearGradient.setFinalStop(60, 20);
        p.fillRect(QRect(40, 20, 20, 20), linearGradient);

        decorationShadow->setShadow(image);

        g_sShadow = decorationShadow;
        setShadow(decorationShadow);
    }

} // namespace


#include "breezedeco.moc"
