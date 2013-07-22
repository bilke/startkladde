#include "src/io/dataStream/SerialDataStream.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include "3rdparty/qserialdevice/src/qserialdevice/abstractserial.h"
#include "3rdparty/qserialdevice/src/qserialdeviceenumerator/serialdeviceenumerator.h"

#include "src/i18n/notr.h"
#include "src/text.h"

/*
 * Note: this class is thread safe. Keep it that way.
 */

// ******************
// ** Construction **
// ******************

SerialDataStream::SerialDataStream (QObject *parent): DataStream (parent),
	_parameterMutex (new QMutex (QMutex::NonRecursive)),
	_backEndMutex   (new QMutex (QMutex::NonRecursive)),
	_baudRate (0)
{
	SerialDeviceEnumerator *serialDeviceEnumerator=SerialDeviceEnumerator::instance ();
	connect (serialDeviceEnumerator, SIGNAL (hasChanged            (const QStringList &)),
	         this                  , SLOT   (availablePortsChanged (const QStringList &)));

	// Create the port and connect the required signals. _port will be deleted
	// automatically by its parent.
	_port=new AbstractSerial (this);
	_port->enableEmitStatus (true);
	connect (_port, SIGNAL (readyRead         ()),
	         this , SLOT   (port_dataReceived ()));
	connect (_port, SIGNAL (signalStatus (const QString &, QDateTime)),
	         this , SLOT   (port_status  (const QString &, QDateTime)));

}

SerialDataStream::~SerialDataStream ()
{
	delete _parameterMutex;
	delete _backEndMutex;
}


// ****************
// ** Parameters **
// ****************

/**
 * Sets the port and baud rate to use when the stream is opened the next time.
 *
 * This method is thread safe.
 */
void SerialDataStream::setPort (const QString &portName, int baudRate)
{
	QMutexLocker parameterLocker (_parameterMutex);

	_baudRate=baudRate;
	_portName=portName;
}


// ************************
// ** DataStream methods **
// ************************

/**
 * Implementation of DataStream::openStream ().
 *
 * This method is thread safe.
 */
void SerialDataStream::openStream ()
{
	// Lock the parameter mutex and make a copy of the parameters. Unlock the
	// parameter mutex afterwards.
	QMutexLocker parameterLocker (_parameterMutex);
	QString portName=_portName;
	int baudRate=_baudRate;
	parameterLocker.unlock ();

	if (isBlank (portName))
	{
		streamError (tr ("No port specified"));
		return;
	}

	// Lock the back-end mutex and open the port. Unlock the mutex before
	// calling the base class. It will also be unlocked when the method returns.
	QMutexLocker backEndLocker (_backEndMutex);

	QStringList availableDevices=SerialDeviceEnumerator::instance ()->devicesAvailable ();
	if (!availableDevices.contains (portName, Qt::CaseInsensitive))
	{
		backEndLocker.unlock ();
		streamError (tr ("The port %1 does not exist").arg (portName));
		return;
	}

	// Open the port and check success.
	_port->setDeviceName (portName);
	_port->open (QIODevice::ReadOnly);
	if (_port->isOpen ())
	{
		// Configure the port. The port is currently configured as the last user of
		// the port left it.
		_port->setBaudRate    (baudRate);
		_port->setDataBits    (AbstractSerial::DataBits8);
		_port->setParity      (AbstractSerial::ParityNone);
		_port->setStopBits    (AbstractSerial::StopBits1);
		_port->setFlowControl (AbstractSerial::FlowControlOff);
		_port->setDtr         (true);
		_port->setRts         (true);

		backEndLocker.unlock ();
		streamOpened ();
	}
	else
	{
		backEndLocker.unlock ();
		streamError (tr ("Connection did not open"));
	}
}

/**
 * Implementation of DataStream::closeStream ().
 *
 * This method is thread safe.
 */
void SerialDataStream::closeStream ()
{
	// Lock the back-end mutex and close the port.
	QMutexLocker backEndLocker (_backEndMutex);

	_port->close ();
}


// *****************
// ** Port events **
// *****************

/**
 * Called when data is received from the port
 *
 * This method is thread safe.
 */
void SerialDataStream::port_dataReceived ()
{
	// Lock the back-end mutex and read the available data. Unlock the mutex
	// before calling the base class.
	QMutexLocker backEndLocker (_backEndMutex);

	QByteArray data=_port->readAll ();

	backEndLocker.unlock ();
	dataReceived (data);
}

/**
 * This method is thread safe.
 */
void SerialDataStream::port_status (const QString &status, QDateTime dateTime)
{
	// This method does not access any properties or call any methods. It is
	// therefore inherently thread safe and does not need locking.

	Q_UNUSED (status);
	Q_UNUSED (dateTime);
	//qDebug () << "Port status" << status << dateTime;
}

/**
 * This method is thread safe.
 */
void SerialDataStream::availablePortsChanged (const QStringList &ports)
{
	// Lock the parameters mutex and make a copy of the configured port name.
	// Unlock the mutex afterwards.
	QMutexLocker parameterLocker (_parameterMutex);
	QString configuredPortName=_portName;
	parameterLocker.unlock ();


	// Lock the back-end mutex, find out if our port exists, and react by
	// closing the connection or emitting a signal if appropriate. Unlock the
	// mutex before calling the base class.
	QMutexLocker backEndLocker (_backEndMutex);

	if (_port && _port->isOpen ())
	{
		// FIXME must use the actually used port name here
		if (!ports.contains (configuredPortName, Qt::CaseInsensitive))
		{
			// Oops - our port is not available any more
			_port->close ();

			backEndLocker.unlock ();
			streamError (tr ("The port is no longer available"));
		}
	}
	else
	{
		// FIXME the criterion should be: did not contain it before.
		if (ports.contains (configuredPortName, Qt::CaseInsensitive))
		{
			backEndLocker.unlock ();
			streamConnectionBecameAvailable ();
		}
	}
}
