#include "VoiceRecordingDialog.h"
#include "AudioOutput.h"
#include "Global.h"
#include "VoiceRecorder.h"
#include "ServerHandler.h"

VoiceRecordingDialog::VoiceRecordingDialog(QWidget *p = NULL) : QDialog(p), qtTimer(new QTimer(this)) {
	qtTimer->setObjectName(QLatin1String("qtTimer"));
	qtTimer->setInterval(200);
	setupUi(this);

	qleTargetDirectory->setText(g.s.qsRecordingPath);
	qleFilename->setText(g.s.qsRecordingFile);
	qrbMixdown->setChecked(g.s.rmRecordingMode == Settings::RecordingMixdown);

	// Populate available codecs
	Q_ASSERT(VoiceRecorderFormat::formatEnumEnd != 0);
	for (int fm = 0; fm < VoiceRecorderFormat::formatEnumEnd; fm++) {
		qcbFormat->addItem(VoiceRecorder::getFormatDescription(static_cast<VoiceRecorderFormat::Format>(fm)));
	}

	if (g.s.iRecordingFormat < 0 || g.s.iRecordingFormat > VoiceRecorderFormat::formatEnumEnd)
		g.s.iRecordingFormat = 0;

	qcbFormat->setCurrentIndex(g.s.iRecordingFormat);
}

VoiceRecordingDialog::~VoiceRecordingDialog() {
	reset();
}

void VoiceRecordingDialog::closeEvent(QCloseEvent *evt) {
	if (g.sh) {
		VoiceRecorderPtr recorder(g.sh->recorder);
		if (recorder && recorder->isRunning()) {
			int ret = QMessageBox::warning(this,
								 tr("Abort recording?"),
								 tr("Closing the recorder will stop your current recording. Do you really want to close the recorder?"),
								 QMessageBox::Yes | QMessageBox::No,
								 QMessageBox::No);

			if (ret == QMessageBox::No) {
				evt->ignore();
				return;
			}
		}
	}

	g.s.qsRecordingPath = qleTargetDirectory->text();
	g.s.qsRecordingFile = qleFilename->text();
	if (qrbMixdown->isChecked())
		g.s.rmRecordingMode = Settings::RecordingMixdown;
	else
		g.s.rmRecordingMode = Settings::RecordingMultichannel;

	int i = qcbFormat->currentIndex();
	g.s.iRecordingFormat = (i == -1) ? 0 : i;

	reset();
	evt->accept();
}

void VoiceRecordingDialog::on_qpbStart_clicked() {
	if (!g.sh) {
		QMessageBox::critical(this,
							  tr("Unable to start recording"),
							  tr("Not connected to server"));
		reset();
		return;
	}

	if (g.sh->recorder) {
		QMessageBox::information(this,
								 tr("Unable to start recording"),
								 tr("Already attached a recorder to this server"));
		return;
	}

	// Check validity of input
	int ifm = qcbFormat->currentIndex();
	if (ifm == -1) {
		QMessageBox::critical(this,
					tr("Unable to start recording"),
					tr("No format selected."));
		return;
	}

	QString dstr = qleTargetDirectory->text();
	if (dstr.isEmpty()) {
		on_qtbTargetDirectoryBrowse_triggered(NULL);
		dstr = qleTargetDirectory->text();
		if (dstr.isEmpty())
			return;
	}

	QDir dir(dstr);
	if (!dir.exists()) {
		int ret = QMessageBox::question(this,
							  tr("Directory does not exist"),
							  tr("The directory '%1' does not exist, should Mumble try to create it?").arg(dir.absolutePath()),
							  QMessageBox::Yes | QMessageBox::Abort,
							  QMessageBox::Yes);

		if (ret == QMessageBox::Abort)
			return;

		if(!dir.mkpath(dir.absolutePath())) {
			QMessageBox::critical(this,
								  tr("Unable to start recording"),
								  tr("Target directory '%1' could not be created. Please check that this path is valid and accessible to Mumble.").arg(dir.absolutePath()));
			return;
		}
	}

	QFileInfo fi(qleFilename->text());
	QString basename(fi.baseName());
	QString suffix(fi.completeSuffix());
	if (suffix.isEmpty())
		suffix = VoiceRecorder::getFormatDefaultExtension(static_cast<VoiceRecorderFormat::Format>(ifm));


	if (basename.indexOf(QLatin1String("%1")) == -1) {
		basename += QLatin1String("%1");
	}

	qleFilename->setText(basename + QLatin1Char('.') + suffix);

	AudioOutputPtr ao(g.ao);
	if (!ao)
		return;

	g.sh->announceRecordingState(true);

	g.sh->recorder.reset(new VoiceRecorder(this));
	VoiceRecorderPtr recorder(g.sh->recorder);

	// Configure it
	recorder->setSampleRate(ao->getMixerFreq());
	recorder->setFileName(dir.absoluteFilePath(qleFilename->text()));
	recorder->setMixDown(qrbMixdown->isChecked());
	recorder->setFormat(static_cast<VoiceRecorderFormat::Format>(ifm));

	recorder->start();
	qtTimer->start();

	qpbStart->setDisabled(true);
	qpbStop->setEnabled(true);
	qgbMode->setDisabled(true);
	qgbOutput->setDisabled(true);
}

void VoiceRecordingDialog::on_qpbStop_clicked() {
	if (!g.sh) {
		reset();
		return;
	}

	VoiceRecorderPtr recorder(g.sh->recorder);
	if (!recorder) {
		reset();
		return;
	}

	qtTimer->stop();
	recorder->stop();
	g.sh->recorder.reset();
	g.sh->announceRecordingState(false);
}

void VoiceRecordingDialog::on_qtTimer_timeout() {
	if (!g.sh) {
		reset();
		return;
	}

	VoiceRecorderPtr recorder(g.sh->recorder);
	if (!g.sh->recorder) {
		reset();
		return;
	}
	QTime t, n;
	quint64 samples = recorder->getRecordedSamples();

	if (samples != 0)
		n = t.addSecs(recorder->getRecordedSamples() / recorder->getSampleRate());

	qlTime->setText(n.toString(QLatin1String("hh:mm:ss")));
}

void VoiceRecordingDialog::on_qtbTargetDirectoryBrowse_triggered(QAction*) {
	QString dir = QFileDialog::getExistingDirectory(this,
													tr("Select target directory"),
													QString(),
													QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!dir.isEmpty())
		qleTargetDirectory->setText(dir);
}

void VoiceRecordingDialog::reset() {
	qtTimer->stop();

	if (g.sh) {
		VoiceRecorderPtr recorder(g.sh->recorder);
		if (recorder) {
			recorder->stop();
			g.sh->recorder.reset();
			g.sh->announceRecordingState(false);
		}
	}

	qpbStart->setEnabled(true);
	qpbStop->setDisabled(true);

	qgbMode->setEnabled(true);
	qgbOutput->setEnabled(true);

	qlTime->setText(QLatin1String("00:00:00"));
}
