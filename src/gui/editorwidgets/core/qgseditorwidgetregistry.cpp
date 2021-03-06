/***************************************************************************
    qgseditorwidgetregistry.cpp
     --------------------------------------
    Date                 : 24.4.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgseditorwidgetregistry.h"

#include "qgsattributeeditorcontext.h"
#include "qgsmessagelog.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgseditorwidgetwrapper.h"
#include "qgssearchwidgetwrapper.h"

// Editors
#include "qgsclassificationwidgetwrapperfactory.h"
#include "qgscheckboxwidgetfactory.h"
#include "qgscolorwidgetfactory.h"
#include "qgsdatetimeeditfactory.h"
#include "qgsenumerationwidgetfactory.h"
#include "qgsexternalresourcewidgetfactory.h"
#include "qgsfilenamewidgetfactory.h"
#include "qgshiddenwidgetfactory.h"
#include "qgskeyvaluewidgetfactory.h"
#include "qgslistwidgetfactory.h"
#include "qgsphotowidgetfactory.h"
#include "qgsrangewidgetfactory.h"
#include "qgsrelationreferencefactory.h"
#include "qgstexteditwidgetfactory.h"
#include "qgsuniquevaluewidgetfactory.h"
#include "qgsuuidwidgetfactory.h"
#include "qgsvaluemapwidgetfactory.h"
#include "qgsvaluerelationwidgetfactory.h"
#ifdef WITH_QTWEBKIT
#include "qgswebviewwidgetfactory.h"
#endif


QgsEditorWidgetRegistry* QgsEditorWidgetRegistry::instance()
{
  static QgsEditorWidgetRegistry sInstance;
  return &sInstance;
}

void QgsEditorWidgetRegistry::initEditors( QgsMapCanvas *mapCanvas, QgsMessageBar *messageBar )
{
  QgsEditorWidgetRegistry *reg = instance();
  reg->registerWidget( QStringLiteral( "TextEdit" ), new QgsTextEditWidgetFactory( tr( "Text Edit" ) ) );
  reg->registerWidget( QStringLiteral( "Classification" ), new QgsClassificationWidgetWrapperFactory( tr( "Classification" ) ) );
  reg->registerWidget( QStringLiteral( "Range" ), new QgsRangeWidgetFactory( tr( "Range" ) ) );
  reg->registerWidget( QStringLiteral( "UniqueValues" ), new QgsUniqueValueWidgetFactory( tr( "Unique Values" ) ) );
  reg->registerWidget( QStringLiteral( "FileName" ), new QgsFileNameWidgetFactory( tr( "File Name" ) ) );
  reg->registerWidget( QStringLiteral( "ValueMap" ), new QgsValueMapWidgetFactory( tr( "Value Map" ) ) );
  reg->registerWidget( QStringLiteral( "Enumeration" ), new QgsEnumerationWidgetFactory( tr( "Enumeration" ) ) );
  reg->registerWidget( QStringLiteral( "Hidden" ), new QgsHiddenWidgetFactory( tr( "Hidden" ) ) );
  reg->registerWidget( QStringLiteral( "CheckBox" ), new QgsCheckboxWidgetFactory( tr( "Check Box" ) ) );
  reg->registerWidget( QStringLiteral( "ValueRelation" ), new QgsValueRelationWidgetFactory( tr( "Value Relation" ) ) );
  reg->registerWidget( QStringLiteral( "UuidGenerator" ), new QgsUuidWidgetFactory( tr( "Uuid Generator" ) ) );
  reg->registerWidget( QStringLiteral( "Photo" ), new QgsPhotoWidgetFactory( tr( "Photo" ) ) );
#ifdef WITH_QTWEBKIT
  reg->registerWidget( QStringLiteral( "WebView" ), new QgsWebViewWidgetFactory( tr( "Web View" ) ) );
#endif
  reg->registerWidget( QStringLiteral( "Color" ), new QgsColorWidgetFactory( tr( "Color" ) ) );
  reg->registerWidget( QStringLiteral( "RelationReference" ), new QgsRelationReferenceFactory( tr( "Relation Reference" ), mapCanvas, messageBar ) );
  reg->registerWidget( QStringLiteral( "DateTime" ), new QgsDateTimeEditFactory( tr( "Date/Time" ) ) );
  reg->registerWidget( QStringLiteral( "ExternalResource" ), new QgsExternalResourceWidgetFactory( tr( "External Resource" ) ) );
  reg->registerWidget( QStringLiteral( "KeyValue" ), new QgsKeyValueWidgetFactory( tr( "Key/Value" ) ) );
  reg->registerWidget( QStringLiteral( "List" ), new QgsListWidgetFactory( tr( "List" ) ) );
}

QgsEditorWidgetRegistry::QgsEditorWidgetRegistry()
{
  connect( QgsProject::instance(), SIGNAL( readMapLayer( QgsMapLayer*, const QDomElement& ) ), this, SLOT( readMapLayer( QgsMapLayer*, const QDomElement& ) ) );
  // connect( QgsProject::instance(), SIGNAL( writeMapLayer( QgsMapLayer*, QDomElement&, QDomDocument& ) ), this, SLOT( writeMapLayer( QgsMapLayer*, QDomElement&, QDomDocument& ) ) );

  connect( QgsProject::instance(), SIGNAL( layerWillBeRemoved( QgsMapLayer* ) ), this, SLOT( mapLayerWillBeRemoved( QgsMapLayer* ) ) );
  connect( QgsProject::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( mapLayerAdded( QgsMapLayer* ) ) );
}

QgsEditorWidgetRegistry::~QgsEditorWidgetRegistry()
{
  qDeleteAll( mWidgetFactories );
}

QgsEditorWidgetSetup QgsEditorWidgetRegistry::findBest( const QgsVectorLayer* vl, const QString& fieldName ) const
{
  const QString fromConfig = vl->editFormConfig().widgetType( fieldName );
  if ( !fromConfig.isNull() )
  {
    return QgsEditorWidgetSetup( fromConfig, vl->editFormConfig().widgetConfig( fieldName ) );
  }
  return mAutoConf.editorWidgetSetup( vl, fieldName );
}

QgsEditorWidgetWrapper* QgsEditorWidgetRegistry::create( QgsVectorLayer* vl, int fieldIdx, QWidget* editor, QWidget* parent, const QgsAttributeEditorContext &context )
{
  const QString fieldName = vl->fields().field( fieldIdx ).name();
  const QgsEditorWidgetSetup setup = findBest( vl, fieldName );
  return create( setup.type(), vl, fieldIdx, setup.config(), editor, parent, context );
}

QgsEditorWidgetWrapper* QgsEditorWidgetRegistry::create( const QString& widgetId, QgsVectorLayer* vl, int fieldIdx, const QgsEditorWidgetConfig& config, QWidget* editor, QWidget* parent, const QgsAttributeEditorContext &context )
{
  if ( mWidgetFactories.contains( widgetId ) )
  {
    QgsEditorWidgetWrapper* ww = mWidgetFactories[widgetId]->create( vl, fieldIdx, editor, parent );

    if ( ww )
    {
      ww->setConfig( config );
      ww->setContext( context );
      // Make sure that there is a widget created at this point
      // so setValue() et al won't crash
      ww->widget();

      // If we tried to set a widget which is not supported by this wrapper
      if ( !ww->valid() )
      {
        delete ww;
        QString wid = findSuitableWrapper( editor, QStringLiteral( "TextEdit" ) );
        ww = mWidgetFactories[wid]->create( vl, fieldIdx, editor, parent );
        ww->setConfig( config );
        ww->setContext( context );
      }

      return ww;
    }
  }

  return nullptr;
}

QgsSearchWidgetWrapper* QgsEditorWidgetRegistry::createSearchWidget( const QString& widgetId, QgsVectorLayer* vl, int fieldIdx, const QgsEditorWidgetConfig& config, QWidget* parent, const QgsAttributeEditorContext &context )
{
  if ( mWidgetFactories.contains( widgetId ) )
  {
    QgsSearchWidgetWrapper* ww = mWidgetFactories[widgetId]->createSearchWidget( vl, fieldIdx, parent );

    if ( ww )
    {
      ww->setConfig( config );
      ww->setContext( context );
      // Make sure that there is a widget created at this point
      // so setValue() et al won't crash
      ww->widget();
      ww->clearWidget();
      return ww;
    }
  }
  return nullptr;
}

QgsEditorConfigWidget* QgsEditorWidgetRegistry::createConfigWidget( const QString& widgetId, QgsVectorLayer* vl, int fieldIdx, QWidget* parent )
{
  if ( mWidgetFactories.contains( widgetId ) )
  {
    return mWidgetFactories[widgetId]->configWidget( vl, fieldIdx, parent );
  }
  return nullptr;
}

QString QgsEditorWidgetRegistry::name( const QString& widgetId )
{
  if ( mWidgetFactories.contains( widgetId ) )
  {
    return mWidgetFactories[widgetId]->name();
  }

  return QString();
}

const QMap<QString, QgsEditorWidgetFactory*>& QgsEditorWidgetRegistry::factories()
{
  return mWidgetFactories;
}

QgsEditorWidgetFactory* QgsEditorWidgetRegistry::factory( const QString& widgetId )
{
  return mWidgetFactories.value( widgetId );
}

bool QgsEditorWidgetRegistry::registerWidget( const QString& widgetId, QgsEditorWidgetFactory* widgetFactory )
{
  if ( !widgetFactory )
  {
    QgsMessageLog::instance()->logMessage( QStringLiteral( "QgsEditorWidgetRegistry: Factory not valid." ) );
    return false;
  }
  else if ( mWidgetFactories.contains( widgetId ) )
  {
    QgsMessageLog::instance()->logMessage( QStringLiteral( "QgsEditorWidgetRegistry: Factory with id %1 already registered." ).arg( widgetId ) );
    return false;
  }
  else
  {
    mWidgetFactories.insert( widgetId, widgetFactory );

    // Use this factory as default where it provides the heighest priority
    QHash<const char*, int> types = widgetFactory->supportedWidgetTypes();
    QHash<const char*, int>::ConstIterator it;
    it = types.constBegin();

    for ( ; it != types.constEnd(); ++it )
    {
      if ( it.value() > mFactoriesByType[it.key()].first )
      {
        mFactoriesByType[it.key()] = qMakePair( it.value(), widgetId );
      }
    }

    return true;
  }
}

void QgsEditorWidgetRegistry::readMapLayer( QgsMapLayer* mapLayer, const QDomElement& layerElem )
{
  if ( mapLayer->type() != QgsMapLayer::VectorLayer )
  {
    return;
  }

  QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>( mapLayer );
  Q_ASSERT( vectorLayer );

  QDomNodeList editTypeNodes = layerElem.namedItem( QStringLiteral( "edittypes" ) ).childNodes();

  QgsEditFormConfig formConfig = vectorLayer->editFormConfig();

  for ( int i = 0; i < editTypeNodes.size(); i++ )
  {
    QDomNode editTypeNode = editTypeNodes.at( i );
    QDomElement editTypeElement = editTypeNode.toElement();

    QString name = editTypeElement.attribute( QStringLiteral( "name" ) );

    int idx = vectorLayer->fields().lookupField( name );
    if ( idx == -1 )
      continue;

    QString ewv2Type = editTypeElement.attribute( QStringLiteral( "widgetv2type" ) );
    QgsEditorWidgetConfig cfg;

    if ( mWidgetFactories.contains( ewv2Type ) )
    {
      formConfig.setWidgetType( name, ewv2Type );
      QDomElement ewv2CfgElem = editTypeElement.namedItem( QStringLiteral( "widgetv2config" ) ).toElement();

      if ( !ewv2CfgElem.isNull() )
      {
        cfg = mWidgetFactories[ewv2Type]->readEditorConfig( ewv2CfgElem, vectorLayer, idx );
      }

      formConfig.setReadOnly( idx, ewv2CfgElem.attribute( QStringLiteral( "fieldEditable" ), QStringLiteral( "1" ) ) != QLatin1String( "1" ) );
      formConfig.setLabelOnTop( idx, ewv2CfgElem.attribute( QStringLiteral( "labelOnTop" ), QStringLiteral( "0" ) ) == QLatin1String( "1" ) );
      if ( ewv2CfgElem.attribute( QStringLiteral( "notNull" ), QStringLiteral( "0" ) ) == QLatin1String( "1" ) )
      {
        // upgrade from older config
        vectorLayer->setFieldConstraint( idx, QgsFieldConstraints::ConstraintNotNull );
      }
      if ( !ewv2CfgElem.attribute( QStringLiteral( "constraint" ), QString() ).isEmpty() )
      {
        // upgrade from older config
        vectorLayer->setConstraintExpression( idx, ewv2CfgElem.attribute( QStringLiteral( "constraint" ), QString() ),
                                              ewv2CfgElem.attribute( QStringLiteral( "constraintDescription" ), QString() ) );
      }

      formConfig.setWidgetConfig( name, cfg );
    }
    else
    {
      QgsMessageLog::logMessage( tr( "Unknown attribute editor widget '%1'" ).arg( ewv2Type ) );
    }
  }

  vectorLayer->setEditFormConfig( formConfig );
}

void QgsEditorWidgetRegistry::writeMapLayer( QgsMapLayer* mapLayer, QDomElement& layerElem, QDomDocument& doc ) const
{
  if ( mapLayer->type() != QgsMapLayer::VectorLayer )
  {
    return;
  }

  QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>( mapLayer );
  if ( !vectorLayer )
  {
    return;
  }

  QDomNode editTypesNode = doc.createElement( QStringLiteral( "edittypes" ) );

  QgsFields fields = vectorLayer->fields();
  for ( int idx = 0; idx < fields.count(); ++idx )
  {
    const QgsField field = fields.at( idx );
    const QString& widgetType = vectorLayer->editFormConfig().widgetType( field.name() );
    if ( widgetType.isNull() )
    {
      // Don't save widget config if it is not manually edited
      continue;
    }
    if ( !mWidgetFactories.contains( widgetType ) )
    {
      QgsMessageLog::logMessage( tr( "Could not save unknown editor widget type '%1'." ).arg( widgetType ) );
      continue;
    }


    QDomElement editTypeElement = doc.createElement( QStringLiteral( "edittype" ) );
    editTypeElement.setAttribute( QStringLiteral( "name" ), field.name() );
    editTypeElement.setAttribute( QStringLiteral( "widgetv2type" ), widgetType );

    if ( mWidgetFactories.contains( widgetType ) )
    {
      QDomElement ewv2CfgElem = doc.createElement( QStringLiteral( "widgetv2config" ) );
      ewv2CfgElem.setAttribute( QStringLiteral( "fieldEditable" ), !vectorLayer->editFormConfig().readOnly( idx ) );
      ewv2CfgElem.setAttribute( QStringLiteral( "labelOnTop" ), vectorLayer->editFormConfig().labelOnTop( idx ) );

      mWidgetFactories[widgetType]->writeConfig( vectorLayer->editFormConfig().widgetConfig( field.name() ), ewv2CfgElem, doc, vectorLayer, idx );

      editTypeElement.appendChild( ewv2CfgElem );
    }

    editTypesNode.appendChild( editTypeElement );
  }

  layerElem.appendChild( editTypesNode );
}

void QgsEditorWidgetRegistry::mapLayerWillBeRemoved( QgsMapLayer* mapLayer )
{
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( mapLayer );

  if ( vl )
  {
    disconnect( vl, SIGNAL( readCustomSymbology( const QDomElement&, QString& ) ), this, SLOT( readSymbology( const QDomElement&, QString& ) ) );
    disconnect( vl, SIGNAL( writeCustomSymbology( QDomElement&, QDomDocument&, QString& ) ), this, SLOT( writeSymbology( QDomElement&, QDomDocument&, QString& ) ) );
  }
}

void QgsEditorWidgetRegistry::mapLayerAdded( QgsMapLayer* mapLayer )
{
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( mapLayer );

  if ( vl )
  {
    connect( vl, SIGNAL( readCustomSymbology( const QDomElement&, QString& ) ), this, SLOT( readSymbology( const QDomElement&, QString& ) ) );
    connect( vl, SIGNAL( writeCustomSymbology( QDomElement&, QDomDocument&, QString& ) ), this, SLOT( writeSymbology( QDomElement&, QDomDocument&, QString& ) ) );
  }
}

void QgsEditorWidgetRegistry::readSymbology( const QDomElement& element, QString& errorMessage )
{
  Q_UNUSED( errorMessage )
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( sender() );

  Q_ASSERT( vl );

  readMapLayer( vl, element );
}

void QgsEditorWidgetRegistry::writeSymbology( QDomElement& element, QDomDocument& doc, QString& errorMessage )
{
  Q_UNUSED( errorMessage )
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( sender() );

  Q_ASSERT( vl );

  writeMapLayer( vl, element, doc );
}

QString QgsEditorWidgetRegistry::findSuitableWrapper( QWidget* editor, const QString& defaultWidget )
{
  QMap<const char*, QPair<int, QString> >::ConstIterator it;

  QString widgetid;

  // Editor can be null
  if ( editor )
  {
    int weight = 0;

    it = mFactoriesByType.constBegin();
    for ( ; it != mFactoriesByType.constEnd(); ++it )
    {
      if ( editor->staticMetaObject.className() == it.key() )
      {
        // if it's a perfect match: return it directly
        return it.value().second;
      }
      else if ( editor->inherits( it.key() ) )
      {
        // if it's a subclass, continue evaluating, maybe we find a more-specific or one with more weight
        if ( it.value().first > weight )
        {
          weight = it.value().first;
          widgetid = it.value().second;
        }
      }
    }
  }

  if ( widgetid.isNull() )
    widgetid = defaultWidget;
  return widgetid;
}
