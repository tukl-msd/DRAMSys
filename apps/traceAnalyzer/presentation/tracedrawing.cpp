/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 */

#include "tracedrawing.h"

void drawVerticalLine(QPainter* painter, int xPos, const QRectF& canvasRect)
{
    /*               P1 (xPos,lowerCanvasYBorder)
     *               |
     *               |
     *               |
     *               |
     *               P2  (xPos,upperCanvasYBorder)
     */

    QPoint P1(xPos, static_cast<int>(canvasRect.top()));
    QPoint P2(xPos, static_cast<int>(canvasRect.bottom()));
    painter->drawLine(QLine(P1, P2));
}

void drawDoubleArrow(QPainter* painter, int xFrom, int xTo, int y)
{

    /*              P1                                    P3
    //             /                                       \
    //            /                  {text}                 \
    // from      (-------------------------------------------) to
    //            \                                         /
    //             \                                       /
    //             P2                                    P4
    */

    QPoint from(xFrom, y);
    QPoint to(xTo, y);
    QPoint P1(xFrom + 10, y - 5);
    QPoint P2(xFrom + 10, y + 5);
    QPoint P3(xTo - 10, y - 5);
    QPoint P4(xTo - 10, y + 5);

    painter->drawLine(from, to);
    painter->drawLine(P1, from);
    painter->drawLine(P2, from);
    painter->drawLine(P3, to);
    painter->drawLine(P4, to);
}

void drawDoubleArrow(
    QPainter* painter, int xFrom, int xTo, int y, const QString& text, const QColor& textColor)
{
    drawDoubleArrow(painter, xFrom, xTo, y);
    drawText(painter, text, QPoint((xTo + xFrom) / 2, y), TextPositioning::topCenter, textColor);
}

void drawHexagon(QPainter* painter, const QPoint& from, const QPoint& to, double height)
{
    //       {text}
    //        P1------------------------P2
    //  From /                           \ To
    //       \                           /
    //       P4-------------------------P3

    int offset = 10;
    if ((to.x() - from.x()) <= 20)
    {
        offset = 5;
    }
    if ((to.x() - from.x()) <= 10)
    {
        offset = 2;
    }
    if ((to.x() - from.x()) <= 4)
    {
        offset = 0;
    }

    QPointF P1(from.x() + offset, from.y() - height / 2);
    QPointF P2(to.x() - offset, to.y() - height / 2);
    QPointF P3(to.x() - offset, to.y() + height / 2);
    QPointF P4(from.x() + offset, from.y() + height / 2);

    QPolygonF polygon;
    polygon << from << P1 << P2 << to << P3 << P4;

    painter->drawPolygon(polygon);
}

void drawText(QPainter* painter,
              const QString& text,
              const QPoint& position,
              const TextPositioning& positioning,
              const QColor& textColor)
{
    //*--------------*
    //|      |       |
    //|------x-------|
    //|      |       |
    //*--------------*

    QPen saved = painter->pen();
    painter->setPen(QPen(textColor));
    QFontMetrics fm = painter->fontMetrics();
    QPoint offset(fm.horizontalAdvance(text), fm.height());
    QRect rect(position - offset, position + offset);

    int flags;
    switch (positioning)
    {
    case TextPositioning::topRight:
        flags = Qt::AlignRight | Qt::AlignTop;
        break;
    case TextPositioning::bottomRight:
        flags = Qt::AlignRight | Qt::AlignBottom;
        break;
    case TextPositioning::bottomLeft:
        flags = Qt::AlignLeft | Qt::AlignBottom;
        break;
    case TextPositioning::topCenter:
        flags = Qt::AlignHCenter | Qt::AlignTop;
        break;
    case TextPositioning::bottomCenter:
        flags = Qt::AlignHCenter | Qt::AlignBottom;
        break;
    case TextPositioning::centerCenter:
        flags = Qt::AlignHCenter | Qt::AlignCenter;
        break;
    case TextPositioning::topLeft:
    default:
        flags = Qt::AlignLeft | Qt::AlignTop;
        break;
    }

    painter->drawText(rect, flags, text);
    painter->setPen(saved);
}
