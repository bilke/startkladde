#include "PluginSettingsDialog.h"

#include <QPushButton>

#include "src/plugin/Plugin.h"
#include "src/plugin/settings/PluginSettingsPane.h"

/**
 * Creates a PluginSettingsPane for a given Plugin instance
 *
 * @param plugin the Plugin instance to be configured
 * @param parent the parent widget
 */
PluginSettingsDialog::PluginSettingsDialog (Plugin *plugin, QWidget *parent):
	QDialog (parent)
{
	ui.setupUi(this);
	ui.buttonBox->button (QDialogButtonBox::Cancel)->setText ("Abbre&chen");

	settingsPane=plugin->createSettingsPane (ui.pluginSettingsPane);
	ui.pluginSettingsPane->layout ()->addWidget (settingsPane);
	settingsPane->readSettings ();
}

PluginSettingsDialog::~PluginSettingsDialog()
{

}

/**
 * Invoked when the OK button is pressed. Instructs the settings pane to write
 * the settings to the plugin and accepts the dialog.
 */
void PluginSettingsDialog::on_buttonBox_accepted ()
{
	if (settingsPane->writeSettings ())
		accept ();
}

/**
 * Displays the dialog for a given plugin, modally
 *
 * @param plugin the plugin to be configured
 * @param parent the parent widget
 * @return the result of the dialog execution
 */
int PluginSettingsDialog::invoke (Plugin *plugin, QWidget *parent)
{
	PluginSettingsDialog *dialog=new PluginSettingsDialog (plugin, parent);
	dialog->setModal (true);
	int result=dialog->exec ();
	delete dialog;
	return result;
}
