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

#include "RichTextNote.h"

#include <QVBoxLayout>
#include <QInputDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QDebug>
#include <QAction>
#include <QSettings>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QKeySequence>
#include <QDir>
#include <QPluginLoader>
#include <QDockWidget>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFile>
#include <QFileDialog>
#include <QTextCodec>
#include <QImageWriter>
#include <QTextDocumentWriter>
#include <QMessageBox>
#include <QScrollArea>
#include <QDesktopServices>
#include <QDateTime>
#include <QPropertyAnimation>

#include "utils/func.h"
#include "JlCompress.h"
#include "FormattingToolbar/FormattingToolbar.h"

static QActionGroup * createGroupActionsOpacity( QObject * parent = 0 )
{
    QActionGroup * group = new QActionGroup( parent );
    group->addAction( Create::Action::get( "100%", QIcon( ":/opacity-100" ), QVariant( 1.0 ) ) );
    group->addAction( Create::Action::get( "90%",  QIcon( ":/opacity-90" ),  QVariant( 0.9 ) ) );
    group->addAction( Create::Action::get( "80%",  QIcon( ":/opacity-80" ),  QVariant( 0.8 ) ) );
    group->addAction( Create::Action::get( "70%",  QIcon( ":/opacity-70" ),  QVariant( 0.7 ) ) );
    group->addAction( Create::Action::get( "60%",  QIcon( ":/opacity-60" ),  QVariant( 0.6 ) ) );
    group->addAction( Create::Action::get( "50%",  QIcon( ":/opacity-50" ),  QVariant( 0.5 ) ) );
    group->addAction( Create::Action::get( "40%",  QIcon( ":/opacity-40" ),  QVariant( 0.4 ) ) );
    group->addAction( Create::Action::get( "30%",  QIcon( ":/opacity-30" ),  QVariant( 0.3 ) ) );
    group->addAction( Create::Action::get( "20%",  QIcon( ":/opacity-20" ),  QVariant( 0.2 ) ) );
    group->addAction( Create::Action::get( QObject::tr( "Other" ), QIcon( "" ), QVariant( -1.0 ) ) );
    return group;
}

RichTextNote::RichTextNote( const QString & fileName, QWidget * parent )
    : AbstractNote( parent ),
      d( new d_RichTextNote )
{    
    setFileName( fileName );
    init(); 
}
RichTextNote::RichTextNote( QWidget * parent )
    : AbstractNote( parent ),
      d( new d_RichTextNote )
{
    init();
}
RichTextNote::~RichTextNote()
{
    delete d;
}

QTextDocument * RichTextNote::document()
{
    return d->editor->document();
}
TextEditor * RichTextNote::textEditor()
{
    return d->editor;
}

QDateTime RichTextNote::created()
{
    return d->created;
}
QDateTime RichTextNote::modified()
{
    return d->modified;
}

void RichTextNote::setModified( bool b )
{
    d->isModified = b;
    updateStates();
}
bool RichTextNote::isModified()
{
    return d->isModified;
}

void RichTextNote::createNew( bool bsave )
{
    QString path = tr( "New note" ) + "_" + QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss__zzz" );
    setFileName( QDir::fromNativeSeparators( getNotesPath() + "/" + path ) );

    // TODO: лучше убрать эту порнуху и сделать нормальную инициацию по умолчанию
    // а лучше брать из настроек
    // пытаемся загрузить
    load();

    // сохраняемся на всякий случай
    if ( bsave )
        save();
}

QString RichTextNote::fileName()
{
    return d->noteFileName;
}
QString RichTextNote::attachDirPath()
{
    return QDir::fromNativeSeparators( d->noteFileName + "/attach" );
}
QString RichTextNote::contentFilePath()
{
    return QDir::fromNativeSeparators( d->noteFileName + "/content.html" );
}
QString RichTextNote::settingsFilePath()
{
    return QDir::fromNativeSeparators( d->noteFileName + "/settings.ini" );
}

void RichTextNote::setFileName( const QString & dirName )
{    
    d->noteFileName = dirName;
    setPath( settingsFilePath() );

    if ( QDir( d->noteFileName ).exists() )
        return;

    QDir().mkdir( d->noteFileName ); // Создадим папку заметки

    // Создадим нужные файлы и папки в папке заметки
    QDir().mkdir( attachDirPath() );
    QFile( contentFilePath() ).open( QIODevice::WriteOnly );
    QFile( settingsFilePath() ).open( QIODevice::WriteOnly );
}


void RichTextNote::init()
{
    setupActions();
    setupGUI();

    updateStates();
}
void RichTextNote::setupActions()
{
    actionVisibleToolBar = Create::Action::get( tr( "Toolbar" ), QIcon( "" ), true );
    actionVisibleQuickFind = Create::Action::get( tr( "Quick find" ), QIcon( "" ), true );
    actionVisibleAttachPanel = Create::Action::get( tr( "Attach panel" ), QIcon( ":/attach-panel" ), true );
    actionVisibleFindAndReplace = Create::Action::get( tr( "Find/replace" ), QIcon( ":/find-replace" ), true );
    actionVisibleFormattingToolbar = Create::Action::get( tr( "Formatting toolbar" ), QIcon( ":/formating" ), true );

    QAction * actionDelete = Create::Action::triggered( tr( "Delete" ), QIcon( ":/remove" ), QKeySequence(), this, SLOT( invokeRemove() ) );
    QAction * actionSetTitle = Create::Action::triggered( tr( "Set title" ), QIcon( ":/title" ), QKeySequence(), this, SLOT( selectTitle() ) );
    QAction * actionSetTitleFont = Create::Action::triggered( tr( "Set title font" ), QIcon( ":/title-font" ), QKeySequence(), this, SLOT( selectTitleFont() ) );
    QAction * actionSetTitleColor = Create::Action::triggered( tr( "Set title color" ), QIcon( ":/title-color" ), QKeySequence(), this, SLOT( selectTitleColor() ) );
    QAction * actionSetColor = Create::Action::triggered( tr( "Set window color" ), QIcon::fromTheme( "", QIcon( ":/color" ) ), QKeySequence(), this, SLOT( selectColor() ) );
    QAction * actionHide = Create::Action::triggered( tr( "Hide" ), QIcon::fromTheme( "", QIcon( ":/hide" ) ), QKeySequence(), this, SLOT( hide() ) );
    actionSetTopBottom = Create::Action::bTriggered( tr( "On top of all windows" ), QIcon::fromTheme( "", QIcon( ":/tacks" ) ), QKeySequence(), this, SLOT( setTop( bool ) ) );
    actionSetReadOnly = Create::Action::bTriggered( tr( "Read only" ), QIcon::fromTheme( "", QIcon( ":/read-only" ) ), QKeySequence(), this, SLOT( setReadOnly( bool ) ) );
    QAction * actionOpen = Create::Action::triggered( tr( "Open" ), QIcon::fromTheme( "", QIcon( ":/open" ) ), QKeySequence( QKeySequence::Open ), this, SLOT( open() ) );
    QAction * actionSaveAs = Create::Action::triggered( tr( "Save as" ), QIcon::fromTheme( "", QIcon( ":/save-as" ) ), QKeySequence( QKeySequence::SaveAs ), this, SLOT( saveAs() ) );
#ifndef QT_NO_PRINTER
    QAction * actionPrint = Create::Action::triggered( tr( "Print" ), QIcon::fromTheme( "", QIcon( ":/print" ) ), QKeySequence( QKeySequence::Print ), this, SLOT( print() ) );
    QAction * actionPreviewPrint = Create::Action::triggered( tr( "Preview print" ), QIcon::fromTheme( "", QIcon( ":/preview-print" ) ), QKeySequence(), this, SLOT( previewPrint() ) );
#endif
    actionSave = Create::Action::triggered( tr( "Save" ), QIcon::fromTheme( "", QIcon( ":/save" ) ), QKeySequence( QKeySequence::Save ), this, SLOT( save() ) );
    QAction * actionDuplicate = Create::Action::triggered( tr( "Duplicate" ), QIcon::fromTheme( "", QIcon( ":/duplicate" ) ), QKeySequence(), this, SLOT( duplicate() ) );
    QAction * actionAttach = Create::Action::triggered( tr( "Attach" ), QIcon::fromTheme( "", QIcon( ":/attach" ) ), QKeySequence(), this, SLOT( selectAttach() ) );

    connect( actionVisibleToolBar, SIGNAL( triggered(bool) ), SLOT( setVisibleToolBar(bool) ) );
    connect( this, SIGNAL( changeVisibleToolbar(bool) ), actionVisibleToolBar, SLOT( setChecked(bool) ) );


    QActionGroup * group = createGroupActionsOpacity( this );
    connect( group, SIGNAL( triggered(QAction*) ), SLOT( changeOpacity(QAction*)) );

    QMenu * menuOpacity = new QMenu( tr( "Opacity" ) );
    menuOpacity->setIcon( QIcon( ":/opacity" ) );
    menuOpacity->addActions( group->actions() );


    addAction( actionVisibleFormattingToolbar );
    addAction( actionVisibleAttachPanel );
    addAction( actionVisibleFindAndReplace );
    addAction( actionVisibleToolBar );
    addAction( actionVisibleQuickFind );
    addSeparator();
    addAction( actionSetTitle );
    addAction( actionSetTitleFont );
    addAction( actionSetTitleColor );
    addSeparator();
    addMenu( menuOpacity );
    addAction( actionSetColor );
    addSeparator();
    addAction( actionSetTopBottom );
    addAction( actionSetReadOnly );
    addSeparator();
    addAction( actionOpen );
    addAction( actionSave );
    addAction( actionSaveAs );
    addSeparator();
    addAction( actionPrint );
    addAction( actionPreviewPrint );
    addSeparator();
    addAction( actionDuplicate );    
    addAction( actionAttach );
    addSeparator();
    addAction( actionHide );
    addAction( actionDelete );    
}
void RichTextNote::setupGUI()
{
    d->editor = new TextEditor();

    Qt::DockWidgetAreas areas = Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;

    FormattingToolbar * formattingToolbar = new FormattingToolbar();
    formattingToolbar->installConnect( d->editor );   
    formattingToolbar->setNote( this );

    dockWidgetFormattingToolbar = new QDockWidget();
    dockWidgetFormattingToolbar->setAllowedAreas( areas );
    dockWidgetFormattingToolbar->setWidget( formattingToolbar );
    dockWidgetFormattingToolbar->setWindowTitle( formattingToolbar->windowTitle() );
    dockWidgetFormattingToolbar->setVisible( false );

    tButtonVisibleFormattingToolbar = Create::Button::get( this, tr( "Formatting toolbar" ), QIcon( ":/formating" ), true );
    connect( tButtonVisibleFormattingToolbar, SIGNAL( clicked(bool) ), dockWidgetFormattingToolbar, SLOT( setVisible(bool) ) );
    connect( dockWidgetFormattingToolbar, SIGNAL( visibilityChanged(bool) ), tButtonVisibleFormattingToolbar, SLOT( setChecked(bool) ) );

    connect( actionVisibleFormattingToolbar, SIGNAL( triggered(bool) ), dockWidgetFormattingToolbar, SLOT( setVisible(bool) ) );
    connect( dockWidgetFormattingToolbar, SIGNAL( visibilityChanged(bool) ), actionVisibleFormattingToolbar, SLOT( setChecked(bool) ) );

    body->addDockWidget( Qt::RightDockWidgetArea, dockWidgetFormattingToolbar );

    tButtonVisibleFormattingToolbar->setChecked( dockWidgetFormattingToolbar->isVisible() );
    actionVisibleFormattingToolbar->setChecked( dockWidgetFormattingToolbar->isVisible() );
    
    
    attachPanel = new AttachPanel();
    attachPanel->setViewTo( this );

    dockWidgetAttachPanel = new QDockWidget();
    dockWidgetAttachPanel->setAllowedAreas( areas );
    dockWidgetAttachPanel->setWidget( attachPanel );
    dockWidgetAttachPanel->setWindowTitle( attachPanel->windowTitle() );
    dockWidgetAttachPanel->setVisible( false );

    tButtonVisibleAttachPanel = Create::Button::get( this, tr( "Attach panel" ), QIcon( ":/attach-panel" ), true );
    connect( tButtonVisibleAttachPanel, SIGNAL( clicked(bool) ), dockWidgetAttachPanel, SLOT( setVisible(bool) ) );
    connect( dockWidgetAttachPanel, SIGNAL( visibilityChanged(bool) ), tButtonVisibleAttachPanel, SLOT( setChecked(bool) ) );

    connect( actionVisibleAttachPanel, SIGNAL( triggered(bool) ), dockWidgetAttachPanel, SLOT( setVisible(bool) ) );
    connect( dockWidgetAttachPanel, SIGNAL( visibilityChanged(bool) ), actionVisibleAttachPanel, SLOT( setChecked(bool) ) );

    body->addDockWidget( Qt::RightDockWidgetArea, dockWidgetAttachPanel );

    tButtonVisibleAttachPanel->setChecked( dockWidgetAttachPanel->isVisible() );
    actionVisibleAttachPanel->setChecked( dockWidgetAttachPanel->isVisible() );
    
    
    findAndReplace = new FindAndReplace( d->editor );

    dockWidgetFindAndReplace = new QDockWidget();
    dockWidgetFindAndReplace->setAllowedAreas( areas );
    dockWidgetFindAndReplace->setWidget( findAndReplace );
    dockWidgetFindAndReplace->setWindowTitle( findAndReplace->windowTitle() );
    dockWidgetFindAndReplace->setVisible( false );

    tButtonVisibleFindAndReplace = Create::Button::get( this, tr( "Find/replace" ), QIcon( ":/find-replace" ), true );
    connect( tButtonVisibleFindAndReplace, SIGNAL( clicked(bool) ), dockWidgetFindAndReplace, SLOT( setVisible(bool) ) );
    connect( dockWidgetFindAndReplace, SIGNAL( visibilityChanged(bool) ), tButtonVisibleFindAndReplace, SLOT( setChecked(bool) ) );

    connect( actionVisibleFindAndReplace, SIGNAL( triggered(bool) ), dockWidgetFindAndReplace, SLOT( setVisible(bool) ) );
    connect( dockWidgetFindAndReplace, SIGNAL( visibilityChanged(bool) ), actionVisibleFindAndReplace, SLOT( setChecked(bool) ) );

    body->addDockWidget( Qt::RightDockWidgetArea, dockWidgetFindAndReplace );

    tButtonVisibleFindAndReplace->setChecked( dockWidgetFindAndReplace->isVisible() );
    actionVisibleFindAndReplace->setChecked( dockWidgetFindAndReplace->isVisible() );
    

    quickFind = new QuickFind( d->editor );
    connect( actionVisibleQuickFind, SIGNAL( triggered(bool) ), quickFind, SLOT( setVisible(bool) ) );
    connect( quickFind, SIGNAL( visibilityChanged(bool) ), actionVisibleQuickFind, SLOT( setChecked(bool) ) );
    actionVisibleQuickFind->setChecked( quickFind->isVisible() );

    QVBoxLayout * mainLayout = new QVBoxLayout();
    mainLayout->setSpacing( 0 );
    mainLayout->setContentsMargins( 5, 0, 5, 0 );
    mainLayout->addWidget( d->editor );
    mainLayout->addWidget( quickFind );

    body->setWidget( new QWidget() );
    body->widget()->setLayout( mainLayout );
    body->setToolBarIconSize( QSize( 15, 15 ) );

    QToolButton * tButtonDelete = Create::Button::clicked( tr( "Delete" ), QIcon::fromTheme( "", QIcon( ":/remove" ) ), this, SLOT( invokeRemove() ) );
    QToolButton * tButtonSetTitle = Create::Button::clicked( tr( "Set title" ), QIcon::fromTheme( "", QIcon( ":/title" ) ), this, SLOT( selectTitle() ) );
    QToolButton * tButtonSetTitleFont = Create::Button::clicked( tr( "Set title font" ), QIcon::fromTheme( "", QIcon( ":/title-font" ) ), this, SLOT( selectTitleFont() ) );
    QToolButton * tButtonSetTitleColor = Create::Button::clicked( tr( "Set title color" ), QIcon::fromTheme( "", QIcon( ":/title-color" ) ), this, SLOT( selectTitleColor() ) );
    QToolButton * tButtonSetColor = Create::Button::clicked( tr( "Set window color" ),QIcon::fromTheme( "", QIcon( ":/color" ) ), this, SLOT( selectColor() ) );
    QToolButton * tButtonSetOpacity = Create::Button::clicked( tr( "Opacity" ), QIcon::fromTheme( "", QIcon( ":/opacity" ) ), this, SLOT( selectOpacity() ) );
    QToolButton * tButtonHide = Create::Button::clicked( tr( "Hide" ), QIcon::fromTheme( "", QIcon( ":/hide" ) ), this, SLOT( hide() ) );
    tButtonSetTopBottom = Create::Button::bClicked( tr( "On top of all windows" ), QIcon::fromTheme( "", QIcon( ":/tacks" ) ), this, SLOT( setTop( bool ) ) );
    tButtonSetReadOnly = Create::Button::bClicked( tr( "Read only" ), QIcon::fromTheme( "", QIcon( ":/read-only" ) ), this, SLOT( setReadOnly( bool ) ) );
    QToolButton * tButtonOpen = Create::Button::clicked( tr( "Open" ), QIcon::fromTheme( "", QIcon( ":/open" ) ), this, SLOT( open() ) );
    QToolButton * tButtonSaveAs = Create::Button::clicked( tr( "Save as" ), QIcon::fromTheme( "", QIcon( ":/save-as" ) ), this, SLOT( saveAs() ) );
    QToolButton * tButtonPrint = Create::Button::clicked( tr( "Print" ),QIcon::fromTheme( "", QIcon( ":/print" ) ), this, SLOT( print() ) );
    QToolButton * tButtonPreviewPrint = Create::Button::clicked( tr( "Preview print" ), QIcon::fromTheme( "", QIcon( ":/preview-print" ) ), this, SLOT( previewPrint() ) );
    tButtonSave = Create::Button::clicked( tr( "Save" ), QIcon::fromTheme( "", QIcon( ":/save" ) ), this, SLOT( save() ) );
    QToolButton * tButtonDuplicate = Create::Button::clicked( tr( "Duplicate" ), QIcon::fromTheme( "", QIcon( ":/duplicate" ) ), this, SLOT( duplicate() ) );
    QToolButton * tButtonAttach = Create::Button::clicked( tr( "Attach" ), QIcon::fromTheme( "", QIcon( ":/attach" ) ), this, SLOT( selectAttach() ) );


    body->addWidgetToToolBar( tButtonHide );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonVisibleFormattingToolbar );
    body->addWidgetToToolBar( tButtonVisibleAttachPanel );
    body->addWidgetToToolBar( tButtonVisibleFindAndReplace );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonSetTitle );
    body->addWidgetToToolBar( tButtonSetTitleFont );
    body->addWidgetToToolBar( tButtonSetTitleColor );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonSetColor );
    body->addWidgetToToolBar( tButtonSetOpacity );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonSetTopBottom );
    body->addWidgetToToolBar( tButtonSetReadOnly );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonOpen );
    body->addWidgetToToolBar( tButtonSaveAs );
    body->addWidgetToToolBar( tButtonSave );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonDuplicate );
    body->addWidgetToToolBar( tButtonAttach );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonPrint );
    body->addWidgetToToolBar( tButtonPreviewPrint );
    body->addSeparator();
    body->addWidgetToToolBar( tButtonDelete );

    QActionGroup * group = createGroupActionsOpacity( this );
    connect( group, SIGNAL( triggered(QAction*) ), SLOT( changeOpacity(QAction*)) );

    QMenu * menuOpacity = new QMenu( tr( "Opacity" ) );
    menuOpacity->setIcon( QIcon( ":/opacity" ) );
    menuOpacity->addActions( group->actions() );

    tButtonSetOpacity->setMenu( menuOpacity );
    tButtonSetOpacity->setPopupMode( QToolButton::MenuButtonPopup );


    connect( d->editor->document(), SIGNAL( contentsChanged() ), SLOT( contentsChanged() ) );
}

void RichTextNote::save()
{
    AbstractNote::blockSignals( true );
    AbstractNote::save();
    AbstractNote::blockSignals( false );

    QSettings ini( settingsFilePath(), QSettings::IniFormat );
    ini.setIniCodec( "utf8" );
    ini.setValue( "ReadOnly", isReadOnly() );
    ini.setValue( "Created", d->created );
    ini.setValue( "Modified", d->modified );

    saveContent();
}
void RichTextNote::saveContent()
{
    // Смысл сохраняться, если изменений не было?
    if( !d->isModified )
        return;

    QFile content( contentFilePath() );
    if ( !content.open( QIODevice::Truncate | QIODevice::WriteOnly ) )
    {
        QMessageBox::critical( this, tr( "Error" ), tr( "An error occurred saving notes" ) );
        return;
    }

    QTextStream in( &content );
    in.setCodec( "utf8" );
    in << text();
    content.close();

    d->isModified = false;
    updateStates();

    emit changed( EventsNote::SaveEnded );
}
void RichTextNote::load()
{
    AbstractNote::blockSignals( true );
    AbstractNote::load();
    AbstractNote::blockSignals( false );

    QSettings ini( settingsFilePath(), QSettings::IniFormat );
    ini.setIniCodec( "utf8" );
    setReadOnly( ini.value( "ReadOnly", false ).toBool() );

    const QDateTime & currentDateTime = QDateTime::currentDateTime();
    d->created = ini.value( "Created", currentDateTime ).toDateTime();
    d->modified = ini.value( "Modified", currentDateTime ).toDateTime();

    loadContent();
    updateStates();
}
void RichTextNote::loadContent()
{
    QFile content( contentFilePath() );
    if ( !content.open( QIODevice::ReadOnly ) )
    {
        qDebug() << "An error occurred reading notes";
        return;
    }

//    QTextStream in( &content );
//    in.setCodec( "utf8" );
//    setText( in.readAll() );
//    content.close();

    d->editor->setSource( QUrl::fromLocalFile( contentFilePath() ) );

    emit changed( EventsNote::LoadEnded );
}
void RichTextNote::setText( const QString & str )
{
    if ( text() == str )
        return;

    d->editor->setHtml( str );
}
QString RichTextNote::text()
{
    return d->editor->document()->toHtml( "utf-8" );
}
void RichTextNote::removeDir()
{
    bool successful = removePath( fileName() );
    if ( !successful )
    {
        QMessageBox::warning( this, tr( "Warning" ), tr( "I can not delete" ) );
        return;
    }
}
void RichTextNote::remove()
{
    invokeRemove();
    removeDir();
    close();
    deleteLater();
}
void RichTextNote::invokeRemove()
{
    emit changed( EventsNote::Remove );
}

void RichTextNote::setReadOnly( bool ro )
{
    if ( isReadOnly() == ro )
        return;

    d->editor->setReadOnly( ro );
    d->isReadOnly = ro;
    updateStates();
    emit changed( EventsNote::ChangeReadOnly );
}
bool RichTextNote::isReadOnly()
{
    return d->isReadOnly;
}

void RichTextNote::setTop( bool b )
{
    if ( isTop() == b )
        return;

    AbstractNote::setTop( b );
    updateStates();
}
void RichTextNote::setMinimize( bool b )
{
    if ( dockWidgetFormattingToolbar )
        dockWidgetFormattingToolbar->setHidden( b );

    AbstractNote::setMinimize( b );
}

void RichTextNote::selectTitle()
{
    const QString & text = QInputDialog::getText( this, tr( "Select title" ), tr( "Title: " ), QLineEdit::Normal, title() );
    if ( text.isEmpty() )
        return;

    setTitle( text );
}
void RichTextNote::selectTitleFont()
{
    bool ok;
    const QFont & font = QFontDialog::getFont( &ok, titleFont(), this, tr( "Select font" ) );
    if ( !ok )
        return;

    setTitleFont( font );
}

void RichTextNote::selectTitleColor()
{
    const QColor & color = QColorDialog::getColor( titleColor(), this, tr( "Select color" ) );
    if ( !color.isValid() )
        return;

    setTitleColor( color );
}
void RichTextNote::selectColor()
{
    const QColor & color = QColorDialog::getColor( backgroundColor(), this, tr( "Select color" ) );
    if ( !color.isValid() )
        return;

    setBackgroundColor( color );
}
void RichTextNote::selectOpacity()
{
    bool b;
    int op = QInputDialog::getInt( this, tr( "Select opacity" ), tr( "Opacity:" ), opacity() * 100.0, 20, 100, 1, &b);

    if ( !b )
        return;

    setOpacity( (qreal)op / 100.0 );
}

void RichTextNote::saveAs()
{
    QStringList imageFormats;
    foreach ( const QByteArray & format, QImageWriter::supportedImageFormats() )
        imageFormats << QString( format );

    QStringList textFormats;
    foreach ( const QByteArray & format, QTextDocumentWriter::supportedDocumentFormats() )
        textFormats << QString( format );

    QString filters;
    filters.append( tr( "File notes" ) + QString( " ( *.%1 )\n" ).arg( getNoteFormat() ) );
    filters.append( "TXT ( *.txt )\n" );
    filters.append( "PDF ( *.pdf )\n" );
    filters.append( "PSF ( *.psf )\n" );

    foreach ( const QString & format, textFormats )
        filters.append( QString( "%1 ( *.%2 )\n" ).arg( format.toUpper() ).arg( format ) );

    foreach ( const QString & format, imageFormats )
        filters.append( QString( "%1 ( *.%2 )\n" ).arg( format.toUpper() ).arg( format ) );

    const QString & saveFileName = QFileDialog::getSaveFileName( this, QString(), QString(), filters );
    if ( saveFileName.isEmpty() )
        return;

    QString suffix = QFileInfo( saveFileName ).suffix();

    // для создания pdf файлов используется другой класс
    if ( suffix.contains( "pdf", Qt::CaseInsensitive ) || suffix.contains( "psf", Qt::CaseInsensitive ) )
    {
        QPrinter printer;
        printer.setOutputFileName( saveFileName );
        textEditor()->print( &printer );

    } else if ( imageFormats.contains( suffix, Qt::CaseInsensitive ) )
    {
        QPixmap::grabWidget( this ).save( saveFileName );

    } else if ( suffix.contains( "txt", Qt::CaseInsensitive ) )
    {
        QTextDocumentWriter textDocumentWriter;
        textDocumentWriter.setFormat( suffix.toAscii() );
        textDocumentWriter.setCodec( QTextCodec::codecForName( "utf8" ) );
        textDocumentWriter.setFileName( saveFileName );
        textDocumentWriter.write( document() );

    } else if ( suffix.contains( "filenotes", Qt::CaseInsensitive ) )
    {
        if ( !JlCompress::compressDir( saveFileName, fileName() ) )
        {
            QMessageBox::information( this, tr( "Information" ), tr( "An error occurred saving notes" ) );
            qDebug() << tr( "An error occurred saving notes" );
            return;
        }
    }
}
void RichTextNote::open()
{   
    const QString & fileName = QFileDialog::getOpenFileName( this );
    if ( fileName.isEmpty() )
        return;

    QFile file( fileName );
    if ( !file.open( QIODevice::ReadOnly ) )
        return;

    QTextStream out( &file );
    out.setCodec( "utf8" );

    setText( out.readAll() );
}
void RichTextNote::print()
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setDocName( tr( "document" ) + QString( " \"%1\"" ).arg( title() ) );

    QPrintDialog dialog( &printer, this );
    if ( dialog.exec() == QDialog::Accepted )
        print( &printer );
}
void RichTextNote::previewPrint()
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setDocName( tr( "document" ) + QString( " \"%1\"" ).arg( title() ) );

    QPrintPreviewDialog preview( &printer, this, Qt::Dialog | Qt::WindowMaximizeButtonHint );

    connect( &preview, SIGNAL( paintRequested( QPrinter * ) ), SLOT( print( QPrinter * ) ) );
    preview.exec();
}
void RichTextNote::duplicate()
{
    emit changed( EventsNote::Duplicate );
}
void RichTextNote::selectAttach()
{
    QString dir = QDesktopServices::storageLocation( QDesktopServices::HomeLocation );
    QStringList files = QFileDialog::getOpenFileNames( this, tr( "Select the files" ), dir );
    if ( files.isEmpty() )
        return;

    foreach ( const QString & fileName, files )
        attach( fileName );
}

void RichTextNote::insertImage( const QString & fileName )
{
    QString newFileName = attach( fileName );
    QString relativePath = QFileInfo( attachDirPath() ).fileName() + "/" + QFileInfo( newFileName ).fileName();
    relativePath = QDir::toNativeSeparators( relativePath );

    document()->addResource( QTextDocument::ImageResource, QUrl( relativePath ), QImage( newFileName ) );
    d->editor->textCursor().insertImage( relativePath );
}
void RichTextNote::insertImage( const QPixmap & pixmap )
{
    // Сохраняем в папке прикрепленных файлов
    const QString & name = tr( "image" ) + QString( "_%1.png" ).arg( QDateTime::currentDateTime().toString( "hh-mm-ss_yyyy-MM-dd" ) );
    QString path = attachDirPath() + "/" + name;
    path = QDir::toNativeSeparators( path );

    if ( !pixmap.save( path ) )
    {
        qDebug() << "Произошла ошибка при сохранении";
        return;
    }

    insertImage( path );
    updateAttachList();
}

QString RichTextNote::attach( const QString & fileName )
{
    QString newFileName = attachDirPath() + QDir::separator() + QFileInfo( fileName ).fileName();
    QFile::copy( fileName, newFileName );

    attachModel.appendRow( new QStandardItem( QFileInfo( fileName ).fileName() ) );
    emit changed( EventsNote::ChangeAttach );
    return newFileName;
}
void RichTextNote::updateAttachList()
{
    attachModel.clear();
    foreach ( const QFileInfo & fileInfo, QDir( attachDirPath() ).entryInfoList( QDir::Files ) )
        attachModel.appendRow( new QStandardItem( fileInfo.fileName() ) );
}
int RichTextNote::numberOfAttachments()
{
    return QDir( attachDirPath() ).entryList( QDir::Files ).size();
}

void RichTextNote::changeOpacity( QAction * action )
{
    qreal o = action->data().toReal();
    if ( o < 0 )
        selectOpacity();
    else
        setOpacity( o );
}
void RichTextNote::print( QPrinter * printer )
{
    d->editor->print( printer );
}
void RichTextNote::updateStates()
{
    bool top = isTop();
    bool readOnly = isReadOnly();
    bool isModified = d->isModified;

    tButtonSetTopBottom->setChecked( top );
    actionSetTopBottom->setChecked( top );
    actionSetReadOnly->setChecked( readOnly );
    tButtonSetReadOnly->setChecked( readOnly );
    tButtonSave->setEnabled( isModified );
    actionSave->setEnabled( isModified );
    actionVisibleToolBar->setChecked( isVisibleToolBar() );
    actionVisibleQuickFind->setChecked( quickFind->isVisible() );
}
void RichTextNote::contentsChanged()
{
    d->modified = QDateTime::currentDateTime();
    d->isModified = true;
    updateStates();
    emit changed( EventsNote::ChangeText );
}

void RichTextNote::enterEvent( QEvent * )
{
    QPropertyAnimation * animation = new QPropertyAnimation( this, "windowOpacity" );
    animation->setDuration( 200 );
    animation->setStartValue( opacity() );
    animation->setEndValue( 1.0 );
    animation->start( QAbstractAnimation::DeleteWhenStopped );
}
void RichTextNote::leaveEvent( QEvent * )
{
    QPropertyAnimation * animation = new QPropertyAnimation( this, "windowOpacity" );
    animation->setDuration( 700 );
    animation->setStartValue( 1.0 );
    animation->setEndValue( opacity() );
    animation->start( QAbstractAnimation::DeleteWhenStopped );
}
