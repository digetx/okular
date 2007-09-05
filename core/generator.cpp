/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator.h"
#include "generator_p.h"

#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>

#include "document.h"
#include "page.h"

using namespace Okular;

GeneratorPrivate::GeneratorPrivate( Generator *parent )
    : m_document( 0 ), m_about( 0 ), m_componentData( 0 ),
      m_generator( parent ),
      mPixmapGenerationThread( 0 ), mTextPageGenerationThread( 0 ),
      mPixmapReady( true ), mTextPageReady( true )
{
}

GeneratorPrivate::~GeneratorPrivate()
{
    if ( mPixmapGenerationThread )
        mPixmapGenerationThread->wait();

    delete mPixmapGenerationThread;

    if ( mTextPageGenerationThread )
        mTextPageGenerationThread->wait();

    delete mTextPageGenerationThread;

    // first delete the component data, then the about
    delete m_componentData;
    delete m_about;
}

PixmapGenerationThread* GeneratorPrivate::pixmapGenerationThread()
{
    if ( mPixmapGenerationThread )
        return mPixmapGenerationThread;

    mPixmapGenerationThread = new PixmapGenerationThread( m_generator );
    QObject::connect( mPixmapGenerationThread, SIGNAL( finished() ),
                      m_generator, SLOT( pixmapGenerationFinished() ),
                      Qt::QueuedConnection );

    return mPixmapGenerationThread;
}

TextPageGenerationThread* GeneratorPrivate::textPageGenerationThread()
{
    if ( mTextPageGenerationThread )
        return mTextPageGenerationThread;

    mTextPageGenerationThread = new TextPageGenerationThread( m_generator );
    QObject::connect( mTextPageGenerationThread, SIGNAL( finished() ),
                      m_generator, SLOT( textpageGenerationFinished() ),
                      Qt::QueuedConnection );

    return mTextPageGenerationThread;
}

void GeneratorPrivate::pixmapGenerationFinished()
{
    PixmapRequest *request = mPixmapGenerationThread->request();
    mPixmapGenerationThread->endGeneration();

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( mPixmapGenerationThread->image() ) ) );

    mPixmapReady = true;

    m_generator->signalPixmapRequestDone( request );
}

void GeneratorPrivate::textpageGenerationFinished()
{
    Page *page = mTextPageGenerationThread->page();
    mTextPageGenerationThread->endGeneration();

    mTextPageReady = true;

    if ( mTextPageGenerationThread->textPage() )
        page->setTextPage( mTextPageGenerationThread->textPage() );
}


Generator::Generator()
    : d( new GeneratorPrivate( this ) )
{
}

Generator::~Generator()
{
    delete d;
}

bool Generator::loadDocumentFromData( const QByteArray &, QVector< Page * > & )
{
    return false;
}

bool Generator::canGeneratePixmap() const
{
    return d->mPixmapReady;
}

void Generator::generatePixmap( PixmapRequest *request )
{
    d->mPixmapReady = false;

    if ( hasFeature( Threaded ) )
    {
        d->pixmapGenerationThread()->startGeneration( request );

        /**
         * We create the text page for every page that is visible to the
         * user, so he can use the text extraction tools without a delay.
         */
        if ( hasFeature( TextExtraction ) && !request->page()->hasTextPage() && canGenerateTextPage() ) {
            d->mTextPageReady = false;
            d->textPageGenerationThread()->startGeneration( request->page() );
        }

        return;
    }

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( image( request ) ) ) );

    d->mPixmapReady = true;

    d->m_generator->signalPixmapRequestDone( request );
}

bool Generator::canGenerateTextPage() const
{
    return d->mTextPageReady;
}

void Generator::generateTextPage( Page *page )
{
    page->setTextPage( textPage( page ) );
    d->mTextPageReady = true;
}

QImage Generator::image( PixmapRequest * )
{
    return QImage();
}

TextPage* Generator::textPage( Page* )
{
    return 0;
}

const DocumentInfo * Generator::generateDocumentInfo()
{
    return 0;
}

const DocumentSynopsis * Generator::generateDocumentSynopsis()
{
    return 0;
}

FontInfo::List Generator::fontsForPage( int )
{
    return FontInfo::List();
}

const QList<EmbeddedFile*> * Generator::embeddedFiles() const
{
    return 0;
}

Generator::PageSizeMetric Generator::pagesSizeMetric() const
{
    return None;
}

bool Generator::isAllowed( Permission ) const
{
    return true;
}

void Generator::rotationChanged( Rotation, Rotation )
{
}

PageSize::List Generator::pageSizes() const
{
    return PageSize::List();
}

void Generator::pageSizeChanged( const PageSize &, const PageSize & )
{
}

bool Generator::print( KPrinter& )
{
    return false;
}

QVariant Generator::metaData( const QString&, const QVariant& ) const
{
    return QVariant();
}

ExportFormat::List Generator::exportFormats() const
{
    return ExportFormat::List();
}

bool Generator::exportTo( const QString&, const ExportFormat& )
{
    return false;
}

bool Generator::hasFeature( GeneratorFeature feature ) const
{
    return d->m_features.contains( feature );
}

const KComponentData* Generator::ownComponentData() const
{
    return d->m_componentData;
}

void Generator::signalPixmapRequestDone( PixmapRequest * request )
{
    if ( d->m_document )
        d->m_document->requestDone( request );
    else
        Q_ASSERT( !"No document set for generator in signalPixmapRequestDone!" );
}

const Document * Generator::document() const
{
    return d->m_document;
}

void Generator::setFeature( GeneratorFeature feature, bool on )
{
    if ( on )
        d->m_features.insert( feature );
    else
        d->m_features.remove( feature );
}

void Generator::setAboutData( KAboutData* data )
{
    delete d->m_componentData;
    delete d->m_about;
    d->m_about = data;
    d->m_componentData = d->m_about ? new KComponentData( d->m_about ) : 0;
}


class PixmapRequest::Private
{
    public:
        int mId;
        int mPageNumber;
        int mWidth;
        int mHeight;
        int mPriority;
        bool mAsynchronous;
        Page *mPage;
};


PixmapRequest::PixmapRequest( int id, int pageNumber, int width, int height, int priority, bool asynchronous )
  : d( new Private )
{
    d->mId = id;
    d->mPageNumber = pageNumber;
    d->mWidth = width;
    d->mHeight = height;
    d->mPriority = priority;
    d->mAsynchronous = asynchronous;
}

PixmapRequest::~PixmapRequest()
{
    delete d;
}

int PixmapRequest::id() const
{
    return d->mId;
}

int PixmapRequest::pageNumber() const
{
    return d->mPageNumber;
}

int PixmapRequest::width() const
{
    return d->mWidth;
}

int PixmapRequest::height() const
{
    return d->mHeight;
}

int PixmapRequest::priority() const
{
    return d->mPriority;
}

bool PixmapRequest::asynchronous() const
{
    return d->mAsynchronous;
}

Page* PixmapRequest::page() const
{
    return d->mPage;
}

void PixmapRequest::setPriority( int priority )
{
    d->mPriority = priority;
}

void PixmapRequest::setAsynchronous( bool asynchronous )
{
    d->mAsynchronous = asynchronous;
}

void PixmapRequest::setPage( Page *page )
{
    d->mPage = page;
}

void PixmapRequest::swap()
{
    qSwap( d->mWidth, d->mHeight );
}

class Okular::ExportFormatPrivate : public QSharedData
{
    public:
        ExportFormatPrivate( const QString &description, const KMimeType::Ptr &mimeType, const KIcon &icon = KIcon() )
            : QSharedData(), mDescription( description ), mMimeType( mimeType ), mIcon( icon )
        {
        }
        ~ExportFormatPrivate()
        {
        }

        QString mDescription;
        KMimeType::Ptr mMimeType;
        KIcon mIcon;
};

ExportFormat::ExportFormat()
    : d( new ExportFormatPrivate( QString(), KMimeType::Ptr() ) )
{
}

ExportFormat::ExportFormat( const QString &description, const KMimeType::Ptr &mimeType )
    : d( new ExportFormatPrivate( description, mimeType ) )
{
}

ExportFormat::ExportFormat( const KIcon &icon, const QString &description, const KMimeType::Ptr &mimeType )
    : d( new ExportFormatPrivate( description, mimeType, icon ) )
{
}

ExportFormat::~ExportFormat()
{
}

ExportFormat::ExportFormat( const ExportFormat &other )
    : d( other.d )
{
}

ExportFormat& ExportFormat::operator=( const ExportFormat &other )
{
    if ( this == &other )
        return *this;

    d = other.d;

    return *this;
}

QString ExportFormat::description() const
{
    return d->mDescription;
}

KMimeType::Ptr ExportFormat::mimeType() const
{
    return d->mMimeType;
}

KIcon ExportFormat::icon() const
{
    return d->mIcon;
}

bool ExportFormat::isNull() const
{
    return d->mMimeType.isNull() || d->mDescription.isNull();
}

ExportFormat ExportFormat::standardFormat( StandardExportFormat type )
{
    switch ( type )
    {
        case PlainText:
            return ExportFormat( KIcon( "text" ), i18n( "Plain &Text..." ), KMimeType::mimeType( "text/plain" ) );
            break;
        case PDF:
            return ExportFormat( i18n( "PDF" ), KMimeType::mimeType( "application/pdf" ) );
            break;
    }
    return ExportFormat();
}

bool ExportFormat::operator==( const ExportFormat &other ) const
{
    return d == other.d;
}

bool ExportFormat::operator!=( const ExportFormat &other ) const
{
    return d != other.d;
}

QDebug operator<<( QDebug str, const Okular::PixmapRequest &req )
{
    QString s = QString( "%1 PixmapRequest (id: %2) (%3x%4), prio %5, pageNo %6" )
        .arg( QString( req.asynchronous() ? "Async" : "Sync" ) )
        .arg( req.id() )
        .arg( req.width() )
        .arg( req.height() )
        .arg( req.priority() )
        .arg( req.pageNumber() );
    str << s;
    return str;
}

#include "generator.moc"