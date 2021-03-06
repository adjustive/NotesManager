/*************************************************************************
**
** Copyright (C) 2013-2014 by Ilya Petrash
** All rights reserved.
** Contact: gil9red@gmail.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the
** Free Software Foundation, Inc.,
** 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
**************************************************************************/

#include "propertyattachable.h"

#include <QDesktopWidget>
#include <QDebug>

PropertyAttachable::PropertyAttachable( QObject * parent )
    : QObject( parent ),
      window( 0 ),
      timerId( -1 ),
      d_distance( 10 ),
      activate( true )
{

}

void PropertyAttachable::setDistance( int d )
{
    d_distance = d;
}
int PropertyAttachable::distance()
{
    return d_distance;
}

void PropertyAttachable::installTo( QWidget * widget )
{
    if ( window )
        remove();

    window = widget;
    window->installEventFilter( this );
}
void PropertyAttachable::remove()
{
    window->removeEventFilter( this );
    window = 0;
}

void PropertyAttachable::setActivate( bool act )
{
    activate = act;
}
bool PropertyAttachable::isActivate()
{
    return activate;
}

void PropertyAttachable::timerEvent( QTimerEvent * )
{
    if ( !window || !activate ) // паранойя
        return;

    const QRect & availableGeometry = QDesktopWidget().availableGeometry();
    int d_x = availableGeometry.x();
    int d_y = availableGeometry.y();
    int d_width = availableGeometry.width();
    int d_height = availableGeometry.height();

    // временный костыль: все дело в составе абстрактной
    // заметки -> доступ к ней закрыт head и body
    // поэтому приходится подлючать head, но рассчет брать на его предка - окно абстракной заметки
    if ( window->parentWidget() )
    {
        int x = window->parentWidget()->x();
        int y = window->parentWidget()->y();
        int width = window->parentWidget()->width();
        int height = window->parentWidget()->height();

        if ( x >= d_x && x <= d_distance )
            window->parentWidget()->move( d_x, y );

        if ( y >= d_y && y <= d_distance )
            window->parentWidget()->move( x, d_y );

        if ( x + width >= d_width - d_distance && x + width <= d_width )
            window->parentWidget()->move( d_width - width, y );

        if ( y + height >= d_height - d_distance && y + height <= d_height )
            window->parentWidget()->move( x, d_height - height );
    } else
    {
        int x = window->x();
        int y = window->y();
        int width = window->width();
        int height = window->height();

        if ( x >= d_x && x <= d_distance )
            window->move( d_x, y );

        if ( y >= d_y && y <= d_distance )
            window->move( x, d_y );

        if ( x + width >= d_width - d_distance && x + width <= d_width )
            window->move( d_width - width, y );

        if ( y + height >= d_height - d_distance && y + height <= d_height )
            window->move( x, d_height - height );
    }
}
bool PropertyAttachable::eventFilter( QObject * object, QEvent * event )
{
    if ( object == window && activate )
    {
        if ( event->type() == QEvent::MouseButtonPress )
            timerId = startTimer( 333 );

        else if ( event->type() == QEvent::MouseButtonRelease )
            killTimer( timerId );
    }

    return QObject::eventFilter( object, event );
}

