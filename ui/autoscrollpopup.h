/***************************************************************************
 *   Copyright (C) 2016 by Dmitry Osipenko <digetx@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _AUTOSCROLLPOPUP_H_
#define _AUTOSCROLLPOPUP_H_

#include <QFrame>

class AutoScrollPopup : public QFrame
{
    Q_OBJECT
    public:
        AutoScrollPopup( QWidget * parent );

        enum ScrollingDirection { Vertical, Horizontal, Both };

        void setDirection( ScrollingDirection dir );
        void show( const QPoint &pos );

private:
        QPixmap m_icon_hv;
        QPixmap m_icon_v;
        QPixmap m_icon_h;
        ScrollingDirection m_scrolling_direction;

        // QWidget interface
protected:
        void paintEvent(QPaintEvent *);
};


#endif
