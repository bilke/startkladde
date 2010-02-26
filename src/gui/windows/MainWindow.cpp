#include "MainWindow.h"

#include <QAction>
#include <QSettings>
#include <QFontDialog>
#include <QTimer>
#include <QGridLayout>
#include <QProgressDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QList>
#include <QModelIndex>

#include "build/kvkbd.xpm"
#include "build/logo.xpm"

// TODO many dependencies - split
#include "src/concurrent/DefaultQThread.h"
#include "src/concurrent/threadUtil.h"
#include "src/concurrent/task/SleepTask.h"
#include "src/config/Options.h"
#include "src/db/task/DeleteObjectTask.h"
#include "src/db/task/FetchFlightsTask.h"
#include "src/db/task/RefreshAllTask.h"
#include "src/db/task/UpdateObjectTask.h"
#include "src/gui/widgets/WeatherWidget.h"
#include "src/gui/windows/DateInputDialog.h"
#include "src/gui/windows/FlightWindow.h"
#include "src/gui/windows/ObjectListWindow.h"
#include "src/gui/windows/SplashScreen.h"
#include "src/gui/windows/StatisticsWindow.h"
#include "src/gui/windows/TaskProgressDialog.h"
#include "src/gui/windows/WeatherDialog.h"
#include "src/model/Plane.h"
#include "src/model/Flight.h"
#include "src/model/Person.h"
#include "src/model/flightList/FlightModel.h"
#include "src/model/flightList/FlightProxyList.h"
#include "src/model/flightList/FlightSortFilterProxyModel.h"
#include "src/model/objectList/AutomaticEntityList.h"
#include "src/model/objectList/EntityList.h"
#include "src/model/objectList/ObjectListModel.h"
#include "src/plugins/ShellPlugin.h"
#include "src/statistics/LaunchMethodStatistics.h"
#include "src/statistics/PilotLog.h"
#include "src/statistics/PlaneLog.h"
#include "src/gui/dialogs.h"
#include "src/logging/messages.h"
#include "src/util/qString.h"

template <class T> class MutableObjectList;

// ******************
// ** Construction **
// ******************

// TODO pass DataStorage instead of Database (?, depending on thread)
// TODO better plugin list passing
MainWindow::MainWindow (QWidget *parent) :
	QMainWindow (parent),
			dbInterface (opts.databaseInfo), db (dbInterface), dataStorage (db),
			weatherWidget (NULL), weatherPlugin (NULL),
			weatherDialog (NULL), flightList (new EntityList<Flight> (this)),
			contextMenu (new QMenu (this))
{
	ui.setupUi (this);

	dbInterface.open ();

	flightModel = new FlightModel (dataStorage);
	proxyList=new FlightProxyList (dataStorage, *flightList, this); // TODO never deleted
	flightListModel = new ObjectListModel<Flight> (proxyList, false, flightModel, true, this);

	proxyModel = new FlightSortFilterProxyModel (dataStorage, this);
	proxyModel->setSourceModel (flightListModel);

	proxyModel->setSortCaseSensitivity (Qt::CaseInsensitive);
	proxyModel->setDynamicSortFilter (true);

	setDatabaseActionsEnabled (false);

	readSettings ();

	setupLabels ();

	// Fenstereinstellungen
	setCaption (opts.title);

	// Info frame
	bool acpiValid = AcpiWidget::valid ();
	ui.powerStateLabel->setVisible (acpiValid);
	ui.powerStateCaptionLabel->setVisible (acpiValid);

	QTimer *timeTimer = new QTimer (this, "timeTimer");
	connect (timeTimer, SIGNAL (timeout ()), this, SLOT (timeTimer_timeout ()));
	timeTimer->start (1000, false);

	timeTimer_timeout ();

	// Plugins
	setupPlugins ();

	setupLayout ();

	// Do this before calling connect
	dataStorageStateChanged (dataStorage.getState ());

	QObject::connect (&dataStorage, SIGNAL (dbEvent (DbEvent)), this, SLOT (dbEvent (DbEvent)));
	QObject::connect (&dataStorage, SIGNAL (status (QString, bool)), this, SLOT (dataStorageStatus (QString, bool)));
	QObject::connect (&dataStorage, SIGNAL (stateChanged (DataStorage::State)), this,
			SLOT (dataStorageStateChanged (DataStorage::State)));

	// Do this after setting the initial state and connecting the signals
	dataStorage.connect ();

	setDisplayDateCurrent (true);

	ui.logDockWidget->setVisible (false);

	// Menu bar
	QAction *logAction = ui.logDockWidget->toggleViewAction ();
	logAction->setText ("Protoko&ll anzeigen");
	ui.menuDebug->addAction (logAction);

	// TODO not working
	ui.menuDebug->setVisible (opts.debug);
	ui.menuDemosystem->setVisible (opts.demosystem);

	ui.actionNetworkDiagnostics->setVisible (!opts.diag_cmd.isEmpty ());

	bool virtualKeyboardEnabled = (system ("which kvkbd >/dev/null") == 0 && system ("which dcop >/dev/null") == 0);
	ui.actionShowVirtualKeyboard->setVisible (virtualKeyboardEnabled);
	// TODO Qt way for icons?
	ui.actionShowVirtualKeyboard->setIcon (QIcon ((const QPixmap&)QPixmap (kvkbd)));

	// Log
	ui.logWidget->document ()->setMaximumBlockCount (100);
	connect (&dataStorage, SIGNAL (executingQuery (QString)), this, SLOT (logMessage (QString)));

	// Signals
	connect (flightList, SIGNAL (rowsInserted (const QModelIndex &, int, int)), this, SLOT (flightListChanged ()));
	connect (flightList, SIGNAL (rowsRemoved (const QModelIndex &, int, int)), this, SLOT (flightListChanged ()));
	connect (flightList, SIGNAL (dataChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (flightListChanged ()));
	connect (flightList, SIGNAL (modelReset ()), this, SLOT (flightListChanged ()));

	// Flight table
	ui.flightTable->setAutoResizeRows (true);
	ui.flightTable->setModel (proxyModel);
	ui.flightTable->resizeColumnsToContents (); // Default sizes
	readColumnWidths (); // Stored sizes

	QObject::connect (
		ui.flightTable, SIGNAL (buttonClicked (QPersistentModelIndex)),
		this, SLOT (flightTable_buttonClicked (QPersistentModelIndex))
		);
	QObject::connect (
		ui.flightTable->horizontalHeader (), SIGNAL (sectionClicked (int)),
		this, SLOT (flightTable_horizontalHeader_sectionClicked (int))
		);

	QObject::connect (ui.actionHideFinished, SIGNAL (toggled (bool)), proxyModel, SLOT (setHideFinishedFlights (bool)));
	QObject::connect (ui.actionAlwaysShowExternal, SIGNAL (toggled (bool)), proxyModel, SLOT (setAlwaysShowExternalFlights (bool)));
	QObject::connect (ui.actionAlwaysShowErroneous, SIGNAL (toggled (bool)), proxyModel, SLOT (setAlwaysShowErroneousFlights (bool)));

	// Initialize all properties of the filter proxy model
	proxyModel->setHideFinishedFlights (ui.actionHideFinished->isChecked ());
	proxyModel->setAlwaysShowExternalFlights (ui.actionAlwaysShowExternal->isChecked ());
	proxyModel->setAlwaysShowErroneousFlights (ui.actionAlwaysShowErroneous->isChecked ());

	proxyModel->setCustomSorting (true);

	ui.flightTable->setFocus ();
}

MainWindow::~MainWindow ()
{
	// QObjects will be deleted automatically
	// TODO make sure this also applies to flightList
}

void MainWindow::setupLabels ()
{
	if (opts.colorful)
	{
		QObjectList labels = ui.infoPane->children ();

		foreach (QObject *object, labels)
		{
			QLabel *label = dynamic_cast<QLabel *> (object);
			if (label)
			{
				if (label->objectName ().contains ("Caption", true))
					label->setPaletteBackgroundColor (QColor (0, 255, 127));
				else
					label->setPaletteBackgroundColor (QColor (0, 127, 255));

				label->setAutoFillBackground (true);
			}
		}
	}
}

void MainWindow::setupLayout ()
{
	// QT 4.3.4 uic ignores the stretch factors (4.5.0 uses it), so set it here.

	QVBoxLayout *centralLayout = (QVBoxLayout *)centralWidget () -> layout ();
	centralLayout->setStretchFactor (ui.topPane, 0);
	centralLayout->setStretchFactor (ui.flightTable, 1);

	//	QHBoxLayout *topPaneLayout     = (QHBoxLayout *) ui.infoPane    -> layout ();
	QHBoxLayout *infoFrameLayout = (QHBoxLayout *)ui.infoFrame -> layout ();
	QGridLayout *infoPaneLayout = (QGridLayout *)ui.infoPane -> layout ();
	QGridLayout *pluginPaneLayout = (QGridLayout *)ui.pluginPane -> layout ();

	/*
	 * This setting gives the plugins more space while still leaving some non-
	 * minimum space between the info and the plugins.
	 * The best behavior would be:
	 *   - normally, info and plugins take up the same amount of space
	 *   - if the plugin values get too large, the info area is made smaller
	 *     in favor of the plugin area
	 *   - if the info area cannot shrink any more, but the plugin area is
	 *     still to small the plugin values are wrapped
	 *   - the plugin values are not wrapped as long as space can be freed by
	 *     shrinking the info area
	 *   - never is the window resized to make space for the plugins if
	 *     something else is possible
	 *   - small changes in the info pane width do not lead to the plugin area
	 *     being moved around
	 */

	// The plugin pane gets twice the space of the info pane
	infoFrameLayout->setStretchFactor (ui.infoPane, 1);
	infoFrameLayout->setStretchFactor (ui.pluginPane, 2);

	// For both the info pane and the plugin pane: all available space goes to
	// the value; the caption is kept at minimum size.
	infoPaneLayout->setColumnStretch (0, 0);
	infoPaneLayout->setColumnStretch (1, 1);
	pluginPaneLayout->setColumnStretch (0, 0);
	pluginPaneLayout->setColumnStretch (1, 1);
}

void MainWindow::setupPlugin (ShellPlugin *plugin, QGridLayout *pluginLayout)
{
	SkLabel *captionLabel = new SkLabel ("", ui.pluginPane);
	SkLabel *valueLabel = new SkLabel ("...", ui.pluginPane);

	valueLabel->setWordWrap (true);

	if (plugin->get_rich_text ())
	{
		captionLabel->setTextFormat (Qt::RichText);
		valueLabel->setTextFormat (Qt::RichText);

		captionLabel->setText ("<nobr>" + plugin->get_caption () + "</nobr>");
	}
	else
	{
		captionLabel->setTextFormat (Qt::PlainText);
		valueLabel->setTextFormat (Qt::PlainText);

		captionLabel->setText (plugin->get_caption ());
	}

	int row = pluginLayout->rowCount ();
	pluginLayout->addWidget (captionLabel, row, 0, Qt::AlignTop);
	pluginLayout->addWidget (valueLabel, row, 1, Qt::AlignTop);

	if (opts.colorful)
	{
		captionLabel->setPaletteBackgroundColor (QColor (255, 63, 127));
		valueLabel ->setPaletteBackgroundColor (QColor (255, 255, 127));

		captionLabel->setAutoFillBackground (true);
		valueLabel ->setAutoFillBackground (true);
	}

	plugin->set_caption_display (captionLabel);
	plugin->set_value_display (valueLabel);

	QObject::connect (captionLabel, SIGNAL (doubleClicked (QMouseEvent *)), plugin, SLOT (restart ()));
	QObject::connect (valueLabel, SIGNAL (doubleClicked (QMouseEvent *)), plugin, SLOT (restart ()));

	plugin->start ();
}

void MainWindow::setupPlugins ()
{
	ui.pluginPane->setVisible (!opts.shellPlugins.isEmpty ());

	if (!opts.shellPlugins.isEmpty ())
	{
		ui.pluginPane->setVisible (true);
		QGridLayout *pluginLayout = dynamic_cast<QGridLayout *> (ui.pluginPane->layout ());

		if (pluginLayout)
		{
			// Remove the dummy labels from the plugin pane
			foreach (QObject *child, ui.pluginPane->children ())
					if (dynamic_cast<QLabel *> (child)) delete child;

			foreach (ShellPlugin *plugin, opts.shellPlugins)
					setupPlugin (plugin, pluginLayout);

			pluginLayout->setRowStretch (pluginLayout->rowCount (), 1);
		}
		else
		{
			log_error ("Invalid plugin layout in MainWindow::setupPlugins, not using shell plugins");
			return;
		}
	}

	ui.weatherFrame->setVisible (!opts.weather_plugin.isEmpty ());

	if (!opts.weather_plugin.isEmpty ())
	{
		// Create and setup the weather widget. The weather widget is located to
		// the right of the info frame.
		weatherWidget = new WeatherWidget (ui.weatherFrame, "weather");
		ui.weatherFrame->layout ()->add (weatherWidget);
		//		std::cout << opts.weather_height << std::endl;
		//		weatherWidget->setMinimumSize (opts.weather_height, opts.weather_height);
		weatherWidget->setFixedSize (opts.weather_height, opts.weather_height);
		//		weatherWidget->setSizePolicy (Qt::);
		//		weatherWidget->setFixedHeight (opts.weather_height);
		//		weatherWidget->setFixedWidth (opts.weather_height); // Wird bei Laden eines Bildes angepasst
		weatherWidget->setText ("[Wetter]");

		// Create and setup the weather plugin and connect it to the weather widget
		std::cout << "Weather plugin is " << opts.weather_plugin << std::endl;
		weatherPlugin = new ShellPlugin ("Wetter", opts.weather_plugin, opts.weather_interval); // Initialize to given values
		QObject::connect (weatherPlugin, SIGNAL (lineRead (QString)), weatherWidget, SLOT (inputLine (QString)));
		QObject::connect (weatherPlugin, SIGNAL (pluginNotFound ()), weatherWidget, SLOT (pluginNotFound ()));
		QObject::connect (weatherWidget, SIGNAL (doubleClicked ()), this, SLOT (weatherWidget_doubleClicked ()));
		weatherPlugin->start ();
	}

}

// *************
// ** Closing **
// *************

bool MainWindow::confirmAndExit (int returnCode, QString title, QString text)
{
	if (yesNoQuestion (this, title, text))
	{
		writeSettings ();
		qApp->exit (returnCode);
		return true;
	}
	else
	{
		return false;
	}
}

void MainWindow::closeEvent (QCloseEvent *event)
{
	if (!confirmAndExit (0, "Wirklich beenden?", "Programm wirklich beenden?"))
		event->ignore ();
}

void MainWindow::on_actionQuit_triggered ()
{
	confirmAndExit (0, "Wirklich beenden?", "Programm wirklich beenden?");
}

void MainWindow::on_actionShutdown_triggered ()
{
	confirmAndExit (69, "Wirklich herunterfahren?", "Rechner wirklich herunterfahren?");
}

// **************
// ** Settings **
// **************

void MainWindow::writeSettings ()
{
	QSettings settings ("startkladde", "startkladde");

	settings.beginGroup ("fonts");
	QFont font = QApplication::font ();
	settings.setValue ("font", font.toString ());
	settings.endGroup ();

	settings.beginGroup ("flightTable");
	ui.flightTable->writeColumnWidths (settings, *flightModel);
	settings.endGroup ();

	settings.sync ();
}

void MainWindow::readColumnWidths ()
{
	QSettings settings ("startkladde", "startkladde");

    settings.beginGroup ("flightTable");
    ui.flightTable->readColumnWidths (settings, *flightModel);
    settings.endGroup ();
}

void MainWindow::readSettings ()
{
	QSettings settings ("startkladde", "startkladde");

	settings.beginGroup ("fonts");
	QString fontDescription = settings.value ("font").toString ();
	settings.endGroup ();

	QFont font;
	if (font.fromString (fontDescription)) QApplication::setFont (font);
}

// *************
// ** Flights **
// *************

/**
 * Refreshes both the flight table and the corresponding info labels, including
 * the display date label (text and color).
 */
bool MainWindow::refreshFlights ()
{
//	int oldColumn = ui.flightTable->currentColumn ();
	//	int oldRow   =ui.flightTable->currentRow    ();

	// TODO: shold be local today (time zone safety)
	bool displayDateIsToday=(displayDate == QDate::currentDate ());
	proxyModel->setShowPreparedFlights (displayDateIsToday);

	ui.displayDateLabel->setPaletteForegroundColor (displayDateIsToday?(Qt::black):(Qt::red));
	ui.displayDateLabel->setText (displayDate.toString (Qt::LocaleDate));

	QList<Flight> flights;
	if (displayDate == QDate::currentDate ())
	{
		flights = dataStorage.getFlightsToday ();
		flights += dataStorage.getPreparedFlights ();
	}
	else
	{
		// Fetch the flights for the other date if we don't already have them
		// TODO here be race conditions
		if (dataStorage.getOtherDate ()!=displayDate)
		{
			while (1)
			{
				FetchFlightsTask task (dataStorage, displayDate);
				dataStorage.addTask (&task);
				TaskProgressDialog::waitTask (this, &task);

				// TODO distinguish between retry errors (timeout, no connection)
				// and failures (permissions, malformed query...)
				if (!task.isCompleted ()) return false;
				if (task.getSuccess ()) break;
				DefaultQThread::sleep (1);
			}

		}

		flights=dataStorage.getFlightsOther ();
	}

	flightList->replaceList (flights);
	// TODO should be done automatically
//	ui.flightTable->resizeColumnsToContents ();
	//	ui.flightTable->resizeRowsToContents ();

	sortCustom ();

//		std::cout << "Got " << flights.size () << " flights from the database" << std::endl;

//	// Set the focus to the end of the table instead of the previously focussed
//	// flight, it's better that way.
//	int newRow = ui.flightTable->rowCount () - 1;
//	ui.flightTable->setCurrentCell (newRow, oldColumn);
//	// TODO make sure it's visible

	// TODO: set cursor to last row, same column as before

//	ui.flightTable->setFocus ();

	// TODO
	//	updateInfo ();

	return true;
}

dbId MainWindow::currentFlightId (bool *isTowflight)
{
	// Get the currently selected index from the table; it refers to the
	// proxy model
	QModelIndex proxyIndex = ui.flightTable->currentIndex ();

	// Map the index from the proxy model to the flight list model
	QModelIndex flightListIndex = proxyModel->mapToSource (proxyIndex);

	// If there is not selection, return an invalid ID
	if (!flightListIndex.isValid ()) return invalidId;

	// Get the flight from the model
	const Flight &flight = flightListModel->at (flightListIndex);

	if (isTowflight) (*isTowflight) = flight.isTowflight ();
	return flight.getId ();
}

void MainWindow::sortCustom ()
{
	// Use custom sorting
	proxyModel->setCustomSorting (true);

	// Show the sort status in the header view
	ui.flightTable->setSortingEnabled (false); // Make sure it is off
	ui.flightTable->horizontalHeader ()->setSortIndicatorShown (false);
}

void MainWindow::sortByColumn (int column)
{
	// Determine the new sorting order: when custom sorting was in effect or the
	// sorting column changed, sort ascending; otherwise, toggle the sorting
	// order
	if (proxyModel->getCustomSorting ())
		sortOrder=Qt::AscendingOrder; // custom sorting was in effect
	else if (column!=sortColumn)
		sortOrder=Qt::AscendingOrder; // different column
	else if (sortOrder==Qt::AscendingOrder)
		sortOrder=Qt::DescendingOrder; // toggle ascending->descending
	else
		sortOrder=Qt::AscendingOrder; // toggle any->ascending

	// Set the new sorting column
	sortColumn=column;

	// Sort the proxy model
	proxyModel->setCustomSorting (false);
	proxyModel->sort (sortColumn, sortOrder);

	// Show the sort status in the header view
	QHeaderView *header=ui.flightTable->horizontalHeader ();
	header->setSortIndicatorShown (true);
	header->setSortIndicator (sortColumn, sortOrder);
}

void MainWindow::flightListChanged ()
{
	QList<Flight> flights=flightList->getList ();

	// Note that there is a race condition if "refresh" is called before
	// midnight and this function is called after midnight.
	// TODO store the todayness somewhere instead.

	if (displayDate == QDate::currentDate ())
		ui.activeFlightsLabel->setNumber (Flight::countFlying (flights));
	else
		ui.activeFlightsLabel->setText ("-");

	ui.totalFlightsLabel->setNumber (Flight::countHappened (flights));
}

// *************************
// ** Flight manipulation **
// *************************

/*
 * Notes:
 *   - for create, repeat and edit: the flight editor may be modeless
 *     and control may return immediately (even for modal dialogs). The
 *     The table entry will be updated when the flight editor updates the
 *     database.
 *
 * TODO:
 *   - hier den ganzen Kram wie displayDate==heute und flug schon
 *     gelandet prüfen, damit man die Menüdeaktivierung weglassen kann.
 *     Außerdem kann man dann hier melden, warum das nicht geht.
 *
 */

void MainWindow::updateFlight (const Flight &flight)
{
	UpdateObjectTask<Flight> task (dataStorage, flight);
	dataStorage.addTask (&task);
	TaskProgressDialog::waitTask (this, &task);
	if (!task.getSuccess ())
	{
		// TODO we should retry instead, and probably in the task
		QMessageBox::critical (
			this,
			"Fehler beim Speichern",
			QString ("Fehler beim Speichern des Flugs: %1").arg (task.getMessage ())
			);
	}
}

void MainWindow::startFlight (dbId id)
{
	// TODO display message
	if (idInvalid (id)) return;

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);
		QString reason;

		if (flight.canDepart (&reason))
		{
			// TODO still flying checks if specified (3 people, 2 planes). Code is
			// already in FlightWindow.
			flight.departNow ();
			updateFlight (flight);
		}
		else
		{
			showWarning (QString::fromUtf8 ("Start nicht möglich"), reason, this);
		}
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight %1 not found in MainWindow::startFlight").arg (ex.id));
	}
}

void MainWindow::landFlight (dbId id)
{
	// TODO display message
	if (idInvalid (id)) return;

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);
		QString reason;

		if (flight.canLand (&reason))
		{
			flight.landNow ();
			updateFlight (flight);
		}
		else
		{
			showWarning (QString::fromUtf8 ("Landung nicht möglich"), reason, this);
		}
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight not found in MainWindow::landFlight").arg (ex.id));
	}
}

void MainWindow::landTowflight (dbId id)
{
	// TODO display message
	if (idInvalid (id)) return;

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);
		QString reason;

		if (flight.canTowflightLand (&reason))
		{
			flight.landTowflightNow ();
			updateFlight (flight);
		}
		else
		{
			showWarning (QString::fromUtf8 ("Landung nicht möglich"), reason, this);
		}
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight %1 not found in MainWindow::landTowFlight").arg (ex.id));
	}
}

void MainWindow::on_actionNew_triggered ()
{
	// TODO: the window should be modeless, but
	//   - be closed when the database connection is closed (?)
	//   - what about landing a flight that is open in the editor?
	//   - there should be only one flight editor at a time
	FlightWindow::createFlight (this, dataStorage, getNewFlightDate ());

}

void MainWindow::on_actionStart_triggered ()
{
	// This will check canStart
	startFlight (currentFlightId ());
}

void MainWindow::on_actionLand_triggered ()
{
	bool isTowflight = false;
	dbId id = currentFlightId (&isTowflight);

	if (isTowflight)
		// This will check canLand
		landTowflight (id);
	else
		landFlight (id);
}

void MainWindow::on_actionTouchngo_triggered ()
{
	bool isTowflight=false;
	dbId id = currentFlightId (&isTowflight);

	if (isTowflight)
	{
		showWarning (
			QString::fromUtf8 ("Zwischenlandung nicht möglich"),
			QString::fromUtf8 ("Der ausgewählte Flug ist ein Schleppflug. Schleppflüge können keine Zwischenlandung machen."),
			this);
	}

	// TODO display message
	if (idInvalid (id)) return;

	try
	{
		// TODO warning if the plane is specified and a glider and the
		// launch method is specified and not an unended airtow

		Flight flight = dataStorage.getObject<Flight> (id);
		QString reason;

		if (flight.canTouchngo (&reason))
		{
			flight.performTouchngo ();
			updateFlight (flight);
		}
		else
		{
			showWarning (QString::fromUtf8 ("Zwischenlandung nicht möglich"), reason, this);
		}
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight %1 not found in MainWindow::on_actionTouchngo_triggered").arg (ex.id));
	}
}

void MainWindow::on_actionEdit_triggered ()
{
	dbId id = currentFlightId ();

	if (idInvalid (id)) return;

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);
		FlightWindow::editFlight (this, dataStorage, flight);
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight not found in MainWindow::on_actionEdit_triggered").arg (ex.id));
	}
}

void MainWindow::on_actionRepeat_triggered ()
{
	bool isTowflight = false;
	dbId id = currentFlightId (&isTowflight);

	// TODO display message
	if (idInvalid (id))
		return;

	else if (isTowflight)
	{
		showWarning (QString::fromUtf8 ("Wiederholen nicht möglich"),
			QString::fromUtf8 ("Der ausgewählte Flug ist ein Schleppflug. Schleppflüge können nicht wiederholt werden."),
			this);
		return;
	}

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);
		FlightWindow::repeatFlight (this, dataStorage, flight, getNewFlightDate ());
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight %1 not found in MainWindow::on_actionRepeat_triggered").arg (id));
	}
}

void MainWindow::on_actionDelete_triggered ()
{
	bool isTowflight = false;
	dbId id = currentFlightId (&isTowflight);

	if (idInvalid (id))
	// TODO display message
	return;

	if (!yesNoQuestion (this, QString::fromUtf8 ("Flug löschen?"), QString::fromUtf8 ("Flug wirklich löschen?"))) return;

	if (isTowflight) if (!yesNoQuestion (this, QString::fromUtf8 ("Geschleppten Flug löschen?"), QString::fromUtf8 (
			"Der gewählte Flug ist ein Schleppflug. Soll der dazu gehörige geschleppte Flug wirklich gelöscht werden?"))) return;

	DeleteObjectTask<Flight> task (dataStorage, id);
	dataStorage.addTask (&task);
	TaskProgressDialog::waitTask (this, &task);

	if (!task.getSuccess ())
		QMessageBox::message (
			QString::fromUtf8 ("Fehler beim Löschen"),
			QString::fromUtf8 ("Fehler beim Löschen: %1").arg (task.getMessage ()),
			"&OK",
			this
		);
}

void MainWindow::on_actionDisplayError_triggered ()
{
	// Note: only the first error is displayed

	// TODO: this method is quite complex and duplicates code found
	// elsewhere - the towplane generation should be simplified

	bool isTowflight;
	dbId id = currentFlightId (&isTowflight);

	if (idInvalid (id))
	{
		showWarning ("Kein Flug ausgewählt", "Es ist kein Flug ausgewählt", this);
		return;
	}

	try
	{
		Flight flight = dataStorage.getObject<Flight> (id);

		Plane *plane=dataStorage.getNewObject<Plane> (flight.planeId);
		LaunchMethod *launchMethod=dataStorage.getNewObject<LaunchMethod> (flight.launchMethodId);
		Plane *towplane=NULL;

		dbId towplaneId=invalidId;
		if (launchMethod && launchMethod->isAirtow ())
		{
			if (launchMethod->towplaneKnown ())
				towplaneId=dataStorage.getPlaneIdByRegistration (launchMethod->towplaneRegistration);
			else
				towplaneId=flight.towplaneId;

			if (idValid (towplaneId))
				towplane=dataStorage.getNewObject<Plane> (towplaneId);
		}

		// As the FlightModel uses fehlerhaft on the towflight generated by
		// the FlightProxyList rather than schlepp_fehlerhaft, we do the same.
		if (isTowflight)
		{
			dbId towLaunchMethod=dataStorage.getLaunchMethodByType (LaunchMethod::typeSelf);

			flight=flight.makeTowflight (towplaneId, towLaunchMethod);

			delete launchMethod;
			launchMethod=dataStorage.getNewObject<LaunchMethod> (towLaunchMethod);

			delete plane;
			plane=towplane;
			towplane=NULL;
		}

		QString errorText;
		bool error=flight.fehlerhaft (plane, NULL, launchMethod, &errorText);

		delete plane;
		delete towplane;
		delete launchMethod;

		QString flightText (isTowflight?"Schleppflug":"Flug");

		if (error)
			showWarning (QString ("%1 fehlerhaft").arg (flightText), QString ("Erster Fehler des %1s: %2").arg (flightText, errorText), this);
		else
			showWarning (QString ("%1 fehlerfrei").arg (flightText), QString ("Der %1 ist fehlerfrei.").arg (flightText), this);
	}
	catch (DataStorage::NotFoundException &ex)
	{
		log_error (QString ("Flight %1 not found in MainWindow::on_actionDisplayError_triggered").arg (ex.id));
	}
}


// **********
// ** Font **
// **********

void MainWindow::on_actionSelectFont_triggered ()
{
	bool ok;
	QFont font = QApplication::font ();
	font = QFontDialog::getFont (&ok, font, this);

	if (ok)
	// The user pressed OK and font is set to the font the user selected
	QApplication::setFont (font);
}

void MainWindow::on_actionIncrementFontSize_triggered ()
{
	QFont font = QApplication::font ();
	int size = font.pointSize ();
	font.setPointSize (size + 1);
	QApplication::setFont (font);
}

void MainWindow::on_actionDecrementFontSize_triggered ()
{
	QFont font = QApplication::font ();
	int size = font.pointSize ();
	if (size>5)
	{
		font.setPointSize (size - 1);
		QApplication::setFont (font);
	}
}

// **********
// ** View **
// **********

void MainWindow::on_actionShowVirtualKeyboard_triggered (bool checked)
{
	if (checked)
	{
		// This call may fail (when the progran is not running), don't display
		// stderr. If it fails, it will be run again.
		int result = system ("dcop kvkbd kvkbd show >/dev/null 2>/dev/null");
		if (result != 0)
		{
			// failed to show; try launch
			system ("kvkbd >/dev/null");
			system ("dcop kvkbd kvkbd show >/dev/null");
		}
	}
	else
	{
		system ("/usr/bin/dcop kvkbd kvkbd hide >/dev/null");
	}
}

void MainWindow::on_actionRefreshTable_triggered ()
{
	// TODO refresh all is not refresh table!
	RefreshAllTask task (dataStorage);
	dataStorage.addTask (&task);
	TaskProgressDialog::waitTask (this, &task);

	refreshFlights ();
}

void MainWindow::on_actionJumpToTow_triggered ()
{
	// Get the currently selected index in the ObjectListModel
	QModelIndex currentIndex=proxyModel->mapToSource (ui.flightTable->currentIndex ());

	if (!currentIndex.isValid ())
	{
		showWarning (QString::fromUtf8 ("Kein Flug ausgewählt"), QString::fromUtf8 ("Es ist kein Flug ausgewählt."), this);
		return;
	}

	// Get the towref from the FlightProxyList. The rows of the ObjectListModel
	// correspond to those of its source, the FlightProxyList.
	int towref=proxyList->findTowref (currentIndex.row ());

	if (towref<0)
	{
		showWarning (QString::fromUtf8 ("Kein Schleppflug"), QString::fromUtf8 ("Der gewählte Flug ist entweder kein F-Schlepp und kein Schleppflug oder noch nicht gestartet."), this);
		return;
	}

	// Generate the index in the ObjectListModel
	QModelIndex towrefIndex=currentIndex.sibling (towref, currentIndex.column ());

	// Jump to the flight
	ui.flightTable->setCurrentIndex (proxyModel->mapFromSource (towrefIndex));
}

void MainWindow::on_actionRestartPlugins_triggered ()
{
	if (weatherPlugin) weatherPlugin->restart ();

	foreach (ShellPlugin *plugin, opts.shellPlugins)
			plugin->restart ();
}

// **********
// ** Help **
// **********

void MainWindow::on_actionInfo_triggered ()
{

	::SplashScreen *splashScreen = new ::SplashScreen (this, logo);
	splashScreen->setAttribute (Qt::WA_DeleteOnClose, true);
	splashScreen->show_version ();

}

void MainWindow::on_actionNetworkDiagnostics_triggered ()
{
	if (!opts.diag_cmd.isEmpty ()) system (opts.diag_cmd.utf8 ().constData ());
}

// ************
// ** Events **
// ************

void MainWindow::keyPressEvent (QKeyEvent *e)
{
	switch (e->key ())
	{
		// The function keys trigger actions
		case Qt::Key_F2:  ui.actionNew          ->trigger (); break;
		case Qt::Key_F3:  ui.actionRepeat       ->trigger (); break;
		case Qt::Key_F4:  ui.actionEdit         ->trigger (); break;
		case Qt::Key_F5:  ui.actionStart        ->trigger (); break;
		case Qt::Key_F6:  ui.actionLand         ->trigger (); break;
		case Qt::Key_F7:  ui.actionTouchngo     ->trigger (); break;
		case Qt::Key_F8:  ui.actionDelete       ->trigger (); break;
		case Qt::Key_F9:  ui.actionSort         ->trigger (); break;
		case Qt::Key_F10: ui.actionQuit         ->trigger (); break;
		case Qt::Key_F11: ui.actionHideFinished ->trigger (); break;
		case Qt::Key_F12: ui.actionRefreshTable ->trigger (); break;

		// Flight manipulation
		case Qt::Key_Insert: if (ui.flightTable->hasFocus ()) ui.actionNew    ->trigger (); break;
		case Qt::Key_Delete: if (ui.flightTable->hasFocus ()) ui.actionDelete ->trigger (); break;

		default: e->ignore (); break;
	}

	QMainWindow::keyPressEvent (e);
}

void MainWindow::on_flightTable_activated (const QModelIndex &index)
{
	QModelIndex sourceIndex=proxyModel->mapToSource (index);

//	std::cout << QString ("Flight table activated at proxy index %1,%2 which maps to source index %3,%4")
//		.arg (index.row()).arg (index.column())
//		.arg (sourceIndex.row()).arg (sourceIndex.column())
//		<< std::endl;

	if (index.isValid ())
		ui.actionEdit->trigger ();
	else
		ui.actionNew->trigger ();
}

void MainWindow::on_flightTable_customContextMenuRequested (const QPoint &pos)
{
	contextMenu->clear ();

	if (ui.flightTable->indexAt (pos).isValid ())
	{
		contextMenu->addAction (ui.actionNew);
		contextMenu->addSeparator ();
		contextMenu->addAction (ui.actionStart);
		contextMenu->addAction (ui.actionLand);
		contextMenu->addAction (ui.actionTouchngo);
		contextMenu->addSeparator ();
		contextMenu->addAction (ui.actionEdit);
		contextMenu->addAction (ui.actionRepeat);
		contextMenu->addAction (ui.actionDelete);
		contextMenu->addSeparator ();
		contextMenu->addAction (ui.actionJumpToTow);
	}
	else
	{
		contextMenu->addAction (ui.actionNew);
	}

	contextMenu->popup (ui.flightTable->mapToGlobal (pos), 0);
}

void MainWindow::flightTable_buttonClicked (QPersistentModelIndex proxyIndex)
{
	if (!proxyIndex.isValid ())
	{
		log_error ("A button with invalid persistent index was clicked in MainWindow::flightTable_buttonClicked");
		return;
	}

	QModelIndex flightListIndex = proxyModel->mapToSource (proxyIndex);
	const Flight &flight = flightListModel->at (flightListIndex);

//	std::cout << QString ("Button clicked at proxy index (%1,%2), flight list index is (%3,%4), flight ID is %5")
//		.arg (proxyIndex.row()).arg (proxyIndex.column())
//		.arg (flightListIndex.row()).arg (flightListIndex.column())
//		.arg (flight.id)
//		<< std::endl;

	if (flightListIndex.column () == flightModel->launchButtonColumn ())
		startFlight (flight.getId ());
	else if (flightListIndex.column () == flightModel->landButtonColumn ())
	{
		if (flight.isTowflight ())
			landTowflight (flight.getId ());
		else
			landFlight (flight.getId ());
	}
	else
		std::cerr << "Unhandled button column in MainWindow::flightTable_buttonClicked" << std::endl;
}

void MainWindow::timeTimer_timeout ()
{
	// TODO: note the race condition, and remove it
	ui.utcTimeLabel->setText (get_current_time_text (tz_utc));
	ui.localTimeLabel->setText (get_current_time_text (tz_local));

	// TODO: on the beginning of a minute, update the fliht duration (probably
	// call from here rather than from a timer in the model so it's
	// synchronized to the minutes)

	static int lastSecond=0;
	int second=QTime::currentTime ().second ();

	if (second<lastSecond)
	{
		int durationColumn=flightModel->durationColumn ();
		flightListModel->columnChanged (durationColumn);
	}

	lastSecond=second;
}

void MainWindow::weatherWidget_doubleClicked ()
{
	if (weatherDialog)
	{
		// How to focus a QDialog?
		weatherDialog->hide ();
		weatherDialog->show ();
	}
	else
	{
		if (!opts.weather_dialog_plugin.isEmpty ())
		{
			// The weather animation plugin will be deleted by the weather dialog
			ShellPlugin *weatherAnimationPlugin = new ShellPlugin (opts.weather_dialog_title,
					opts.weather_dialog_plugin, opts.weather_dialog_interval);

			// The weather dialog will be deleted when it's closed, and
			// weatherDialog is a QPointer, so it will be set to NULL.
			weatherDialog = new WeatherDialog (weatherAnimationPlugin, this);
			weatherDialog->setCaption (opts.weather_dialog_title);
			weatherDialog->show ();
		}
	}
}

// ********************
// ** View - Flights **
// ********************

void on_actionSort_triggered ()
{

}

// **********
// ** Date **
// **********

/**
 *
 * @param displayDate null means current
 * @param force true means even if already that date
 */
void MainWindow::setDisplayDate (QDate newDisplayDate, bool force)
{
	// If the display date is null, use the current date
	if (newDisplayDate.isNull ()) newDisplayDate = QDate::currentDate ();

	// TODO when setting the current date, don't refresh from the database -
	// it's still in the cache

	// Change it only if it is different from the current one or force is true
	if (displayDate!=newDisplayDate || force)
	{
		// TODO: if refreshing fails or is canceled, we may not change the
		// display date; this solution is crap
		displayDate = newDisplayDate;
		bool refreshFinished=refreshFlights ();

//		if (!refreshFinished)
//			setDisplayDateCurrent (false);
	}
}

void MainWindow::on_actionSetDisplayDate_triggered ()
{
	QDate newDisplayDate = displayDate;
	if (DateInputDialog::editDate (this, &newDisplayDate, NULL, "Anzeigedatum einstellen", "Anzeigedatum:",
		true, true, true)) setDisplayDate (newDisplayDate, true);
}

// ****************
// ** Statistics **
// ****************

void MainWindow::on_actionPlaneLogs_triggered ()
{
	PlaneLog *planeLog = PlaneLog::createNew (flightList->getList (), dataStorage);
	StatisticsWindow::display (planeLog, true, QString::fromUtf8 ("Bordbücher"), this);
}

void MainWindow::on_actionPersonLogs_triggered ()
{
	PilotLog *pilotLog = PilotLog::createNew (flightList->getList (), dataStorage);
	StatisticsWindow::display (pilotLog, true, QString::fromUtf8 ("Flugbücher"), this);
}

void MainWindow::on_actionLaunchMethodStatistics_triggered ()
{
	LaunchMethodStatistics *stats = LaunchMethodStatistics::createNew (flightList->getList (), dataStorage);
	StatisticsWindow::display (stats, true, "Startartstatistik", this);
}

// **************
// ** Database **
// **************

void MainWindow::on_actionEditPlanes_triggered ()
{
	MutableObjectList<Plane> *list = new AutomaticEntityList<Plane> (dataStorage, dataStorage.getPlanes (), this);
	ObjectListModel<Plane> *listModel = new ObjectListModel<Plane> (list, true, new Plane::DefaultObjectModel (), true,
			this);
	ObjectListWindowBase *window = new ObjectListWindow<Plane> (dataStorage, listModel, true, this);
	window->show ();
}

void MainWindow::on_actionEditPeople_triggered ()
{
	MutableObjectList<Person> *list = new AutomaticEntityList<Person> (dataStorage, dataStorage.getPeople (), this);
	ObjectListModel<Person> *listModel = new ObjectListModel<Person> (list, true, new Person::DefaultObjectModel (),
			true, this);
	ObjectListWindowBase *window = new ObjectListWindow<Person> (dataStorage, listModel, true, this);
	window->show ();
}

void MainWindow::on_actionEditLaunchMethods_triggered ()
{
	MutableObjectList<LaunchMethod> *list = new AutomaticEntityList<LaunchMethod> (dataStorage, dataStorage.getLaunchMethods (), this);
	ObjectListModel<LaunchMethod> *listModel = new ObjectListModel<LaunchMethod> (list, true, new LaunchMethod::DefaultObjectModel (),
			true, this);
	ObjectListWindowBase *window = new ObjectListWindow<LaunchMethod> (dataStorage, listModel, true, this);
	window->show ();
}


// **************
// ** Database **
// **************

void MainWindow::dbEvent (DbEvent event)
{
	assert (isGuiThread ());

	std::cout << event.toString () << std::endl;

	try
	{
		// TODO when a plane, person or launch method is changed, the flight list
		// has to be updated, too. But that's a feature of the FlightListModel (?).
		if (event.table == DbEvent::tableFlights)
		{
			switch (event.type)
			{
				case DbEvent::typeNone:
					break;
				case DbEvent::typeAdd:
				{
					const Flight &flight=dataStorage.getObject<Flight> (event.id);
					if (flight.isPrepared () || flight.effdatum ()==displayDate)
						flightList->append (flight);

					// TODO: set the cursor position to the flight

					// TODO introduce Flight::hasDate (timeZone)
					if (ui.actionResetDisplayDateOnNewFlight)
					{
						if (flight.isPrepared ())
							setDisplayDateCurrent (false);
						else
							setDisplayDate (flight.effdatum (), false);
					}
				} break;
				case DbEvent::typeChange:
				{
					try
					{
						const Flight &flight=dataStorage.getObject<Flight> (event.id);

						std::cout << "effdatum: " << flight.effdatum ().toString () << std::endl;
						if (flight.isPrepared () || flight.effdatum ()==displayDate)
							flightList->replaceOrAdd (event.id, flight);
						else
							flightList->removeById (event.id);
					}
					catch (DataStorage::NotFoundException)
					{
						// The flight was not found. This means that it didn't
						// happen today or on the dataStorage's "other" date.
						// Since the display date is always today or the "other"
						// date, we can conclude that we have to remove the
						// flight.
						flightList->removeById (event.id);
					}
				} break;
				case DbEvent::typeDelete:
					flightList->removeById (event.id);
					break;
				case DbEvent::typeRefresh:
					refreshFlights ();
					break;
			}
		}
	}
	catch (DataStorage::NotFoundException)
	{
		// TODO log error
	}
}

bool MainWindow::initializeDatabase ()
{
	// TODO add reason, as parameter to this method, from
	// Database::ex_unusable.description ()

	// We need to initialize the database. Therefore, we ask the
	// root password from the user.
	QString title=QString::fromUtf8 ("Datenbank-Passwort benötigt");
	QString userText=QString ("%1@%2").arg (opts.root_name).arg (opts.server_display_name);
//	QString text=QString::fromUtf8 (
//		"Die Datenbank %1 ist nicht benutzbar. Grund: %2. Zur Korrektur"
//		" wird das Passwort von %3 benötigt.")
//		.arg (opts.database)
//		.arg (reason)
//		.arg (userText);
	QString text=QString::fromUtf8 (
		"Die Datenbank %1 ist nicht benutzbar. Zur Korrektur"
		" wird das Passwort von %2 benötigt.")
		.arg (opts.databaseInfo.database)
		.arg (userText);

	bool retry=false;
	do
	{
		retry=false;
		bool ok;

		QString rootPassword=QInputDialog::getText (
			title, text, QLineEdit::Password, QString::null, &ok, this);

		if (ok)
		{
			// OK pressed
//			try
//			{
//				Database rootDatabase;
				// FIXME
//				rootDatabase.display_queries = opts.display_queries;
//				initialize_database (rootDatabase, opts.server, opts.port, opts.root_name, rootPassword);
//			}
			// FIXME
//			catch (OldDatabase::ex_access_denied &e)
//			{
//				retry=true;
//				text=QString::fromUtf8 ("%1. Passwort für %2:")
//					.arg (e.description (true))
//					.arg (userText);
//			}
//			catch (OldDatabase::ex_init_failed &e)
//			{
////				db_error = e.description (true);
//				QMessageBox::critical (this, e.description (true), e.description (true),
//						QMessageBox::Ok, QMessageBox::NoButton);
//				return false;
//			}
//			// TODO show output from creation
//			catch (SkException &e)
//			{
//				// Database initialization failed. That means that there is no point in
//				// trying the connection again.
////				db_error = "Datenbankfehler: " + e.getDescription ();
//				QMessageBox::critical (this, "Datenbankfehler", e.description (true),
//						QMessageBox::Ok, QMessageBox::NoButton);
//				return false;
//			}
		}
		else
		{
			// Cancel pressed
//			db_error = "Datenbank nicht benutzbar";
			return false;
		}
	} while (retry);

	return true;
}

// ************************************
// ** Database connection management **
// ************************************

void MainWindow::setDatabaseActionsEnabled (bool enabled)
{
	ui.actionDelete                 ->setEnabled (enabled);
	ui.actionEdit                   ->setEnabled (enabled);
	ui.actionEditPeople             ->setEnabled (enabled);
	ui.actionEditPlanes             ->setEnabled (enabled);
	ui.actionJumpToTow              ->setEnabled (enabled);
	ui.actionLand                   ->setEnabled (enabled);
	ui.actionLaunchMethodStatistics ->setEnabled (enabled);
	ui.actionNew                    ->setEnabled (enabled);
	ui.actionPersonLogs             ->setEnabled (enabled);
	ui.actionPingServer             ->setEnabled (enabled);
	ui.actionPlaneLogs              ->setEnabled (enabled);
	ui.actionRefreshAll             ->setEnabled (enabled);
	ui.actionRefreshTable           ->setEnabled (enabled);
	ui.actionRepeat                 ->setEnabled (enabled);
	ui.actionSetDisplayDate         ->setEnabled (enabled);
	ui.actionStart                  ->setEnabled (enabled);
	ui.actionTouchngo               ->setEnabled (enabled);

	// Connect/disconnect are special
	ui.actionConnect    ->setEnabled (!enabled);
	ui.actionDisconnect ->setEnabled ( enabled);
}

void MainWindow::dataStorageStatus (QString state, bool isError)
{
	assert (isGuiThread ());
	ui.databaseStateLabel->setText (state);
	ui.databaseStateLabel->setError (isError);
}

void MainWindow::dataStorageStateChanged (DataStorage::State state)
{
	assert (isGuiThread ());

	ui.databaseStateLabel->setText (dataStorage.stateGetText (state));
	// stateOffline is not an error, but we colorize it red anyway to get the
	// user's attention.
	ui.databaseStateLabel->setError (
		state==DataStorage::stateOffline ||
		dataStorage.stateIsError (state)
	);

	setDatabaseActionsEnabled (dataStorage.isConnectionEstablished ());

	switch (state)
	{
		case DataStorage::stateOffline:
			flightList->clear ();
			break;
		case DataStorage::stateConnecting:
			flightList->clear ();
			break;
		case DataStorage::stateConnected:
			refreshFlights ();
			break;
		case DataStorage::stateUnusable:
			flightList->clear ();
			if (initializeDatabase ())
				dataStorage.connect ();
			break;
		case DataStorage::stateLost:
			break;
	}
}

// **********
// ** Misc **
// **********

void MainWindow::on_actionSetTime_triggered ()
{
	// Get an intermediate QDateTime to avoid a midnight race condition
	QDateTime oldDateTime = QDateTime::currentDateTime ();

	QDate date = oldDateTime.date ();
	QTime time = oldDateTime.time ();

	if (DateInputDialog::editDate (this, &date, &time, "Systemdatum einstellen", "Datum:", false, false, false))
	{
		QString command = QString ("sudo date -s '%1-%2-%3 %4:%5:%6'") .arg (date.year ()).arg (date.month ()).arg (
				date.day ()) .arg (time.hour ()).arg (time.minute ()).arg (time.second ());

		system (command.utf8 ().constData ());

		showWarning (QString::fromUtf8 ("Systemzeit geändert"),
			QString::fromUtf8 ("Die Systemzeit wurde geändert. Um "
			"die Änderung dauerhaft zu speichern, muss der Rechner einmal "
			"heruntergefahren werden, bevor er ausgeschaltet wird."), this);

		//	s="sudo /etc/init.d/hwclock.sh stop &";
		//	r=system (s.c_str ());
		//	if (r!=0) show_warning ("Zurückschreiben der Zeit fehlgeschlagen. Bitte den Rechner\n"
		//	                        "herunterfahren, um die Zeiteinstellung dauerhaft zu speichern.", this);
	}
}

void MainWindow::on_actionTest_triggered ()
{
	// Perform a sleep task in the background
	Task *task = new SleepTask (5);
	//	Task *task=new DataStorage::SleepTask (dataStorage, 5);


	dataStorage.addTask (task);
	TaskProgressDialog::waitTask (this, task);

	delete task;
}

void MainWindow::logMessage (QString message)
{
	QString timeString = QTime::currentTime ().toString ();

	ui.logWidget->append (QString ("[%1] %2").arg (timeString).arg (message));
}

