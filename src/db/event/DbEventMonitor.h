/*
 * DbEventDbEventMonitor.h
 *
 *  Created on: 28.02.2010
 *      Author: Martin Herrmann
 */

#ifndef DBEVENTDbEventMonitor_H_
#define DBEVENTDbEventMonitor_H_

#include <QObject>

#include "src/db/event/DbEvent.h"

/**
 * Implements the listener pattern for signals with a Db::Event::Event
 * parameter.
 *
 * This is useful for template classes which are to react to database events
 * because template classes cannot be QObjects and thus cannot define slots.
 * An alternative solution may be to use a QObject base class to the template
 * class, but that usually involves diamond inheritance.
 */
namespace Db
{
	namespace Event
	{
		class DbEventMonitor: public QObject
		{
			Q_OBJECT

			public:
				// *** Types
				class Listener
				{
					public:
						virtual void dbEvent (DbEvent event)=0;
				};

				// *** Construction
				DbEventMonitor (QObject &source, const char *signal, Listener &listener);
				virtual ~DbEventMonitor ();

			public slots:
				virtual void dbEvent (Db::Event::DbEvent event); // full type name

			private:
				Listener &listener;
		};
	}
}

#endif
