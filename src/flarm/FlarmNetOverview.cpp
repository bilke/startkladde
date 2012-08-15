#include "FlarmNetOverview.h"

#include <QSettings>
#include <QSortFilterProxyModel>

#include "src/db/DbManager.h"
#include "src/flarm/FlarmNetRecord.h"
#include "src/flarm/FlarmNetRecordModel.h"
#include "src/model/objectList/ObjectListModel.h"

class FlarmNetRecord;

FlarmNetOverview::FlarmNetOverview (DbManager &dbManager, QWidget *parent):
	SkDialog<Ui::FlarmNetOverviewDialog> (parent)
{
	ui.setupUi (this);

	// Search button
	QStyle* style = QApplication::style ();
	ui.clearButton->setIcon (style->standardIcon (QStyle::SP_DialogDiscardButton));

	connect (ui.searchEdit,  SIGNAL(textChanged(const QString&)), this, SLOT(searchTextChanged(const QString&)));
	connect (ui.clearButton, SIGNAL(pressed()), this, SLOT(searchClear()));

	// Get the list of FlarmNet records from the database. It will be deleted by
	// the ObjectListModel.
	EntityList<FlarmNetRecord> *flarmNetRecords=new EntityList<FlarmNetRecord> (dbManager.getCache ().getFlarmNetRecords ());
	
	// Create the object model. It will be deleted by the ObjectListModel.
	ObjectModel<FlarmNetRecord> *flarmNetRecordModel = new FlarmNetRecordModel ();

	// Create the object list model. It will be deleted by its parent, this.
	ObjectListModel<FlarmNetRecord> *objectListModel = new ObjectListModel<FlarmNetRecord> (
		flarmNetRecords, true,
		flarmNetRecordModel, true,
		this);

	// The default row height is too big. Resizing the rows to the contents
	// (using resizeRowsToContents or setautoResizeRows) is very slow for larger
	// lists. Therefore, we use a dummy list to determine the default row height
	// and use setDefaultSectionSize of the vertical header to set the height
	// for rows added later.
	// FIXME this should be encapsulated. However, we'd like to be able to do
	// this without explicitly knowing the type of the object, so the generic
	// model viewer window does not have to be a template. The type is known to
	// the model which is passed into this class, so the model should be able
	// to return dummy contents.
	// The functionality could also be in the view, but note that at the current
	// time, the model may be empty, so we'd have to set a flag to consider the
	// first item added.
    EntityList<FlarmNetRecord> dummyList;
    dummyList.append (FlarmNetRecord ());
    FlarmNetRecordModel dummyObjectModel;
    ObjectListModel<FlarmNetRecord> dummyListModel (&dummyList, false, &dummyObjectModel, false);
    ui.flarmNetTable->setModel (&dummyListModel);
    ui.flarmNetTable->resizeRowsToContents ();
    int rowHeight=ui.flarmNetTable->verticalHeader ()->sectionSize (0);
	ui.flarmNetTable->verticalHeader ()->setDefaultSectionSize (rowHeight);
	ui.flarmNetTable->setModel (NULL);

	// Create a sort/filter proxy model. It will be deleted by its parent, this.
	proxyModel = new QSortFilterProxyModel (this);
	proxyModel->setSourceModel (objectListModel);
	proxyModel->setSortCaseSensitivity (Qt::CaseInsensitive);
	proxyModel->setDynamicSortFilter (true);

	// filter all columns
	proxyModel->setFilterKeyColumn (-1);
	ui.flarmNetTable->setModel (proxyModel);
	
	// Setup the table view
	ui.flarmNetTable->setModel (proxyModel);

	// FIXME row height, but autoResizeRows is very slow
//	ui.flarmNetTable->resizeColumnsToContents ();
//	ui.flarmNetTable->resizeRowsToContents ();
//	ui.flarmNetTable->setAutoResizeColumns (true);
//	ui.flarmNetTable->setAutoResizeRows (true);
}

FlarmNetOverview::~FlarmNetOverview ()
{
} 

void FlarmNetOverview::searchClear () {
        // qDebug () << "FlarmNetOverview::searchClear: " << endl;
        ui.searchEdit->clear();
}

void FlarmNetOverview::searchTextChanged (const QString& search) {
        // qDebug () << "FlarmNetOverview::searchTextChanged: " << search << endl;
        proxyModel->setFilterRegExp (QRegExp (search, Qt::CaseInsensitive, QRegExp::FixedString));
} 
