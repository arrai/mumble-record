/* Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2008-2009, Mikkel Krautz <mikkel@krautz.dk>

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

#include "GlobalShortcut_macx.h"

#define MOD_OFFSET   0x10000
#define MOUSE_OFFSET 0x20000

/*
 * We use DeferInit in here to pop up a dialog box if their system isn't
 * properly set up to receieve global keyboard/mouse events.
 */

GlobalShortcutMacInit::GlobalShortcutMacInit() : QObject(NULL) {
}

void GlobalShortcutMacInit::initialize() {
	if (!accessibilityApiEnabled())
		accessibilityDialog();
}

bool GlobalShortcutMacInit::accessibilityApiEnabled() const {
	return QFile::exists("/private/var/db/.AccessibilityAPIEnabled");
}

void GlobalShortcutMacInit::openPrefsPane(const QString &) const {
	system("open /Applications/System\\ Preferences.app/ /System/Library/PreferencePanes/UniversalAccessPref.prefPane/");
}

void GlobalShortcutMacInit::accessibilityDialog() const {
	QMessageBox mb("Mumble",
	               tr("Mumble has detected that it is unable to receive Global Shortcut events when it is in the background.<br /><br />"
	                  "This is because the Universal Access feature called 'Enable access for assistive devices' is currently disabled.<br /><br />"
	                  "Please <a href=\" \">enable this setting</a> and continue when done."),
	               QMessageBox::Question, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);

	QLabel *label = mb.findChild<QLabel *>(QLatin1String("qt_msgbox_label"));
	label->setOpenExternalLinks(false);
	connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(openPrefsPane(const QString &)));

	mb.exec();
}

static GlobalShortcutMacInit gsminit;

/* --- */

GlobalShortcutEngine *GlobalShortcutEngine::platformInit() {
	return new GlobalShortcutMac();
}

CGEventRef GlobalShortcutMac::callback(CGEventTapProxy proxy, CGEventType type,
                                       CGEventRef event, void *udata) {
	GlobalShortcutMac *gs = reinterpret_cast<GlobalShortcutMac *>(udata);
	unsigned int keycode;
	bool suppress = false;
	bool down = false;
	int64_t repeat = 0;

	switch (type) {
		case kCGEventLeftMouseDown:
		case kCGEventRightMouseDown:
		case kCGEventOtherMouseDown:
			down = true;
		case kCGEventLeftMouseUp:
		case kCGEventRightMouseUp:
		case kCGEventOtherMouseUp:
			keycode = static_cast<unsigned int>(CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber));
			suppress = gs->handleButton(MOUSE_OFFSET+keycode, down);
			/* Suppressing "the" mouse button is probably not a good idea :-) */
			if (keycode == 0)
				suppress = false;
			break;

		case kCGEventKeyDown:
			down = true;
		case kCGEventKeyUp:
			repeat = CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat);
			if (! repeat) {
				keycode = static_cast<unsigned int>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
				suppress = gs->handleButton(keycode, down);
			}
			break;

		case kCGEventFlagsChanged:
			suppress = gs->handleModButton(CGEventGetFlags(event));
			break;

		case kCGEventTapDisabledByTimeout:
			qWarning("GlobalShortcutMac: EventTap disabled by timeout. Re-enabling.");
			/*
			 * On Snow Leopard, we get this event type quite often. It disables our event
			 * tap completely. Possible Apple bug.
			 *
			 * For now, simply call CGEventTapEnable() to enable our event tap again.
			 *
			 * See: http://lists.apple.com/archives/quartz-dev/2009/Sep/msg00007.html
			 */
			CGEventTapEnable(gs->port, true);
			break;
		case kCGEventTapDisabledByUserInput:
			qWarning("GlobalShortcutMac: EventTap disabled by user input.");
			break;

		default:
			qWarning("GlobalShortcutMac: Unknown event intercepted.");
			break;
	}

	return suppress ? NULL : event;
}

GlobalShortcutMac::GlobalShortcutMac() : modmask(0) {
#ifndef QT_NO_DEBUG
	qWarning("GlobalShortcutMac: Debug build detected. Disabling shortcut engine.");
	return;
#endif

	static const CGEventType evmask = CGEventMaskBit(kCGEventLeftMouseDown) |
	                                  CGEventMaskBit(kCGEventLeftMouseUp) |
	                                  CGEventMaskBit(kCGEventRightMouseDown) |
	                                  CGEventMaskBit(kCGEventRightMouseUp) |
	                                  CGEventMaskBit(kCGEventOtherMouseDown) |
	                                  CGEventMaskBit(kCGEventOtherMouseUp) |
	                                  CGEventMaskBit(kCGEventKeyDown) |
	                                  CGEventMaskBit(kCGEventKeyUp) |
	                                  CGEventMaskBit(kCGEventFlagsChanged) |
	                                  CGEventMaskBit(kCGEventTapDisabledByTimeout) |
	                                  CGEventMaskBit(kCGEventTapDisabledByUserInput);

	port = CGEventTapCreate(kCGSessionEventTap,
	                        kCGHeadInsertEventTap,
	                        0, evmask, GlobalShortcutMac::callback,
	                        this);

	if (! port) {
		qWarning("GlobalShortcutMac: Unable to create EventTap. Global Shortcuts will not be available.");
		return;
	}

	kbdLayout = NULL;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
	if (TISCopyCurrentKeyboardInputSource && TISGetInputSourceProperty) {
		TISInputSourceRef inputSource = TISCopyCurrentKeyboardInputSource();
		if (inputSource) {
			CFDataRef data = static_cast<CFDataRef>(TISGetInputSourceProperty(inputSource, kTISPropertyUnicodeKeyLayoutData));
			if (data)
				kbdLayout = reinterpret_cast<UCKeyboardLayout *>(const_cast<UInt8 *>(CFDataGetBytePtr(data)));
		}
	}
#endif
#ifndef __LP64__
	if (! kbdLayout) {
		SInt16 currentKeyScript = GetScriptManagerVariable(smKeyScript);
		SInt16 lastKeyLayoutID = GetScriptVariable(currentKeyScript, smScriptKeys);
		Handle handle = GetResource('uchr', lastKeyLayoutID);
		if (handle)
			kbdLayout = reinterpret_cast<UCKeyboardLayout *>(*handle);
	}
#endif
	if (! kbdLayout)
		qWarning("GlobalShortcutMac: No keyboard layout mapping available. Unable to perform key translation.");

	start(QThread::TimeCriticalPriority);
}

GlobalShortcutMac::~GlobalShortcutMac() {
#ifndef QT_NO_DEBUG
	return;
#endif
	CFRunLoopStop(loop);
	loop = NULL;
	wait();
}

void GlobalShortcutMac::run() {
	loop = CFRunLoopGetCurrent();
	CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, port, 0);
	CFRunLoopAddSource(loop, src, kCFRunLoopCommonModes);
	CFRunLoopRun();
}

void GlobalShortcutMac::needRemap() {
	remap();
}

bool GlobalShortcutMac::handleModButton(const CGEventFlags newmask) {
	bool down;
	bool suppress = false;

#define MOD_CHANGED(mask, btn) do { \
	    if ((newmask & mask) != (modmask & mask)) { \
	        down = newmask & mask; \
	        suppress = handleButton(MOD_OFFSET+btn, down); \
	        modmask = newmask; \
	        return suppress; \
	    }} while (0)

	MOD_CHANGED(kCGEventFlagMaskAlphaShift, 0);
	MOD_CHANGED(kCGEventFlagMaskShift, 1);
	MOD_CHANGED(kCGEventFlagMaskControl, 2);
	MOD_CHANGED(kCGEventFlagMaskAlternate, 3);
	MOD_CHANGED(kCGEventFlagMaskCommand, 4);
	MOD_CHANGED(kCGEventFlagMaskHelp, 5);
	MOD_CHANGED(kCGEventFlagMaskSecondaryFn, 6);
	MOD_CHANGED(kCGEventFlagMaskNumericPad, 7);
}

QString GlobalShortcutMac::translateMouseButton(const unsigned int keycode) const {
	return QString("Mouse Button %1").arg(keycode-MOUSE_OFFSET+1);
}

QString GlobalShortcutMac::translateModifierKey(const unsigned int keycode) const {
	unsigned int key = keycode - MOD_OFFSET;
	switch (key) {
		case 0:
			return QString("Caps Lock");
		case 1:
			return QString("Shift");
		case 2:
			return QString("Control");
		case 3:
			return QString("Alt/Option");
		case 4:
			return QString("Command");
		case 5:
			return QString("Help");
		case 6:
			return QString("Fn");
		case 7:
			return QString("Num Lock");
	}
	return QString("Modifier %1").arg(key);
}

QString GlobalShortcutMac::translateKeyName(const unsigned int keycode) const {
	UInt32 junk = 0;
	UniCharCount len = 64;
	UniChar unicodeString[len];

	if (! kbdLayout)
		return QString();

	OSStatus err = UCKeyTranslate(kbdLayout, static_cast<UInt16>(keycode),
	                              kUCKeyActionDisplay, 0, LMGetKbdType(),
	                              kUCKeyTranslateNoDeadKeysBit, &junk,
	                              len, &len, unicodeString);
	if (err != noErr)
		return QString();

	if (len == 1) {
		switch (unicodeString[0]) {
			case '\t':
				return QString("Tab");
			case '\r':
				return QString("Enter");
			case '\b':
				return QString("Backspace");
			case '\e':
				return QString("Escape");
			case ' ':
				return QString("Space");
			case 28:
				return QString("Left");
			case 29:
				return QString("Right");
			case 30:
				return QString("Up");
			case 31:
				return QString("Down");
		}

		if (unicodeString[0] < ' ') {
			qWarning("GlobalShortcutMac: Unknown translation for keycode %u: %u", keycode, unicodeString[0]);
			return QString();
		}
	}

	return QString(reinterpret_cast<const QChar *>(unicodeString), len).toUpper();
}

QString GlobalShortcutMac::buttonName(const QVariant &v) {
	bool ok;
	unsigned int key = v.toUInt(&ok);
	if (!ok)
		return QString();

	if (key >= MOUSE_OFFSET)
		return translateMouseButton(key);
	else if (key >= MOD_OFFSET)
		return translateModifierKey(key);
	else {
		QString str = translateKeyName(key);
		if (!str.isEmpty())
			return str;
	}

	return QString("Keycode %1").arg(key);
}

bool GlobalShortcutMac::canSuppress() {
	return true;
}
