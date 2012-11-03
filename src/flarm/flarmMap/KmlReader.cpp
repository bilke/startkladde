#include "src/flarm/flarmMap/KmlReader.h"

#include <iostream>

#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QFile>
#include <QIODevice>
#include <QStringList>

#include "src/util/qString.h"

KmlReader::KmlReader ()
{
}

KmlReader::~KmlReader ()
{
}

void KmlReader::readStyle (const QDomNode &styleNode)
{
	QDomElement styleElement=styleNode.toElement ();
	if (styleElement.isNull ())
		return;

	QString styleId=styleElement.attribute ("id");

	Kml::Style style;
	style.labelColor=Kml::parseColor (styleElement.firstChildElement ("LabelStyle").firstChildElement ("color").text ());
	style.lineColor =Kml::parseColor (styleElement.firstChildElement ("LineStyle" ).firstChildElement ("color").text ());
	style.lineWidth =                 styleElement.firstChildElement ("LineStyle" ).firstChildElement ("width").text ().toDouble ();

	styles.insert (styleId, style);
}

void KmlReader::readStyleMap (const QDomNode &styleMapNode)
{
	QDomElement styleMapElement=styleMapNode.toElement ();
	if (styleMapElement.isNull ())
		return;

	QString styleMapId=styleMapElement.attribute ("id");

	Kml::StyleMap styleMap;
	QDomNodeList pairElements=styleMapElement.elementsByTagName ("Pair");
	for (int i=0, n=pairElements.size (); i<n; ++i)
	{
		QDomElement pairElement=pairElements.at (i).toElement ();
		QString key     =pairElement.firstChildElement ("key"     ).text ();
		QString styleUrl=pairElement.firstChildElement ("styleUrl").text ();
		styleMap.styles.insert (key, styleUrl);
	}

	styleMaps.insert (styleMapId, styleMap);
}

void KmlReader::readMarker (const QString &name, const QString &styleUrl, const QDomElement &lookAtElement)
{
	 double longitude=lookAtElement.firstChildElement ("longitude").text ().toDouble ();
	 double latitude =lookAtElement.firstChildElement ("latitude" ).text ().toDouble ();
	 GeoPosition position=GeoPosition::fromDegrees (latitude, longitude);

	 Kml::Marker marker;
	 marker.position=position;
	 marker.name=name;
	 marker.styleUrl=styleUrl;

	 addToBoundingRect (position);

	 markers.append (marker);
}

void KmlReader::readPath (const QString &name, const QString &styleUrl, const QDomElement &lineStringElement)
{
	 QList<GeoPosition> points;
	 QDomElement coordinatesElement=lineStringElement.firstChildElement ("coordinates");
	 foreach (const QString &pointCoordinates, coordinatesElement.text ().trimmed ().split (" "))
	 {
		 QStringList parts=pointCoordinates.split (",");
		 // FIXME only if exists
		 double longitude=parts[0].toDouble ();
		 double latitude =parts[1].toDouble ();
		 GeoPosition position=GeoPosition::fromDegrees (latitude, longitude);

		 points.append (position);

		 addToBoundingRect (position);
	 }

	 Kml::Path path;
	 path.name=name;
	 path.styleUrl=styleUrl;
	 path.positions=points;

	 paths.append (path);
}

void KmlReader::readPolygon (const QString &name, const QString &styleUrl, const QDomElement &polygonElement)
{
	QList<GeoPosition> points;
	QDomElement coordinatesElemnt=polygonElement
			.firstChildElement ("outerBoundaryIs")
			.firstChildElement ("LinearRing")
			.firstChildElement ("coordinates");
	foreach (const QString &pointCoordinates, coordinatesElemnt.text ().trimmed ().split (" "))
	{
		QStringList parts=pointCoordinates.split (",");
		// FIXME only if exists
		double longitude=parts[0].toDouble ();
		double latitude =parts[1].toDouble ();
		GeoPosition position=GeoPosition::fromDegrees (latitude, longitude);

		points.append (position);

		addToBoundingRect (position);
	}

	Kml::Polygon polygon;
	polygon.name=name;
	polygon.styleUrl=styleUrl;
	polygon.positions=points;

	polygons.append (polygon);
}

/**
 * A placemark is either a marker (LookAt element), a path (LineString element)
 * or a polygon (Polygon element).
 *
 * @param placemarkNode
 */
void KmlReader::readPlacemark (const QDomNode &placemarkNode)
{
	// FIXME is it possible that the node is not an element?
	 // FIXME only if it exists, or can we use it as a null element if not?
	 QString placemarkName=placemarkNode.firstChildElement ("name").text ();
	 QString styleUrl=placemarkNode.firstChildElement ("styleUrl").text ();

	 QDomElement lookAtElement=placemarkNode.firstChildElement ("LookAt");
	 if (!lookAtElement.isNull ())
		 readMarker (placemarkName, styleUrl, lookAtElement);

	 QDomElement lineStringElement=placemarkNode.firstChildElement ("LineString");
	 if (!lineStringElement.isNull ())
		 readPath (placemarkName, styleUrl, lineStringElement);

	 QDomElement polygonElement=placemarkNode.firstChildElement ("Polygon");
	 if (!polygonElement.isNull ())
		 readPolygon (placemarkName, styleUrl, polygonElement);
}

/**
 * This method is not reentrant because it calls a non-reentrant method of
 * QDomDocument.
 *
 * @param filename
 */
KmlReader::ReadResult KmlReader::read (const QString &filename)
{
	// Check if the file exists at all
	if (!QFile::exists (filename))
		return readNotFound;

	// Try to open the file
	QFile file (filename);
	if (!file.open (QIODevice::ReadOnly))
		return readOpenError;

	// Try to parse the file
	QDomDocument document ("kmlDocument");
	 if (!document.setContent (&file))
	     return readParseError;

	 // The file has been read and can be closed
	 file.close();

	 // Extract the placemarks (includes markers, paths and polygons) from the
	 // DOM structure
	 QDomNodeList placemarkNodes=document.elementsByTagName ("Placemark");
	 for (int i=0, n=placemarkNodes.size (); i<n; ++i)
		 readPlacemark (placemarkNodes.at (i));

	 // Extract the styles from the DOM structure
	 QDomNodeList styleNodes=document.elementsByTagName ("Style");
	 for (int i=0, n=styleNodes.size (); i<n; ++i)
		 readStyle (styleNodes.at (i));

	 // Extract the style maps from the DOM structure
	 QDomNodeList styleMapNodes=document.elementsByTagName ("StyleMap");
	 for (int i=0, n=styleMapNodes.size (); i<n; ++i)
		 readStyleMap (styleMapNodes.at (i));

	 return readOk;
}

bool KmlReader::isEmpty () const
{
	if (!markers .isEmpty ()) return false;
	if (!paths   .isEmpty ()) return false;
	if (!polygons.isEmpty ()) return false;
	return true;
}

Kml::Style KmlReader::findStyle (const QString &styleUrl)
{
	if (!styleUrl.startsWith ("#"))
		return Kml::Style ();

	QString styleId=styleUrl.mid (1);

	if (styles.contains (styleId))
		return styles.value (styleId);

	if (styleMaps.contains (styleId))
	{
		Kml::StyleMap styleMap=styleMaps.value (styleId);
		if (styleMap.styles.contains ("normal"))
			return findStyle (styleMap.styles.value ("normal"));
	}

	return Kml::Style ();
}

void KmlReader::addToBoundingRect (const GeoPosition &position)
{
	if (boundingRectSouthWest.isValid ())
		boundingRectSouthWest=GeoPosition::southWest (boundingRectSouthWest, position);
	else
		boundingRectSouthWest=position;

	if (boundingRectNorthEast.isValid ())
		boundingRectNorthEast=GeoPosition::northEast (boundingRectNorthEast, position);
	else
		boundingRectNorthEast=position;
}