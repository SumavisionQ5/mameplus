/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  mui_opts.c

  Stores global options and per-game options;

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>

// standard C headers
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <direct.h>
#include <stddef.h>
#include <tchar.h>
#include <assert.h>

// MAME/MAMEUI headers
#include "emu.h"
#include "emuopts.h"
#include "bitmask.h"
#include "winui.h"
#include "mui_util.h"
#include "treeview.h"
#include "splitters.h"
#include "mui_opts.h"
#include "winutf8.h"
#include "strconv.h"
#include "clifront.h"
#include "translate.h"
#include "game_opts.h"

#ifdef MESS
#include "optionsms.h"
#endif // MESS

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#undef malloc
#undef realloc
#undef free

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

// static void LoadFolderFilter(int folder_index,int filters);

static file_error LoadSettingsFile(winui_options &opts, const char *filename);
static file_error SaveSettingsFile(winui_options &opts, winui_options *baseopts, const char *filename);
static file_error LoadSettingsFile(windows_options &opts, const char *filename);
static file_error SaveSettingsFile(windows_options &opts, windows_options *baseopts, const char *filename);

static void LoadOptionsAndSettings(void);

static void  CusColorEncodeString(const COLORREF *value, char* str);
static void  CusColorDecodeString(const char* str, COLORREF *value);

static void  SplitterEncodeString(const int *value, char* str);
static void  SplitterDecodeString(const char *str, int *value);

static void  FontEncodeString(const LOGFONTW *f, char *str);
static void  FontDecodeString(const char* str, LOGFONTW *f);

static void TabFlagsEncodeString(int data, char *str);
static void TabFlagsDecodeString(const char *str, int *data);

//static DWORD DecodeFolderFlags(const char *buf);
//static const char * EncodeFolderFlags(DWORD value);

static void ResetToDefaults(windows_options &opts, int priority);

static void ui_parse_ini_file(windows_options &opts, const char *name);
static void remove_all_source_options(void);


#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal defines
 ***************************************************************************/

#define UI_INI_FILENAME				MAMEUINAME ".ini"
#define DEFAULT_OPTIONS_INI_FILENAME		emulator_info::get_configname()
#define GAMEINFO_INI_FILENAME			"GameInfo.ini"



#define MUIOPTION_LIST_MODE				"list_mode"
#define MUIOPTION_CHECK_GAME				"check_game"
#define MUIOPTION_JOYSTICK_IN_INTERFACE			"joystick_in_interface"
#define MUIOPTION_KEYBOARD_IN_INTERFACE			"keyboard_in_interface"
#define MUIOPTION_CYCLE_SCREENSHOT			"cycle_screenshot"
#define MUIOPTION_STRETCH_SCREENSHOT_LARGER		"stretch_screenshot_larger"
#define MUIOPTION_SCREENSHOT_BORDER_SIZE		"screenshot_bordersize"
#define MUIOPTION_SCREENSHOT_BORDER_COLOR		"screenshot_bordercolor"
#define MUIOPTION_INHERIT_FILTER			"inherit_filter"
#define MUIOPTION_OFFSET_CLONES				"offset_clones"
#ifdef USE_SHOW_SPLASH_SCREEN
#define MUIOPTION_DISPLAY_SPLASH_SCREEN			"display_splash_screen"
#endif /* USE_SHOW_SPLASH_SCREEN */
#ifdef TREE_SHEET
#define MUIOPTION_SHOW_TREE_SHEET			"show_tree_sheet"
#endif /* TREE_SHEET */
#define MUIOPTION_BROADCAST_GAME_NAME			"broadcast_game_name"
#define MUIOPTION_RANDOM_BACKGROUND			"random_background"
#define MUIOPTION_DEFAULT_FOLDER_ID			"default_folder_id"
#define MUIOPTION_SHOW_IMAGE_SECTION			"show_image_section"
#define MUIOPTION_SHOW_FOLDER_SECTION			"show_folder_section"
#define MUIOPTION_HIDE_FOLDERS				"hide_folders"
#define MUIOPTION_SHOW_STATUS_BAR			"show_status_bar"
#define MUIOPTION_SHOW_TABS				"show_tabs"
#define MUIOPTION_SHOW_TOOLBAR				"show_tool_bar"
#define MUIOPTION_CURRENT_TAB				"current_tab"
#define MUIOPTION_WINDOW_X				"window_x"
#define MUIOPTION_WINDOW_Y				"window_y"
#define MUIOPTION_WINDOW_WIDTH				"window_width"
#define MUIOPTION_WINDOW_HEIGHT				"window_height"
#define MUIOPTION_WINDOW_STATE				"window_state"
#define MUIOPTION_CUSTOM_COLOR				"custom_color"
#define MUIOPTION_FOLDER_FLAG				"folder_flag"
#define MUIOPTION_USE_BROKEN_ICON			"use_broken_icon"
#define MUIOPTION_LIST_FONT				"list_font"
#define MUIOPTION_LIST_FONTFACE				"list_fontface"
#define MUIOPTION_TEXT_COLOR				"text_color"
#define MUIOPTION_CLONE_COLOR				"clone_color"
#define MUIOPTION_BROKEN_COLOR				"broken_color"
#define MUIOPTION_HIDE_TABS				"hide_tabs"
#define MUIOPTION_HISTORY_TAB				"history_tab"
#define MUIOPTION_COLUMN_WIDTHS				"column_widths"
#define MUIOPTION_COLUMN_ORDER				"column_order"
#define MUIOPTION_COLUMN_SHOWN				"column_shown"
#define MUIOPTION_SPLITTERS				"splitters"
#define MUIOPTION_SORT_COLUMN				"sort_column"
#define MUIOPTION_SORT_REVERSED				"sort_reversed"
#if 0 //mamep
#define MUIOPTION_LANGUAGE				"language"
#endif
#define MUIOPTION_FLYER_DIRECTORY			"flyer_directory"
#define MUIOPTION_CABINET_DIRECTORY			"cabinet_directory"
#define MUIOPTION_MARQUEE_DIRECTORY			"marquee_directory"
#define MUIOPTION_TITLE_DIRECTORY			"title_directory"
#define MUIOPTION_CPANEL_DIRECTORY			"cpanel_directory"
#define MUIOPTION_PCB_DIRECTORY				"pcb_directory"
#ifdef USE_VIEW_PCBINFO
#define MUIOPTION_PCBINFO_DIRECTORY			"pcbinfo_directory"
#endif /* USE_VIEW_PCBINFO */
#define MUIOPTION_ICONS_DIRECTORY			"icons_directory"
#define MUIOPTION_BACKGROUND_DIRECTORY			"background_directory"
#define MUIOPTION_FOLDER_DIRECTORY			"folder_directory"
#define MUIOPTION_UI_DIRECTORY				"ui_directory"
#define MUIOPTION_UI_KEY_UP				"ui_key_up"
#define MUIOPTION_UI_KEY_DOWN				"ui_key_down"
#define MUIOPTION_UI_KEY_LEFT				"ui_key_left"
#define MUIOPTION_UI_KEY_RIGHT				"ui_key_right"
#define MUIOPTION_UI_KEY_START				"ui_key_start"
#define MUIOPTION_UI_KEY_PGUP				"ui_key_pgup"
#define MUIOPTION_UI_KEY_PGDWN				"ui_key_pgdwn"
#define MUIOPTION_UI_KEY_HOME				"ui_key_home"
#define MUIOPTION_UI_KEY_END				"ui_key_end"
#define MUIOPTION_UI_KEY_SS_CHANGE			"ui_key_ss_change"
#define MUIOPTION_UI_KEY_HISTORY_UP			"ui_key_history_up"
#define MUIOPTION_UI_KEY_HISTORY_DOWN			"ui_key_history_down"
#define MUIOPTION_UI_KEY_CONTEXT_FILTERS		"ui_key_context_filters"
#define MUIOPTION_UI_KEY_SELECT_RANDOM			"ui_key_select_random"
#define MUIOPTION_UI_KEY_GAME_AUDIT			"ui_key_game_audit"
#define MUIOPTION_UI_KEY_GAME_PROPERTIES		"ui_key_game_properties"
#define MUIOPTION_UI_KEY_HELP_CONTENTS			"ui_key_help_contents"
#define MUIOPTION_UI_KEY_UPDATE_GAMELIST		"ui_key_update_gamelist"
#define MUIOPTION_UI_KEY_VIEW_FOLDERS			"ui_key_view_folders"
#define MUIOPTION_UI_KEY_VIEW_FULLSCREEN		"ui_key_view_fullscreen"
#define MUIOPTION_UI_KEY_VIEW_PAGETAB			"ui_key_view_pagetab"
#define MUIOPTION_UI_KEY_VIEW_PICTURE_AREA		"ui_key_view_picture_area"
#define MUIOPTION_UI_KEY_VIEW_STATUS			"ui_key_view_status"
#define MUIOPTION_UI_KEY_VIEW_TOOLBARS			"ui_key_view_toolbars"
#define MUIOPTION_UI_KEY_VIEW_TAB_CABINET		"ui_key_view_tab_cabinet"
#define MUIOPTION_UI_KEY_VIEW_TAB_CPANEL		"ui_key_view_tab_cpanel"
#define MUIOPTION_UI_KEY_VIEW_TAB_FLYER			"ui_key_view_tab_flyer"
#define MUIOPTION_UI_KEY_VIEW_TAB_HISTORY		"ui_key_view_tab_history"
#ifdef STORY_DATAFILE
#define MUIOPTION_UI_KEY_VIEW_TAB_STORY			"ui_key_view_tab_story"
#endif /* STORY_DATAFILE */
#define MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE		"ui_key_view_tab_marquee"
#define MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT		"ui_key_view_tab_screenshot"
#define MUIOPTION_UI_KEY_VIEW_TAB_TITLE			"ui_key_view_tab_title"
#define MUIOPTION_UI_KEY_VIEW_TAB_PCB   		"ui_key_view_tab_pcb"
#define MUIOPTION_UI_KEY_QUIT				"ui_key_quit"
#define MUIOPTION_UI_JOY_UP				"ui_joy_up"
#define MUIOPTION_UI_JOY_DOWN				"ui_joy_down"
#define MUIOPTION_UI_JOY_LEFT				"ui_joy_left"
#define MUIOPTION_UI_JOY_RIGHT				"ui_joy_right"
#define MUIOPTION_UI_JOY_START				"ui_joy_start"
#define MUIOPTION_UI_JOY_PGUP				"ui_joy_pgup"
#define MUIOPTION_UI_JOY_PGDWN				"ui_joy_pgdwn"
#define MUIOPTION_UI_JOY_HOME				"ui_joy_home"
#define MUIOPTION_UI_JOY_END				"ui_joy_end"
#define MUIOPTION_UI_JOY_SS_CHANGE			"ui_joy_ss_change"
#define MUIOPTION_UI_JOY_HISTORY_UP			"ui_joy_history_up"
#define MUIOPTION_UI_JOY_HISTORY_DOWN			"ui_joy_history_down"
#define MUIOPTION_UI_JOY_EXEC				"ui_joy_exec"
#define MUIOPTION_EXEC_COMMAND				"exec_command"
#define MUIOPTION_EXEC_WAIT				"exec_wait"
#define MUIOPTION_HIDE_MOUSE				"hide_mouse"
#define MUIOPTION_FULL_SCREEN				"full_screen"

#ifdef MESS
// Options names
#define MUIOPTION_DEFAULT_GAME				"default_system"
#define MUIOPTION_HISTORY_FILE				"sysinfo_file"
#define MUIOPTION_MAMEINFO_FILE				"messinfo_file"
// Option values
#define MUIDEFAULT_SELECTION				"nes"
#define MUIDEFAULT_SPLITTERS				"152,310,468"
#define MUIHISTORY_FILE					"sysinfo.dat"
#define MUIMAMEINFO_FILE				"messinfo.dat"
#else
// Options names
#define MUIOPTION_DEFAULT_GAME				"default_game"
//#define MUIOPTION_HISTORY_FILE			"history_file"
//#define MUIOPTION_MAMEINFO_FILE			"mameinfo_file"
// Options values
#define MUIDEFAULT_SELECTION				"puckman"
#define MUIDEFAULT_SPLITTERS				"152,362"
#define MUIHISTORY_FILE					"history.dat"
#define MUIMAMEINFO_FILE				"mameinfo.dat"
#ifdef STORY_DATAFILE
#define MUISTORY_FILE   				"story.dat"
#endif /* STORY_DATAFILE */
#endif

#define MUIOPTION_VERSION				"version"
#define MUIOPTION_EXE_NAME				"exe_name"


/***************************************************************************
    Internal structures
 ***************************************************************************/

 /***************************************************************************
    Internal variables
 ***************************************************************************/

//static object_pool *options_memory_pool;

static winui_options settings;

static windows_options global;			// Global 'default' options

static game_options game_opts;

// UI options in mameui.ini
const options_entry winui_options::s_option_entries[] =
{
	// UI options
	{ NULL,                                           NULL,                         OPTION_HEADER,     "APPLICATION OPTIONS" },
	{ MUIOPTION_EXE_NAME,                             "",                           OPTION_STRING,     NULL },
	{ MUIOPTION_VERSION,                              "",                           OPTION_STRING,     NULL },
#ifdef DRIVER_SWITCH
	{ OPTION_DRIVER_CONFIG,                           "all",                        OPTION_STRING,     "switch drivers"},
	{ OPTION_DISABLE_MECHANICAL_DRIVER,               "0",                          OPTION_BOOLEAN,    "disable mechanical drivers"},
#endif /* DRIVER_SWITCH */

	{ NULL,                                           NULL,                         OPTION_HEADER,     "DISPLAY STATE OPTIONS" },
	{ MUIOPTION_DEFAULT_GAME,                         MUIDEFAULT_SELECTION,         OPTION_STRING,     NULL },
//	{ MUIOPTION_DEFAULT_GAME,                         "puckman",                    OPTION_STRING,     NULL },
	{ MUIOPTION_DEFAULT_FOLDER_ID,                    "0",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_SHOW_IMAGE_SECTION,                   "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_FULL_SCREEN,                          "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_CURRENT_TAB,                          "0",                          OPTION_STRING,     NULL },
	{ MUIOPTION_SHOW_TOOLBAR,                         "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_SHOW_STATUS_BAR,                      "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_HIDE_FOLDERS,                         "",                           OPTION_STRING,     NULL },
#ifdef MESS
	{ MUIOPTION_SHOW_FOLDER_SECTION,                  "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_SHOW_TABS,                            "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_HIDE_TABS,                            "flyer, cabinet, marquee, title, cpanel, pcb", OPTION_STRING, NULL },
	{ MUIOPTION_HISTORY_TAB,                          "1",                          OPTION_INTEGER,    NULL },
#else
	{ MUIOPTION_SHOW_FOLDER_SECTION,                  "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_SHOW_TABS,                            "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_HIDE_TABS,                            "marquee, title, cpanel, pcb, history", OPTION_STRING, NULL },
	{ MUIOPTION_HISTORY_TAB,                          "0",                          OPTION_INTEGER,    NULL },
#endif

	{ MUIOPTION_SORT_COLUMN,                          "0",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_SORT_REVERSED,                        "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_WINDOW_X,                             "0",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_WINDOW_Y,                             "0",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_WINDOW_WIDTH,                         "800",                        OPTION_INTEGER,    NULL },
	{ MUIOPTION_WINDOW_HEIGHT,                        "600",                        OPTION_INTEGER,    NULL },
	{ MUIOPTION_WINDOW_STATE,                         "1",                          OPTION_INTEGER,    NULL },

	{ MUIOPTION_TEXT_COLOR,                           "-1",                         OPTION_INTEGER,    NULL },
	{ MUIOPTION_CLONE_COLOR,                          "-1",                         OPTION_INTEGER,    NULL },
	{ MUIOPTION_CUSTOM_COLOR,                         "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", OPTION_STRING,NULL },
	/* ListMode needs to be before ColumnWidths settings */
	{ MUIOPTION_LIST_MODE,                            "5",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_SPLITTERS,                            MUIDEFAULT_SPLITTERS,         OPTION_STRING,     NULL },
	{ MUIOPTION_LIST_FONT,                            "-8,0,0,0,400,0,0,0,0,0,0,0,0", OPTION_STRING,   NULL },
	{ MUIOPTION_LIST_FONTFACE,                        "MS Sans Serif",              OPTION_STRING,     NULL },
	{ MUIOPTION_COLUMN_WIDTHS,                        "185,78,84,84,64,88,74,108,60,144,84,60,60", OPTION_STRING, NULL },
	{ MUIOPTION_COLUMN_ORDER,                         "0,1,3,4,5,6,7,8,9,10,11,12,2", OPTION_STRING,   NULL },
	{ MUIOPTION_COLUMN_SHOWN,                         "1,1,0,1,1,1,1,1,1,1,1,1,1",  OPTION_STRING,     NULL },

	{ NULL,                                           NULL,                         OPTION_HEADER,     "INTERFACE OPTIONS" },
#if 0 //mamep
	{ MUIOPTION_LANGUAGE,                             "english",                    OPTION_STRING,     NULL },
#endif
	{ MUIOPTION_CHECK_GAME,                           "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_JOYSTICK_IN_INTERFACE,                "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_KEYBOARD_IN_INTERFACE,                "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_RANDOM_BACKGROUND,                    "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_BROADCAST_GAME_NAME,                  "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_HIDE_MOUSE,                           "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_INHERIT_FILTER,                       "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_OFFSET_CLONES,                        "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_STRETCH_SCREENSHOT_LARGER,            "0",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_CYCLE_SCREENSHOT,                     "0",                          OPTION_INTEGER,    NULL },
	{ MUIOPTION_SCREENSHOT_BORDER_SIZE,               "11",                         OPTION_INTEGER,    NULL },
	{ MUIOPTION_SCREENSHOT_BORDER_COLOR,              "-1",                         OPTION_INTEGER,    NULL },
	{ MUIOPTION_EXEC_COMMAND,                         "",                           OPTION_STRING,     NULL },
	{ MUIOPTION_EXEC_WAIT,                            "0",                          OPTION_INTEGER,    NULL },
#ifdef USE_SHOW_SPLASH_SCREEN
	{ MUIOPTION_DISPLAY_SPLASH_SCREEN,                "1",                          OPTION_BOOLEAN,    NULL },
#endif /* USE_SHOW_SPLASH_SCREEN */
#ifdef TREE_SHEET
	{ MUIOPTION_SHOW_TREE_SHEET,                      "1",                          OPTION_BOOLEAN,    NULL },
#endif /* TREE_SHEET */
	{ MUIOPTION_BROKEN_COLOR,                         "202",                        OPTION_INTEGER,    NULL },
	{ MUIOPTION_USE_BROKEN_ICON,                      "1",                          OPTION_BOOLEAN,    NULL },
	{ MUIOPTION_FOLDER_FLAG,                          NULL,                         OPTION_STRING,     NULL },

	{ NULL,                                           NULL,                         OPTION_HEADER,     "SEARCH PATH OPTIONS" },
	{ MUIOPTION_FLYER_DIRECTORY,                      "flyers",                     OPTION_STRING,     NULL },
	{ MUIOPTION_CABINET_DIRECTORY,                    "cabinets",                   OPTION_STRING,     NULL },
	{ MUIOPTION_MARQUEE_DIRECTORY,                    "marquees",                   OPTION_STRING,     NULL },
	{ MUIOPTION_TITLE_DIRECTORY,                      "titles",                     OPTION_STRING,     NULL },
	{ MUIOPTION_CPANEL_DIRECTORY,                     "cpanel",                     OPTION_STRING,     NULL },
	{ MUIOPTION_PCB_DIRECTORY,                        "pcb",                        OPTION_STRING,     NULL },
	{ MUIOPTION_BACKGROUND_DIRECTORY,                 "bkground",                   OPTION_STRING,     NULL },
	{ MUIOPTION_FOLDER_DIRECTORY,                     "folders",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_DIRECTORY,                         "ui",                         OPTION_STRING,     NULL },
	{ MUIOPTION_ICONS_DIRECTORY,                      "icons",                      OPTION_STRING,     NULL },
#ifdef USE_VIEW_PCBINFO
	{ MUIOPTION_PCBINFO_DIRECTORY,                    "pcbinfo",                    OPTION_STRING,     NULL },
#endif /* USE_VIEW_PCBINFO */


	{ NULL,                                           NULL,                         OPTION_HEADER,     "FILENAME OPTIONS" },
	{ MUIOPTION_HISTORY_FILE,                         MUIHISTORY_FILE,              OPTION_STRING,     NULL },
	{ MUIOPTION_MAMEINFO_FILE,                        MUIMAMEINFO_FILE,             OPTION_STRING,     NULL },
#ifdef STORY_DATAFILE
	{ MUIOPTION_STORY_FILE,                           MUISTORY_FILE,                OPTION_STRING,     NULL },
#endif /* STORY_DATAFILE */

	{ NULL,                                           NULL,                         OPTION_HEADER,     "NAVIGATION KEY CODES" },
	{ MUIOPTION_UI_KEY_UP,                            "KEYCODE_UP",                 OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_DOWN,                          "KEYCODE_DOWN",               OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_LEFT,                          "KEYCODE_LEFT",               OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_RIGHT,                         "KEYCODE_RIGHT",              OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_START,                         "KEYCODE_ENTER NOT KEYCODE_LALT", OPTION_STRING, NULL },
	{ MUIOPTION_UI_KEY_PGUP,                          "KEYCODE_PGUP",               OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_PGDWN,                         "KEYCODE_PGDN",               OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_HOME,                          "KEYCODE_HOME",               OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_END,                           "KEYCODE_END",                OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_SS_CHANGE,                     "KEYCODE_INSERT",             OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_HISTORY_UP,                    "KEYCODE_DEL",                OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_HISTORY_DOWN,                  "KEYCODE_LALT KEYCODE_0",     OPTION_STRING,     NULL },

	{ MUIOPTION_UI_KEY_CONTEXT_FILTERS,               "KEYCODE_LCONTROL KEYCODE_F", OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_SELECT_RANDOM,                 "KEYCODE_LCONTROL KEYCODE_R", OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_GAME_AUDIT,                    "KEYCODE_LALT KEYCODE_A",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_GAME_PROPERTIES,               "KEYCODE_LALT KEYCODE_ENTER", OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_HELP_CONTENTS,                 "KEYCODE_F1",                 OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_UPDATE_GAMELIST,               "KEYCODE_F5",                 OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_FOLDERS,                  "KEYCODE_LALT KEYCODE_D",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_FULLSCREEN,               "KEYCODE_F11",                OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_PAGETAB,                  "KEYCODE_LALT KEYCODE_B",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_PICTURE_AREA,             "KEYCODE_LALT KEYCODE_P",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_STATUS,                   "KEYCODE_LALT KEYCODE_S",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TOOLBARS,                 "KEYCODE_LALT KEYCODE_T",     OPTION_STRING,     NULL },

	{ MUIOPTION_UI_KEY_VIEW_TAB_CABINET,              "KEYCODE_LALT KEYCODE_3",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_CPANEL,               "KEYCODE_LALT KEYCODE_6",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_FLYER,                "KEYCODE_LALT KEYCODE_2",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_HISTORY,              "KEYCODE_LALT KEYCODE_8",     OPTION_STRING,     NULL },
#ifdef STORY_DATAFILE
	{ MUIOPTION_UI_KEY_VIEW_TAB_STORY,                "KEYCODE_LALT KEYCODE_9",     OPTION_STRING,     NULL },
#endif /* STORY_DATAFILE */
	{ MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE,              "KEYCODE_LALT KEYCODE_4",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT,           "KEYCODE_LALT KEYCODE_1",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_TITLE,                "KEYCODE_LALT KEYCODE_5",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_VIEW_TAB_PCB,                  "KEYCODE_LALT KEYCODE_7",     OPTION_STRING,     NULL },
	{ MUIOPTION_UI_KEY_QUIT,                          "KEYCODE_LALT KEYCODE_Q",     OPTION_STRING,     NULL },

	{ NULL,                                           NULL,                         OPTION_HEADER,     "NAVIGATION JOYSTICK CODES" },
	{ MUIOPTION_UI_JOY_UP,                            "1,1,1,1",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_DOWN,                          "1,1,1,2",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_LEFT,                          "1,1,2,1",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_RIGHT,                         "1,1,2,2",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_START,                         "1,0,1,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_PGUP,                          "2,1,2,1",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_PGDWN,                         "2,1,2,2",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_HOME,                          "0,0,0,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_END,                           "0,0,0,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_SS_CHANGE,                     "2,0,3,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_HISTORY_UP,                    "2,0,4,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_HISTORY_DOWN,                  "2,0,1,0",                    OPTION_STRING,     NULL },
	{ MUIOPTION_UI_JOY_EXEC,                          "0,0,0,0",                    OPTION_STRING,     NULL },

#ifndef MESS
	{ NULL,                                           NULL,                         OPTION_HEADER,     "GAME STATISTICS" },
#endif
	{ NULL }
};

#define MESS_MARK_CONSOLE_ONLY	"Console"

static const options_entry filterOptions[] =
{
	// filters
	{ "_filters",                                     "0",                          OPTION_INTEGER,    NULL },
	{ NULL }
};



// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)
static const WCHAR *const image_tabs_long_name[MAX_TAB_TYPES] =
{
	L"Snapshot",
	L"Flyer",
	L"Cabinet",
	L"Marquee",
	L"Title",
	L"Control Panel",
	L"PCB",
	L"History",
#ifdef STORY_DATAFILE
	L"Story",
#endif /* STORY_DATAFILE */
};

static const char *const image_tabs_short_name[MAX_TAB_TYPES] =
{
	"snapshot",
	"flyer",
	"cabinet",
	"marquee",
	"title",
	"cpanel",
	"pcb",
	"history"
#ifdef STORY_DATAFILE
	,"story"
#endif /* STORY_DATAFILE */
};


static HANDLE hOptsMutex = NULL;

#define MUTEX_STR TEXT(MAMEUINAME "PLUS_OPTION_MUTEX")

/***************************************************************************
    External functions
 ***************************************************************************/
winui_options::winui_options()
{
	add_entries(s_option_entries);
}

/*static void memory_error(const char *message)
{
	win_message_box_utf8(NULL, message, NULL, MB_OK);
	exit(-1);
}
*/
/*

void AddOptions(winui_options *opts, const options_entry *entrylist, BOOL is_global)
{
	static const char *blacklist[] =
	{
		WINOPTION_SCREEN,
		WINOPTION_ASPECT,
		WINOPTION_RESOLUTION,
		WINOPTION_VIEW
	};
	options_entry entries[2];
	BOOL good_option;
	int i;

	for ( ; entrylist->name != NULL || (entrylist->flags & OPTION_HEADER); entrylist++)
	{
		good_option = TRUE;

		// check blacklist
		//mamep: blacklist is disalbed
		if (0 && entrylist->name)
		{
			for (i = 0; i < ARRAY_LENGTH(blacklist); i++)
			{
				int len = strlen(blacklist[i]);

				if (strncmp(entrylist->name, blacklist[i], len) == 0
				 && (entrylist->name[len] == '\0' || entrylist->name[len] == ';'))
				{
					good_option = FALSE;
					break;
				}
			}
		}

		// if is_global is FALSE, blacklist global options
		if (entrylist->name && !is_global && IsGlobalOption(entrylist->name))
			good_option = FALSE;

		if (good_option)
		{
			memcpy(entries, entrylist, sizeof(options_entry));
			memset(&entries[1], 0, sizeof(entries[1]));
			opts->add_entries(entries);
		}
	}
}

*/

void CreateGameOptions(windows_options &opts, int driver_index)
{
	BOOL is_global = (driver_index == OPTIONS_TYPE_GLOBAL);
	// create the options
	//opts = options_create(memory_error);

	// add the options
	//AddOptions(opts, mame_winui_options, is_global);
	//AddOptions(opts, mame_win_options, is_global);

	// customize certain options
	if (is_global)
		opts.set_default_value(OPTION_INIPATH, "ini");
}



BOOL OptionsInit()
{
 	//mamep: for save/load mameui.ini
 	hOptsMutex = CreateMutex(NULL , FALSE , MUTEX_STR);

	// create a memory pool for our data
	//options_memory_pool = pool_alloc_lib(memory_error);
	//if (!options_memory_pool)
//		return FALSE;

	// set up the MAME32 settings (these get placed in MAME32ui.ini
	//settings = options_create(memory_error);
	//options_add_entries(settings, regSettings);

	// set up per game options
	game_opts.add_entries();

	// set up global options
	CreateGameOptions(global,OPTIONS_TYPE_GLOBAL);
	lang_set_langcode(global, UI_LANG_EN_US);
	// now load the options and settings
	LoadOptionsAndSettings();

	return TRUE;

}

void OptionsExit(void)
{
	//mamep: close mutex handle
	if (hOptsMutex) CloseHandle(hOptsMutex);

	// free global options
	//options_free(global);
	//global = NULL;

	// free settings
	//options_free(settings);
	//settings = NULL;

	// free the memory pool
	//pool_free_lib(options_memory_pool);
	//options_memory_pool = NULL;
}

winui_options & MameUISettings(void)
{
	return settings;
}

windows_options & MameUIGlobal(void)
{
	return global;
}

// Restore ui settings to factory
void ResetGUI(void)
{
	settings.revert(OPTION_PRIORITY_NORMAL);
	// Save the new MAME32ui.ini
	SaveOptions();
	SetLangcode(GetLangcode());
	SetUseLangList(UseLangList());
}

const WCHAR * GetImageTabLongName(int tab_index)
{
	assert(tab_index >= 0);
	assert(tab_index < ARRAY_LENGTH(image_tabs_long_name));
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	assert(tab_index >= 0);
	assert(tab_index < ARRAY_LENGTH(image_tabs_short_name));
	return image_tabs_short_name[tab_index];
}

//============================================================
//  OPTIONS WRAPPERS
//============================================================

static COLORREF options_get_color(winui_options &opts, const char *name)
{
	const char *value_str;
	unsigned int r, g, b;
	COLORREF value;

	value_str = opts.value(name);

	if (sscanf(value_str, "%u,%u,%u", &r, &g, &b) == 3)
		value = RGB(r,g,b);
	else
		value = (COLORREF) - 1;
	return value;
}

static void options_set_color(winui_options &opts, const char *name, COLORREF value)
{
	char value_str[32];

	if (value == (COLORREF) -1)
	{
		snprintf(value_str, ARRAY_LENGTH(value_str), "%d", (int) value);
	}
	else
	{
		snprintf(value_str, ARRAY_LENGTH(value_str), "%d,%d,%d",
			(((int) value) >>  0) & 0xFF,
			(((int) value) >>  8) & 0xFF,
			(((int) value) >> 16) & 0xFF);
	}
	std::string error_string;
	opts.set_value(name, value_str, OPTION_PRIORITY_CMDLINE, error_string);
}

static COLORREF options_get_color_default(winui_options &opts, const char *name, int default_color)
{
	COLORREF value = options_get_color(opts, name);
	if (value == (COLORREF) -1)
		value = GetSysColor(default_color);
	return value;
}

static void options_set_color_default(winui_options &opts, const char *name, COLORREF value, int default_color)
{
	if (value == GetSysColor(default_color))
		options_set_color(settings, name, (COLORREF) -1);
	else
		options_set_color(settings, name, value);
}

static input_seq *options_get_input_seq(winui_options &opts, const char *name)
{
/*	static input_seq seq;
	const char *seq_string;

	seq_string = opts.value(name);
	input_seq_from_tokens(NULL, seq_string, &seq);   // HACK
	return &seq;*/
	return NULL;
}



//============================================================
//  OPTIONS CALLS
//============================================================

void SetViewMode(int val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_LIST_MODE, val, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetViewMode(void)
{
	return settings.int_value(MUIOPTION_LIST_MODE);
}

void SetGameCheck(BOOL game_check)
{
	std::string error_string;
	settings.set_value(MUIOPTION_CHECK_GAME, game_check, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetGameCheck(void)
{
	return settings.bool_value(MUIOPTION_CHECK_GAME);
}

void SetJoyGUI(BOOL use_joygui)
{
	std::string error_string;
	settings.set_value(MUIOPTION_JOYSTICK_IN_INTERFACE, use_joygui, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetJoyGUI(void)
{
	return settings.bool_value(MUIOPTION_JOYSTICK_IN_INTERFACE);
}

void SetKeyGUI(BOOL use_keygui)
{
	std::string error_string;
	settings.set_value(MUIOPTION_KEYBOARD_IN_INTERFACE, use_keygui, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetKeyGUI(void)
{
	return settings.bool_value(MUIOPTION_KEYBOARD_IN_INTERFACE);
}

void SetCycleScreenshot(int cycle_screenshot)
{
	std::string error_string;
	settings.set_value(MUIOPTION_CYCLE_SCREENSHOT, cycle_screenshot, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetCycleScreenshot(void)
{
	return settings.int_value(MUIOPTION_CYCLE_SCREENSHOT);
}

void SetStretchScreenShotLarger(BOOL stretch)
{
	std::string error_string;
	settings.set_value(MUIOPTION_STRETCH_SCREENSHOT_LARGER, stretch, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetStretchScreenShotLarger(void)
{
	return settings.bool_value(MUIOPTION_STRETCH_SCREENSHOT_LARGER);
}

void SetScreenshotBorderSize(int size)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SCREENSHOT_BORDER_SIZE, size, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetScreenshotBorderSize(void)
{
	return settings.int_value(MUIOPTION_SCREENSHOT_BORDER_SIZE);
}

void SetScreenshotBorderColor(COLORREF uColor)
{
	options_set_color_default(settings, MUIOPTION_SCREENSHOT_BORDER_COLOR, uColor, COLOR_3DFACE);
}

COLORREF GetScreenshotBorderColor(void)
{
	return options_get_color_default(settings, MUIOPTION_SCREENSHOT_BORDER_COLOR, COLOR_3DFACE);
}

void SetFilterInherit(BOOL inherit)
{
	std::string error_string;
	settings.set_value(MUIOPTION_INHERIT_FILTER, inherit, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetFilterInherit(void)
{
	return settings.bool_value(MUIOPTION_INHERIT_FILTER);
}

void SetOffsetClones(BOOL offset)
{
	std::string error_string;
	settings.set_value(MUIOPTION_OFFSET_CLONES, offset, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetOffsetClones(void)
{
	return settings.bool_value(MUIOPTION_OFFSET_CLONES);
}

void SetBroadcast(BOOL broadcast)
{
	std::string error_string;
	settings.set_value(MUIOPTION_BROADCAST_GAME_NAME, broadcast, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetBroadcast(void)
{
	return settings.bool_value(MUIOPTION_BROADCAST_GAME_NAME);
}

void SetRandomBackground(BOOL random_bg)
{
	std::string error_string;
	settings.set_value(MUIOPTION_RANDOM_BACKGROUND, random_bg, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetRandomBackground(void)
{
	return settings.bool_value(MUIOPTION_RANDOM_BACKGROUND);
}

void SetSavedFolderID(UINT val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_DEFAULT_FOLDER_ID, (int) val, OPTION_PRIORITY_CMDLINE, error_string);
}

UINT GetSavedFolderID(void)
{
	return (UINT) settings.int_value(MUIOPTION_DEFAULT_FOLDER_ID);
}

void SetShowScreenShot(BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_IMAGE_SECTION, val, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetShowScreenShot(void)
{
	return settings.bool_value(MUIOPTION_SHOW_IMAGE_SECTION);
}

void SetShowFolderList(BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_FOLDER_SECTION, val, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetShowFolderList(void)
{
	return settings.bool_value(MUIOPTION_SHOW_FOLDER_SECTION);
}

static void GetsShowFolderFlags(LPBITS bits)
{
	char s[2000];
	extern const FOLDERDATA g_folderData[];
	char *token;

	snprintf(s, ARRAY_LENGTH(s), "%s", settings.value(MUIOPTION_HIDE_FOLDERS));

	SetAllBits(bits, TRUE);

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
		{
			if (strcmp(g_folderData[j].short_name,token) == 0)
			{
				ClearBit(bits, g_folderData[j].m_nFolderId);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}
}

BOOL GetShowFolder(int folder)
{
	BOOL result;
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	GetsShowFolderFlags(show_folder_flags);
	result = TestBit(show_folder_flags, folder);
	DeleteBits(show_folder_flags);
	return result;
}

void SetShowFolder(int folder,BOOL show)
{
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	int i;
	int num_saved = 0;
	char str[10000];
	extern const FOLDERDATA g_folderData[];

	GetsShowFolderFlags(show_folder_flags);

	if (show)
		SetBit(show_folder_flags, folder);
	else
		ClearBit(show_folder_flags, folder);

	strcpy(str, "");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (TestBit(show_folder_flags, i) == FALSE)
		{
			int j;

			if (num_saved != 0)
				strcat(str,", ");

			for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
			{
				if (g_folderData[j].m_nFolderId == i)
				{
					strcat(str,g_folderData[j].short_name);
					num_saved++;
					break;
				}
			}
		}
	}
	std::string error_string;
	settings.set_value(MUIOPTION_HIDE_FOLDERS, str, OPTION_PRIORITY_CMDLINE, error_string);
	DeleteBits(show_folder_flags);
}

void SetShowStatusBar(BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_STATUS_BAR, val, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetShowStatusBar(void)
{
	return settings.bool_value(MUIOPTION_SHOW_STATUS_BAR);
}

void SetShowTabCtrl (BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_TABS, val, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetShowTabCtrl (void)
{
	return settings.bool_value(MUIOPTION_SHOW_TABS);
}

void SetShowToolBar(BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_TOOLBAR, val, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetShowToolBar(void)
{
	return settings.bool_value(MUIOPTION_SHOW_TOOLBAR);
}

void SetCurrentTab(const char *shortname)
{
	std::string error_string;
	settings.set_value(MUIOPTION_CURRENT_TAB, shortname, OPTION_PRIORITY_CMDLINE, error_string);
}

const char *GetCurrentTab(void)
{
	return settings.value(MUIOPTION_CURRENT_TAB);
}

void SetDefaultGame(const char *name)
{
	std::string error_string;
	settings.set_value(MUIOPTION_DEFAULT_GAME, name, OPTION_PRIORITY_CMDLINE, error_string);
}

const char *GetDefaultGame(void)
{
	return settings.value(MUIOPTION_DEFAULT_GAME);
}

void SetWindowArea(const AREA *area)
{
	std::string error_string;
	settings.set_value(MUIOPTION_WINDOW_X,		area->x, OPTION_PRIORITY_CMDLINE, error_string);
	settings.set_value(MUIOPTION_WINDOW_Y,		area->y, OPTION_PRIORITY_CMDLINE, error_string);
	settings.set_value(MUIOPTION_WINDOW_WIDTH,	area->width, OPTION_PRIORITY_CMDLINE, error_string);
	settings.set_value(MUIOPTION_WINDOW_HEIGHT,	area->height, OPTION_PRIORITY_CMDLINE, error_string);
}

void GetWindowArea(AREA *area)
{
	area->x      = settings.int_value(MUIOPTION_WINDOW_X);
	area->y      = settings.int_value(MUIOPTION_WINDOW_Y);
	area->width  = settings.int_value(MUIOPTION_WINDOW_WIDTH);
	area->height = settings.int_value(MUIOPTION_WINDOW_HEIGHT);
}

void SetWindowState(UINT state)
{
	std::string error_string;
	settings.set_value(MUIOPTION_WINDOW_STATE, (int)state, OPTION_PRIORITY_CMDLINE, error_string);
}

UINT GetWindowState(void)
{
	return settings.int_value(MUIOPTION_WINDOW_STATE);
}

void SetCustomColor(int iIndex, COLORREF uColor)
{
	const char *custom_color_string;
	COLORREF custom_color[256];
	char buffer[10000];

	custom_color_string = settings.value(MUIOPTION_CUSTOM_COLOR);
	CusColorDecodeString(custom_color_string, custom_color);

	custom_color[iIndex] = uColor;

	CusColorEncodeString(custom_color, buffer);
	std::string error_string;
	settings.set_value(MUIOPTION_CUSTOM_COLOR, buffer, OPTION_PRIORITY_CMDLINE, error_string);
}

COLORREF GetCustomColor(int iIndex)
{
	const char *custom_color_string;
	COLORREF custom_color[256];

	custom_color_string = settings.value(MUIOPTION_CUSTOM_COLOR);
	CusColorDecodeString(custom_color_string, custom_color);

	if (custom_color[iIndex] == (COLORREF)-1)
		return (COLORREF)RGB(0,0,0);

	return custom_color[iIndex];
}

void SetListFont(const LOGFONTW *font)
{
	char font_string[10000];
	FontEncodeString(font, font_string);
	std::string error_string;
	settings.set_value(MUIOPTION_LIST_FONT, font_string, OPTION_PRIORITY_CMDLINE, error_string);
	options_set_wstring(settings, MUIOPTION_LIST_FONTFACE, font->lfFaceName, OPTION_PRIORITY_CMDLINE);
}

void GetListFont(LOGFONTW *font)
{
	const char *font_string = settings.value(MUIOPTION_LIST_FONT);
	const WCHAR *stemp = options_get_wstring(settings, MUIOPTION_LIST_FONTFACE);

	FontDecodeString(font_string, font);

	if (stemp)
		snwprintf(font->lfFaceName, ARRAY_LENGTH(font->lfFaceName), L"%s", stemp);
}

void SetListFontColor(COLORREF uColor)
{
	options_set_color_default(settings, MUIOPTION_TEXT_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListFontColor(void)
{
	return options_get_color_default(settings, MUIOPTION_TEXT_COLOR, COLOR_WINDOWTEXT);
}

void SetListCloneColor(COLORREF uColor)
{
	options_set_color_default(settings, MUIOPTION_CLONE_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListCloneColor(void)
{
	return options_get_color_default(settings, MUIOPTION_CLONE_COLOR, COLOR_WINDOWTEXT);
}

int GetShowTab(int tab)
{
	const char *show_tabs_string;
	int show_tab_flags;

	show_tabs_string = settings.value(MUIOPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);
	return (show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab,BOOL show)
{
	const char *show_tabs_string;
	int show_tab_flags;
	char buffer[10000];

	show_tabs_string = settings.value(MUIOPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);

	if (show)
		show_tab_flags |= 1 << tab;
	else
		show_tab_flags &= ~(1 << tab);

	TabFlagsEncodeString(show_tab_flags, buffer);
	std::string error_string;
	settings.set_value(MUIOPTION_HIDE_TABS, buffer, OPTION_PRIORITY_CMDLINE, error_string);
}

// don't delete the last one
BOOL AllowedToSetShowTab(int tab,BOOL show)
{
	const char *show_tabs_string;
	int show_tab_flags;

	if (show == TRUE)
		return TRUE;

	show_tabs_string = settings.value(MUIOPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
}

int GetHistoryTab(void)
{
	return settings.int_value(MUIOPTION_HISTORY_TAB);
}

void SetHistoryTab(int tab, BOOL show)
{
	std::string error_string;
	if (show)
		settings.set_value(MUIOPTION_HISTORY_TAB, tab, OPTION_PRIORITY_CMDLINE, error_string);
	else
		settings.set_value(MUIOPTION_HISTORY_TAB, TAB_NONE, OPTION_PRIORITY_CMDLINE, error_string);
}

void SetColumnWidths(int width[])
{
	char column_width_string[10000];
	ColumnEncodeStringWithCount(width, column_width_string, COLUMN_MAX);
	std::string error_string;
	settings.set_value(MUIOPTION_COLUMN_WIDTHS, column_width_string, OPTION_PRIORITY_CMDLINE, error_string);
}

void GetColumnWidths(int width[])
{
	const char *column_width_string;
	column_width_string = settings.value(MUIOPTION_COLUMN_WIDTHS);
	ColumnDecodeStringWithCount(column_width_string, width, COLUMN_MAX);
}

void SetSplitterPos(int splitterId, int pos)
{
	const char *splitter_string;
	int *splitter;
	char buffer[10000];

	if (splitterId < GetSplitterCount())
	{
		splitter_string = settings.value(MUIOPTION_SPLITTERS);
		splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
		SplitterDecodeString(splitter_string, splitter);

		splitter[splitterId] = pos;

		SplitterEncodeString(splitter, buffer);
		std::string error_string;
		settings.set_value(MUIOPTION_SPLITTERS, buffer, OPTION_PRIORITY_CMDLINE, error_string);
	}
}

int  GetSplitterPos(int splitterId)
{
	const char *splitter_string;
	int *splitter;

	splitter_string = settings.value(MUIOPTION_SPLITTERS);
	splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
	SplitterDecodeString(splitter_string, splitter);

	if (splitterId < GetSplitterCount())
		return splitter[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	char column_order_string[10000];
	ColumnEncodeStringWithCount(order, column_order_string, COLUMN_MAX);
	std::string error_string;
	settings.set_value(MUIOPTION_COLUMN_ORDER, column_order_string, OPTION_PRIORITY_CMDLINE, error_string);
}

void GetColumnOrder(int order[])
{
	const char *column_order_string;
	column_order_string = settings.value(MUIOPTION_COLUMN_ORDER);
	ColumnDecodeStringWithCount(column_order_string, order, COLUMN_MAX);
}

void SetColumnShown(int shown[])
{
	char column_shown_string[10000];
	ColumnEncodeStringWithCount(shown, column_shown_string, COLUMN_MAX);
	std::string error_string;
	settings.set_value(MUIOPTION_COLUMN_SHOWN, column_shown_string, OPTION_PRIORITY_CMDLINE, error_string);
}

void GetColumnShown(int shown[])
{
	const char *column_shown_string;
	column_shown_string = settings.value(MUIOPTION_COLUMN_SHOWN);
	ColumnDecodeStringWithCount(column_shown_string, shown, COLUMN_MAX);
}

void SetSortColumn(int column)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SORT_COLUMN, column, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetSortColumn(void)
{
	return settings.int_value(MUIOPTION_SORT_COLUMN);
}

void SetSortReverse(BOOL reverse)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SORT_REVERSED, reverse, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetSortReverse(void)
{
	return settings.bool_value(MUIOPTION_SORT_REVERSED);
}

#if 0 //mamep
const char* GetLanguage(void)
{
	return settings.value(MUIOPTION_LANGUAGE);
}

void SetLanguage(const char* lang)
{
	std::string error_string;
	settings.set_value(MUIOPTION_LANGUAGE, lang, OPTION_PRIORITY_CMDLINE,error_string);
}
#endif

const WCHAR* GetRomDirs(void)
{
	return options_get_wstring(global, OPTION_MEDIAPATH);
}

void SetRomDirs(const WCHAR* paths)
{
	options_set_wstring(global, OPTION_MEDIAPATH, paths, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetSampleDirs(void)
{
	return options_get_wstring(global, OPTION_SAMPLEPATH);
}

void SetSampleDirs(const WCHAR* paths)
{
	options_set_wstring(global, OPTION_SAMPLEPATH, paths, OPTION_PRIORITY_CMDLINE);
}

const WCHAR * GetIniDir(void)
{
	const WCHAR *ini_dir;
	const WCHAR *s;

	ini_dir = options_get_wstring(global, OPTION_INIPATH);
	while((s = wcschr(ini_dir, ';')) != NULL)
	{
		ini_dir = s + 1;
	}
	return ini_dir;

}

void SetIniDir(const WCHAR *path)
{
	options_set_wstring(global, OPTION_INIPATH, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCtrlrDir(void)
{
	return options_get_wstring(global, OPTION_CTRLRPATH);
}

void SetCtrlrDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_CTRLRPATH, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCommentDir(void)
{
	return options_get_wstring(global, OPTION_COMMENT_DIRECTORY);
}

void SetCommentDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_COMMENT_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCfgDir(void)
{
	return options_get_wstring(global, OPTION_CFG_DIRECTORY);
}

void SetCfgDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_CFG_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetNvramDir(void)
{
	return options_get_wstring(global, OPTION_NVRAM_DIRECTORY);
}

void SetNvramDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_NVRAM_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetInpDir(void)
{
	return options_get_wstring(global, OPTION_INPUT_DIRECTORY);
}

void SetInpDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_INPUT_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetImgDir(void)
{
	return options_get_wstring(global, OPTION_SNAPSHOT_DIRECTORY);
}

void SetImgDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_SNAPSHOT_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetStateDir(void)
{
	return options_get_wstring(global, OPTION_STATE_DIRECTORY);
}

void SetStateDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_STATE_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetArtDir(void)
{
	return options_get_wstring(global, OPTION_ARTPATH);
}

void SetArtDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_ARTPATH, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetFontDir(void)
{
	return options_get_wstring(global, OPTION_FONTPATH);
}

void SetFontDir(const WCHAR* paths)
{
	options_set_wstring(global, OPTION_FONTPATH, paths, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCrosshairDir(void)
{
	return options_get_wstring(global, OPTION_CROSSHAIRPATH);
}

void SetCrosshairDir(const WCHAR* paths)
{
	options_set_wstring(global, OPTION_CROSSHAIRPATH, paths, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetFlyerDir(void)
{
	return options_get_wstring(settings, MUIOPTION_FLYER_DIRECTORY);
}

void SetFlyerDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_FLYER_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCabinetDir(void)
{
	return options_get_wstring(settings, MUIOPTION_CABINET_DIRECTORY);
}

void SetCabinetDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_CABINET_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetMarqueeDir(void)
{
	return options_get_wstring(settings, MUIOPTION_MARQUEE_DIRECTORY);
}

void SetMarqueeDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_MARQUEE_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetTitlesDir(void)
{
	return options_get_wstring(settings, MUIOPTION_TITLE_DIRECTORY);
}

void SetTitlesDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_TITLE_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR * GetControlPanelDir(void)
{
	return options_get_wstring(settings, MUIOPTION_CPANEL_DIRECTORY);
}

void SetControlPanelDir(const WCHAR *path)
{
	options_set_wstring(settings, MUIOPTION_CPANEL_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR * GetPcbDir(void)
{
	return options_get_wstring(settings, MUIOPTION_PCB_DIRECTORY);
}

void SetPcbDir(const WCHAR *path)
{
	options_set_wstring(settings, MUIOPTION_PCB_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR * GetDiffDir(void)
{
	return options_get_wstring(global, OPTION_DIFF_DIRECTORY);
}

void SetDiffDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_DIFF_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetIconsDir(void)
{
	return options_get_wstring(settings, MUIOPTION_ICONS_DIRECTORY);
}

void SetIconsDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_ICONS_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetBgDir (void)
{
	return options_get_wstring(settings, MUIOPTION_BACKGROUND_DIRECTORY);
}

void SetBgDir (const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_BACKGROUND_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetFolderDir(void)
{
	return options_get_wstring(settings, MUIOPTION_FOLDER_DIRECTORY);
}

void SetFolderDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_FOLDER_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetUIDir(void)
{
	return options_get_wstring(settings, MUIOPTION_UI_DIRECTORY);
}

void SetUIDir(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_UI_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetCheatDir(void)
{
	return options_get_wstring(global, OPTION_CHEATPATH);
}

void SetCheatDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_CHEATPATH, path, OPTION_PRIORITY_CMDLINE);
}

const WCHAR* GetHistoryFileName(void)
{
	return options_get_wstring(settings, MUIOPTION_HISTORY_FILE);
}

void SetHistoryFileName(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_HISTORY_FILE, path, OPTION_PRIORITY_CMDLINE);
}


const WCHAR* GetMAMEInfoFileName(void)
{
	return options_get_wstring(settings, MUIOPTION_MAMEINFO_FILE);
}

void SetMAMEInfoFileName(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_MAMEINFO_FILE, path, OPTION_PRIORITY_CMDLINE);
}

#ifdef STORY_DATAFILE
const WCHAR* GetStoryFileName(void)
{
	return options_get_wstring(settings, MUIOPTION_STORY_FILE);
}
void SetStoryFileName(const WCHAR* path)
{
	options_set_wstring(settings, MUIOPTION_STORY_FILE, path, OPTION_PRIORITY_CMDLINE);
}
#endif /* STORY_DATAFILE */
#ifdef USE_VIEW_PCBINFO
const WCHAR * GetPcbInfoDir(void)
{
	return options_get_wstring(settings, MUIOPTION_PCBINFO_DIRECTORY);
}
void SetPcbInfoDir(const WCHAR *path)
{
	options_set_wstring(settings, MUIOPTION_PCBINFO_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}
#endif /* USE_VIEW_PCBINFO */

const char* GetSnapName(void)
{
	return global.value(OPTION_SNAPNAME);
}

void SetSnapName(const char* pattern)
{
	std::string error_string;
	global.set_value(OPTION_SNAPNAME, pattern, OPTION_PRIORITY_CMDLINE, error_string);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < driver_list::total());

	//save_options(OPTIONS_GAME, NULL, driver_index);
}

void ResetGameDefaults(void)
{
	// Walk the global settings and reset everything to defaults;
	ResetToDefaults(global, OPTION_PRIORITY_MAXIMUM);
	save_options(OPTIONS_GLOBAL, global, 1);
}

/*
 * Reset all game, vector and source options to defaults.
 * No reason to reboot if this is done.
 */
void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < driver_list::total(); i++)
	{
		ResetGameOptions(i);
	}

	/* Walk the ini/source folder deleting all ini's. */
	remove_all_source_options();

	/* finally vector.ini */
	//save_options(OPTIONS_VECTOR, NULL, 0);
	/* finally horizont.ini */
	//save_options(OPTIONS_HORIZONTAL, NULL, 0);
	/* finally vertical.ini */
	//save_options(OPTIONS_VERTICAL, NULL, 0);
}

int GetRomAuditResults(int driver_index)
{
	return game_opts.rom(driver_index);
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	game_opts.rom(driver_index, audit_results);
}

int  GetSampleAuditResults(int driver_index)
{
	return game_opts.sample(driver_index);
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	game_opts.sample(driver_index, audit_results);
}

static void IncrementPlayVariable(int driver_index, const char *play_variable, int increment)
{
	int count;

	if (strcmp(play_variable, "count") == 0)
	{
		count = game_opts.play_count(driver_index);
		game_opts.play_count(driver_index, count + increment);
	}
	else if (strcmp(play_variable, "time") == 0)
	{
		count = game_opts.play_time(driver_index);
		game_opts.play_time(driver_index, count + increment);
	}
}

void IncrementPlayCount(int driver_index)
{
	IncrementPlayVariable(driver_index, "count", 1);
}

int GetPlayCount(int driver_index)
{
	return game_opts.play_count(driver_index);
}

static void ResetPlayVariable(int driver_index, const char *play_variable)
{
	int i;
	if (driver_index < 0)
	{
		/* all games */
		for (i = 0; i< driver_list::total(); i++)
		{
			/* 20070808 MSH - Was passing in driver_index instead of i. Doh! */
			ResetPlayVariable(i, play_variable);
		}
	}
	else
	{
		if (strcmp(play_variable, "count") == 0)
			game_opts.play_count(driver_index, 0);
		else if (strcmp(play_variable, "time") == 0)
			game_opts.play_time(driver_index, 0);
	}
}

void ResetPlayCount(int driver_index)
{
	ResetPlayVariable(driver_index, "count");
}

void ResetPlayTime(int driver_index)
{
	ResetPlayVariable(driver_index, "time");
}

int GetPlayTime(int driver_index)
{
	return game_opts.play_time(driver_index);
}

void IncrementPlayTime(int driver_index,int playtime)
{
	IncrementPlayVariable(driver_index, "time", playtime);
}

void GetTextPlayTime(int driver_index, WCHAR *buf)
{
	int hour, minute, second;
	int temp = GetPlayTime(driver_index);

	assert(0 <= driver_index && driver_index < driver_list::total());

	hour = temp / 3600;
	temp = temp - 3600*hour;
	minute = temp / 60; //Calc Minutes
	second = temp - 60*minute;

	if (hour == 0)
		swprintf(buf, L"%d:%02d", minute, second );
	else
		swprintf(buf, L"%d:%02d:%02d", hour, minute, second );
}

input_seq* Get_ui_key_up(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_UP);
}
input_seq* Get_ui_key_down(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_DOWN);
}
input_seq* Get_ui_key_left(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_LEFT);
}
input_seq* Get_ui_key_right(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_RIGHT);
}
input_seq* Get_ui_key_start(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_START);
}
input_seq* Get_ui_key_pgup(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_PGUP);
}
input_seq* Get_ui_key_pgdwn(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_PGDWN);
}
input_seq* Get_ui_key_home(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_HOME);
}
input_seq* Get_ui_key_end(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_END);
}
input_seq* Get_ui_key_ss_change(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_SS_CHANGE);
}
input_seq* Get_ui_key_history_up(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_HISTORY_UP);
}
input_seq* Get_ui_key_history_down(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_HISTORY_DOWN);
}


input_seq* Get_ui_key_context_filters(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_CONTEXT_FILTERS);
}
input_seq* Get_ui_key_select_random(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_SELECT_RANDOM);
}
input_seq* Get_ui_key_game_audit(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_GAME_AUDIT);
}
input_seq* Get_ui_key_game_properties(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_GAME_PROPERTIES);
}
input_seq* Get_ui_key_help_contents(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_HELP_CONTENTS);
}
input_seq* Get_ui_key_update_gamelist(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_UPDATE_GAMELIST);
}
input_seq* Get_ui_key_view_folders(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_FOLDERS);
}
input_seq* Get_ui_key_view_fullscreen(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_FULLSCREEN);
}
input_seq* Get_ui_key_view_pagetab(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_PAGETAB);
}
input_seq* Get_ui_key_view_picture_area(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_PICTURE_AREA);
}
input_seq* Get_ui_key_view_status(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_STATUS);
}
input_seq* Get_ui_key_view_toolbars(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TOOLBARS);
}

input_seq* Get_ui_key_view_tab_cabinet(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_CABINET);
}
input_seq* Get_ui_key_view_tab_cpanel(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_CPANEL);
}
input_seq* Get_ui_key_view_tab_flyer(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_FLYER);
}
input_seq* Get_ui_key_view_tab_history(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_HISTORY);
}
#ifdef STORY_DATAFILE
input_seq *Get_ui_key_view_tab_story(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_STORY);
}
#endif /* STORY_DATAFILE */
input_seq* Get_ui_key_view_tab_marquee(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE);
}
input_seq* Get_ui_key_view_tab_screenshot(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT);
}
input_seq* Get_ui_key_view_tab_title(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_TITLE);
}
input_seq* Get_ui_key_view_tab_pcb(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_VIEW_TAB_PCB);
}
input_seq* Get_ui_key_quit(void)
{
	return options_get_input_seq(settings, MUIOPTION_UI_KEY_QUIT);
}



static int GetUIJoy(const char *option_name, int joycodeIndex)
{
	const char *joycodes_string;
	int joycodes[4];

	assert(0 <= joycodeIndex && joycodeIndex < 4);
	joycodes_string = settings.value(option_name);
	ColumnDecodeStringWithCount(joycodes_string, joycodes, ARRAY_LENGTH(joycodes));
	return joycodes[joycodeIndex];
}

static void SetUIJoy(const char *option_name, int joycodeIndex, int val)
{
	const char *joycodes_string;
	int joycodes[4];
	char buffer[1024];

	assert(0 <= joycodeIndex && joycodeIndex < 4);
	joycodes_string = settings.value(option_name);
	ColumnDecodeStringWithCount(joycodes_string, joycodes, ARRAY_LENGTH(joycodes));

	joycodes[joycodeIndex] = val;
	ColumnEncodeStringWithCount(joycodes, buffer, ARRAY_LENGTH(joycodes));
	std::string error_string;
	settings.set_value(option_name, buffer, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetUIJoyUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_UP, joycodeIndex);
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_UP, joycodeIndex, val);
}

int GetUIJoyDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_DOWN, joycodeIndex);
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_DOWN, joycodeIndex, val);
}

int GetUIJoyLeft(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_LEFT, joycodeIndex);
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_LEFT, joycodeIndex, val);
}

int GetUIJoyRight(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_RIGHT, joycodeIndex);
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_RIGHT, joycodeIndex, val);
}

int GetUIJoyStart(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_START, joycodeIndex);
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_START, joycodeIndex, val);
}

int GetUIJoyPageUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_PGUP, joycodeIndex);
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_PGUP, joycodeIndex, val);
}

int GetUIJoyPageDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_PGDWN, joycodeIndex);
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_PGDWN, joycodeIndex, val);
}

int GetUIJoyHome(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HOME, joycodeIndex);
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HOME, joycodeIndex, val);
}

int GetUIJoyEnd(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_END, joycodeIndex);
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_END, joycodeIndex, val);
}

int GetUIJoySSChange(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_SS_CHANGE, joycodeIndex);
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_SS_CHANGE, joycodeIndex, val);
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HISTORY_UP, joycodeIndex);
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HISTORY_UP, joycodeIndex, val);
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HISTORY_DOWN, joycodeIndex);
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HISTORY_DOWN, joycodeIndex, val);
}

void SetUIJoyExec(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_EXEC, joycodeIndex, val);
}

int GetUIJoyExec(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_EXEC, joycodeIndex);
}

WCHAR * GetExecCommand(void)
{
	return options_get_wstring(settings, MUIOPTION_EXEC_COMMAND);
}

void SetExecCommand(WCHAR *cmd)
{
	options_set_wstring(settings, MUIOPTION_EXEC_COMMAND, cmd, OPTION_PRIORITY_CMDLINE);
}

int GetExecWait(void)
{
	return settings.int_value(MUIOPTION_EXEC_WAIT);
}

void SetExecWait(int wait)
{
	std::string error_string;
	settings.set_value(MUIOPTION_EXEC_WAIT, wait, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetHideMouseOnStartup(void)
{
	return settings.bool_value(MUIOPTION_HIDE_MOUSE);
}

void SetHideMouseOnStartup(BOOL hide)
{
	std::string error_string;
	settings.set_value(MUIOPTION_HIDE_MOUSE, hide, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetRunFullScreen(void)
{
	return settings.bool_value(MUIOPTION_FULL_SCREEN);
}

void SetRunFullScreen(BOOL fullScreen)
{
	std::string error_string;
	settings.set_value(MUIOPTION_FULL_SCREEN, fullScreen, OPTION_PRIORITY_CMDLINE, error_string);
}

int GetLangcode(void)
{
	return lang_get_langcode();
}

void SetLangcode(int langcode)
{
	/* apply to emulator core for datafile.c */
	lang_set_langcode(global, langcode);
	langcode = GetLangcode();

	std::string error_string;
	global.set_value(OPTION_LANGUAGE, ui_lang_info[langcode].name, OPTION_PRIORITY_CMDLINE, error_string);

	/* apply for osd core functions */
	set_osdcore_acp(ui_lang_info[langcode].codepage);

	InitTranslator(langcode);
}

BOOL UseLangList(void)
{
	return global.bool_value(OPTION_USE_LANG_LIST);
}

void SetUseLangList(BOOL is_use)
{
	std::string error_string;
	global.set_value(OPTION_USE_LANG_LIST, is_use, OPTION_PRIORITY_CMDLINE, error_string);

	/* apply to emulator core for datafile.c */
	lang_message_enable(UI_MSG_LIST, is_use);
	lang_message_enable(UI_MSG_MANUFACTURE, is_use);
}

const WCHAR* GetLanguageDir(void)
{
	return options_get_wstring(global, OPTION_LANGPATH);
}

void SetLanguageDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_LANGPATH, path, OPTION_PRIORITY_CMDLINE);
}

#ifdef USE_HISCORE
const WCHAR* GetHiDir(void)
{
	return options_get_wstring(global, OPTION_HISCORE_DIRECTORY);
}

void SetHiDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_HISCORE_DIRECTORY, path, OPTION_PRIORITY_CMDLINE);
}
#endif /* USE_HISCORE */

#ifdef USE_IPS
const WCHAR* GetIPSDir(void)
{
	return options_get_wstring(global, OPTION_IPSPATH);
}

void SetIPSDir(const WCHAR* path)
{
	options_set_wstring(global, OPTION_IPSPATH, path, OPTION_PRIORITY_CMDLINE);
}
#endif /* USE_IPS */

#ifdef UI_COLOR_DISPLAY
struct ui_palette_assign
{
	const char *name;
	int code;
};

static struct ui_palette_assign ui_palette_tbl[] =
{
	{ OPTION_SYSTEM_BACKGROUND,     SYSTEM_COLOR_BACKGROUND },
	{ OPTION_CURSOR_SELECTED_TEXT,  CURSOR_SELECTED_TEXT },
	{ OPTION_CURSOR_SELECTED_BG,    CURSOR_SELECTED_BG },
	{ OPTION_CURSOR_HOVER_TEXT,     CURSOR_HOVER_TEXT },
	{ OPTION_CURSOR_HOVER_BG,       CURSOR_HOVER_BG },
	{ OPTION_BUTTON_RED,            BUTTON_COLOR_RED },
	{ OPTION_BUTTON_YELLOW,         BUTTON_COLOR_YELLOW },
	{ OPTION_BUTTON_GREEN,          BUTTON_COLOR_GREEN },
	{ OPTION_BUTTON_BLUE,           BUTTON_COLOR_BLUE },
	{ OPTION_BUTTON_PURPLE,         BUTTON_COLOR_PURPLE },
	{ OPTION_BUTTON_PINK,           BUTTON_COLOR_PINK },
	{ OPTION_BUTTON_AQUA,           BUTTON_COLOR_AQUA },
	{ OPTION_BUTTON_SILVER,         BUTTON_COLOR_SILVER },
	{ OPTION_BUTTON_NAVY,           BUTTON_COLOR_NAVY },
	{ OPTION_BUTTON_LIME,           BUTTON_COLOR_LIME }
};

const char *GetUIPaletteString(int n)
{
	if (n < 0 || n >= ARRAY_LENGTH(ui_palette_tbl))
		return NULL;

	return global.value(ui_palette_tbl[n].name);
}

void SetUIPaletteString(int n, const char *s)
{
	if (n < 0 || n >= ARRAY_LENGTH(ui_palette_tbl))
		return;

	std::string error_string;
	global.set_value(ui_palette_tbl[n].name, s, OPTION_PRIORITY_CMDLINE, error_string);
}
#endif /* UI_COLOR_DISPLAY */

BOOL FolderHasVector(const WCHAR *name)
{
	int i;

	for (i = 0; i < driver_list::total(); i++)
		if (DriverIsVector(i) && wcscmp(name, GetDriverFilename(i)) == 0)
			return TRUE;

	return FALSE;
}

COLORREF GetListBrokenColor(void)
{
	COLORREF broken_color = (COLORREF)settings.int_value(MUIOPTION_BROKEN_COLOR);

	if (broken_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return broken_color;
}

void SetListBrokenColor(COLORREF uColor)
{
	COLORREF broken_color = GetListBrokenColor();

	if (broken_color == GetSysColor(COLOR_WINDOWTEXT))
		broken_color = (COLORREF)-1;
	else
		broken_color = uColor;

	std::string error_string;
	settings.set_value(MUIOPTION_BROKEN_COLOR, (int)broken_color, OPTION_PRIORITY_CMDLINE, error_string);
}

void SetUseBrokenIcon(BOOL use_broken_icon)
{
	std::string error_string;
	settings.set_value(MUIOPTION_USE_BROKEN_ICON, use_broken_icon, OPTION_PRIORITY_CMDLINE, error_string);
}

BOOL GetUseBrokenIcon(void)
{
	return settings.bool_value(MUIOPTION_USE_BROKEN_ICON);
}

#ifdef USE_SHOW_SPLASH_SCREEN
BOOL GetDisplaySplashScreen (void)
{
	return settings.bool_value(MUIOPTION_DISPLAY_SPLASH_SCREEN);
}

void SetDisplaySplashScreen (BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_DISPLAY_SPLASH_SCREEN, val, OPTION_PRIORITY_CMDLINE, error_string);
}
#endif /* USE_SHOW_SPLASH_SCREEN */

#ifdef TREE_SHEET
BOOL GetShowTreeSheet(void)
{
	return settings.bool_value(MUIOPTION_SHOW_TREE_SHEET);
}

void SetShowTreeSheet(BOOL val)
{
	std::string error_string;
	settings.set_value(MUIOPTION_SHOW_TREE_SHEET, val, OPTION_PRIORITY_CMDLINE, error_string);
}
#endif /* TREE_SHEET */



#include <ctype.h>

typedef struct
{
	char *name;
	DWORD flags;
} f_flag_entry;

typedef struct
{
	f_flag_entry *entry;
	int           num;
} f_flag;

#define ALLOC_FOLDERFLAG	8
#define ALLOC_FOLDERS		100

static void set_folder_flag(f_flag *flag, const char *path, DWORD dwFlags)
{
	int i;

	if (flag->entry == NULL)
	{
		flag->entry = (f_flag_entry *)malloc(ALLOC_FOLDERFLAG * sizeof (*flag->entry));
		if (!flag->entry)
		{
			dprintf("error: malloc failed in set_folder_flag\n");
			return;
		}

		flag->num = ALLOC_FOLDERFLAG;
		memset(flag->entry, 0, flag->num * sizeof (*flag->entry));
	}

	for (i = 0; i < flag->num; i++)
		if (flag->entry[i].name && strcmp(flag->entry[i].name, path) == 0)
		{
			if (dwFlags == 0)
			{
				osd_free(flag->entry[i].name);
				flag->entry[i].name = NULL;
			}
			else
				flag->entry[i].flags = dwFlags;

			return;
		}

	if (dwFlags == 0)
		return;

	for (i = 0; i < flag->num; i++)
		if (flag->entry[i].name == NULL)
			break;

	if (i == flag->num)
	{
		f_flag_entry *tmp;

		tmp = (f_flag_entry *)realloc(flag->entry, (flag->num + ALLOC_FOLDERFLAG) * sizeof (*tmp));
		if (!tmp)
		{
			dprintf("error: realloc failed in set_folder_flag\n");
			return;
		}

		flag->entry = tmp;
		memset(tmp + flag->num, 0, ALLOC_FOLDERFLAG * sizeof (*tmp));
		flag->num += ALLOC_FOLDERFLAG;
	}

	flag->entry[i].name = core_strdup(path);
	flag->entry[i].flags = dwFlags;
}

static void free_folder_flag(f_flag *flag)
{
	int i;

	for (i = 0; i < flag->num; i++)
		FreeIfAllocated(&flag->entry[i].name);

	if (flag->entry)
		free(flag->entry);
	flag->entry = NULL;
	flag->num = 0;
}

static void options_get_folder_flag(winui_options &opts, f_flag *flags, const char *name)
{
	const char *stemp = opts.value(name);

	free_folder_flag(flags);

	if (stemp == NULL)
		return;

	while (*stemp)
	{
		char buf[256];
		char *p1;
		char *p2;

		if (*stemp == ',')
			break;

		p1 = buf;
		while (*stemp != '\0' && *stemp != ',')
			*p1++ = *stemp++;
		*p1++ = '\0';

		if (*stemp++ != ',')
			return;

		while (*stemp == ' ')
			stemp++;

		if (!isdigit(*stemp))
			return;

		p2 = p1;
		while (isdigit(*stemp))
			*p2++ = *stemp++;
		*p2 = '\0';

		set_folder_flag(flags, buf, atoi(p1));

		if (*stemp++ != ',')
			return;

		while (*stemp == ' ')
			stemp++;
	}
}

static void options_set_folder_flag(winui_options &opts, const char *name, const f_flag *flags, int priority)
{
	char *buf;
	int size;
	int len;
	int i;

	size = 1024;
	buf = (char *)malloc(size * sizeof (*buf));
	*buf = '\0';
	len = 0;

	for (i = 0; i < flags->num; i++)
		if (flags->entry[i].name != NULL)
		{
			DWORD dwFlags = flags->entry[i].flags;
			if (dwFlags == 0)
				continue;

			if (len + strlen(flags->entry[i].name) + 16 > size)
			{
				size += 1024;
				buf = (char *)realloc(buf, size * sizeof (*buf));
			}

			if (len)
				buf[len++] = ',';

			len += sprintf(buf + len, "%s,%ld", flags->entry[i].name, dwFlags);
		}

	std::string error_string;
	opts.set_value(name, buf, priority, error_string);
	free(buf);
}

static f_flag settings_folder_flag;

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void  CusColorEncodeString(const COLORREF *value, char* str)
{
	int  i;
	char tmpStr[256];

	sprintf(tmpStr, "%u", (int) value[0]);

	strcpy(str, tmpStr);

	for (i = 1; i < 16; i++)
	{
		sprintf(tmpStr, ",%u", (unsigned) value[i]);
		strcat(str, tmpStr);
	}
}

static void CusColorDecodeString(const char* str, COLORREF *value)
{
	int  i;
	char *s, *p;
	char tmpStr[256];

	strcpy(tmpStr, str);
	p = tmpStr;

	for (i = 0; p && i < 16; i++)
	{
		s = p;

		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}


void ColumnEncodeStringWithCount(const int *value, char *str, int count)
{
	int  i;
	char buffer[100];

	snprintf(buffer,sizeof(buffer),"%d",value[0]);

	strcpy(str,buffer);

    for (i = 1; i < count; i++)
	{
		snprintf(buffer,sizeof(buffer),",%d",value[i]);
		strcat(str,buffer);
	}
}

void ColumnDecodeStringWithCount(const char* str, int *value, int count)
{
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;

    for (i = 0; p && i < count; i++)
	{
		s = p;

		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
    }
	}

static void SplitterEncodeString(const int *value, char* str)
{
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);

	strcpy(str, tmpStr);

	for (i = 1; i < GetSplitterCount(); i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void SplitterDecodeString(const char *str, int *value)
{
	int  i;
	char *s, *p;
	char tmpStr[100];

	strcpy(tmpStr, str);
	p = tmpStr;

	for (i = 0; p && i < GetSplitterCount(); i++)
	{
		s = p;

		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

/* Parse the given comma-delimited string into a LOGFONT structure */
static void FontDecodeString(const char* str, LOGFONTW *f)
{
	const char*	ptr;
	WCHAR*		w_ptr;

	sscanf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
		   &f->lfHeight,
		   &f->lfWidth,
		   &f->lfEscapement,
		   &f->lfOrientation,
		   &f->lfWeight,
		   (int*)&f->lfItalic,
		   (int*)&f->lfUnderline,
		   (int*)&f->lfStrikeOut,
		   (int*)&f->lfCharSet,
		   (int*)&f->lfOutPrecision,
		   (int*)&f->lfClipPrecision,
		   (int*)&f->lfQuality,
		   (int*)&f->lfPitchAndFamily);
	ptr = strrchr(str, ',');
	if (ptr != NULL) {
		w_ptr = wstring_from_utf8(ptr + 1);
		if( !w_ptr )
			return;
		wcscpy(f->lfFaceName, w_ptr);
		osd_free(w_ptr);
	}
}

/* Encode the given LOGFONT structure into a comma-delimited string */
static void FontEncodeString(const LOGFONTW *f, char *str)
{
	//char* utf8_FaceName = utf8_from_tstring(f->lfFaceName);
	//if( !utf8_FaceName )
	//	return;

	sprintf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
			f->lfHeight,
			f->lfWidth,
			f->lfEscapement,
			f->lfOrientation,
			f->lfWeight,
			f->lfItalic,
			f->lfUnderline,
			f->lfStrikeOut,
			f->lfCharSet,
			f->lfOutPrecision,
			f->lfClipPrecision,
			f->lfQuality,
			f->lfPitchAndFamily);
	//		utf8_FaceName);
        //
	//osd_free(utf8_FaceName);
}

static void TabFlagsEncodeString(int data, char *str)
{
	int i;
	int num_saved = 0;

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (((data & (1 << i)) == 0) && GetImageTabShortName(i))
		{
			if (num_saved != 0)
				strcat(str, ", ");

			strcat(str,GetImageTabShortName(i));
			num_saved++;
		}
	}
}

static void TabFlagsDecodeString(const char *str, int *data)
{
	char s[2000];
	char *token;

	snprintf(s, ARRAY_LENGTH(s), "%s", str);

	// simple way to set all tab bits "on"
	*data = (1 << MAX_TAB_TYPES) - 1;

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;j<MAX_TAB_TYPES;j++)
		{
			if (!GetImageTabShortName(j) || (strcmp(GetImageTabShortName(j), token) == 0))
			{
				// turn off this bit
				*data &= ~(1 << j);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}

	if (*data == 0)
	{
		// not allowed to hide all tabs, because then why even show the area?
		*data = (1 << TAB_SCREENSHOT);
	}
}

static file_error LoadSettingsFile(winui_options &opts, const char *filename)
{
	emu_file file(OPEN_FLAG_READ);

	file_error filerr = file.open(filename);
	if (filerr == FILERR_NONE)
	{
		std::string error_string;
		opts.parse_ini_file(file, OPTION_PRIORITY_CMDLINE, OPTION_PRIORITY_CMDLINE, error_string);
	}
	return filerr;
}
static file_error LoadSettingsFile(windows_options &opts, const char *filename)
{
	emu_file file(OPEN_FLAG_READ);

	file_error filerr = file.open(filename);
	if (filerr == FILERR_NONE)
	{
		std::string error_string;
		opts.parse_ini_file(file, OPTION_PRIORITY_CMDLINE, OPTION_PRIORITY_CMDLINE, error_string);
	}
	return filerr;
}


static file_error SaveSettingsFile(winui_options &opts, winui_options *baseopts, const char *filename)
{
	file_error filerr;

	//if ((opts != NULL) && ((baseopts == NULL) || !options_equal(opts, baseopts)))
	{
		std::string inistring;
		//inistring.expand(8 * 1024);

		opts.output_ini(inistring,baseopts);

		emu_file file(OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
		filerr = file.open(filename);
		if (filerr == FILERR_NONE)
		{
			file.puts(inistring.c_str());
		}
	}
	/*else
    {
        filerr = osd_rmfile(filename);
    }*/

	return filerr;
}
static file_error SaveSettingsFile(windows_options &opts, windows_options *baseopts, const char *filename)
{
	file_error filerr;

	//if ((opts != NULL) && ((baseopts == NULL) || !options_equal(opts, baseopts)))
	{
		std::string inistring;
		//inistring.expand(8 * 1024);

		opts.output_ini(inistring,baseopts);

		emu_file file(OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
		filerr = file.open(filename);
		if (filerr == FILERR_NONE)
		{
			file.puts(inistring.c_str());
		}
	}
	/*else
    {
        filerr = osd_rmfile(filename);
    }*/

	return filerr;
}



static void GetGlobalOptionsFileName(char *filename, size_t filename_size)
{
	// always in the current directory.
	snprintf(filename, filename_size, "%s%s", DEFAULT_OPTIONS_INI_FILENAME, ".ini");
}

static void GetGameOptionsFileName(char *filename, size_t filename_size)
{
	// copy INI directory
	char *inidir = utf8_from_wstring(GetIniDir());
	char *s = core_strdup(GAMEINFO_INI_FILENAME);
	_strlwr(s);
	snprintf(filename, filename_size, "%s\\%s", inidir, s);
	osd_free(inidir);
	osd_free(s);
}

static void GetSettingsFileName(char *filename, size_t filename_size)
{
	// copy INI directory
	char *inidir = utf8_from_wstring(GetIniDir());
	char *s = core_strdup(UI_INI_FILENAME);
	_strlwr(s);
	snprintf(filename, filename_size, "%s\\%s", inidir, s);
	osd_free(inidir);
	osd_free(s);
}

/* Register access functions below */
static void LoadOptionsAndSettings(void)
{
	char buffer[MAX_PATH];

	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS , FALSE , MUTEX_STR);
	dprintf("waiting for mutex...");
	WaitForSingleObject(hMutex , INFINITE);
	dprintf("loading %s...", UI_INI_FILENAME);

	// parse global options mame.ini
	GetGlobalOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	LoadSettingsFile(global, buffer);

	// parse GameInfo.ini - game options.
	GetGameOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	game_opts.load_file(buffer);

	// parse ui/mameui.ini
	GetSettingsFileName(buffer, ARRAY_LENGTH(buffer));
	LoadSettingsFile(settings, buffer);

	dprintf("%s loaded", UI_INI_FILENAME);
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	std::string error_string;
	global.set_value(OPTION_LANGUAGE, global.value(OPTION_LANGUAGE), OPTION_PRIORITY_CMDLINE, error_string);
	GetLanguageDir();

	setup_language(global);
	SetLangcode(GetLangcode());
	SetUseLangList(UseLangList());
}

void SetDirectories(windows_options &opts)
{
/*
	std::string error_string;
	opts.set_value(OPTION_MEDIAPATH, GetRomDirs(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_SAMPLEPATH, GetSampleDirs(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_INIPATH, GetIniDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_CFG_DIRECTORY, GetCfgDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_SNAPSHOT_DIRECTORY, GetImgDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_INPUT_DIRECTORY, GetInpDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_STATE_DIRECTORY, GetStateDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_ARTPATH, GetArtDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_NVRAM_DIRECTORY, GetNvramDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_CTRLRPATH, GetCtrlrDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_CHEATPATH, GetCheatDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_CROSSHAIRPATH, GetCrosshairDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_FONTPATH, GetFontDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_DIFF_DIRECTORY, GetDiffDir(), OPTION_PRIORITY_CMDLINE, error_string);
	opts.set_value(OPTION_SNAPNAME, GetSnapName(), OPTION_PRIORITY_CMDLINE, error_string);
*/
}


DWORD LoadFolderFlags(const char *path)
{
	int i;

	options_get_folder_flag(settings, &settings_folder_flag, MUIOPTION_FOLDER_FLAG);

	if (settings_folder_flag.entry == NULL)
		return 0;

	for (i = 0; i < settings_folder_flag.num; i++)
		if (settings_folder_flag.entry[i].name
		 && strcmp(settings_folder_flag.entry[i].name, path) == 0)
			return settings_folder_flag.entry[i].flags;

	return 0;
}

void SaveFolderFlags(const char *path, DWORD flags)
{
	DWORD current = LoadFolderFlags(path);

	if (current == flags)
		return;

	set_folder_flag(&settings_folder_flag, path, flags);

	options_set_folder_flag(settings, MUIOPTION_FOLDER_FLAG, &settings_folder_flag, OPTION_PRIORITY_CMDLINE);
}



// Adds our folder flags to a temporarty winui_options, for saving.
static void AddFolderFlags(winui_options &opts)
{
#if 1 //mamep: folder flags are already registered into variable settings by function SaveFolderFlags()
	return;
#else
	int numFolders;
	int i;
	LPTREEFOLDER lpFolder;
	int num_entries = 0;
	options_entry entries[2] = { { 0 }, { 0 } };

	entries[0].name = NULL;
	entries[0].defvalue = NULL;
	entries[0].flags = OPTION_HEADER;
	entries[0].description = "FOLDER FILTERS";
	opts.add_entries(entries);

	memcpy(entries, filterOptions, sizeof(filterOptions));

	numFolders = GetNumFolders();

	for (i = 0; i < numFolders; i++)
	{
		lpFolder = GetFolder(i);
		if (lpFolder && (lpFolder->m_dwFlags & F_MASK) != 0)
		{
			char folder_name[80];
			char *ptr;
			std::string *option_name;

			// Convert spaces to underscores
			strcpy(folder_name, lpFolder->m_lpTitle);
			ptr = folder_name;
			while (*ptr && *ptr != '\0')
			{
				if (*ptr == ' ')
				{
					*ptr = '_';
				}
				ptr++;
			}

			std::string option_name (folder_name, "_filters" );

			// create entry
			entries[0].name = option_name;
			opts.add_entries(entries);

			// store entry
			std::string error_string;
			opts.set_value(option_name, EncodeFolderFlags(lpFolder->m_dwFlags), OPTION_PRIORITY_CMDLINE,error_string);


			// increment counter
			num_entries++;
		}
	}
#endif
}

// Save the UI ini
void SaveOptions(void)
{
	// Add the folder flag to settings.
	AddFolderFlags(settings);
	// Save opts if it is non-null, else save settings.
	// It will be null if there are no filters set.
	char buffer[MAX_PATH];
	GetSettingsFileName(buffer, ARRAY_LENGTH(buffer));
	SaveSettingsFile(settings, NULL, buffer);
}

void SaveGameListOptions(void)
{
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS , FALSE , MUTEX_STR);
	dprintf("waiting for mutex...");
	WaitForSingleObject(hMutex , INFINITE);
	dprintf("saving %s...", UI_INI_FILENAME);

	// Save GameInfo.ini - game options.
	char buffer[MAX_PATH];
	GetGameOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	game_opts.save_file(buffer);

	dprintf("%s saved", UI_INI_FILENAME);
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
}

void SaveDefaultOptions(void)
{
	char buffer[MAX_PATH];
	GetGlobalOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	SaveSettingsFile(global, NULL, buffer);
}

const char * GetVersionString(void)
{
	return build_version;
}


BOOL IsGlobalOption(const char *option_name)
{
/*	static const char *global_options[] =
	{
		OPTION_MEDIAPATH,
#ifdef MESS
		OPTION_HASHPATH,
#endif // MESS
		OPTION_SAMPLEPATH,
		OPTION_ARTPATH,
		OPTION_CTRLRPATH,
		OPTION_INIPATH,
		OPTION_FONTPATH,
		OPTION_CHEATPATH,
		OPTION_LANGPATH,
#ifdef USE_IPS
		OPTION_IPSPATH,
#endif // USE_IPS
		OPTION_CFG_DIRECTORY,
		OPTION_NVRAM_DIRECTORY,
		OPTION_MEMCARD_DIRECTORY,
		OPTION_INPUT_DIRECTORY,
		OPTION_STATE_DIRECTORY,
		OPTION_SNAPSHOT_DIRECTORY,
		OPTION_DIFF_DIRECTORY,
		OPTION_COMMENT_DIRECTORY,
#ifdef USE_HISCORE
		OPTION_HISCORE_DIRECTORY,
#endif // USE_HISCORE
#ifdef UI_COLOR_DISPLAY
		OPTION_SYSTEM_BACKGROUND,
		OPTION_CURSOR_SELECTED_TEXT,
		OPTION_CURSOR_SELECTED_BG,
		OPTION_CURSOR_HOVER_TEXT,
		OPTION_CURSOR_HOVER_BG,
		OPTION_BUTTON_RED,
		OPTION_BUTTON_YELLOW,
		OPTION_BUTTON_GREEN,
		OPTION_BUTTON_BLUE,
		OPTION_BUTTON_PURPLE,
		OPTION_BUTTON_PINK,
		OPTION_BUTTON_AQUA,
		OPTION_BUTTON_SILVER,
		OPTION_BUTTON_NAVY,
		OPTION_BUTTON_LIME,
#endif // UI_COLOR_DISPLAY
		OPTION_LANGUAGE,
		OPTION_USE_LANG_LIST
	};
	int i;
	char command_name[128];
	char *s;

	// determine command name
	snprintf(command_name, ARRAY_LENGTH(command_name), "%s", option_name);
	s = strchr(command_name, ';');
	if (s)
		*s = '\0';

	for (i = 0; i < ARRAY_LENGTH(global_options); i++)
	{
		if (!strcmp(command_name, global_options[i]))
			return TRUE;
	}*/
	return FALSE;
}


/* ui_parse_ini_file - parse a single INI file */
static void ui_parse_ini_file(windows_options &opts, const char *name)
{
	/* open the file; if we fail, that's ok */
	char *inidir = utf8_from_wstring(GetIniDir());
	std::string fname = std::string(inidir).append(PATH_SEPARATOR).append(name).append(".ini");
	osd_free(inidir);
	LoadSettingsFile(opts, fname.c_str());
	SetDirectories(opts);
}

static void ui_parse_global_ini_file(windows_options &opts)
{
	/* open the file; if we fail, that's ok */
	std::string fname = std::string(emulator_info::get_configname()).append(".ini");
	LoadSettingsFile(opts, fname.c_str());
}


/*  get options, based on passed in option level. */
void load_options(windows_options &opts, OPTIONS_TYPE opt_type, int game_num)
{
	const game_driver *driver = NULL;
	std::string basename;

	CreateGameOptions(opts, game_num);
	// Copy over the defaults
	ui_parse_global_ini_file(opts);

	if (opt_type == OPTIONS_GLOBAL)
	{
		return;
	}

	// debug builds: parse "debug.ini" as well
#ifdef MAME_DEBUG
	ui_parse_ini_file(opts, "debug");
#endif
	if (game_num >= 0)
	{
		driver = &driver_list::driver(game_num);
	}

	// if we have a valid game driver, parse game-specific INI files
	if (driver != NULL)
	{
		const game_driver *parent = NULL;
		int cl = driver_list::clone(*driver);
		if (cl!=-1) parent = &driver_list::driver(cl);
		int gp = -1;
		if (parent!=NULL) gp = driver_list::clone(*parent);
		const game_driver *gparent = NULL;
		if (parent != NULL) {
			if (gp!=-1) gparent= &driver_list::driver(gp);
		}

		// parse "vector.ini" for vector games
		if (DriverIsVector(game_num))
		{
			ui_parse_ini_file(opts, "vector");
		}
		if (opt_type == OPTIONS_VECTOR)
		{
			return;
		}
		// parse "horizont.ini" for horizontal games
		if (!DriverIsVertical(game_num))
		{
			ui_parse_ini_file(opts, "horizont");
		}
		if (opt_type == OPTIONS_HORIZONTAL)
		{
			return;
		}
		// parse "vertical.ini" for vertical games
		if (DriverIsVertical(game_num))
		{
			ui_parse_ini_file(opts, "vertical");
		}
		if (opt_type == OPTIONS_VERTICAL)
		{
			return;
		}


		// then parse "<sourcefile>.ini"
		core_filename_extract_base(basename, driver->source_file, TRUE);
		std::string srcname = std::string("source").append(PATH_SEPARATOR).append(basename.c_str());
		ui_parse_ini_file(opts, srcname.c_str());

		if (opt_type == OPTIONS_SOURCE)
		{
			return;
		}

		// then parent the grandparent, parent, and game-specific INIs
		if (gparent != NULL)
			ui_parse_ini_file(opts, gparent->name);
		if (parent != NULL)
			ui_parse_ini_file(opts, parent->name);

		if (opt_type == OPTIONS_PARENT)
		{
			return;
		}

#ifdef USE_IPS
		//mamep: DO NOT INHERIT IPS CONFIGURATION
		std::string error_string;
		opts.set_value(OPTION_IPS, "", OPTION_PRIORITY_CMDLINE, error_string);
#endif /* USE_IPS */

		ui_parse_ini_file(opts, driver->name);

		if (opt_type == OPTIONS_GAME)
		{
			return;
		}
	}
}



/*
 * Save ini file based on game_num and passed in opt_type.  If opts are
 * NULL, the ini will be removed.
 *
 * game_num must be valid or the driver cannot be expanded and anything
 * with a higher priority than OPTIONS_VECTOR will not be saved.
 */
void save_options(OPTIONS_TYPE opt_type, windows_options &opts, int game_num)
{
	windows_options *baseopts = NULL;
	const game_driver *driver = NULL;
	std::string filename, basename;

	//mamep: to remove ini file, load baseopts even if it is equals global
	if (OPTIONS_GLOBAL != opt_type) // && NULL != opts && !(opts == global))
	{
		baseopts = global_alloc(windows_options());
		if (OPTIONS_VERTICAL == opt_type) {
			//since VERTICAL and HORIZONTAL are equally ranked
			//we need to subtract 2 from vertical to also get to global
			load_options(*baseopts, (OPTIONS_TYPE)(opt_type - 2), game_num);
		}
		else {
			load_options(*baseopts, (OPTIONS_TYPE)(opt_type - 1), game_num);
		}
	}

	if (game_num >= 0)
	{
		driver = &driver_list::driver(game_num);
	}

	if (opt_type == OPTIONS_GLOBAL)
	{
		// Don't try to save a null global options file,  or it will be erased.
		//if (NULL == opts)
			//return;
		//global = opts;
		filename.assign(emulator_info::get_configname());
	} else if (opt_type == OPTIONS_VECTOR)
	{
		filename.assign("vector");
	} else if (opt_type == OPTIONS_VERTICAL)
	{
		filename.assign("vertical");
	} else if (opt_type == OPTIONS_HORIZONTAL)
	{
		filename.assign("horizont");
	} else if (driver != NULL)
	{
		if (opt_type == OPTIONS_SOURCE)
		{
			// determine the <sourcefile>
			core_filename_extract_base(basename, driver->source_file, TRUE);
			std::string srcname = std::string("source").append(PATH_SEPARATOR).append(basename.c_str());
			filename.assign(srcname.c_str());
		}
		else
		if (opt_type == OPTIONS_GAME)
			filename.assign(driver->name);
	}
	if (!filename.empty())
	{
		std::string filepath;
		if (opt_type == OPTIONS_GLOBAL)
		{
			char buffer[MAX_PATH];

			GetGlobalOptionsFileName(buffer, ARRAY_LENGTH(buffer));
			filepath.assign(buffer);
		}
		else
		{
			char *inidir = utf8_from_wstring(GetIniDir());
			std::string temppath = std::string(inidir).append(PATH_SEPARATOR).append(filename.c_str()).append(".ini");
			filepath.assign(temppath);
			osd_free(inidir);
		}

		SetDirectories(opts);
		SaveSettingsFile(opts, baseopts, filepath.c_str());

		if (baseopts != NULL)
			global_free(baseopts);

		//FIXME global option reload
		if (opt_type == OPTIONS_GLOBAL)
			ui_parse_global_ini_file(global);
	}
}

/* Remove all the srcname.ini files from inipath/source directory. */
static void remove_all_source_options(void) {
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile;
	char* utf8_filename;

	/*
	 * Easiest to just open the ini/source folder if it exists,
	 * then remove all the files in it that end in ini.
	 */
	char *inidir = utf8_from_wstring(GetIniDir());
	std::string pathname = std::string(inidir).append(PATH_SEPARATOR).append("source");
	osd_free(inidir);
	std::string match = std::string(pathname.c_str()).append(PATH_SEPARATOR).append("*.ini");
	if ((hFindFile = win_find_first_file_utf8(match.c_str(), &findFileData)) != INVALID_HANDLE_VALUE)
	{
		utf8_filename = utf8_from_tstring(findFileData.cFileName);
		std::string match = std::string(pathname.c_str()).append(PATH_SEPARATOR).append(utf8_filename);
		osd_free(utf8_filename);
		osd_rmfile(match.c_str());

		while (0 != FindNextFile(hFindFile, &findFileData))
		{
			utf8_filename = utf8_from_tstring(findFileData.cFileName);
			std::string match = std::string(pathname.c_str()).append(PATH_SEPARATOR).append(utf8_filename);
			osd_free(utf8_filename);
			osd_rmfile(match.c_str());
		}

		FindClose(hFindFile);

	}
}

// Reset the given windows_options to their default settings.
static void ResetToDefaults(windows_options &opts, int priority)
{
	// iterate through the options setting each one back to the default value.
	opts.revert(priority);
}

int GetDriverCache(int driver_index)
{
	return game_opts.cache(driver_index);
}

void SetDriverCache(int driver_index, int val)
{
	game_opts.cache(driver_index, val);
}

int GetDriverCachePlayers(int driver_index)
{
	return game_opts.players(driver_index);
}

void SetDriverCachePlayers(int driver_index, int val)
{
	game_opts.players(driver_index, val);
}

int GetDriverCacheButtons(int driver_index)
{
	return game_opts.buttons(driver_index);
}

void SetDriverCacheButtons(int driver_index, int val)
{
	game_opts.buttons(driver_index, val);
}

int GetDriverCacheParentIndex(int driver_index)
{
	return game_opts.parent_index(driver_index);
}

void SetDriverCacheParentIndex(int driver_index, int val)
{
	game_opts.parent_index(driver_index, val);
}

int GetDriverCacheBiosIndex(int driver_index)
{
	return game_opts.bios_index(driver_index);
}

void SetDriverCacheBiosIndex(int driver_index, int val)
{
	game_opts.bios_index(driver_index, val);
}

int GetDriverCacheUsesController(int driver_index)
{
	return game_opts.uses_controler(driver_index);
}

void SetDriverCacheUsesController(int driver_index, int val)
{
	game_opts.uses_controler(driver_index, val);
}


static BOOL RequiredDriverCacheStatus = FALSE;
void SetRequiredDriverCacheStatus(void)
{
	static bool bFirst = true;

	if (bFirst)
	{
		RequiredDriverCacheStatus = RequiredDriverCache(1);

		bFirst = false;
	}
}
BOOL GetRequiredDriverCacheStatus(void)
{
	SetRequiredDriverCacheStatus();

	return RequiredDriverCacheStatus;
}

BOOL RequiredDriverCache(int check_only)
{
	bool ret = false;

	WCHAR fname[_MAX_FNAME];
	char *utf8_filename;

	_wsplitpath(GetCommandLine(), NULL, NULL, fname, NULL);
	utf8_filename = utf8_from_wstring(fname);

	if ( strcmp(settings.value(MUIOPTION_EXE_NAME), utf8_filename) != 0 )
		ret = true;

	if ( strcmp(settings.value(MUIOPTION_VERSION), GetVersionString()) != 0 )
		ret = true;

#ifdef DRIVER_SWITCH
	if ( strcmp(settings.value(OPTION_DRIVER_CONFIG), global.value(OPTION_DRIVER_CONFIG)) != 0 )
		ret = true;

	if ( settings.bool_value(OPTION_DISABLE_MECHANICAL_DRIVER) != global.bool_value(OPTION_DISABLE_MECHANICAL_DRIVER) )
		ret = true;
#endif /* DRIVER_SWITCH */

	if (!check_only) {
		std::string error_string;
		settings.set_value(MUIOPTION_EXE_NAME, utf8_filename, OPTION_PRIORITY_CMDLINE,error_string);
		settings.set_value(MUIOPTION_VERSION, GetVersionString(), OPTION_PRIORITY_CMDLINE,error_string);
#ifdef DRIVER_SWITCH
		settings.set_value(OPTION_DRIVER_CONFIG, global.value(OPTION_DRIVER_CONFIG), OPTION_PRIORITY_CMDLINE,error_string);
		settings.set_value(OPTION_DISABLE_MECHANICAL_DRIVER, global.value(OPTION_DISABLE_MECHANICAL_DRIVER), OPTION_PRIORITY_CMDLINE,error_string);
#endif /* DRIVER_SWITCH */
	}

	osd_free(utf8_filename);

	return ret;
}



#include "strconv.h"

WCHAR *options_get_wstring(winui_options &opts, const char *name)
{
	const char *stemp = opts.value(name);

	if (stemp == NULL)
		return NULL;

	return wstring_from_utf8(stemp);
}

void options_set_wstring(winui_options &opts, const char *name, const WCHAR *value, int priority)
{
	char *utf8_value = NULL;

	if (value)
		utf8_value = utf8_from_wstring(value);

	std::string error_string;
	opts.set_value(name, utf8_value, priority, error_string);

	osd_free(utf8_value);
}

WCHAR *options_get_wstring(windows_options &opts, const char *name)
{
	const char *stemp = opts.value(name);

	if (stemp == NULL)
		return NULL;

	return wstring_from_utf8(stemp);
}

void options_set_wstring(windows_options &opts, const char *name, const WCHAR *value, int priority)
{
	char *utf8_value = NULL;

	if (value)
		utf8_value = utf8_from_wstring(value);

	std::string error_string;
	opts.set_value(name, utf8_value, priority, error_string);

	osd_free(utf8_value);
}
