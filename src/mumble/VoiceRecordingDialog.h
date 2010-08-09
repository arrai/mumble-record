#ifndef VOICERECORDINGDIALOG_H
#define VOICERECORDINGDIALOG_H

#include "ui_VoiceRecordingDialog.h"

class VoiceRecordingDialog : public QDialog, private Ui::VoiceRecordingDialog {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(VoiceRecordingDialog)

		QTimer *qtTimer;
	public:
		explicit VoiceRecordingDialog(QWidget *p);
		~VoiceRecordingDialog();

		void closeEvent(QCloseEvent *evt);
	public slots:
		void on_qpbStart_clicked();
		void on_qpbStop_clicked();
		void on_qtTimer_timeout();
		void on_qtbTargetDirectoryBrowse_triggered(QAction*);
		void reset();
};

#endif // VOICERECORDINGDIALOG_H
