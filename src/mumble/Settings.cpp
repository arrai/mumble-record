/* Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2009, Stefan Hacker <dd0t@users.sourceforge.net>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Settings.h"
#include "Log.h"
#include "Global.h"
#include "AudioInput.h"
#include "Cert.h"
#include "../../overlay/overlay.h"

bool Shortcut::isServerSpecific() const {
	if (qvData.canConvert<ShortcutTarget>()) {
		const ShortcutTarget &sc = qvariant_cast<ShortcutTarget>(qvData);
		return sc.isServerSpecific();
	}
	return false;
}

bool Shortcut::operator <(const Shortcut &other) const {
	return (iIndex < other.iIndex);
}

bool Shortcut::operator ==(const Shortcut &other) const {
	return (iIndex == other.iIndex) && (qlButtons == other.qlButtons) && (qvData == other.qvData) && (bSuppress == other.bSuppress);
}

ShortcutTarget::ShortcutTarget() {
	bUsers = true;
	iChannel = -3;
	bLinks = bChildren = bForceCenter = false;
}

bool ShortcutTarget::isServerSpecific() const {
	return (! bUsers && (iChannel >= 0));
}

bool ShortcutTarget::operator ==(const ShortcutTarget &o) const {
	if ((bUsers != o.bUsers) || (bForceCenter != o.bForceCenter))
		return false;
	if (bUsers)
		return (qlUsers == o.qlUsers) && (qlSessions == o.qlSessions);
	else
		return (iChannel == o.iChannel) && (bLinks == o.bLinks) && (bChildren == o.bChildren) && (qsGroup == o.qsGroup);
}

quint32 qHash(const ShortcutTarget &t) {
	quint32 h = t.bForceCenter ? 0x55555555 : 0xaaaaaaaa;
	if (t.bUsers) {
		foreach(unsigned int u, t.qlSessions)
			h ^= u;
	} else {
		h ^= t.iChannel;
		if (t.bLinks)
			h ^= 0x80000000;
		if (t.bChildren)
			h ^= 0x40000000;
		h ^= qHash(t.qsGroup);
		h = ~h;
	}
	return h;
}

quint32 qHash(const QList<ShortcutTarget> &l) {
	quint32 h = l.count();
	foreach(const ShortcutTarget &st, l)
		h ^= qHash(st);
	return h;
}

QDataStream &operator<<(QDataStream &qds, const ShortcutTarget &st) {
	qds << st.bUsers << st.bForceCenter;

	if (st.bUsers)
		return qds << st.qlUsers;
	else
		return qds << st.iChannel << st.qsGroup << st.bLinks << st.bChildren;
}

QDataStream &operator>>(QDataStream &qds, ShortcutTarget &st) {
	qds >> st.bUsers >> st.bForceCenter;
	if (st.bUsers)
		return qds >> st.qlUsers;
	else
		return qds >> st.iChannel >> st.qsGroup >> st.bLinks >> st.bChildren;
}

const QString Settings::cqsDefaultPushClickOn = QLatin1String(":/on.ogg");
const QString Settings::cqsDefaultPushClickOff = QLatin1String(":/off.ogg");

OverlaySettings::OverlaySettings() {
	osShow = LinkedChannels;
	bAlwaysSelf = true;

	fX = 0.8f;
	fY = 0.0f;
	fZoom = 1.f;
	uiColumns = 2;

	qcUserName[Settings::Passive] = QColor(128, 128, 128);
	qcUserName[Settings::Talking] = QColor(255, 255, 255);
	qcUserName[Settings::Whispering] = QColor(128, 255, 128);
	qcUserName[Settings::Shouting] = QColor(255, 128, 255);
	qcChannel = QColor(192,192,255,192);

	fUserName = 0.75f;
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
	qfUserName = QFont(QLatin1String("Verdana"), 20);
#else
	qfUserName = QFont(QLatin1String("Arial"), 20);
#endif

	qcChannel = QColor(255, 255, 128);
	fChannel = 0.75f;
	qfChannel = qfUserName;

	qcFps = Qt::white;
	fFps = 0.75f;
	qfFps = qfUserName;

	fMutedDeafened = 0.5f;
	fAvatar = 1.0f;

	qcBoxPen = QColor(0, 0, 0, 224);
	qcBoxFill = QColor(128, 128, 128, 128);
	fBoxPenWidth = (1.f / 256.0f);
	fBoxPad = (1.f / 256.0f);

	fUser[Settings::Passive] = 0.5f;
	fUser[Settings::Talking] = (7.0f / 8.0f);
	fUser[Settings::Whispering] = (7.0f / 8.0f);
	fUser[Settings::Shouting] = (7.0f / 8.0f);

	// Nice and exact float values.
	qrfUserName = QRectF(-0.0625f, 0.101563f - 0.0625f, 0.125f, 0.023438f);
	qrfChannel = QRectF(-0.03125f, -0.0625f, 0.09375f, 0.015625f);
	qrfMutedDeafened = QRectF(-0.0625f, -0.0625f, 0.0625f, 0.0625f);
	qrfAvatar = QRectF(-0.0625f, -0.0625f, 0.125f, 0.125f);
	qrfFps = QRectF(10, 10, -1, 0.023438f);

	bUserName = true;
	bChannel = true;
	bMutedDeafened = true;
	bAvatar = true;
	bBox = false;
	bFps = false;

	qaUserName = Qt::AlignCenter;
	qaMutedDeafened = Qt::AlignLeft | Qt::AlignTop;
	qaAvatar = Qt::AlignCenter;
	qaChannel = Qt::AlignCenter;

	bUseWhitelist = false;

#ifdef Q_OS_WIN
	int i = 0;
	while (overlayBlacklist[i]) {
		qslBlacklist << QLatin1String(overlayBlacklist[i]);
		i++;
	}
#endif

}

Settings::Settings() {
	qRegisterMetaType<ShortcutTarget>("ShortcutTarget");
	qRegisterMetaTypeStreamOperators<ShortcutTarget>("ShortcutTarget");
	qRegisterMetaType<QVariant>("QVariant");

	atTransmit = VAD;
	bTransmitPosition = false;
	bMute = bDeaf = false;
	bTTS = true;
	iTTSVolume = 75;
	iTTSThreshold = 250;
	iQuality = 40000;
	fVolume = 1.0f;
	fOtherVolume = 0.5f;
	bAttenuateOthersOnTalk = false;
	bAttenuateOthers = true;
	iMinLoudness = 1000;
	iVoiceHold = 50;
	iJitterBufferSize = 1;
	iFramesPerPacket = 2;
	iNoiseSuppress = -30;
	iIdleTime = 0;
	vsVAD = SignalToNoise;
	fVADmin = 0.80f;
	fVADmax = 0.98f;

	bPushClick = false;
	qsPushClickOn = cqsDefaultPushClickOn;
	qsPushClickOff = cqsDefaultPushClickOff;

	bUserTop = false;

	bWhisperFriends = false;

	uiDoublePush = 0;
	bExpert = false;

#ifdef NO_UPDATE_CHECK
	bUpdateCheck = false;
	bPluginCheck = false;
#else
	bUpdateCheck = true;
	bPluginCheck = true;
#endif

	qsImagePath = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);

	bFirstTime = true;
	ceExpand = ChannelsWithUsers;
	ceChannelDrag = Ask;
	bMinimalView = false;
	bHideFrame = false;
	aotbAlwaysOnTop = OnTopNever;
	bAskOnQuit = true;
#ifdef Q_OS_WIN
	// Don't enable minimize to tray by default on win7
	bHideInTray = (QSysInfo::windowsVersion() != QSysInfo::WV_6_1);
#else
	bHideInTray = true;
#endif
	bStateInTray = true;
	bUsage = true;
	bShowUserCount = false;
	wlWindowLayout = LayoutClassic;
	bShowContextMenuInMenuBar = false;

	ssFilter = ShowReachable;

	iOutputDelay = 5;

	qsALSAInput=QLatin1String("default");
	qsALSAOutput=QLatin1String("default");

	bEcho = false;
	bEchoMulti = true;

	bExclusiveInput = true;
	bExclusiveOutput = false;

	iPortAudioInput = -1; // default device
	iPortAudioOutput = -1; // default device

	bPositionalAudio = true;
	bPositionalHeadphone = false;
	fAudioMinDistance = 1.0f;
	fAudioMaxDistance = 15.0f;
	fAudioMaxDistVolume = 0.80f;
	fAudioBloom = 0.5f;

	bOverlayEnable = true;

	iLCDUserViewMinColWidth = 50;
	iLCDUserViewSplitterWidth = 2;

	// Network settings
	bTCPCompat = false;
	bQoS = true;
	bReconnect = true;
	bAutoConnect = false;
	ptProxyType = NoProxy;
	usProxyPort = 0;

	iMaxImageSize = ciDefaultMaxImageSize;
	iMaxImageWidth = 1024; // Allow 1024x1024 resolution
	iMaxImageHeight = 1024;
	bSuppressIdentity = false;

	// Accessibility
	bHighContrast = false;

	// Recording
	qsRecordingPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
	qsRecordingFile = QLatin1String("MumbleRecording-%1");
	rmRecordingMode = RecordingMixdown;
	iRecordingFormat = 0;

#if defined(AUDIO_TEST)
	lmLoopMode = Server;
#else
	lmLoopMode = None;
#endif
	dPacketLoss = 0;
	dMaxPacketDelay = 0.0f;

	for (int i=Log::firstMsgType;i<=Log::lastMsgType;++i)
		qmMessages.insert(i, Settings::LogConsole | Settings::LogBalloon | Settings::LogTTS);

	for (int i=Log::firstMsgType;i<=Log::lastMsgType;++i)
		qmMessageSounds.insert(i, QString());

	qmMessageSounds[Log::CriticalError] = QLatin1String(":/Critical.ogg");
	qmMessageSounds[Log::PermissionDenied] = QLatin1String(":/PermissionDenied.ogg");
	qmMessageSounds[Log::SelfMute] = QLatin1String(":/SelfMutedDeafened.ogg");
	qmMessageSounds[Log::ServerConnected] = QLatin1String(":/ServerConnected.ogg");
	qmMessageSounds[Log::ServerDisconnected] = QLatin1String(":/ServerDisconnected.ogg");
	qmMessageSounds[Log::TextMessage] = QLatin1String(":/TextMessage.ogg");
	qmMessageSounds[Log::ChannelJoin] = QLatin1String(":/UserJoinedChannel.ogg");
	qmMessageSounds[Log::ChannelLeave] = QLatin1String(":/UserLeftChannel.ogg");
	qmMessageSounds[Log::YouMutedOther] = QLatin1String(":/UserMutedYouOrByYou.ogg");
	qmMessageSounds[Log::YouMuted] = QLatin1String(":/UserMutedYouOrByYou.ogg");
	qmMessageSounds[Log::YouKicked] = QLatin1String(":/UserKickedYouOrByYou.ogg");
	qmMessageSounds[Log::Recording] = QLatin1String(":/Recording.ogg");

	qmMessages[Log::DebugInfo] = Settings::LogConsole;
	qmMessages[Log::Warning] = Settings::LogConsole | Settings::LogBalloon;
	qmMessages[Log::Information] = Settings::LogConsole;
	qmMessages[Log::UserJoin] = Settings::LogConsole;
	qmMessages[Log::UserLeave] = Settings::LogConsole;
	qmMessages[Log::UserKicked] = Settings::LogConsole;
	qmMessages[Log::OtherSelfMute] = Settings::LogConsole;
	qmMessages[Log::OtherMutedOther] = Settings::LogConsole;

    // Unnoticed recording has a chance to violate privacy. Make it as obvious as possible.
	qmMessages[Log::Recording] = Settings::LogConsole | Settings::LogSoundfile | Settings::LogBalloon | Settings::LogTTS;
}

bool Settings::doEcho() const {
	if (! bEcho)
		return false;

	if (AudioInputRegistrar::qmNew) {
		AudioInputRegistrar *air = AudioInputRegistrar::qmNew->value(qsAudioInput);
		if (air) {
			if (air->canEcho(qsAudioOutput))
				return true;
		}
	}
	return false;
}

bool Settings::doPositionalAudio() const {
	return bPositionalAudio;
}

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()


BOOST_TYPEOF_REGISTER_TYPE(Qt::Alignment)
BOOST_TYPEOF_REGISTER_TYPE(Settings::AudioTransmit)
BOOST_TYPEOF_REGISTER_TYPE(Settings::VADSource)
BOOST_TYPEOF_REGISTER_TYPE(Settings::LoopMode)
BOOST_TYPEOF_REGISTER_TYPE(Settings::OverlayShow)
BOOST_TYPEOF_REGISTER_TYPE(Settings::ProxyType)
BOOST_TYPEOF_REGISTER_TYPE(Settings::ChannelExpand)
BOOST_TYPEOF_REGISTER_TYPE(Settings::ChannelDrag)
BOOST_TYPEOF_REGISTER_TYPE(Settings::ServerShow)
BOOST_TYPEOF_REGISTER_TYPE(Settings::WindowLayout)
BOOST_TYPEOF_REGISTER_TYPE(Settings::AlwaysOnTopBehaviour)
BOOST_TYPEOF_REGISTER_TYPE(Settings::RecordingMode)
BOOST_TYPEOF_REGISTER_TYPE(QString)
BOOST_TYPEOF_REGISTER_TYPE(QByteArray)
BOOST_TYPEOF_REGISTER_TYPE(QColor)
BOOST_TYPEOF_REGISTER_TYPE(QVariant)
BOOST_TYPEOF_REGISTER_TYPE(QFont)
BOOST_TYPEOF_REGISTER_TEMPLATE(QList, 1)

#define SAVELOAD(var,name) var = qvariant_cast<BOOST_TYPEOF(var)>(g.qs->value(QLatin1String(name), var))
#define LOADENUM(var, name) var = static_cast<BOOST_TYPEOF(var)>(g.qs->value(QLatin1String(name), var).toInt())
#define LOADFLAG(var, name) var = static_cast<BOOST_TYPEOF(var)>(g.qs->value(QLatin1String(name), static_cast<int>(var)).toInt())

void OverlaySettings::load() {
	LOADENUM(osShow, "show");
	SAVELOAD(bAlwaysSelf, "alwaysself");

	SAVELOAD(fX, "x");
	SAVELOAD(fY, "y");
	SAVELOAD(fZoom, "zoom");
	SAVELOAD(uiColumns, "columns");

	g.qs->beginReadArray(QLatin1String("states"));
	for (int i=0;i<4;++i) {
		g.qs->setArrayIndex(i);
		SAVELOAD(qcUserName[i], "color");
		SAVELOAD(fUser[i], "opacity");
	}
	g.qs->endArray();

	SAVELOAD(qfUserName, "userfont");
	SAVELOAD(qfChannel, "channelfont");
	SAVELOAD(qcChannel, "channelcolor");
	SAVELOAD(qfFps, "fpsfont");
	SAVELOAD(qcFps, "fpscolor");

	SAVELOAD(fBoxPad, "padding");
	SAVELOAD(fBoxPenWidth, "penwidth");
	SAVELOAD(qcBoxPen, "pencolor");
	SAVELOAD(qcBoxFill, "fillcolor");

	SAVELOAD(bUserName, "usershow");
	SAVELOAD(bChannel, "channelshow");
	SAVELOAD(bMutedDeafened, "mutedshow");
	SAVELOAD(bAvatar, "avatarshow");
	SAVELOAD(bBox, "boxshow");
	SAVELOAD(bFps, "fpsshow");

	SAVELOAD(fUserName, "useropacity");
	SAVELOAD(fChannel, "channelopacity");
	SAVELOAD(fMutedDeafened, "mutedopacity");
	SAVELOAD(fAvatar, "avataropacity");
	SAVELOAD(fFps, "fpsopacity");

	SAVELOAD(qrfUserName, "userrect");
	SAVELOAD(qrfChannel, "channelrect");
	SAVELOAD(qrfMutedDeafened, "mutedrect");
	SAVELOAD(qrfAvatar, "avatarrect");
	SAVELOAD(qrfFps, "fpsrect");

	LOADFLAG(qaUserName, "useralign");
	LOADFLAG(qaChannel, "channelalign");
	LOADFLAG(qaMutedDeafened, "mutedalign");
	LOADFLAG(qaAvatar, "avataralign");

	SAVELOAD(bUseWhitelist, "usewhitelist");
	SAVELOAD(qslBlacklist, "blacklist");
	SAVELOAD(qslWhitelist, "whitelist");
}

void Settings::load() {
	SAVELOAD(bMute, "audio/mute");
	SAVELOAD(bDeaf, "audio/deaf");
	LOADENUM(atTransmit, "audio/transmit");
	SAVELOAD(uiDoublePush, "audio/doublepush");
	SAVELOAD(bPushClick, "audio/pushclick");
	SAVELOAD(qsPushClickOn, "audio/pushclickon");
	SAVELOAD(qsPushClickOff, "audio/pushclickoff");
	SAVELOAD(iQuality, "audio/quality");
	SAVELOAD(iMinLoudness, "audio/loudness");
	SAVELOAD(fVolume, "audio/volume");
	SAVELOAD(fOtherVolume, "audio/othervolume");
	SAVELOAD(bAttenuateOthers, "audio/attenuateothers");
	SAVELOAD(bAttenuateOthersOnTalk, "audio/attenuateothersontalk");
	LOADENUM(vsVAD, "audio/vadsource");
	SAVELOAD(fVADmin, "audio/vadmin");
	SAVELOAD(fVADmax, "audio/vadmax");
	SAVELOAD(iNoiseSuppress, "audio/noisesupress");
	SAVELOAD(iVoiceHold, "audio/voicehold");
	SAVELOAD(iOutputDelay, "audio/outputdelay");
	SAVELOAD(iIdleTime, "audio/idletime");
	SAVELOAD(fAudioMinDistance, "audio/mindistance");
	SAVELOAD(fAudioMaxDistance, "audio/maxdistance");
	SAVELOAD(fAudioMaxDistVolume, "audio/maxdistancevolume");
	SAVELOAD(fAudioBloom, "audio/bloom");
	SAVELOAD(bEcho, "audio/echo");
	SAVELOAD(bEchoMulti, "audio/echomulti");
	SAVELOAD(bExclusiveInput, "audio/exclusiveinput");
	SAVELOAD(bExclusiveOutput, "audio/exclusiveoutput");
	SAVELOAD(bPositionalAudio, "audio/positional");
	SAVELOAD(bPositionalHeadphone, "audio/headphone");
	SAVELOAD(qsAudioInput, "audio/input");
	SAVELOAD(qsAudioOutput, "audio/output");
	SAVELOAD(bWhisperFriends, "audio/whisperfriends");
	SAVELOAD(bTransmitPosition, "audio/postransmit");

	SAVELOAD(iJitterBufferSize, "net/jitterbuffer");
	SAVELOAD(iFramesPerPacket, "net/framesperpacket");

	SAVELOAD(qsASIOclass, "asio/class");
	SAVELOAD(qlASIOmic, "asio/mic");
	SAVELOAD(qlASIOspeaker, "asio/speaker");

	SAVELOAD(qsWASAPIInput, "wasapi/input");
	SAVELOAD(qsWASAPIOutput, "wasapi/output");

	SAVELOAD(qsALSAInput, "alsa/input");
	SAVELOAD(qsALSAOutput, "alsa/output");

	SAVELOAD(qsPulseAudioInput, "pulseaudio/input");
	SAVELOAD(qsPulseAudioOutput, "pulseaudio/output");

	SAVELOAD(qsOSSInput, "oss/input");
	SAVELOAD(qsOSSOutput, "oss/output");

	SAVELOAD(qsCoreAudioInput, "coreaudio/input");
	SAVELOAD(qsCoreAudioOutput, "coreaudio/output");

	SAVELOAD(iPortAudioInput, "portaudio/input");
	SAVELOAD(iPortAudioOutput, "portaudio/output");

	SAVELOAD(qbaDXInput, "directsound/input");
	SAVELOAD(qbaDXOutput, "directsound/output");

	SAVELOAD(bTTS, "tts/enable");
	SAVELOAD(iTTSVolume, "tts/volume");
	SAVELOAD(iTTSThreshold, "tts/threshold");

	SAVELOAD(bOverlayEnable, "overlay/enable");

	// Network settings
	SAVELOAD(bTCPCompat, "net/tcponly");
	SAVELOAD(bQoS, "net/qos");
	SAVELOAD(bReconnect, "net/reconnect");
	SAVELOAD(bAutoConnect, "net/autoconnect");
	SAVELOAD(bSuppressIdentity, "net/suppress");
	LOADENUM(ptProxyType, "net/proxytype");
	SAVELOAD(qsProxyHost, "net/proxyhost");
	SAVELOAD(usProxyPort, "net/proxyport");
	SAVELOAD(qsProxyUsername, "net/proxyusername");
	SAVELOAD(qsProxyPassword, "net/proxypassword");
	SAVELOAD(iMaxImageSize, "net/maximagesize");
	SAVELOAD(iMaxImageWidth, "net/maximagewidth");
	SAVELOAD(iMaxImageHeight, "net/maximageheight");

	SAVELOAD(bExpert, "ui/expert");
	SAVELOAD(qsLanguage, "ui/language");
	SAVELOAD(qsStyle, "ui/style");
	SAVELOAD(qsSkin, "ui/skin");
	LOADENUM(ceExpand, "ui/expand");
	LOADENUM(ceChannelDrag, "ui/drag");
	LOADENUM(aotbAlwaysOnTop, "ui/alwaysontop");
	SAVELOAD(bAskOnQuit, "ui/askonquit");
	SAVELOAD(bMinimalView, "ui/minimalview");
	SAVELOAD(bHideFrame, "ui/hideframe");
	SAVELOAD(bUserTop, "ui/usertop");
	SAVELOAD(bFirstTime, "ui/firsttime120");
	SAVELOAD(qbaMainWindowGeometry, "ui/geometry");
	SAVELOAD(qbaMainWindowState, "ui/state");
	SAVELOAD(qbaMinimalViewGeometry, "ui/minimalviewgeometry");
	SAVELOAD(qbaMinimalViewState, "ui/minimalviewstate");
	SAVELOAD(qbaConfigGeometry, "ui/ConfigGeometry");
	LOADENUM(wlWindowLayout, "ui/WindowLayout");
	SAVELOAD(qbaSplitterState, "ui/splitter");
	SAVELOAD(qbaHeaderState, "ui/header");
	SAVELOAD(qsUsername, "ui/username");
	SAVELOAD(qsLastServer, "ui/server");
	LOADENUM(ssFilter, "ui/serverfilter");
#ifndef NO_UPDATE_CHECK
	SAVELOAD(bUpdateCheck, "ui/updatecheck");
	SAVELOAD(bPluginCheck, "ui/plugincheck");
#endif
	SAVELOAD(bHideInTray, "ui/hidetray");
	SAVELOAD(bStateInTray, "ui/stateintray");
	SAVELOAD(bUsage, "ui/usage");
	SAVELOAD(bShowUserCount, "ui/showusercount");
	SAVELOAD(qsImagePath, "ui/imagepath");
	SAVELOAD(bShowContextMenuInMenuBar, "ui/showcontextmenuinmenubar");
	SAVELOAD(qbaConnectDialogGeometry, "ui/connect/geometry");
	SAVELOAD(qbaConnectDialogHeader, "ui/connect/header");
	SAVELOAD(bHighContrast, "ui/HighContrast");

	// Recording
	SAVELOAD(qsRecordingPath, "recording/path");
	SAVELOAD(qsRecordingFile, "recording/file");
	LOADENUM(rmRecordingMode, "recording/mode");
	SAVELOAD(iRecordingFormat, "recording/format");

	// LCD
	SAVELOAD(iLCDUserViewMinColWidth, "lcd/userview/mincolwidth");
	SAVELOAD(iLCDUserViewSplitterWidth, "lcd/userview/splitterwidth");

	QByteArray qba = qvariant_cast<QByteArray>(g.qs->value(QLatin1String("net/certificate")));
	if (! qba.isEmpty())
		kpCertificate = CertWizard::importCert(qba);

	int nshorts = g.qs->beginReadArray(QLatin1String("shortcuts"));
	for (int i=0;i<nshorts;i++) {
		g.qs->setArrayIndex(i);
		Shortcut s;

		s.iIndex = -2;

		SAVELOAD(s.iIndex, "index");
		SAVELOAD(s.qlButtons, "keys");
		SAVELOAD(s.bSuppress, "suppress");
		s.qvData = g.qs->value(QLatin1String("data"));
		if (s.iIndex >= -1)
			qlShortcuts << s;
	}
	g.qs->endArray();

	g.qs->beginReadArray(QLatin1String("messages"));
	for (QMap<int, quint32>::const_iterator it = qmMessages.constBegin(); it != qmMessages.constEnd(); ++it) {
		g.qs->setArrayIndex(it.key());
		SAVELOAD(qmMessages[it.key()], "log");
	}
	g.qs->endArray();

	g.qs->beginReadArray(QLatin1String("messagesounds"));
	for (QMap<int, QString>::const_iterator it = qmMessageSounds.constBegin(); it != qmMessageSounds.constEnd(); ++it) {
		g.qs->setArrayIndex(it.key());
		SAVELOAD(qmMessageSounds[it.key()], "logsound");
	}
	g.qs->endArray();

	g.qs->beginGroup(QLatin1String("lcd/devices"));
	foreach(const QString &d, g.qs->childKeys()) {
		qmLCDDevices.insert(d, g.qs->value(d, true).toBool());
	}
	g.qs->endGroup();

	g.qs->beginGroup(QLatin1String("audio/plugins"));
	foreach(const QString &d, g.qs->childKeys()) {
		qmPositionalAudioPlugins.insert(d, g.qs->value(d, true).toBool());
	}
	g.qs->endGroup();

	g.qs->beginGroup(QLatin1String("overlay"));
	os.load();
	g.qs->endGroup();
}

#undef SAVELOAD
#define SAVELOAD(var,name) if (var != def.var) g.qs->setValue(QLatin1String(name), var); else g.qs->remove(QLatin1String(name))
#define SAVEFLAG(var,name) if (var != def.var) g.qs->setValue(QLatin1String(name), static_cast<int>(var)); else g.qs->remove(QLatin1String(name))

void OverlaySettings::save() {
	OverlaySettings def;

	SAVELOAD(osShow, "show");
	SAVELOAD(bAlwaysSelf, "alwaysself");
	SAVELOAD(fX, "x");
	SAVELOAD(fY, "y");
	SAVELOAD(fZoom, "zoom");
	SAVELOAD(uiColumns, "columns");

	g.qs->beginReadArray(QLatin1String("states"));
	for (int i=0;i<4;++i) {
		g.qs->setArrayIndex(i);
		SAVELOAD(qcUserName[i], "color");
		SAVELOAD(fUser[i], "opacity");
	}
	g.qs->endArray();

	SAVELOAD(qfUserName, "userfont");
	SAVELOAD(qfChannel, "channelfont");
	SAVELOAD(qcChannel, "channelcolor");
	SAVELOAD(qfFps, "fpsfont");
	SAVELOAD(qcFps, "fpscolor");

	SAVELOAD(fBoxPad, "padding");
	SAVELOAD(fBoxPenWidth, "penwidth");
	SAVELOAD(qcBoxPen, "pencolor");
	SAVELOAD(qcBoxFill, "fillcolor");

	SAVELOAD(bUserName, "usershow");
	SAVELOAD(bChannel, "channelshow");
	SAVELOAD(bMutedDeafened, "mutedshow");
	SAVELOAD(bAvatar, "avatarshow");
	SAVELOAD(bBox, "boxshow");
	SAVELOAD(bFps, "fpsshow");

	SAVELOAD(fUserName, "useropacity");
	SAVELOAD(fChannel, "channelopacity");
	SAVELOAD(fMutedDeafened, "mutedopacity");
	SAVELOAD(fAvatar, "avataropacity");
	SAVELOAD(fFps, "fpsopacity");

	SAVELOAD(qrfUserName, "userrect");
	SAVELOAD(qrfChannel, "channelrect");
	SAVELOAD(qrfMutedDeafened, "mutedrect");
	SAVELOAD(qrfAvatar, "avatarrect");
	SAVELOAD(qrfFps, "fpsrect");

	SAVEFLAG(qaUserName, "useralign");
	SAVEFLAG(qaChannel, "channelalign");
	SAVEFLAG(qaMutedDeafened, "mutedalign");
	SAVEFLAG(qaAvatar, "avataralign");

	g.qs->setValue(QLatin1String("usewhitelist"), bUseWhitelist);
	g.qs->setValue(QLatin1String("blacklist"), qslBlacklist);
	g.qs->setValue(QLatin1String("whitelist"), qslWhitelist);
}

void Settings::save() {
	Settings def;

	SAVELOAD(bMute, "audio/mute");
	SAVELOAD(bDeaf, "audio/deaf");
	SAVELOAD(atTransmit, "audio/transmit");
	SAVELOAD(uiDoublePush, "audio/doublepush");
	SAVELOAD(bPushClick, "audio/pushclick");
	SAVELOAD(qsPushClickOn, "audio/pushclickon");
	SAVELOAD(qsPushClickOff, "audio/pushclickoff");
	SAVELOAD(iQuality, "audio/quality");
	SAVELOAD(iMinLoudness, "audio/loudness");
	SAVELOAD(fVolume, "audio/volume");
	SAVELOAD(fOtherVolume, "audio/othervolume");
	SAVELOAD(bAttenuateOthers, "audio/attenuateothers");
	SAVELOAD(bAttenuateOthersOnTalk, "audio/attenuateothersontalk");
	SAVELOAD(vsVAD, "audio/vadsource");
	SAVELOAD(fVADmin, "audio/vadmin");
	SAVELOAD(fVADmax, "audio/vadmax");
	SAVELOAD(iNoiseSuppress, "audio/noisesupress");
	SAVELOAD(iVoiceHold, "audio/voicehold");
	SAVELOAD(iOutputDelay, "audio/outputdelay");
	SAVELOAD(iIdleTime, "audio/idletime");
	SAVELOAD(fAudioMinDistance, "audio/mindistance");
	SAVELOAD(fAudioMaxDistance, "audio/maxdistance");
	SAVELOAD(fAudioMaxDistVolume, "audio/maxdistancevolume");
	SAVELOAD(fAudioBloom, "audio/bloom");
	SAVELOAD(bEcho, "audio/echo");
	SAVELOAD(bEchoMulti, "audio/echomulti");
	SAVELOAD(bExclusiveInput, "audio/exclusiveinput");
	SAVELOAD(bExclusiveOutput, "audio/exclusiveoutput");
	SAVELOAD(bPositionalAudio, "audio/positional");
	SAVELOAD(bPositionalHeadphone, "audio/headphone");
	SAVELOAD(qsAudioInput, "audio/input");
	SAVELOAD(qsAudioOutput, "audio/output");
	SAVELOAD(bWhisperFriends, "audio/whisperfriends");
	SAVELOAD(bTransmitPosition, "audio/postransmit");

	SAVELOAD(iJitterBufferSize, "net/jitterbuffer");
	SAVELOAD(iFramesPerPacket, "net/framesperpacket");

	SAVELOAD(qsASIOclass, "asio/class");
	SAVELOAD(qlASIOmic, "asio/mic");
	SAVELOAD(qlASIOspeaker, "asio/speaker");

	SAVELOAD(qsWASAPIInput, "wasapi/input");
	SAVELOAD(qsWASAPIOutput, "wasapi/output");

	SAVELOAD(qsALSAInput, "alsa/input");
	SAVELOAD(qsALSAOutput, "alsa/output");

	SAVELOAD(qsPulseAudioInput, "pulseaudio/input");
	SAVELOAD(qsPulseAudioOutput, "pulseaudio/output");

	SAVELOAD(qsOSSInput, "oss/input");
	SAVELOAD(qsOSSOutput, "oss/output");

	SAVELOAD(qsCoreAudioInput, "coreaudio/input");
	SAVELOAD(qsCoreAudioOutput, "coreaudio/output");

	SAVELOAD(iPortAudioInput, "portaudio/input");
	SAVELOAD(iPortAudioOutput, "portaudio/output");

	SAVELOAD(qbaDXInput, "directsound/input");
	SAVELOAD(qbaDXOutput, "directsound/output");

	SAVELOAD(bTTS, "tts/enable");
	SAVELOAD(iTTSVolume, "tts/volume");
	SAVELOAD(iTTSThreshold, "tts/threshold");

	SAVELOAD(bOverlayEnable, "overlay/enable");

	SAVELOAD(bOverlayEnable, "overlay/enable");

	// Network settings
	SAVELOAD(bTCPCompat, "net/tcponly");
	SAVELOAD(bQoS, "net/qos");
	SAVELOAD(bReconnect, "net/reconnect");
	SAVELOAD(bAutoConnect, "net/autoconnect");
	SAVELOAD(ptProxyType, "net/proxytype");
	SAVELOAD(qsProxyHost, "net/proxyhost");
	SAVELOAD(usProxyPort, "net/proxyport");
	SAVELOAD(qsProxyUsername, "net/proxyusername");
	SAVELOAD(qsProxyPassword, "net/proxypassword");
	SAVELOAD(iMaxImageSize, "net/maximagesize");
	SAVELOAD(iMaxImageWidth, "net/maximagewidth");
	SAVELOAD(iMaxImageHeight, "net/maximageheight");

	SAVELOAD(bExpert, "ui/expert");
	SAVELOAD(qsLanguage, "ui/language");
	SAVELOAD(qsStyle, "ui/style");
	SAVELOAD(qsSkin, "ui/skin");
	SAVELOAD(ceExpand, "ui/expand");
	SAVELOAD(ceChannelDrag, "ui/drag");
	SAVELOAD(aotbAlwaysOnTop, "ui/alwaysontop");
	SAVELOAD(bAskOnQuit, "ui/askonquit");
	SAVELOAD(bMinimalView, "ui/minimalview");
	SAVELOAD(bHideFrame, "ui/hideframe");
	SAVELOAD(bUserTop, "ui/usertop");
	SAVELOAD(bFirstTime, "ui/firsttime120");
	SAVELOAD(qbaMainWindowGeometry, "ui/geometry");
	SAVELOAD(qbaMainWindowState, "ui/state");
	SAVELOAD(qbaMinimalViewGeometry, "ui/minimalviewgeometry");
	SAVELOAD(qbaMinimalViewState, "ui/minimalviewstate");
	SAVELOAD(qbaConfigGeometry, "ui/ConfigGeometry");
	SAVELOAD(wlWindowLayout, "ui/WindowLayout");
	SAVELOAD(qbaSplitterState, "ui/splitter");
	SAVELOAD(qbaHeaderState, "ui/header");
	SAVELOAD(qsUsername, "ui/username");
	SAVELOAD(qsLastServer, "ui/server");
	SAVELOAD(ssFilter, "ui/serverfilter");
	SAVELOAD(bUpdateCheck, "ui/updatecheck");
	SAVELOAD(bPluginCheck, "ui/plugincheck");
	SAVELOAD(bHideInTray, "ui/hidetray");
	SAVELOAD(bStateInTray, "ui/stateintray");
	SAVELOAD(bUsage, "ui/usage");
	SAVELOAD(bShowUserCount, "ui/showusercount");
	SAVELOAD(qsImagePath, "ui/imagepath");
	SAVELOAD(bShowContextMenuInMenuBar, "ui/showcontextmenuinmenubar");
	SAVELOAD(qbaConnectDialogGeometry, "ui/connect/geometry");
	SAVELOAD(qbaConnectDialogHeader, "ui/connect/header");
	SAVELOAD(bHighContrast, "ui/HighContrast");

	// Recording
	SAVELOAD(qsRecordingPath, "recording/path");
	SAVELOAD(qsRecordingFile, "recording/file");
	SAVELOAD(rmRecordingMode, "recording/mode");
	SAVELOAD(iRecordingFormat, "recording/format");

	// LCD
	SAVELOAD(iLCDUserViewMinColWidth, "lcd/userview/mincolwidth");
	SAVELOAD(iLCDUserViewSplitterWidth, "lcd/userview/splitterwidth");

	QByteArray qba = CertWizard::exportCert(kpCertificate);
	g.qs->setValue(QLatin1String("net/certificate"), qba);

	g.qs->beginWriteArray(QLatin1String("shortcuts"));
	int idx = 0;
	foreach(const Shortcut &s, qlShortcuts) {
		if (! s.isServerSpecific()) {
			g.qs->setArrayIndex(idx++);
			g.qs->setValue(QLatin1String("index"), s.iIndex);
			g.qs->setValue(QLatin1String("keys"), s.qlButtons);
			g.qs->setValue(QLatin1String("suppress"), s.bSuppress);
			g.qs->setValue(QLatin1String("data"), s.qvData);
		}
	}
	g.qs->endArray();

	g.qs->beginWriteArray(QLatin1String("messages"));
	for (QMap<int, quint32>::const_iterator it = qmMessages.constBegin(); it != qmMessages.constEnd(); ++it) {
		g.qs->setArrayIndex(it.key());
		SAVELOAD(qmMessages[it.key()], "log");
	}
	g.qs->endArray();

	g.qs->beginWriteArray(QLatin1String("messagesounds"));
	for (QMap<int, QString>::const_iterator it = qmMessageSounds.constBegin(); it != qmMessageSounds.constEnd(); ++it) {
		g.qs->setArrayIndex(it.key());
		SAVELOAD(qmMessageSounds[it.key()], "logsound");
	}
	g.qs->endArray();

	g.qs->beginGroup(QLatin1String("lcd/devices"));
	foreach(const QString &d, qmLCDDevices.keys()) {
		bool v = qmLCDDevices.value(d);
		if (!v)
			g.qs->setValue(d, v);
		else
			g.qs->remove(d);
	}
	g.qs->endGroup();

	g.qs->beginGroup(QLatin1String("audio/plugins"));
	foreach(const QString &d, qmPositionalAudioPlugins.keys()) {
		bool v = qmPositionalAudioPlugins.value(d);
		if (!v)
			g.qs->setValue(d, v);
		else
			g.qs->remove(d);
	}
	g.qs->endGroup();

	g.qs->beginGroup(QLatin1String("overlay"));
	os.save();
	g.qs->endGroup();
}
