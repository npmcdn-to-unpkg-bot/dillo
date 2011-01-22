/*
 * File: menu.cc
 *
 * Copyright (C) 2005-2007 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

// Functions/Methods for menus

#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>

#include "lout/misc.hh"    /* SimpleVector */
#include "msg.h"
#include "menu.hh"
#include "uicmd.hh"
#include "history.h"
#include "html.hh"
#include "ui.hh" // for (UI *)
#include "keys.hh"
#include "timeout.hh"

/*
 * Local data
 */

// (This data can be encapsulated inside a class for each popup, but
//  as popups are modal, there's no need).
static DilloUrl *popup_url = NULL;
// Weak reference to the popup's bw
static BrowserWindow *popup_bw = NULL;
static void *popup_form = NULL;
// Where to place the filemenu popup
static int popup_x, popup_y;
// History popup direction (-1 = back, 1 = forward).
static int history_direction = -1;
// History popup, list of URL-indexes.
static int *history_list = NULL;

#if 0
My guess is that a modified copy of Fl_Menu_ that handles something similar
to Fl_Menu_Item may be necessary. I sure hope not, though.

/*
 * Local sub class.
 * Used to add the hint for history popup menus, and to remember
 * the mouse button pressed over a menu item.
 */
class CustItem : public Fl_Menu_Item {
   int EventButton;
public:
   CustItem (const char* label) : Item(label) { EventButton = 0; };
   int button () { return EventButton; };
   void draw();
   int handle(int e) {
      EventButton = Fl::event_button();
      return Item::handle(e);
   }
};

/*
 * This adds a call to a_UIcmd_set_msg() to show the URL in the status bar
 * TODO: erase the URL on popup close.
 */
void CustItem::draw() {
   const DilloUrl *url;

   if (flags() & SELECTED) {
      url = a_History_get_url(history_list[(VOIDP2INT(user_data()))-1]);
      a_UIcmd_set_msg(popup_bw, "%s", URL_STR(url));
   }
   Item::draw();
}
#endif

//--------------------------------------------------------------------------
/*
 * Static function for File menu callbacks.
 */
static void filemenu_cb(Fl_Widget *wid, void *data)
{
   if (strcmp((char*)data, "nw") == 0) {
      UI *ui = (UI*)popup_bw->ui;
      a_UIcmd_browser_window_new(ui->w(), ui->h(), 0, popup_bw);
   } else if (strcmp((char*)data, "nt") == 0) {
      a_UIcmd_open_url_nt(popup_bw, NULL, 1);
   } else if (strcmp((char*)data, "of") == 0) {
      a_UIcmd_open_file(popup_bw);
   } else if (strcmp((char*)data, "ou") == 0) {
      a_UIcmd_focus_location(popup_bw);
   } else if (strcmp((char*)data, "cw") == 0) {
      a_Timeout_add(0.0, a_UIcmd_close_bw, popup_bw);
   } else if (strcmp((char*)data, "ed") == 0) {
      a_Timeout_add(0.0, a_UIcmd_close_all_bw, NULL);
   }
}


static void Menu_copy_urlstr_cb(Fl_Widget*, void*)
{
   if (popup_url)
      a_UIcmd_copy_urlstr(popup_bw, URL_STR(popup_url));
}

static void Menu_link_cb(Fl_Widget*, void *user_data)
{
   DilloUrl *url = (DilloUrl *) user_data ;
   _MSG("Menu_link_cb: click! :-)\n");

   if (url)
      a_Menu_link_popup(popup_bw, url);
}

/*
 * Open URL
 */
static void Menu_open_url_cb(Fl_Widget*, void* )
{
   _MSG("Open URL cb: click! :-)\n");
   a_UIcmd_open_url(popup_bw, popup_url);
}

/*
 * Open URL in new window
 */
static void Menu_open_url_nw_cb(Fl_Widget*, void* )
{
   _MSG("Open URL in new window cb: click! :-)\n");
   a_UIcmd_open_url_nw(popup_bw, popup_url);
}

/*
 * Open URL in new Tab
 */
static void Menu_open_url_nt_cb(Fl_Widget*, void* )
{
   int focus = prefs.focus_new_tab ? 1 : 0;
   if (Fl::event_state(FL_SHIFT)) focus = !focus;
   a_UIcmd_open_url_nt(popup_bw, popup_url, focus);
}

/*
 * Add bookmark
 */
static void Menu_add_bookmark_cb(Fl_Widget*, void* )
{
   a_UIcmd_add_bookmark(popup_bw, popup_url);
}

/*
 * Find text
 */
static void Menu_find_text_cb(Fl_Widget*, void* )
{
   ((UI *)popup_bw->ui)->set_findbar_visibility(1);
}

/*
 * Save link
 */
static void Menu_save_link_cb(Fl_Widget*, void* )
{
   a_UIcmd_save_link(popup_bw, popup_url);
}

/*
 * Save current page
 */
static void Menu_save_page_cb(Fl_Widget*, void* )
{
   a_UIcmd_save(popup_bw);
}

/*
 * View current page source
 */
static void Menu_view_page_source_cb(Fl_Widget*, void* )
{
   a_UIcmd_view_page_source(popup_bw, popup_url);
}

/*
 * View current page's bugs
 */
static void Menu_view_page_bugs_cb(Fl_Widget*, void* )
{
   a_UIcmd_view_page_bugs(popup_bw);
}

/*
 * Load images on current page that match URL pattern
 */
static void Menu_load_images_cb(Fl_Widget*, void *user_data)
{
   DilloUrl *page_url = (DilloUrl *) user_data;
   void *doc = a_Bw_get_url_doc(popup_bw, page_url);

   if (doc)
      a_Html_load_images(doc, popup_url);
}

/*
 * Submit form
 */
static void Menu_form_submit_cb(Fl_Widget*, void *)
{
   void *doc = a_Bw_get_url_doc(popup_bw, popup_url);

   if (doc)
      a_Html_form_submit(doc, popup_form);
}

/*
 * Reset form
 */
static void Menu_form_reset_cb(Fl_Widget*, void *)
{
   void *doc = a_Bw_get_url_doc(popup_bw, popup_url);

   if (doc)
      a_Html_form_reset(doc, popup_form);
}

/*
 * Toggle display of 'hidden' form controls.
 */
static void Menu_form_hiddens_cb(Fl_Widget *w, void *user_data)
{
   bool visible = *((bool *) user_data);
   void *doc = a_Bw_get_url_doc(popup_bw, popup_url);

   if (doc)
      a_Html_form_display_hiddens(doc, popup_form, !visible);
}

static void Menu_stylesheet_cb(Fl_Widget *w, void *vUrl)
{
   const DilloUrl *url = (const DilloUrl *) vUrl;
   a_UIcmd_open_url(popup_bw, url);
}

/*
 * Validate URL with the W3C
 */
static void Menu_bugmeter_validate_w3c_cb(Fl_Widget*, void* )
{
   Dstr *dstr = dStr_sized_new(128);

   dStr_sprintf(dstr, "http://validator.w3.org/check?uri=%s",
                URL_STR(popup_url));
   a_UIcmd_open_urlstr(popup_bw, dstr->str);
   dStr_free(dstr, 1);
}

/*
 * Validate URL with the WDG
 */
static void Menu_bugmeter_validate_wdg_cb(Fl_Widget*, void* )
{
   Dstr *dstr = dStr_sized_new(128);

   dStr_sprintf(dstr,
      "http://www.htmlhelp.org/cgi-bin/validate.cgi?url=%s&warnings=yes",
      URL_STR(popup_url));
   a_UIcmd_open_urlstr(popup_bw, dstr->str);
   dStr_free(dstr, 1);
}

/*
 * Show info page for the bug meter
 */
static void Menu_bugmeter_about_cb(Fl_Widget*, void* )
{
   a_UIcmd_open_urlstr(popup_bw, "http://www.dillo.org/help/bug_meter.html");
}

/*
 * Navigation History callback.
 * Go to selected URL.
 */
static void Menu_history_cb(Fl_Widget *wid, void *data)
{
#if 0
   int mb = ((CustItem*)wid)->button();
#endif
   int offset = history_direction * VOIDP2INT(data);
#if 0
   const DilloUrl *url = a_History_get_url(history_list[VOIDP2INT(data)-1]);

   if (mb == 2) {
      // Middle button, open in a new window/tab
      if (prefs.middle_click_opens_new_tab) {
         int focus = prefs.focus_new_tab ? 1 : 0;
         if (Fl::event_state(FL_SHIFT)) focus = !focus;
         a_UIcmd_open_url_nt(popup_bw, url, focus);
      } else {
         a_UIcmd_open_url_nw(popup_bw, url);
      }
   } else {
#endif
      a_UIcmd_nav_jump(popup_bw, offset, 0);
#if 0
   }
#endif
}

/*
 * Menus are popped-up from this timeout callback so the events
 * associated with the button are gone when it pops. This way we
 * avoid a segfault when a new page replaces the page that issued
 * the popup menu.
 */
static void Menu_popup_cb(void *data)
{
   const Fl_Menu_Item *m = ((Fl_Menu_Item *)data)->popup(popup_x, popup_y);

   if (m)
      ((Fl_Widget *)m)->do_callback();
   a_Timeout_remove();
}

/*
 * Page popup menu (construction & popup)
 */
void a_Menu_page_popup(BrowserWindow *bw, const DilloUrl *url,
                       bool_t has_bugs, void *v_cssUrls)
{
   lout::misc::SimpleVector <DilloUrl*> *cssUrls =
                            (lout::misc::SimpleVector <DilloUrl*> *) v_cssUrls;
   int j = 0;

   static Fl_Menu_Item *stylesheets = NULL;
   static Fl_Menu_Item pm[] = {
      {"View page source", 0, Menu_view_page_source_cb,0,0,0,0,0,0},
      {"View page bugs", 0, Menu_view_page_bugs_cb,0,0,0,0,0,0},
      {"View stylesheets", 0, 0, 0,FL_SUBMENU_POINTER|FL_MENU_DIVIDER,0,0,0,0},
      {"Bookmark this page", 0,Menu_add_bookmark_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Find text", 0, Menu_find_text_cb,0,0,0,0,0,0},
      {"Save page as...", 0, Menu_save_page_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   popup_bw = bw;
   a_Url_free(popup_url);
   popup_url = a_Url_dup(url);

   has_bugs == TRUE ? pm[1].activate() : pm[1].deactivate();

   if (strncmp(URL_STR(url), "dpi:/vsource/", 13) == 0)
      pm[0].deactivate();
   else
      pm[0].activate();

   if (stylesheets) {
      while (stylesheets[j].text) {
         dFree((char *) stylesheets[j].label());
         a_Url_free((DilloUrl *) stylesheets[j].user_data());
         j++;
      }
      delete [] stylesheets;
      stylesheets = NULL;
   }

   if (cssUrls && cssUrls->size () > 0) {
      stylesheets = new Fl_Menu_Item[cssUrls->size() + 1];
      memset(stylesheets, '\0', sizeof(Fl_Menu_Item[cssUrls->size() + 1]));

      for (j = 0; j < cssUrls->size(); j++) {

         /* may want ability to Load individual unloaded stylesheets as well */
         const char *action = "View ";
         DilloUrl *url = cssUrls->get(j);
         const char *url_str = URL_STR(url);
         const uint_t head_length = 30, tail_length = 40,
                      url_len = strlen(url_str);;
         char *label;

         if (url_len > head_length + tail_length + 3) {
            /* trim long URLs when making the label */
            char *url_head = dStrndup(url_str, head_length);
            const char *url_tail = url_str + (url_len - tail_length);
            label = dStrconcat(action, url_head, "...", url_tail, NULL);
            dFree(url_head);
         } else {
            label = dStrconcat(action, url_str, NULL);
         }

         stylesheets[j].label(FL_NORMAL_LABEL, label);
         stylesheets[j].callback(Menu_stylesheet_cb, a_Url_dup(url));
      }

      pm[2].user_data(stylesheets);
      pm[2].activate();
   } else {
      pm[2].deactivate();
   }

   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Link popup menu (construction & popup)
 */
void a_Menu_link_popup(BrowserWindow *bw, const DilloUrl *url)
{
   static Fl_Menu_Item pm[] = {
      {"Open link in new window", 0, Menu_open_url_nw_cb,0,0,0,0,0,0},
      {"Open link in new tab",0,Menu_open_url_nt_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Bookmark this link", 0, Menu_add_bookmark_cb,0,0,0,0,0,0},
      {"Copy link location", 0, Menu_copy_urlstr_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Save link as...", 0, Menu_save_link_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   popup_bw = bw;
   a_Url_free(popup_url);
   popup_url = a_Url_dup(url);

   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Image popup menu (construction & popup)
 */
void a_Menu_image_popup(BrowserWindow *bw, const DilloUrl *url,
                        bool_t loaded_img, DilloUrl *page_url,
                        DilloUrl *link_url)
{
   static DilloUrl *popup_page_url = NULL;
   static DilloUrl *popup_link_url = NULL;
   static Fl_Menu_Item pm[] = {
      {"Isolate image", 0, Menu_open_url_cb,0,0,0,0,0,0},
      {"Open image in new window", 0, Menu_open_url_nw_cb,0,0,0,0,0,0},
      {"Open image in new tab", 0, Menu_open_url_nt_cb, 0, FL_MENU_DIVIDER,
       0,0,0,0},
      {"Load image", 0, Menu_load_images_cb,0,0,0,0,0,0},
      {"Bookmark this image", 0, Menu_add_bookmark_cb,0,0,0,0,0,0},
      {"Copy image location", 0,Menu_copy_urlstr_cb,0,FL_MENU_DIVIDER,0,0,0,0},
      {"Save image as...", 0, Menu_save_link_cb, 0, FL_MENU_DIVIDER,0,0,0,0},
      {"Link menu", 0, Menu_link_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   popup_bw = bw;
   a_Url_free(popup_url);
   popup_url = a_Url_dup(url);
   a_Url_free(popup_page_url);
   popup_page_url = a_Url_dup(page_url);
   a_Url_free(popup_link_url);
   popup_link_url = a_Url_dup(link_url);


   if (loaded_img) {
      pm[3].deactivate();
   } else {
      pm[3].activate();
      pm[3].user_data(popup_page_url);
   }

   if (link_url) {
      pm[7].activate();
      pm[7].user_data(popup_link_url);
   } else {
      pm[7].deactivate();
   }

   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Form popup menu (construction & popup)
 */
void a_Menu_form_popup(BrowserWindow *bw, const DilloUrl *page_url,
                       void *formptr, bool_t hidvis)
{
   static bool hiddens_visible;
   static Fl_Menu_Item pm[] = {
      {"Submit form", 0, Menu_form_submit_cb,0,0,0,0,0,0},
      {"Reset form", 0, Menu_form_reset_cb,0,0,0,0,0,0},
      {0, 0, Menu_form_hiddens_cb, &hiddens_visible, 0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   popup_bw = bw;
   a_Url_free(popup_url);
   popup_url = a_Url_dup(page_url);
   popup_form = formptr;

   hiddens_visible = hidvis;
   pm[2].label(hiddens_visible ? "Hide hiddens": "Show hiddens");

   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * File popup menu (construction & popup)
 */
void a_Menu_file_popup(BrowserWindow *bw, void *v_wid)
{
   UI *ui = (UI *)bw->ui;
   Fl_Widget *wid = (Fl_Widget*)v_wid;

   static Fl_Menu_Item pm[] = {
      {"New window", Keys::getShortcut(KEYS_NEW_WINDOW), filemenu_cb,
       (void*)"nw",0,0,0,0,0},
      {"New tab", Keys::getShortcut(KEYS_NEW_TAB), filemenu_cb,
       (void*)"nt", FL_MENU_DIVIDER,0,0,0,0},
      {"Open file...", Keys::getShortcut(KEYS_OPEN), filemenu_cb,
       (void*)"of",0,0,0,0,0},
      {"Open URL...", Keys::getShortcut(KEYS_GOTO), filemenu_cb,
       (void*)"ou",0,0,0,0,0},
      {"Close", Keys::getShortcut(KEYS_CLOSE_TAB), filemenu_cb,
       (void*)"cw", FL_MENU_DIVIDER,0,0,0,0},
      {"Exit Dillo", Keys::getShortcut(KEYS_CLOSE_ALL), filemenu_cb,
       (void*)"ed",0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_bw = bw;
   popup_x = wid->x();
   popup_y = wid->y() + wid->h() +
             // WORKAROUND: ?? wid->y() doesn't count tabs ??
             (((Fl_Group*)ui->tabs())->children() > 1 ? 20 : 0);
   a_Url_free(popup_url);
   popup_url = NULL;

   //pm->label(wid->visible() ? NULL : "File");
   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Bugmeter popup menu (construction & popup)
 */
void a_Menu_bugmeter_popup(BrowserWindow *bw, const DilloUrl *url)
{
   static Fl_Menu_Item pm[] = {
      {"Validate URL with W3C", 0, Menu_bugmeter_validate_w3c_cb,0,0,0,0,0,0},
      {"Validate URL with WDG", 0, Menu_bugmeter_validate_wdg_cb, 0,
       FL_MENU_DIVIDER,0,0,0,0},
      {"About bug meter", 0, Menu_bugmeter_about_cb,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}     
   };      

   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   popup_bw = bw;
   a_Url_free(popup_url);
   popup_url = a_Url_dup(url);

   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Navigation History popup menu (construction & popup)
 *
 * direction: {backward = -1, forward = 1}
 */
void a_Menu_history_popup(BrowserWindow *bw, int direction)
{
   static Fl_Menu_Item *pm = 0;
   int i, n;

   popup_bw = bw;
   popup_x = Fl::event_x();
   popup_y = Fl::event_y();
   history_direction = direction;

   // TODO: hook popdown event with delete or similar.
   if (pm)
      delete [] pm;
   if (history_list)
      dFree(history_list);

   // Get a list of URLs for this popup
   history_list = a_UIcmd_get_history(bw, direction);

   for (n = 0; history_list[n] != -1; n++)
      ;

   pm = new Fl_Menu_Item[n + 1];
   memset(pm, '\0', sizeof(Fl_Menu_Item[n + 1]));

   for (i = 0; i < n; i++) {
      pm[i].label(FL_NORMAL_LABEL, a_History_get_title(history_list[i], 1));
      pm[i].callback(Menu_history_cb, INT2VOIDP(i+1));
   }
   a_Timeout_add(0.0, Menu_popup_cb, (void *)pm);
}

/*
 * Toggle use of remote stylesheets
 */
static void Menu_remote_css_cb(Fl_Widget *wid, void*)
{
   Fl_Menu_Item *item = (Fl_Menu_Item*) wid;

   item->flags ^= FL_MENU_VALUE;
   prefs.load_stylesheets = item->flags & FL_MENU_VALUE ? 1 : 0;
   a_UIcmd_repush(popup_bw);
}

/*
 * Toggle use of embedded CSS style
 */
static void Menu_embedded_css_cb(Fl_Widget *wid, void*)
{
   Fl_Menu_Item *item = (Fl_Menu_Item*) wid;

   item->flags ^= FL_MENU_VALUE;
   prefs.parse_embedded_css = item->flags & FL_MENU_VALUE ? 1 : 0;
   a_UIcmd_repush(popup_bw);
}

/*
 * Toggle loading of images -- and load them if enabling.
 */
static void Menu_imgload_toggle_cb(Fl_Widget *wid, void*)
{
   Fl_Menu_Item *item = (Fl_Menu_Item*) wid;

   item->flags ^= FL_MENU_VALUE;

   if ((prefs.load_images = item->flags & FL_MENU_VALUE ? 1 : 0)) {
      void *doc = a_Bw_get_current_doc(popup_bw);

      if (doc) {
         DilloUrl *pattern = NULL;
         a_Html_load_images(doc, pattern);
      }
   }
}

/*
 * Tools popup menu (construction & popup)
 */
void a_Menu_tools_popup(BrowserWindow *bw, void *v_wid)
{
   const Fl_Menu_Item *item;
   Fl_Widget *wid = (Fl_Widget*)v_wid;

   static Fl_Menu_Item pm[] = {
      {"Use remote CSS", 0, Menu_remote_css_cb, 0, FL_MENU_TOGGLE,0,0,0,0},
      {"Use embedded CSS", 0, Menu_embedded_css_cb, 0,
       FL_MENU_TOGGLE|FL_MENU_DIVIDER,0,0,0,0},
      {"Load images", 0, Menu_imgload_toggle_cb, 0, FL_MENU_TOGGLE,0,0,0,0},
      {0,0,0,0,0,0,0,0,0}
   };

   popup_bw = bw;

   if (prefs.load_stylesheets)
      pm[0].set();
   if (prefs.parse_embedded_css)
      pm[1].set();
   if (prefs.load_images)
      pm[2].set();

   item = pm->popup(wid->x(), wid->y() + wid->h());
   if (item)
      ((Fl_Widget *)item)->do_callback();
}

