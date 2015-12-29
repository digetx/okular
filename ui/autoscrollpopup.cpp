/***************************************************************************
 *   Copyright (C) 2016 by Dmitry Osipenko <digetx@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kstandarddirs.h>
#include <QPainter>

#include "autoscrollpopup.h"

AutoScrollPopup::AutoScrollPopup( QWidget * parent )
    : QFrame( parent, Qt::SubWindow ),
      m_scrolling_direction( Both )
{
    QPixmap p;

    p.load( KStandardDirs::locate( "data", "okular/pics/autoscroll.png" ) );

    // crop autoscroll subicons
    m_icon_hv = p.copy( 0, 0,             p.width(), p.width() );
    m_icon_v  = p.copy( 0, p.width(),     p.width(), p.width() );
    m_icon_h  = p.copy( 0, p.width() * 2, p.width(), p.width() );

    // setup popup window size
    setGeometry( 0, 0, p.width(), p.width() );

    // start hidden
    hide();
}

void AutoScrollPopup::setDirection( ScrollingDirection dir )
{
    m_scrolling_direction = dir;
}

void AutoScrollPopup::show( const QPoint &pos )
{
    // centre scroll icon at the given position
    int x = pos.x() - width()  / 2;
    int y = pos.y() - height() / 2;

    // move popup window to the new position
    move( x, y );

    // show popup
    QFrame::show();
}

void AutoScrollPopup::paintEvent(QPaintEvent *)
{
    QPainter paint(this);

    switch ( m_scrolling_direction ) {
    case Vertical:
        paint.drawPixmap( 0, 0, m_icon_v );
        break;
    case Horizontal:
        paint.drawPixmap( 0, 0, m_icon_h );
        break;
    case Both:
        paint.drawPixmap( 0, 0, m_icon_hv );
        break;
    }
}
