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


#ifndef FADCLEARLINEEDIT_H
#define FADCLEARLINEEDIT_H

#include "FadLineEdit.h"
#include <QToolButton>

//! Однострочный редактор с возможностью добавления на него кнопок.
/*! По умолчанию, имеет кнопку, которая очищает строку ввода.
 *  Кнопка очищения видима только тогда, когда есть текст.
 *  \sa FadLineEdit
 */
class FadClearLineEdit: public FadLineEdit
{
    Q_OBJECT

public:
    //! Создает однострочный редактор уже с текстом.
    /*! \param text строка редактора
     *  \param parent указатель на виджет-родитель
     */
    explicit FadClearLineEdit( const QString & text, QWidget * parent = 0 );

    /*! Перегруженный конструктор. */
    explicit FadClearLineEdit( QWidget * parent = 0 );

private:
    void init(); //!< Инициализация класса

private slots:
    void updateStates(); //!< Обновление состояния класса

protected:
    QToolButton tButtonClearText; //!< Кнопка, очищающая строку ввода

signals:
    //! Вызывается, при нажатии на кнопку очищения строки ввода
    void cleansingText();
};

#endif // FADCLEARLINEEDIT_H
