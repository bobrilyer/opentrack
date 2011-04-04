/********************************************************************************
* FaceTrackNoIR		This program is a private project of the some enthusiastic	*
*					gamers from Holland, who don't like to pay much for			*
*					head-tracking.												*
*																				*
* Copyright (C) 2010	Wim Vriend (Developing)									*
*						Ron Hendriks (Researching and Testing)					*
*																				*
* Homepage																		*
*																				*
* This program is free software; you can redistribute it and/or modify it		*
* under the terms of the GNU General Public License as published by the			*
* Free Software Foundation; either version 3 of the License, or (at your		*
* option) any later version.													*
*																				*
* This program is distributed in the hope that it will be useful, but			*
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY	*
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for	*
* more details.																	*
*																				*
* You should have received a copy of the GNU General Public License along		*
* with this program; if not, see <http://www.gnu.org/licenses/>.				*
*********************************************************************************/
/*
	Modifications (last one on top):
		20110404 - WVR: Migrated the FlightGear protocol to a separate DLL. The rest must follow...
		20110401 - WVR: The about-dialog was shown 'misplaced'. It was corrected.
		20110328 - WVR: Added the display for output-pose.
		20110207 - WVR: RadioButtons for 'Stop engine' added. It is now possible to choose Stop or Keep tracking.
		20110109 - WVR: Added minimizeTaskBar option added. It is now possible to choose minimized or tray.
*/
#include "FaceTrackNoIR.h"
#include "tracker.h"
#include "PPJoyServer.h"
#include "FSUIPCServer.h"
//#include "FTIRServer.h"
//#include "FGServer.h"
#include "FTNServer.h"


//
// Setup the Main Dialog
//
FaceTrackNoIR::FaceTrackNoIR(QWidget *parent, Qt::WFlags flags) : 
QMainWindow(parent, flags)
{	
	cameraDetected = false;

	//
	// Initialize Widget handles, to prevent memory-access errors.
	//
	_engine_controls = 0;
	_server_controls = 0;
	_keyboard_shortcuts = 0;
	_preferences = 0;
	_keyboard_shortcuts = 0;
	_curve_config = 0;

	tracker = 0;
//	_display = 0;
	l = 0;
	trayIcon = 0;

	setupFaceTrackNoIR();

	//
	// Read the AutoStartTracking value from the registry. If it is '1', start the Tracker and Minimize...
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	if (settings.value ( "AutoStartTracking", 0 ).toBool()) {
		startTracker();
		showMinimized();
	}

    Q_INIT_RESOURCE(PoseWidget);
	_pose_display = new GLWidget(ui.widget4logo, 0);
    _pose_display->rotateBy(0, 0, 0);

	ui.lcdNumOutputRotX->setVisible(false);
	ui.lcdNumOutputRotY->setVisible(false);
	ui.lcdNumOutputRotZ->setVisible(false);
}

/** sets up all objects and connections to buttons */
void FaceTrackNoIR::setupFaceTrackNoIR() {
	
	ui.setupUi(this);

	ui.headPoseWidget->show();
	ui.video_frame->hide();

	// menu objects will be connected with the functions in FaceTrackNoIR class
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(ui.btnLoad, SIGNAL(clicked()), this, SLOT(open()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(save()));
	connect(ui.actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(ui.btnSaveAs, SIGNAL(clicked()), this, SLOT(saveAs()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(exit()));

	connect(ui.actionPreferences, SIGNAL(triggered()), this, SLOT(showPreferences()));
	connect(ui.actionKeyboard_Shortcuts, SIGNAL(triggered()), this, SLOT(showKeyboardShortcuts()));
	connect(ui.actionCurve_Configuration, SIGNAL(triggered()), this, SLOT(showCurveConfiguration()));
	connect(ui.btnEditCurves, SIGNAL(clicked()), this, SLOT(showCurveConfiguration()));

	connect(ui.actionSupport, SIGNAL(triggered()), this, SLOT(openurl_support()));
	connect(ui.actionYour_Support, SIGNAL(triggered()), this, SLOT(openurl_donation()));
	connect(ui.btnDonate, SIGNAL(clicked()), this, SLOT(openurl_donation()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));

	connect(ui.actionVideoWidget, SIGNAL(triggered()), this, SLOT(showVideoWidget()));
	connect(ui.actionHeadPoseWidget, SIGNAL(triggered()), this, SLOT(showHeadPoseWidget()));
	connect(ui.btnShowEngineControls, SIGNAL(clicked()), this, SLOT(showEngineControls()));
	connect(ui.btnShowServerControls, SIGNAL(clicked()), this, SLOT(showServerControls()));

	// button methods connect with methods in this class
	connect(ui.btnStartTracker, SIGNAL(clicked()), this, SLOT(startTracker()));
	connect(ui.btnStopTracker, SIGNAL(clicked()), this, SLOT(stopTracker()));

	// Connect checkboxes
	connect(ui.chkInvertYaw, SIGNAL(stateChanged(int)), this, SLOT(setInvertYaw(int)));
	connect(ui.chkInvertRoll, SIGNAL(stateChanged(int)), this, SLOT(setInvertRoll(int)));
	connect(ui.chkInvertPitch, SIGNAL(stateChanged(int)), this, SLOT(setInvertPitch(int)));
	connect(ui.chkInvertX, SIGNAL(stateChanged(int)), this, SLOT(setInvertX(int)));
	connect(ui.chkInvertY, SIGNAL(stateChanged(int)), this, SLOT(setInvertY(int)));
	connect(ui.chkInvertZ, SIGNAL(stateChanged(int)), this, SLOT(setInvertZ(int)));

	connect(ui.chkUseEWMA, SIGNAL(stateChanged(int)), this, SLOT(setUseFilter(int)));

	// Connect sliders for reduction factor
	connect(ui.minSmooth, SIGNAL(valueChanged(int)), this, SLOT(setMinSmooth(int)));
	connect(ui.maxSmooth, SIGNAL(valueChanged(int)), this, SLOT(setMaxSmooth(int)));
	connect(ui.powCurve, SIGNAL(valueChanged(int)), this, SLOT(setPowCurve(int)));

	// Connect slider for smoothing
	connect(ui.slideSmoothing, SIGNAL(valueChanged(int)), this, SLOT(setSmoothing(int)));

	//read the camera-name, using DirectShow
	GetCameraNameDX();
	
	//Create the system-tray and connect the events for that.
	createIconGroupBox();
	createActions();
	createTrayIcon();

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

	//Load the tracker-settings, from the INI-file
	loadSettings();
	trayIcon->show();

	connect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));
	connect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	connect(ui.iconcomboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setIcon(int)));

	//Setup the timer for automatically minimizing after StartTracker.
	timMinimizeFTN = new QTimer(this);
    connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(showMinimized()));

	//Setup the timer for showing the headpose.
	timUpdateHeadPose = new QTimer(this);
    connect(timUpdateHeadPose, SIGNAL(timeout()), this, SLOT(showHeadPose()));
}

/** destructor stops the engine and quits the faceapi **/
FaceTrackNoIR::~FaceTrackNoIR() {

	//
	// Stop the tracker, by simulating a button-push
	//
	stopTracker();

	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard, QMessageBox::Discard );

		switch (ret) {
			case QMessageBox::Save:
				saveAs();
				break;
			case QMessageBox::Discard:
				// Don't Save was clicked
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
}

//
// Get the ProgramName from a connected game and display it.
//
void FaceTrackNoIR::getGameProgramName() {
	if ( tracker != NULL ) {
		ui.cameraName->setText( tracker->getGameProgramName() );
	}
}

//
// Update the Settings, after a value has changed. This way, the Tracker does not have to re-start.
//
void FaceTrackNoIR::updateSettings() {
	if ( tracker != NULL ) {
		tracker->loadSettings();
	}
}

//
// Get a pointer to the video-widget, to use in the DLL
//
QFrame *FaceTrackNoIR::getVideoWidget() {
	return ui.video_frame;
}

/** read the name of the first video-capturing device at start up **/
/** FaceAPI can only use this first one... **/
void FaceTrackNoIR::GetCameraNameDX() {
	
////	ui.widget->setCameraName("No video-capturing device was found in your system: check if it's connected!");

	ui.cameraName->setText("No video-capturing device was found in your system: check if it's connected!");

	// Create the System Device Enumerator.
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum = NULL;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr))
	{
		qDebug() << "GetWDM says: CoCreateInstance Failed!";
		return;
	}

	qDebug() << "GetWDM says: CoCreateInstance succeeded!";
	
	// Obtain a class enumerator for the video compressor category.
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK) {
		qDebug() << "GetWDM says: CreateClassEnumerator succeeded!";

		// Enumerate the monikers.
		IMoniker *pMoniker = NULL;
		ULONG cFetched;
		if (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) {
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (SUCCEEDED(hr))	{
				// To retrieve the filter's friendly name, do the following:
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr))
				{
					// Display the name in your UI somehow.
					QString str((QChar*)varName.bstrVal, wcslen(varName.bstrVal));
					qDebug() << "GetWDM says: Moniker found:" << str;
					ui.cameraName->setText(str);
					return;
				}
				VariantClear(&varName);

				////// To create an instance of the filter, do the following:
				////IBaseFilter *pFilter;
				////hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
				////	(void**)&pFilter);
				// Now add the filter to the graph. 
				//Remember to release pFilter later.
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();

}

//
// Open an INI-file with the QFileDialog
// If succesfull, the settings in it will be read
//
void FaceTrackNoIR::open() {
	 QFileDialog::Options options;
	 QFileDialog::FileMode mode;

     options |= QFileDialog::DontUseNativeDialog;
	 mode = QFileDialog::ExistingFile;
     QString selectedFilter;
	 QStringList fileNames = QFileDialog::getOpenFileNames(
								this,
                                 tr("Select one FTNoir settings file"),
								 QCoreApplication::applicationDirPath() + "/Settings",
                                 tr("Settings file (*.ini);;All Files (*)"));

	//
	// If a file was selected, save it's name and read it's contents.
	//
	if (! fileNames.isEmpty() ) {
		QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
		settings.setValue ("SettingsFile", fileNames.at(0));
		loadSettings();
	}
}

//
// Save the current Settings to the currently 'active' INI-file.
//
void FaceTrackNoIR::save() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	iniFile.beginGroup ( "Tracking" );
	iniFile.setValue ( "Smooth", ui.slideSmoothing->value() );
	iniFile.setValue ( "invertYaw", ui.chkInvertYaw->isChecked() );
	iniFile.setValue ( "invertPitch", ui.chkInvertPitch->isChecked() );
	iniFile.setValue ( "invertRoll", ui.chkInvertRoll->isChecked() );
	iniFile.setValue ( "invertX", ui.chkInvertX->isChecked() );
	iniFile.setValue ( "invertY", ui.chkInvertY->isChecked() );
	iniFile.setValue ( "invertZ", ui.chkInvertZ->isChecked() );
	iniFile.setValue ( "useEWMA", ui.chkUseEWMA->isChecked() );
	iniFile.setValue ( "minSmooth", ui.minSmooth->value() );
	iniFile.setValue ( "powCurve", ui.powCurve->value() );
	iniFile.setValue ( "maxSmooth", ui.maxSmooth->value() );
	iniFile.endGroup ();

	iniFile.beginGroup ( "GameProtocol" );
	iniFile.setValue ( "Selection", ui.iconcomboBox->currentIndex() );
	iniFile.endGroup ();

	iniFile.beginGroup ( "TrackerSource" );
	iniFile.setValue ( "Selection", ui.iconcomboTrackerSource->currentIndex() );
	iniFile.endGroup ();

	settingsDirty = false;
}

//
// Get the new name of the INI-file and save the settings to it.
//
// The user may choose to overwrite an existing file. This will be deleted, before copying the current file to it.
//
void FaceTrackNoIR::saveAs()
{
	//
	// Get the current filename of the INI-file.
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	QString oldFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();

	//
	// Get the new filename of the INI-file.
	//
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),
													oldFile,
//													QCoreApplication::applicationDirPath() + "/Settings",
													tr("Settings file (*.ini);;All Files (*)"));
	if (!fileName.isEmpty()) {

		//
		// Remove the file, if it already exists.
		//
		QFileInfo newFileInfo ( fileName );
		if ((newFileInfo.exists()) && (oldFile != fileName)) {
			QFile newFileFile ( fileName );
			newFileFile.remove();
		}

		//
		// Copy the current INI-file to the new name.
		//
		QFileInfo oldFileInfo ( oldFile );
		if (oldFileInfo.exists()) {
			QFile oldFileFile ( oldFile );
			oldFileFile.copy( fileName );
		}

		//
		// Write the new name to the Registry and save the other INI-values.
		//
		settings.setValue ("SettingsFile", fileName);
		save();

		//
		// Reload the settings, to get the GUI right again...
		//
		loadSettings();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void FaceTrackNoIR::loadSettings() {

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	//
	// Put the filename in the window-title.
	//
    QFileInfo pathInfo ( currentFile );
    setWindowTitle ( "FaceTrackNoIR (1.6) - " + pathInfo.fileName() );

	//
	// Get a List of all the INI-files in the (currently active) Settings-folder.
	//
	QDir settingsDir( pathInfo.dir() );
    QStringList filters;
    filters << "*.ini";
	iniFileList.clear();
	iniFileList = settingsDir.entryList( filters, QDir::Files, QDir::Name );
	
	//
	// Add strings to the Listbox.
	//
	disconnect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));
	ui.iconcomboProfile->clear();
	for ( int i = 0; i < iniFileList.size(); i++) {
		ui.iconcomboProfile->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/Settings16.png"), iniFileList.at(i));
		if (iniFileList.at(i) == pathInfo.fileName()) {
			ui.iconcomboProfile->setItemIcon(i, QIcon(QCoreApplication::applicationDirPath() + "/images/SettingsOpen16.png"));
			ui.iconcomboProfile->setCurrentIndex( i );
		}
	}
	connect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "Tracking" );
	ui.slideSmoothing->setValue (iniFile.value ( "Smooth", 10 ).toInt());
	ui.chkInvertYaw->setChecked (iniFile.value ( "invertYaw", 0 ).toBool());
	ui.chkInvertPitch->setChecked (iniFile.value ( "invertPitch", 0 ).toBool());
	ui.chkInvertRoll->setChecked (iniFile.value ( "invertRoll", 0 ).toBool());
	ui.chkInvertX->setChecked (iniFile.value ( "invertX", 0 ).toBool());
	ui.chkInvertY->setChecked (iniFile.value ( "invertY", 0 ).toBool());
	ui.chkInvertZ->setChecked (iniFile.value ( "invertZ", 0 ).toBool());
	ui.chkUseEWMA->setChecked (iniFile.value ( "useEWMA", 1 ).toBool());

	ui.minSmooth->setValue (iniFile.value ( "minSmooth", 15 ).toInt());
	ui.maxSmooth->setValue (iniFile.value ( "maxSmooth", 50 ).toInt());
	ui.powCurve->setValue (iniFile.value ( "powCurve", 10 ).toInt());
	iniFile.endGroup ();

	iniFile.beginGroup ( "GameProtocol" );
	ui.iconcomboBox->setCurrentIndex(iniFile.value ( "Selection", 0 ).toInt());
	setIcon( ui.iconcomboBox->currentIndex() );
	iniFile.endGroup ();

	iniFile.beginGroup ( "TrackerSource" );
	ui.iconcomboTrackerSource->setCurrentIndex(iniFile.value ( "Selection", 0 ).toInt());
	trackingSourceSelected( ui.iconcomboTrackerSource->currentIndex() );
	iniFile.endGroup ();

	settingsDirty = false;
}

/** show support page in web-browser **/
void FaceTrackNoIR::openurl_support() {
	QDesktopServices::openUrl(QUrl("http://facetracknoir.sourceforge.net/manual/manual.htm", QUrl::TolerantMode));
}

/** show donations page in web-browser **/
void FaceTrackNoIR::openurl_donation() {
	QDesktopServices::openUrl(QUrl("http://facetracknoir.sourceforge.net/information_links/donate.htm", QUrl::TolerantMode));
}


/** show about dialog **/
void FaceTrackNoIR::about() {

	QPoint offsetpos(100, 100);
	aboutDialog.move(this->pos() + offsetpos);
	aboutDialog.show();

	/** ABOUT DIALOG **/
	aboutDialog.setBaseSize(270, 440);

	aboutDialog.setMaximumWidth(270);
	aboutDialog.setMaximumHeight(440);

	aboutDialog.setMinimumWidth(270);
	aboutDialog.setMinimumHeight(440);
	aboutDialog.setStyleSheet("background:#fff url(UIElements/aboutFaceTrackNoIR.png) no-repeat;");
}

/** start tracking the face **/
void FaceTrackNoIR::startTracker( ) {	

	//
	// Create the Tracker and setup
	//
	tracker = new Tracker ( ui.iconcomboBox->currentIndex(), ui.iconcomboTrackerSource->currentIndex(), this );

	//
	// Setup the Tracker and send the settings.
	// This is necessary, because the events are only triggered 'on change'
	//
	tracker->setup();
	tracker->setSmoothing ( ui.slideSmoothing->value() );
	tracker->setUseFilter (ui.chkUseEWMA->isChecked() );

	tracker->setInvertYaw (ui.chkInvertYaw->isChecked() );
	tracker->setInvertPitch (ui.chkInvertPitch->isChecked() );
	tracker->setInvertRoll (ui.chkInvertRoll->isChecked() );
	tracker->setInvertX (ui.chkInvertX->isChecked() );
	tracker->setInvertY (ui.chkInvertY->isChecked() );
	tracker->setInvertZ (ui.chkInvertZ->isChecked() );

	tracker->setMinSmooth (ui.minSmooth->value() );
	tracker->setPowCurve (ui.powCurve->value() );
	tracker->setMaxSmooth (ui.maxSmooth->value() );

	tracker->start( QThread::TimeCriticalPriority );

	ui.headPoseWidget->show();

	// 
	ui.btnStartTracker->setEnabled ( false );
	ui.btnStopTracker->setEnabled ( true );

	// Engine controls
	switch (ui.iconcomboTrackerSource->currentIndex()) {
	case FT_SM_FACEAPI:										// Face API
		ui.btnShowEngineControls->setEnabled ( true );		// Active only when started!
		break;
	case FT_FTNOIR:											// FTNoir server
		ui.btnShowEngineControls->setEnabled ( false );
		break;
	default:
		break;
	}

	// Enable/disable Protocol-server Settings
	ui.iconcomboBox->setEnabled ( false );
	ui.btnShowServerControls->setEnabled ( false );

	//
	// Update the camera-name, FaceAPI can only use the 1st one found!
	//
	GetCameraNameDX();


	//
	// Get the TimeOut value for minimizing FaceTrackNoIR
	// Only start the Timer if value > 0
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	int timevalue = settings.value ( "AutoMinimizeTime", 0 ).toInt() * 1000;
	if (timevalue > 0) {

		bool minimizeTaskBar = settings.value ( "MinimizeTaskBar", 1 ).toBool();
		if (minimizeTaskBar) {
			connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(showMinimized()));
		}
		else {
			connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(hide()));
		}

		timMinimizeFTN->setSingleShot( true );
		timMinimizeFTN->start(timevalue);
	}

	//
	// Start the timer to update the head-pose (digits and 'man in black')
	//
	timUpdateHeadPose->start(10);
	ui.lcdNumOutputRotX->setVisible(true);
	ui.lcdNumOutputRotY->setVisible(true);
	ui.lcdNumOutputRotZ->setVisible(true);
}

/** stop tracking the face **/
void FaceTrackNoIR::stopTracker( ) {	

	//
	// Stop displaying the head-pose.
	//
	timUpdateHeadPose->stop();
    _pose_display->rotateBy(0, 0, 0);
	ui.lcdNumOutputRotX->setVisible(false);
	ui.lcdNumOutputRotY->setVisible(false);
	ui.lcdNumOutputRotZ->setVisible(false);

	//
	// Delete the tracker (after stopping things and all).
	//
	if ( tracker ) {
		qDebug() << "stopTracker says: Deleting tracker!";
		delete tracker;
		qDebug() << "stopTracker says: Tracker deleted!";
		tracker = 0;
	}
	ui.btnStartTracker->setEnabled ( true );
	ui.btnStopTracker->setEnabled ( false );
	ui.btnShowEngineControls->setEnabled ( false );
	ui.iconcomboBox->setEnabled ( true );

	// Enable/disable Protocol-server Settings
	ui.btnShowServerControls->setEnabled ( true );

	// Engine controls
	switch (ui.iconcomboTrackerSource->currentIndex()) {
	case FT_SM_FACEAPI:										// Face API
		ui.btnShowEngineControls->setEnabled ( false );		// Active only when started!
		break;
	case FT_FTNOIR:											// FTNoir server
		ui.btnShowEngineControls->setEnabled ( true );
		break;
	default:
		break;
	}

	//
	// Stop the timer, so it won't go off again...
	//
	timMinimizeFTN->stop();

}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertYaw( int invert ) {
	Tracker::setInvertYaw ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertPitch( int invert ) {
	Tracker::setInvertPitch ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertRoll( int invert ) {
	Tracker::setInvertRoll ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertX( int invert ) {
	Tracker::setInvertX ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertY( int invert ) {
	Tracker::setInvertY ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertZ( int invert ) {
	Tracker::setInvertZ ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set Use Filter from the checkbox **/
void FaceTrackNoIR::setUseFilter( int set ) {
	Tracker::setUseFilter ( (set != 0)?true:false );
	settingsDirty = true;
}

/** Show the headpose in the widget (triggered by timer) **/
void FaceTrackNoIR::showHeadPose() {
THeadPoseData newdata;

	//
	// Get the pose and also display it.
	// Updating the pose from within the Tracker-class caused crashes...
	//
	Tracker::getHeadPose(&newdata);
	ui.lcdNumX->display((double) (((int)(newdata.x * 10.0f))/10.0f));
	ui.lcdNumY->display((double) (((int)(newdata.y * 10.0f))/10.0f));
	ui.lcdNumZ->display((double) (((int)(newdata.z * 10.0f))/10.0f));

	ui.lcdNumRotX->display((double) (((int)(newdata.yaw * 10.0f))/10.0f));
	ui.lcdNumRotY->display((double) (((int)(newdata.pitch * 10.0f))/10.0f));
	ui.lcdNumRotZ->display((double) (((int)(newdata.roll * 10.0f))/10.0f));

	//
	// Get the output-pose and also display it.
	//
	if (_pose_display) {
		Tracker::getOutputHeadPose(&newdata);
		_pose_display->rotateBy(newdata.pitch, newdata.yaw, newdata.roll);

		ui.lcdNumOutputRotX->display((double) (((int)(newdata.yaw * 10.0f))/10.0f));
		ui.lcdNumOutputRotY->display((double) (((int)(newdata.pitch * 10.0f))/10.0f));
		ui.lcdNumOutputRotZ->display((double) (((int)(newdata.roll * 10.0f))/10.0f));
	}
}

/** set the redhold from the slider **/
void FaceTrackNoIR::setMinSmooth( int redh ) {
	Tracker::setMinSmooth ( redh );
	settingsDirty = true;
}

/** set the redhold from the slider **/
void FaceTrackNoIR::setPowCurve( int redh ) {
	Tracker::setPowCurve ( redh );
	settingsDirty = true;
}

/** set the redhold from the slider **/
void FaceTrackNoIR::setMaxSmooth( int redh ) {
	Tracker::setMaxSmooth ( redh );
	settingsDirty = true;
}

/** set the smoothing from the slider **/
void FaceTrackNoIR::setSmoothing( int smooth ) {
	
	//
	// Pass the smoothing setting, if the Tracker exists.
	//
	if ( tracker ) {
		tracker->setSmoothing ( smooth );
		settingsDirty = true;
	}
}


/** toggles Video Widget **/
void FaceTrackNoIR::showVideoWidget() {
	if(ui.video_frame->isHidden())
		ui.video_frame->show();
	else
		ui.video_frame->hide();
}

/** toggles Video Widget **/
void FaceTrackNoIR::showHeadPoseWidget() {
	if(ui.headPoseWidget->isHidden())
		ui.headPoseWidget->show();
	else
		ui.headPoseWidget->hide();
}

/** toggles Engine Controls Dialog **/
void FaceTrackNoIR::showEngineControls() {
importGetTrackerDialog getIT;
QLibrary *trackerLib;


	qDebug() << "FaceTrackNoIR::showEngineControls started.";

	//
	// Delete the existing QDialog
	//
	if (_engine_controls) {
		delete _engine_controls;
		_engine_controls = 0;
	}

	qDebug() << "FaceTrackNoIR::showEngineControls after remove engine_controls.";

	// Create  new
    if (!_engine_controls)
    {
		switch (ui.iconcomboTrackerSource->currentIndex()) {
		case FT_SM_FACEAPI:										// Face API
//			_engine_controls = new EngineControls( tracker->getEngine(), true, false, this, Qt::Dialog );
			break;
		case FT_FTNOIR:											// FTNoir server
			qDebug() << "FaceTrackNoIR::showEngineControls case FT_FTNOIR.";

			trackerLib = new QLibrary("FTNoIR_Tracker_UDP.dll");

			qDebug() << "FaceTrackNoIR::showEngineControls Loaded trackerLib." << trackerLib;

			getIT = (importGetTrackerDialog) trackerLib->resolve("GetTrackerDialog");

			qDebug() << "FaceTrackNoIR::showEngineControls resolved?." << getIT;

			if (getIT) {
				ITrackerDialogPtr ptrXyz(getIT());
				if (ptrXyz)
				{
					pTrackerDialog = ptrXyz;
					pTrackerDialog->Initialize( this );
					qDebug() << "FaceTrackNoIR::showEngineControls GetTrackerDialog Function Resolved!";
				}
			}
			else {
				QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
			}

			break;
		default:
			break;
		}

    }

	// Show if already created
	if (_engine_controls) {
		_engine_controls->show();
		_engine_controls->raise();
	}
}

/** toggles Server Controls Dialog **/
void FaceTrackNoIR::showServerControls() {
importGetProtocolDialog getIT;
QLibrary *protocolLib;


	//
	// Delete the existing QDialog
	//
	if (_server_controls) {
		delete _server_controls;
		_server_controls = 0;
	}

	// Create if new
	if (!_server_controls)
    {

		// Show the appropriate Protocol-server Settings
		switch (ui.iconcomboBox->currentIndex()) {
		case FREE_TRACK:
		case SIMCONNECT:
			break;
		case PPJOY:
	        _server_controls = new PPJoyControls( this, Qt::Dialog );
			break;
		case FSUIPC:
	        _server_controls = new FSUIPCControls( this, Qt::Dialog );
			break;
		case TRACKIR:
			protocolLib = new QLibrary("FTNoIR_Protocol_FTIR.dll");

			getIT = (importGetProtocolDialog) protocolLib->resolve("GetProtocolDialog");
			if (getIT) {
				IProtocolDialogPtr ptrXyz(getIT());
				if (ptrXyz)
				{
					pProtocolDialog = ptrXyz;
					pProtocolDialog->Initialize( this );
					qDebug() << "FaceTrackNoIR::showServerControls GetProtocolDialog Function Resolved!";
				}
				else {
					qDebug() << "FaceTrackNoIR::showServerControls Function NOT Resolved!";
				}	
			}
			else {
				QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
			}
			break;

		case FLIGHTGEAR:
			protocolLib = new QLibrary("FTNoIR_Protocol_FG.dll");

			getIT = (importGetProtocolDialog) protocolLib->resolve("GetProtocolDialog");
			if (getIT) {
				IProtocolDialogPtr ptrXyz(getIT());
				if (ptrXyz)
				{
					pProtocolDialog = ptrXyz;
					pProtocolDialog->Initialize( this );
					qDebug() << "FaceTrackNoIR::showServerControls GetProtocolDialog Function Resolved!";
				}
				else {
					qDebug() << "FaceTrackNoIR::showServerControls Function NOT Resolved!";
				}	
			}
			else {
				QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
			}
			break;

		case FTNOIR:
	        _server_controls = new FTNServerControls( this, Qt::Dialog );
			break;
		default:
			break;
		}
    }

	// Show if already created
	if (_server_controls) {
		_server_controls->show();
		_server_controls->raise();
	}
}

/** toggles FaceTrackNoIR Preferences Dialog **/
void FaceTrackNoIR::showPreferences() {

	// Create if new
	if (!_preferences)
    {
        _preferences = new PreferencesDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_preferences) {
		_preferences->show();
		_preferences->raise();
	}
}

/** toggles Keyboard Shortcut Dialog **/
void FaceTrackNoIR::showKeyboardShortcuts() {

	// Create if new
	if (!_keyboard_shortcuts)
    {
        _keyboard_shortcuts = new KeyboardShortcutDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_keyboard_shortcuts) {
		_keyboard_shortcuts->show();
		_keyboard_shortcuts->raise();
	}
}

/** toggles Curve Configuration Dialog **/
void FaceTrackNoIR::showCurveConfiguration() {

	// Create if new
	if (!_curve_config)
    {
        _curve_config = new CurveConfigurationDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_curve_config) {
		_curve_config->show();
		_curve_config->raise();
	}
}

/** exit application **/
void FaceTrackNoIR::exit() {
	QCoreApplication::exit(0);
}

//
// Setup the icons for the comboBoxes
//
void FaceTrackNoIR::createIconGroupBox()
{
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/Freetrack.ico"), tr("Freetrack"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/FlightGear.ico"), tr("FlightGear"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/FaceTrackNoIR.ico"), tr("FTNoir client"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/PPJoy.ico"), tr("Virtual Joystick"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/TrackIR.ico"), tr("Fake TrackIR"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/FSX.ico"), tr("SimConnect (FSX)"));
	ui.iconcomboBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/FS9.ico"), tr("FS2002/FS2004"));

	ui.iconcomboTrackerSource->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/SeeingMachines.ico"), tr("Face API"));
	ui.iconcomboTrackerSource->addItem(QIcon(QCoreApplication::applicationDirPath() + "/images/FaceTrackNoIR.ico"), tr("FTNoir server"));
}

//
// Create the Actions in the System tray and connect them to Application events
//
void FaceTrackNoIR::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize FaceTrackNoIR"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    //maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore FaceTrackNoIR"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    quitAction = new QAction(tr("&Quit FaceTrackNoIR"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

//
// Create the SystemTray and set the default Icon
//
void FaceTrackNoIR::createTrayIcon()
{
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		trayIconMenu = new QMenu(this);
		trayIconMenu->addAction(minimizeAction);
		trayIconMenu->addAction(restoreAction);
		trayIconMenu->addSeparator();
		trayIconMenu->addAction(quitAction);

		trayIcon = new QSystemTrayIcon(this);
		trayIcon->setContextMenu(trayIconMenu);

		trayIcon->setIcon(QIcon(QCoreApplication::applicationDirPath() + "/images/FaceTrackNoIR.ico"));
	}
}

//
// Set the Tray icon, using the Game-protocol combobox as source
//
void FaceTrackNoIR::setIcon(int index)
{
    QIcon icon = ui.iconcomboBox->itemIcon(index);
	if (trayIcon != 0) {
		trayIcon->setIcon(icon);
	    trayIcon->setToolTip(ui.iconcomboBox->itemText(index));
		trayIcon->show();
		trayIcon->showMessage( "FaceTrackNoIR", ui.iconcomboBox->itemText(index));
	}
    setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/images/FaceTrackNoIR.ico"));
	ui.btnShowServerControls->setIcon(icon);

	settingsDirty = true;

	// Enable/disable Protocol-server Settings
	switch (ui.iconcomboBox->currentIndex()) {
	case FREE_TRACK:
	case SIMCONNECT:
		ui.btnShowServerControls->hide();
		break;
	case PPJOY:
	case FSUIPC:
	case TRACKIR:
	case FLIGHTGEAR:
	case FTNOIR:
		ui.btnShowServerControls->show();
		ui.btnShowServerControls->setEnabled ( true );
		break;

	default:
		break;
	}

}
 
//
// Handle SystemTray events
//
void FaceTrackNoIR::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
     switch (reason) {
     case QSystemTrayIcon::Trigger:
     case QSystemTrayIcon::DoubleClick:
         //ui.iconcomboBox->setCurrentIndex((ui.iconcomboBox->currentIndex() + 1)
         //                              % ui.iconcomboBox->count());
         break;
     ////case QSystemTrayIcon::MiddleClick:
     ////    showMessage();
     ////    break;
     default:
         ;
     }
 }

//
// Handle changes of the Tracking Source selection
//
void FaceTrackNoIR::trackingSourceSelected(int index)
{
	settingsDirty = true;
	switch ( index ) {
		case FT_SM_FACEAPI:										// Face API
			ui.btnShowEngineControls->setEnabled ( false );
			break;
		case FT_FTNOIR:											// FTNoir server
			ui.video_frame->hide();
			ui.headPoseWidget->show();
			ui.btnShowEngineControls->setEnabled ( true );
			break;
		default:
			break;
	}
}

//
// Handle changes of the Profile selection
//
void FaceTrackNoIR::profileSelected(int index)
{
	//
	// Read the current INI-file setting, to get the folder in which it's located...
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
    QFileInfo pathInfo ( currentFile );

	//
	// Get a List of all the INI-files in the (currently active) Settings-folder.
	//
	settings.setValue ("SettingsFile", pathInfo.absolutePath() + "/" + iniFileList.at(ui.iconcomboProfile->currentIndex()));
	loadSettings();
}


//
// Constructor for FaceTrackNoIR=Preferences-dialog
//
PreferencesDialog::PreferencesDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(100, 100);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	connect(ui.spinAutoMinimizeTime, SIGNAL(valueChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkAutoStartTracking, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.radioMinimize, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
PreferencesDialog::~PreferencesDialog() {
	qDebug() << "~PreferencesDialog() says: started";
}

//
// OK clicked on server-dialog
//
void PreferencesDialog::doOK() {
	save();
	this->close();
}

// override show event
void PreferencesDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void PreferencesDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void PreferencesDialog::loadSettings() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	ui.spinAutoMinimizeTime->setValue( settings.value ( "AutoMinimizeTime", 0 ).toInt() );
	ui.chkAutoStartTracking->setChecked( settings.value ( "AutoStartTracking", 0 ).toBool() );
	ui.radioMinimize->setChecked( settings.value ( "MinimizeTaskBar", 1 ).toBool() );

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void PreferencesDialog::save() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	settings.setValue( "AutoMinimizeTime", ui.spinAutoMinimizeTime->value() );
	settings.setValue( "AutoStartTracking", ui.chkAutoStartTracking->isChecked() );
	settings.setValue( "MinimizeTaskBar", ui.radioMinimize->isChecked() );

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();

	settingsDirty = false;
}

//**************************************************************************************************//
//**************************************************************************************************//
//
// Constructor for Keyboard-shortcuts-dialog
//
KeyboardShortcutDialog::KeyboardShortcutDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(100, 100);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	connect(ui.cbxCenterKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkCenterShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkCenterCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkCenterAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	connect(ui.cbxStartStopKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkStartStopShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkStartStopCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkStartStopAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.radioSetZero, SIGNAL(toggled(bool)), this, SLOT(keyChanged(bool)));
	connect(ui.radioSetEngineStop, SIGNAL(toggled(bool)), this, SLOT(keyChanged(bool)));

	connect(ui.cbxInhibitKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkInhibitShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	// Also add events for the Axis-checkboxes
	connect(ui.chkInhibitShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitYaw, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitRoll, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitX, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitY, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitZ, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	//
	// Clear the Lists with key-descriptions and keycodes and build the Lists
	// The strings will all be added to the ListBoxes for each Shortkey
	//
	stringList.clear();
	stringList.append("NONE");
	stringList.append("F1");
	stringList.append("F2");
	stringList.append("F3");
	stringList.append("F4");
	stringList.append("F5");
	stringList.append("F6");
	stringList.append("F7");
	stringList.append("F8");
	stringList.append("F9");
	stringList.append("F10");
	stringList.append("F11");
	stringList.append("F12");
	stringList.append("MINUS");
	stringList.append("EQUALS");
	stringList.append("BACK");
	stringList.append("A");
	stringList.append("B");
	stringList.append("C");
	stringList.append("D");
	stringList.append("E");
	stringList.append("F");
	stringList.append("G");
	stringList.append("H");
	stringList.append("I");
	stringList.append("J");
	stringList.append("K");
	stringList.append("L");
	stringList.append("M");
	stringList.append("N");
	stringList.append("O");
	stringList.append("P");
	stringList.append("Q");
	stringList.append("R");
	stringList.append("S");
	stringList.append("T");
	stringList.append("U");
	stringList.append("V");
	stringList.append("W");
	stringList.append("X");
	stringList.append("Y");
	stringList.append("Z");
	stringList.append("NUMPAD0");
	stringList.append("NUMPAD1");
	stringList.append("NUMPAD2");
	stringList.append("NUMPAD3");
	stringList.append("NUMPAD4");
	stringList.append("NUMPAD5");
	stringList.append("NUMPAD6");
	stringList.append("NUMPAD7");
	stringList.append("NUMPAD8");
	stringList.append("NUMPAD9");
	stringList.append("HOME");
	stringList.append("UP");
	stringList.append("PGUP");		/* PgUp on arrow keypad */
	stringList.append("LEFT");
	stringList.append("RIGHT");
	stringList.append("END");
	stringList.append("DOWN");
	stringList.append("PGDWN");		/* PgDn on arrow keypad */
	stringList.append("INSERT");
	stringList.append("DELETE");

	keyList.clear();
	keyList.append(0);				// NONE = 0
	keyList.append(DIK_F1);
	keyList.append(DIK_F2);
	keyList.append(DIK_F3);
	keyList.append(DIK_F4);
	keyList.append(DIK_F5);
	keyList.append(DIK_F6);
	keyList.append(DIK_F7);
	keyList.append(DIK_F8);
	keyList.append(DIK_F9);
	keyList.append(DIK_F10);
	keyList.append(DIK_F11);
	keyList.append(DIK_F12);
	keyList.append(DIK_MINUS);
	keyList.append(DIK_EQUALS);
	keyList.append(DIK_BACK);
	keyList.append(DIK_A);
	keyList.append(DIK_B);
	keyList.append(DIK_C);
	keyList.append(DIK_D);
	keyList.append(DIK_E);
	keyList.append(DIK_F);
	keyList.append(DIK_G);
	keyList.append(DIK_H);
	keyList.append(DIK_I);
	keyList.append(DIK_J);
	keyList.append(DIK_K);
	keyList.append(DIK_L);
	keyList.append(DIK_M);
	keyList.append(DIK_N);
	keyList.append(DIK_O);
	keyList.append(DIK_P);
	keyList.append(DIK_Q);
	keyList.append(DIK_R);
	keyList.append(DIK_S);
	keyList.append(DIK_T);
	keyList.append(DIK_U);
	keyList.append(DIK_V);
	keyList.append(DIK_W);
	keyList.append(DIK_X);
	keyList.append(DIK_Y);
	keyList.append(DIK_Z);
	keyList.append(DIK_NUMPAD0);
	keyList.append(DIK_NUMPAD1);
	keyList.append(DIK_NUMPAD2);
	keyList.append(DIK_NUMPAD3);
	keyList.append(DIK_NUMPAD4);
	keyList.append(DIK_NUMPAD5);
	keyList.append(DIK_NUMPAD6);
	keyList.append(DIK_NUMPAD7);
	keyList.append(DIK_NUMPAD8);
	keyList.append(DIK_NUMPAD9);
	keyList.append(DIK_HOME);
	keyList.append(DIK_UP);
	keyList.append(DIK_PRIOR);		/* PgUp on arrow keypad */
	keyList.append(DIK_LEFT);
	keyList.append(DIK_RIGHT);
	keyList.append(DIK_END);
	keyList.append(DIK_DOWN);
	keyList.append(DIK_NEXT);		/* PgDn on arrow keypad */
	keyList.append(DIK_INSERT);
	keyList.append(DIK_DELETE);

	//
	// Add strings to the Listboxes.
	//
	for ( int i = 0; i < stringList.size(); i++) {
		ui.cbxCenterKey->addItem(stringList.at(i));
		ui.cbxStartStopKey->addItem(stringList.at(i));
		ui.cbxInhibitKey->addItem(stringList.at(i));
	}

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
KeyboardShortcutDialog::~KeyboardShortcutDialog() {
	qDebug() << "~KeyboardShortcutDialog() says: started";
}

//
// OK clicked on server-dialog
//
void KeyboardShortcutDialog::doOK() {
	save();
	this->close();
}

// override show event
void KeyboardShortcutDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void KeyboardShortcutDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void KeyboardShortcutDialog::loadSettings() {
int keyindex;

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "KB_Shortcuts" );
	
	// Center key
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_Center", 1 ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxCenterKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxCenterKey->setCurrentIndex( 0 );
	}
	ui.chkCenterShift->setChecked (iniFile.value ( "Shift_Center", 0 ).toBool());
	ui.chkCenterCtrl->setChecked (iniFile.value ( "Ctrl_Center", 0 ).toBool());
	ui.chkCenterAlt->setChecked (iniFile.value ( "Alt_Center", 0 ).toBool());

	// Start/stop key
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_StartStop", 1 ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxStartStopKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxStartStopKey->setCurrentIndex( 0 );
	}
	ui.chkStartStopShift->setChecked (iniFile.value ( "Shift_StartStop", 0 ).toBool());
	ui.chkStartStopCtrl->setChecked (iniFile.value ( "Ctrl_StartStop", 0 ).toBool());
	ui.chkStartStopAlt->setChecked (iniFile.value ( "Alt_StartStop", 0 ).toBool());
	ui.radioSetZero->setChecked (iniFile.value ( "SetZero", 1 ).toBool());
	ui.radioSetFreeze->setChecked(!ui.radioSetZero->isChecked());
	ui.radioSetEngineStop->setChecked (iniFile.value ( "SetEngineStop", 1 ).toBool());
	ui.radioSetKeepTracking->setChecked(!ui.radioSetEngineStop->isChecked());

	// Axis-inhibitor key
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_Inhibit", 1 ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxInhibitKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxInhibitKey->setCurrentIndex( 0 );
	}
	ui.chkInhibitShift->setChecked (iniFile.value ( "Shift_Inhibit", 0 ).toBool());
	ui.chkInhibitCtrl->setChecked (iniFile.value ( "Ctrl_Inhibit", 0 ).toBool());
	ui.chkInhibitAlt->setChecked (iniFile.value ( "Alt_Inhibit", 0 ).toBool());

	ui.chkInhibitPitch->setChecked (iniFile.value ( "Inhibit_Pitch", 0 ).toBool());
	ui.chkInhibitYaw->setChecked (iniFile.value ( "Inhibit_Yaw", 0 ).toBool());
	ui.chkInhibitRoll->setChecked (iniFile.value ( "Inhibit_Roll", 0 ).toBool());
	ui.chkInhibitX->setChecked (iniFile.value ( "Inhibit_X", 0 ).toBool());
	ui.chkInhibitY->setChecked (iniFile.value ( "Inhibit_Y", 0 ).toBool());
	ui.chkInhibitZ->setChecked (iniFile.value ( "Inhibit_Z", 0 ).toBool());

	iniFile.endGroup ();

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void KeyboardShortcutDialog::save() {

	qDebug() << "save() says: started";

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	iniFile.beginGroup ( "KB_Shortcuts" );
	iniFile.setValue ( "Keycode_Center", keyList.at( ui.cbxCenterKey->currentIndex() ) );
	iniFile.setValue ( "Shift_Center", ui.chkCenterShift->isChecked() );
	iniFile.setValue ( "Ctrl_Center", ui.chkCenterCtrl->isChecked() );
	iniFile.setValue ( "Alt_Center", ui.chkCenterAlt->isChecked() );

	iniFile.setValue ( "Keycode_StartStop", keyList.at( ui.cbxStartStopKey->currentIndex() ) );
	iniFile.setValue ( "Shift_StartStop", ui.chkStartStopShift->isChecked() );
	iniFile.setValue ( "Ctrl_StartStop", ui.chkStartStopCtrl->isChecked() );
	iniFile.setValue ( "Alt_StartStop", ui.chkStartStopAlt->isChecked() );
	iniFile.setValue ( "SetZero", ui.radioSetZero->isChecked() );
	iniFile.setValue ( "SetEngineStop", ui.radioSetEngineStop->isChecked() );

	iniFile.setValue ( "Keycode_Inhibit", keyList.at( ui.cbxInhibitKey->currentIndex() ) );
	iniFile.setValue ( "Shift_Inhibit", ui.chkInhibitShift->isChecked() );
	iniFile.setValue ( "Ctrl_Inhibit", ui.chkInhibitCtrl->isChecked() );
	iniFile.setValue ( "Alt_Inhibit", ui.chkInhibitAlt->isChecked() );

	iniFile.setValue ( "Inhibit_Pitch", ui.chkInhibitPitch->isChecked() );
	iniFile.setValue ( "Inhibit_Yaw", ui.chkInhibitYaw->isChecked() );
	iniFile.setValue ( "Inhibit_Roll", ui.chkInhibitRoll->isChecked() );
	iniFile.setValue ( "Inhibit_X", ui.chkInhibitX->isChecked() );
	iniFile.setValue ( "Inhibit_Y", ui.chkInhibitY->isChecked() );
	iniFile.setValue ( "Inhibit_Z", ui.chkInhibitZ->isChecked() );

	iniFile.endGroup ();

	settingsDirty = false;

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();
}

//**************************************************************************************************//
//**************************************************************************************************//
//
// Constructor for Curve-configuration-dialog
//
CurveConfigurationDialog::CurveConfigurationDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(120, 30);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	connect(ui.curveYaw, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));
	connect(ui.curvePitch, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));
	connect(ui.curveRoll, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));
	connect(ui.curveX, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));
	connect(ui.curveY, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));
	connect(ui.curveZ, SIGNAL(BezierCurveChanged(bool)), this, SLOT(curveChanged(bool)));

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
CurveConfigurationDialog::~CurveConfigurationDialog() {
	qDebug() << "~CurveConfigurationDialog() says: started";
}

//
// OK clicked on server-dialog
//
void CurveConfigurationDialog::doOK() {
	save();
	this->close();
}

// override show event
void CurveConfigurationDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void CurveConfigurationDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void CurveConfigurationDialog::loadSettings() {
int NeutralZone;
int sensYaw, sensPitch, sensRoll;
int sensX, sensY, sensZ;
QPointF point1, point2, point3, point4;

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "Tracking" );
	NeutralZone = iniFile.value ( "NeutralZone", 5 ).toInt();
	sensYaw = iniFile.value ( "sensYaw", 100 ).toInt();
	sensPitch = iniFile.value ( "sensPitch", 100 ).toInt();
	sensRoll = iniFile.value ( "sensRoll", 100 ).toInt();
	sensX = iniFile.value ( "sensX", 100 ).toInt();
	sensY = iniFile.value ( "sensY", 100 ).toInt();
	sensZ = iniFile.value ( "sensZ", 100 ).toInt();

	iniFile.endGroup ();

	iniFile.beginGroup ( "Curves" );
	getCurvePoints( &iniFile, "Yaw_", &point1, &point2, &point3, &point4, NeutralZone, sensYaw, 50, 180 );
	ui.curveYaw->setPointOne( point1 );
	ui.curveYaw->setPointTwo( point2 );
	ui.curveYaw->setPointThree( point3 );
	ui.curveYaw->setPointFour( point4 );

	getCurvePoints( &iniFile, "Pitch_", &point1, &point2, &point3, &point4, NeutralZone, sensPitch, 50, 180 );
	ui.curvePitch->setPointOne( point1 );
	ui.curvePitch->setPointTwo( point2 );
	ui.curvePitch->setPointThree( point3 );
	ui.curvePitch->setPointFour( point4 );

	getCurvePoints( &iniFile, "Roll_", &point1, &point2, &point3, &point4, NeutralZone, sensRoll, 50, 180 );
	ui.curveRoll->setPointOne( point1 );
	ui.curveRoll->setPointTwo( point2 );
	ui.curveRoll->setPointThree( point3 );
	ui.curveRoll->setPointFour( point4 );

	getCurvePoints( &iniFile, "X_", &point1, &point2, &point3, &point4, NeutralZone, sensX, 50, 180 );
	ui.curveX->setPointOne( point1 );
	ui.curveX->setPointTwo( point2 );
	ui.curveX->setPointThree( point3 );
	ui.curveX->setPointFour( point4 );

	getCurvePoints( &iniFile, "Y_", &point1, &point2, &point3, &point4, NeutralZone, sensY, 50, 180 );
	ui.curveY->setPointOne( point1 );
	ui.curveY->setPointTwo( point2 );
	ui.curveY->setPointThree( point3 );
	ui.curveY->setPointFour( point4 );

	getCurvePoints( &iniFile, "Z_", &point1, &point2, &point3, &point4, NeutralZone, sensZ, 50, 180 );
	ui.curveZ->setPointOne( point1 );
	ui.curveZ->setPointTwo( point2 );
	ui.curveZ->setPointThree( point3 );
	ui.curveZ->setPointFour( point4 );
	iniFile.endGroup ();

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void CurveConfigurationDialog::save() {

	qDebug() << "save() says: started";

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	iniFile.beginGroup ( "Curves" );
	iniFile.setValue ("Yaw_point1", ui.curveYaw->getPointOne() );
	iniFile.setValue ("Yaw_point2", ui.curveYaw->getPointTwo() );
	iniFile.setValue ("Yaw_point3", ui.curveYaw->getPointThree() );
	iniFile.setValue ("Yaw_point4", ui.curveYaw->getPointFour() );

	iniFile.setValue ("Pitch_point1", ui.curvePitch->getPointOne() );
	iniFile.setValue ("Pitch_point2", ui.curvePitch->getPointTwo() );
	iniFile.setValue ("Pitch_point3", ui.curvePitch->getPointThree() );
	iniFile.setValue ("Pitch_point4", ui.curvePitch->getPointFour() );

	iniFile.setValue ("Roll_point1", ui.curveRoll->getPointOne() );
	iniFile.setValue ("Roll_point2", ui.curveRoll->getPointTwo() );
	iniFile.setValue ("Roll_point3", ui.curveRoll->getPointThree() );
	iniFile.setValue ("Roll_point4", ui.curveRoll->getPointFour() );
	
	iniFile.setValue ("X_point1", ui.curveX->getPointOne() );
	iniFile.setValue ("X_point2", ui.curveX->getPointTwo() );
	iniFile.setValue ("X_point3", ui.curveX->getPointThree() );
	iniFile.setValue ("X_point4", ui.curveX->getPointFour() );

	iniFile.setValue ("Y_point1", ui.curveY->getPointOne() );
	iniFile.setValue ("Y_point2", ui.curveY->getPointTwo() );
	iniFile.setValue ("Y_point3", ui.curveY->getPointThree() );
	iniFile.setValue ("Y_point4", ui.curveY->getPointFour() );

	iniFile.setValue ("Z_point1", ui.curveZ->getPointOne() );
	iniFile.setValue ("Z_point2", ui.curveZ->getPointTwo() );
	iniFile.setValue ("Z_point3", ui.curveZ->getPointThree() );
	iniFile.setValue ("Z_point4", ui.curveZ->getPointFour() );
	
	iniFile.endGroup ();
	settingsDirty = false;

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();
}

void getCurvePoints(QSettings *iniFile, QString prefix, QPointF *point1, QPointF *point2, QPointF *point3, QPointF *point4, int NeutralZone, int Sensitivity, int MaxInput, int MaxOutput) {
bool setMax;
float newMax;

	setMax = FALSE;
	newMax = MaxInput;

	//
	// If Point 1 exists, read it from the file.
	// If not: get the y-coord from the global (deprecated) NeutralZone setting.
	//
	if (iniFile->contains(prefix + "point1")) {
		*point1 = iniFile->value ( prefix + "point1", 0 ).toPoint();
	}
	else {
		point1->setY(NeutralZone);
	}

	//
	// If Point 4 exists, read it from the file.
	// If not: derive the x-coord from the (deprecated) 'Sensitivity' setting and set y to max.
	//
	if (iniFile->contains(prefix + "point4")) {
		*point4 = iniFile->value ( prefix + "point4", 0 ).toPoint();
	}
	else {
		point4->setY(MaxInput);									// Max. Input for rotations
		point4->setX((Sensitivity/100.0f) * MaxInput);
		if (point4->x() > MaxOutput) {
			point4->setX(MaxOutput);
			setMax = TRUE;
		}
		else {
			newMax = (Sensitivity/100.0f) * MaxInput;
		}
	}

	//
	// If Point 2 exists, read it from the file.
	// If not: derive it from the (deprecated) 'Sensitivity' setting.
	//
	if (iniFile->contains(prefix + "point2")) {
		*point2 = iniFile->value ( prefix + "point2", 0 ).toPoint();
	}
	else {
		point2->setY(0.333f * MaxInput);						// Set the Point at 1/3 of Max. Input
		if (!setMax) {
			point2->setX(0.333f * newMax);
		}
		else {
			point2->setX(0.333f * MaxOutput);
		}
	}

	//
	// If Point 3 exists, read it from the file.
	// If not: derive it from the (deprecated) 'Sensitivity' setting.
	//
	if (iniFile->contains(prefix + "point3")) {
		*point3 = iniFile->value ( prefix + "point3", 0 ).toPoint();
	}
	else {
		point3->setY(0.666f * MaxInput);						// Set the Point at 2/3 of Max. Input
		if (!setMax) {
			point3->setX(0.666f * newMax);
		}
		else {
			point3->setX(0.666f * MaxOutput);
		}
	}
}