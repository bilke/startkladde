#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "src/gui/SkDialog.h"
#include "src/i18n/LanguageConfiguration.h"
#include "src/plugin/weather/WeatherPlugin.h"

#include "ui_SettingsWindow.h"

class InfoPlugin;

class SettingsWindow: public SkDialog<Ui::SettingsWindowClass>
{
    Q_OBJECT

	public:
		SettingsWindow (QWidget *parent=0);
		~SettingsWindow ();

		QStringList getPluginPaths ();

	protected:
		void prepareText ();
		void setupText ();

		void readSettings ();
		void writeSettings ();

		void readItem (QTreeWidgetItem *item, const InfoPlugin *plugin);
		void makeItemEditable (QListWidgetItem *item);

		bool allowEdit ();
		void warnEdit ();

		virtual void showEvent (QShowEvent *event);
		virtual void languageChanged ();

		LanguageConfiguration getSelectedLanguageConfiguration ();
		void setSelectedLanguageConfiguration (const LanguageConfiguration &languageConfiguration);

		void reject ();

	private slots:
		void on_mysqlDefaultPortCheckBox_toggled () { updateWidgets (); }

		void on_languageInput_activated (int index);

		void on_addPluginPathButton_clicked ();
		void on_removePluginPathButton_clicked ();
		void on_pluginPathUpButton_clicked ();
		void on_pluginPathDownButton_clicked ();

		void on_addInfoPluginButton_clicked ();
		void on_removeInfoPluginButton_clicked ();
		void on_infoPluginUpButton_clicked ();
		void on_infoPluginDownButton_clicked ();
		void on_infoPluginSettingsButton_clicked ();

		void on_infoPluginList_itemDoubleClicked (QTreeWidgetItem *item, int column);

		void on_weatherPluginInput_currentIndexChanged ();
		void on_weatherWindowPluginInput_currentIndexChanged ();

		void updateWidgets ();

		void on_buttonBox_accepted ();

		void on_browseWeatherPluginCommandButton_clicked ();
		void on_browseWeatherWindowCommandButton_clicked ();

	private:
		bool warned;
		QList<InfoPlugin *> infoPlugins;
		QList<const WeatherPlugin::Descriptor *> weatherPlugins;

	public:
		bool databaseSettingsChanged;
};

#endif
