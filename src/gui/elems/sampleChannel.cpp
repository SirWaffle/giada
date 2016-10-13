/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * ge_sampleChannel
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2016 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */


#include "../../core/pluginHost.h"
#include "../../core/mixer.h"
#include "../../core/conf.h"
#include "../../core/patch_DEPR_.h"
#include "../../core/recorder.h"
#include "../../core/graphics.h"
#include "../../core/wave.h"
#include "../../glue/glue.h"
#include "../../glue/io.h"
#include "../../glue/channel.h"
#include "../../glue/storage.h"
#include "../../utils/gui.h"
#include "../../utils/string.h"
#include "../dialogs/gd_mainWindow.h"
#include "../dialogs/gd_pluginList.h"
#include "../dialogs/gd_keyGrabber.h"
#include "../dialogs/gd_midiOutput.h"
#include "../dialogs/gd_midiInput.h"
#include "../dialogs/gd_editor.h"
#include "../dialogs/gd_actionEditor.h"
#include "../dialogs/gd_warnings.h"
#include "../dialogs/gd_browser.h"
#include "ge_status.h"
#include "ge_modeBox.h"
#include "ge_keyboard.h"
#include "sampleChannel.h"


extern Mixer 		     G_Mixer;
extern Conf  		     G_Conf;
extern Recorder			 G_Recorder;
extern Patch_DEPR_   G_Patch_DEPR_;
extern gdMainWindow *G_MainWin;


geSampleChannel::geSampleChannel(int X, int Y, int W, int H, SampleChannel *ch)
	: geChannel(X, Y, W, H, CHANNEL_SAMPLE), ch(ch)
{
	begin();

#if defined(WITH_VST)
  int delta = 216; // (9 widgets * 20) + (9 paddings * 4)
#else
	int delta = 192; // (8 widgets * 20) + (8 paddings * 4)
#endif

	button      = new gButton(x(), y(), 20, 20, "", channelStop_xpm, channelPlay_xpm);
	arm         = new gClick(button->x()+button->w()+4, y(), 20, 20, "", armOff_xpm, armOn_xpm);
	status      = new gStatus(arm->x()+arm->w()+4, y(), 20, 20, ch);
	mainButton  = new geSampleChannelButton(status->x()+status->w()+4, y(), w() - delta, 20, "-- no sample --");
	readActions = new gClick(mainButton->x()+mainButton->w()+4, y(), 20, 20, "", readActionOff_xpm, readActionOn_xpm);
	modeBox     = new gModeBox(readActions->x()+readActions->w()+4, y(), 20, 20, ch);
	mute        = new gClick(modeBox->x()+modeBox->w()+4, y(), 20, 20, "", muteOff_xpm, muteOn_xpm);
	solo        = new gClick(mute->x()+mute->w()+4, y(), 20, 20, "", soloOff_xpm, soloOn_xpm);

#if defined(WITH_VST)
	fx          = new gFxButton(solo->x()+solo->w()+4, y(), 20, 20, fxOff_xpm, fxOn_xpm);
	vol         = new gDial(fx->x()+fx->w()+4, y(), 20, 20);
#else
	vol         = new gDial(solo->x()+solo->w()+4, y(), 20, 20);
#endif

	end();

  resizable(mainButton);

	update();

	button->callback(cb_button, (void*)this);
	button->when(FL_WHEN_CHANGED);   // do callback on keypress && on keyrelease

#ifdef WITH_VST
	fx->callback(cb_openFxWindow, (void*)this);
#endif

	mute->type(FL_TOGGLE_BUTTON);
	mute->callback(cb_mute, (void*)this);

	solo->type(FL_TOGGLE_BUTTON);
	solo->callback(cb_solo, (void*)this);

	mainButton->callback(cb_openMenu, (void*)this);

	readActions->type(FL_TOGGLE_BUTTON);
	readActions->value(ch->readActions);
	readActions->callback(cb_readActions, (void*)this);

	vol->callback(cb_changeVol, (void*)this);

	ch->guiChannel = this;
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::cb_button      (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_button(); }
void geSampleChannel::cb_mute        (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_mute(); }
void geSampleChannel::cb_solo        (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_solo(); }
void geSampleChannel::cb_openMenu    (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_openMenu(); }
void geSampleChannel::cb_changeVol   (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_changeVol(); }
void geSampleChannel::cb_readActions (Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_readActions(); }
#ifdef WITH_VST
void geSampleChannel::cb_openFxWindow(Fl_Widget *v, void *p) { ((geSampleChannel*)p)->__cb_openFxWindow(); }
#endif


/* -------------------------------------------------------------------------- */


void geSampleChannel::__cb_mute()
{
	glue_setMute(ch);
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::__cb_solo()
{
	solo->value() ? glue_setSoloOn(ch) : glue_setSoloOff(ch);
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::__cb_changeVol()
{
	glue_setChanVol(ch, vol->value());
}


/* -------------------------------------------------------------------------- */


#ifdef WITH_VST
void geSampleChannel::__cb_openFxWindow()
{
	gu_openSubWindow(G_MainWin, new gdPluginList(PluginHost::CHANNEL, ch), WID_FX_LIST);
}
#endif


/* -------------------------------------------------------------------------- */



void geSampleChannel::__cb_button()
{
	if (button->value())    // pushed
		glue_keyPress(ch, Fl::event_ctrl(), Fl::event_shift());
	else                    // released
		glue_keyRelease(ch, Fl::event_ctrl(), Fl::event_shift());
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::__cb_openMenu()
{
	/* if you're recording (actions or input) no menu is allowed; you can't
	 * do anything, especially deallocate the channel */

	if (G_Mixer.chanInput == ch || G_Recorder.active)
		return;

	/* the following is a trash workaround for a FLTK menu. We need a gMenu
	 * widget asap */

	Fl_Menu_Item rclick_menu[] = {
		{"Load new sample..."},                     // 0
		{"Export sample to file..."},               // 1
		{"Setup keyboard input..."},                // 2
		{"Setup MIDI input..."},                    // 3
		{"Setup MIDI output..."},                   // 4
		{"Edit sample..."},                         // 5
		{"Edit actions..."},                        // 6
		{"Clear actions", 0, 0, 0, FL_SUBMENU},     // 7
			{"All"},                                  // 8
			{"Mute"},                                 // 9
			{"Volume"},                               // 10
			{"Start/Stop"},                           // 11
			{0},                                      // 12
		{"Clone channel"},                          // 13
		{"Free channel"},                           // 14
		{"Delete channel"},                         // 15
		{0}
	};

	if (ch->status & (STATUS_EMPTY | STATUS_MISSING)) {
		rclick_menu[1].deactivate();
		rclick_menu[5].deactivate();
		rclick_menu[14].deactivate();
	}

	/* no 'clear actions' if there are no actions */

	if (!ch->hasActions)
		rclick_menu[7].deactivate();

	/* no 'clear start/stop actions' for those channels in loop mode:
	 * they cannot have start/stop actions. */

	if (ch->mode & LOOP_ANY)
		rclick_menu[11].deactivate();

	Fl_Menu_Button *b = new Fl_Menu_Button(0, 0, 100, 50);
	b->box(G_BOX);
	b->textsize(GUI_FONT_SIZE_BASE);
	b->textcolor(COLOR_TEXT_0);
	b->color(COLOR_BG_0);

	const Fl_Menu_Item *m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, b);
	if (!m) return;

	if (strcmp(m->label(), "Load new sample...") == 0) {
		openBrowser(BROWSER_LOAD_SAMPLE);
		return;
	}

	if (strcmp(m->label(), "Setup keyboard input...") == 0) {
		new gdKeyGrabber(ch); /// FIXME - use gu_openSubWindow
		return;
	}

	if (strcmp(m->label(), "Setup MIDI input...") == 0) {
		gu_openSubWindow(G_MainWin, new gdMidiInputChannel(ch), 0);
		return;
	}

	if (strcmp(m->label(), "Setup MIDI output...") == 0) {
		gu_openSubWindow(G_MainWin, new gdMidiOutputSampleCh(ch), 0);
		return;
	}

	if (strcmp(m->label(), "Edit sample...") == 0) {
		gu_openSubWindow(G_MainWin, new gdEditor(ch), WID_SAMPLE_EDITOR); /// FIXME title it's up to gdEditor
		return;
	}

	if (strcmp(m->label(), "Export sample to file...") == 0) {
		openBrowser(BROWSER_SAVE_SAMPLE);
		return;
	}

	if (strcmp(m->label(), "Delete channel") == 0) {
		if (!gdConfirmWin("Warning", "Delete channel: are you sure?"))
			return;
		glue_deleteChannel(ch);
		return;
	}

	if (strcmp(m->label(), "Free channel") == 0) {
		if (ch->status == STATUS_PLAY) {
			if (!gdConfirmWin("Warning", "This action will stop the channel: are you sure?"))
				return;
		}
		else if (!gdConfirmWin("Warning", "Free channel: are you sure?"))
			return;

		glue_freeChannel(ch);

		/* delete any related subwindow */

		/** FIXME - use gu_closeAllSubwindows() */

		G_MainWin->delSubWindow(WID_FILE_BROWSER);
		G_MainWin->delSubWindow(WID_ACTION_EDITOR);
		G_MainWin->delSubWindow(WID_SAMPLE_EDITOR);
		G_MainWin->delSubWindow(WID_FX_LIST);

		return;
	}

	if (strcmp(m->label(), "Clone channel") == 0) {
		glue_cloneChannel(ch);
		return;
	}

	if (strcmp(m->label(), "Mute") == 0) {
		if (!gdConfirmWin("Warning", "Clear all mute actions: are you sure?"))
			return;
		G_Recorder.clearAction(ch->index, ACTION_MUTEON | ACTION_MUTEOFF);
		if (!ch->hasActions)
			hideActionButton();

		/* TODO - set mute=false */

		gu_refreshActionEditor(); // refresh a.editor window, it could be open
		return;
	}

	if (strcmp(m->label(), "Start/Stop") == 0) {
		if (!gdConfirmWin("Warning", "Clear all start/stop actions: are you sure?"))
			return;
		G_Recorder.clearAction(ch->index, ACTION_KEYPRESS | ACTION_KEYREL | ACTION_KILLCHAN);
		if (!ch->hasActions)
			hideActionButton();
		gu_refreshActionEditor();  // refresh a.editor window, it could be open
		return;
	}

	if (strcmp(m->label(), "Volume") == 0) {
		if (!gdConfirmWin("Warning", "Clear all volume actions: are you sure?"))
			return;
		G_Recorder.clearAction(ch->index, ACTION_VOLUME);
		if (!ch->hasActions)
			hideActionButton();
		gu_refreshActionEditor();  // refresh a.editor window, it could be open
		return;
	}

	if (strcmp(m->label(), "All") == 0) {
		if (!gdConfirmWin("Warning", "Clear all actions: are you sure?"))
			return;
		G_Recorder.clearChan(ch->index);
		hideActionButton();
		gu_refreshActionEditor(); // refresh a.editor window, it could be open
		return;
	}

	if (strcmp(m->label(), "Edit actions...") == 0) {
		gu_openSubWindow(G_MainWin, new gdActionEditor(ch),	WID_ACTION_EDITOR);
		return;
	}
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::__cb_readActions()
{
	glue_startStopReadingRecs(ch);
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::openBrowser(int type)
{
	gWindow *childWin = NULL;
	switch (type) {
		case BROWSER_LOAD_SAMPLE:
			childWin = new gdLoadBrowser(G_Conf.browserX, G_Conf.browserY,
					G_Conf.browserW, G_Conf.browserH, "Browse sample",
					G_Conf.samplePath.c_str(), glue_loadSample, ch);
			break;
		case BROWSER_SAVE_SAMPLE:
			childWin = new gdSaveBrowser(G_Conf.browserX, G_Conf.browserY,
					G_Conf.browserW, G_Conf.browserH, "Save sample", \
					G_Conf.samplePath.c_str(), "", glue_saveSample, ch);
			break;
	}
	if (childWin)
		gu_openSubWindow(G_MainWin, childWin,	WID_FILE_BROWSER);
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::refresh()
{
  if (!mainButton->visible()) // mainButton invisible? status too (see below)
    return;

	setColorsByStatus(ch->status, ch->recStatus);

	if (ch->wave != NULL) {
		if (G_Mixer.chanInput == ch)
			mainButton->setInputRecordMode();
		if (G_Recorder.active) {
			if (G_Recorder.canRec(ch))
				mainButton->setActionRecordMode();
		}
		status->redraw(); // status invisible? sampleButton too (see below)
	}
	mainButton->redraw();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::reset()
{
	hideActionButton();
	mainButton->setDefaultMode("-- no sample --");
 	mainButton->redraw();
	status->redraw();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::update()
{
	/* update sample button's label */

	switch (ch->status) {
		case STATUS_EMPTY:
			mainButton->label("-- no sample --");
			break;
		case STATUS_MISSING:
		case STATUS_WRONG:
			mainButton->label("* file not found! *");
			break;
		default:
			mainButton->label(ch->wave->name.c_str());
			break;
	}

	/* update channels. If you load a patch with recorded actions, the 'R'
	 * button must be shown. Moreover if the actions are active, the 'R'
	 * button must be activated accordingly. */

	if (ch->hasActions)
		showActionButton();
	else
		hideActionButton();

	/* updates modebox */

	modeBox->value(ch->mode);
	modeBox->redraw();

	/* update volumes+mute+solo */

	vol->value(ch->volume);
	mute->value(ch->mute);
	solo->value(ch->solo);

	mainButton->setKey(ch->key);

#ifdef WITH_VST
	fx->full = ch->plugins.size() > 0;
	fx->redraw();
#endif
}


/* -------------------------------------------------------------------------- */


int geSampleChannel::keyPress(int e)
{
	return handleKey(e, ch->key);
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::showActionButton()
{
	if (readActions->visible())
		return;
	mainButton->size(mainButton->w()-24, mainButton->h());
	readActions->resize(mainButton->x()+mainButton->w()+4, y(), 20, 20);
	readActions->value(ch->readActions);
	readActions->show();
	redraw();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::hideActionButton()
{
	if (!readActions->visible())
		return;
	mainButton->size(mainButton->w()+24, mainButton->h());
	readActions->hide();
	redraw();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::resize(int X, int Y, int W, int H)
{
  geChannel::resize(X, Y, W, H);

	if (w() < BREAK_ARM) {
		puts("BREAK_ARM");
		breakAtArm();
	}
	else
	if (w() < BREAK_FX) {
		puts("BREAK_FX");
		breakAtFx();
	}
	else
	if (w() < BREAK_MODE_BOX) {
		puts("BREAK_MODE_BOX");
		breakAtModeBox();
	}
	else
	if (w() < BREAK_READ_ACTIONS) {
		puts("BREAK_READ_ACTIONS");
		breakAtReadActions();
	}
	else {
		puts("no breaks");
		breakAtNone();
	}
	geChannel::init_sizes();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::breakAtArm()
{
	arm->hide();
	status->resize(button->x()+button->w()+4, y(), 20, 20);
#ifdef WITH_VST
	mainButton->resize(status->x()+status->w()+4, y(), w() - BREAK_DELTA, 20);
#else
	mainButton->resize(status->x()+status->w()+4, y(), w() - (BREAK_DELTA + BREAK_UNIT), 20);
#endif
	/* The followings are useless on manual resizing, but useful when a channel
	 * is added from a patch with a small width.
	 TODO - is it true? */
	modeBox->hide();
	readActions->hide();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::breakAtFx(int delta)
{
#ifdef WITH_VST
	fx->hide();
#endif
	arm->show();
	arm->size(20, 20);
	status->resize(arm->x()+arm->w()+4, y(), 20, 20);
	mainButton->resize(status->x()+status->w()+4, y(), w() - delta, 20);
	mute->resize(mainButton->x()+mainButton->w()+4, y(), 20, 20);
	solo->resize(mute->x()+mute->w()+4, y(), 20, 20);
}

/* -------------------------------------------------------------------------- */


void geSampleChannel::breakAtModeBox()
{
	breakAtFx(BREAK_DELTA + (BREAK_UNIT*2));
#ifdef WITH_VST
	fx->show();
#endif
  modeBox->hide();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::breakAtReadActions()
{
	modeBox->show();
	mainButton->size(w() - (BREAK_DELTA + (BREAK_UNIT*3)), 20);
	modeBox->resize(mainButton->x()+mainButton->w()+4, y(), 20, 20);
	readActions->hide();
}


/* -------------------------------------------------------------------------- */


void geSampleChannel::breakAtNone()
{
	if (!ch->hasActions)
		return;
	mainButton->size(w() - (BREAK_DELTA + (BREAK_UNIT*4)), 20);
	readActions->resize(mainButton->x()+mainButton->w()+4, y(), 20, 20);
	readActions->show();
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


geSampleChannelButton::geSampleChannelButton(int x, int y, int w, int h, const char *l)
	: geChannelButton(x, y, w, h, l) {}


/* -------------------------------------------------------------------------- */


int geSampleChannelButton::handle(int e)
{
	int ret = gClick::handle(e);
	switch (e) {
		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_RELEASE: {
			ret = 1;
			break;
		}
		case FL_PASTE: {
      geSampleChannel *gch = (geSampleChannel*) parent();   // parent is geSampleChannel
      SampleChannel  *ch  = gch->ch;
      int result = glue_loadChannel(ch, gu_trim(gu_stripFileUrl(Fl::event_text())).c_str());
			if (result != SAMPLE_LOADED_OK)
				G_MainWin->keyboard->printChannelMessage(result);
			ret = 1;
			break;
		}
	}
	return ret;
}
