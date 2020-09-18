/* Copyright (C) 2006-2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/skin/ui.h"

#include <stdbool.h>                     // for bool, true, false
#include <stdio.h>                       // for NULL, snprintf
#include <string.h>                      // for strcmp

#include "android/emulator-window.h"     // for emulator_window_set_no_skin
#include "android/skin/event.h"          // for SkinEvent, (anonymous struct...
#include "android/skin/file.h"           // for SkinLayout, SkinFile, skin_l...
#include "android/skin/generic-event.h"  // for skin_generic_event_create
#include "android/skin/image.h"          // for skin_image_unref, skin_image...
#include "android/skin/keyboard.h"       // for skin_keyboard_process_event
#include "android/skin/rect.h"           // for SkinRotation
#include "android/skin/trackball.h"      // for skin_trackball_create, skin_...
#include "android/skin/window.h"         // for skin_window_process_event
#include "android/utils/bufprint.h"      // for bufprint
#include "android/utils/debug.h"         // for dprint, VERBOSE_CHECK, VERBO...
#include "android/utils/system.h"        // for AFREE, ANEW0

#ifdef _WIN32
#include <windows.h>
#endif

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)
#define  DE(...) do { if (VERBOSE_CHECK(keys)) dprint(__VA_ARGS__); } while (0)
#define PD(...) {printf("[%s:%d:%s] ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__);printf("\n");}
#define PDE(...) {printf( __VA_ARGS__);printf("\n");}
#define DD(...) ((void)0)


struct SkinUI {
    SkinUIParams           ui_params;
    const SkinUIFuncs*     ui_funcs;

    SkinFile*              layout_file;
    SkinLayout*            layout;

    SkinKeyboard*          keyboard;
    SkinGenericEvent*      generic_events;

    SkinWindow*            window;

    bool                   show_trackball;
    SkinTrackBall*         trackball;

    int                    lcd_brightness;

    SkinImage*             onion;
    SkinRotation           onion_rotation;
    int                    onion_alpha;
};

static SkinLayout* skin_file_select_layout(SkinLayout* layouts,
        const char* layout_name) {
    if (!layout_name) return layouts;
    SkinLayout* currLayout = layouts;
    while (currLayout) {
        if (currLayout->name && !strcmp(currLayout->name, layout_name)) {
            return currLayout;
        }
        currLayout = currLayout->next;
    }
    return layouts;
}

SkinUI* skin_ui_create(SkinFile* layout_file,
                       const char* initial_orientation,
                       const SkinUIFuncs* ui_funcs,
                       const SkinUIParams* ui_params,
                       bool use_emugl_subwindow) {

	DD("layout_file 0x%x",(int)layout_file);
	DD("ui_fincs 0x%x",(int)ui_funcs);

    SkinUI* ui;

    ANEW0(ui);

    ui->layout_file = layout_file;
    ui->layout = skin_file_select_layout(layout_file->layouts, initial_orientation);

    ui->ui_funcs = ui_funcs;
    ui->ui_params = ui_params[0];

	DD("ui 0x%x",(int)ui);
	DD("ui-params name %s",ui->ui_params.window_name);
	DD("	win x,y[%d %d]",ui->ui_params.window_x,ui->ui_params.window_y);
	DD("	enable_touch %d",ui->ui_params.enable_touch);
	DD("	enable_dpad %d",ui->ui_params.enable_dpad);
	DD("	enable_keyboard %d",ui->ui_params.enable_keyboard);
	DD("	enable_trackball %d",ui->ui_params.enable_trackball);
	DD("	enable_scale %d",ui->ui_params.enable_scale);
	DD("ui-layout dpad_rotatione %d",ui->layout->dpad_rotation);

    ui->keyboard = skin_keyboard_create(ui->ui_params.keyboard_charmap,
                                        ui->layout->dpad_rotation,
                                        ui_funcs->keyboard_flush);
    ui->generic_events =
            skin_generic_event_create(ui_funcs->generic_event_flush);
    ui->window = NULL;

    DD("skin_window_create");
    DD("use emugl_subwinow %d",use_emugl_subwindow);
    DD("ui->ui_funcs->window_funcs 0x%x",(int)ui->ui_funcs->window_funcs);
    ui->window = skin_window_create(
            ui->layout, ui->ui_params.window_x, ui->ui_params.window_y,
            ui->ui_params.enable_scale,
            use_emugl_subwindow, ui->ui_funcs->window_funcs);
    DD("ui->window 0x%x",(int)ui->window);
    if (!ui->window) {
        skin_ui_free(ui);
        return NULL;
    }

    if (ui->ui_params.enable_trackball) {
        ui->trackball = skin_trackball_create(ui->ui_funcs->trackball_params);
        skin_window_set_trackball(ui->window, ui->trackball);
    }

    ui->lcd_brightness = 128;  /* 50% */
    skin_window_set_lcd_brightness(ui->window, ui->lcd_brightness );

    if (ui->onion) {
        skin_window_set_onion(ui->window,
                              ui->onion,
                              ui->onion_rotation,
                              ui->onion_alpha);
    }

    skin_ui_reset_title(ui);

    skin_window_enable_touch(ui->window, ui->ui_params.enable_touch);
    skin_window_enable_dpad(ui->window, ui->ui_params.enable_dpad);
    skin_window_enable_qwerty(ui->window, ui->ui_params.enable_keyboard);
    skin_window_enable_trackball(ui->window, ui->ui_params.enable_trackball);

    return ui;
}

void skin_ui_free(SkinUI* ui) {
    if (ui->window) {
        skin_window_free(ui->window);
        ui->window = NULL;
    }
    if (ui->trackball) {
        skin_trackball_destroy(ui->trackball);
        ui->trackball = NULL;
    }
    if (ui->keyboard) {
        skin_keyboard_free(ui->keyboard);
        ui->keyboard = NULL;
    }
    if (ui->generic_events) {
        skin_generic_event_free(ui->generic_events);
        ui->generic_events = NULL;
    }

    skin_image_unref(&ui->onion);

    ui->layout = NULL;

    AFREE(ui);
}

void skin_ui_set_lcd_brightness(SkinUI* ui, int lcd_brightness) {
    ui->lcd_brightness = lcd_brightness;
    if (ui->window) {
        skin_window_set_lcd_brightness(ui->window, lcd_brightness);
    }
}

void skin_ui_set_scale(SkinUI* ui, double scale) {
    if (ui->window) {
        skin_window_set_scale(ui->window, scale);
    }
}

void skin_ui_reset_title(SkinUI* ui) {
    char  temp[128], *p=temp, *end = p + sizeof(temp);

    if (ui->window == NULL)
        return;

    if (ui->show_trackball) {
        p = bufprint(p, end, "Press Ctrl-T to leave trackball mode. ");
    }

    p = bufprint(p, end, "%s", ui->ui_params.window_name);
    skin_window_set_title(ui->window, temp);
}

void skin_ui_set_onion(SkinUI* ui,
                       SkinImage* onion,
                       SkinRotation onion_rotation,
                       int onion_alpha) {
    if (onion) {
        skin_image_ref(onion);
    }
    skin_image_unref(&ui->onion);

    ui->onion = onion;
    ui->onion_rotation = onion_rotation;
    ui->onion_alpha = onion_alpha;

    if (ui->window) {
        skin_window_set_onion(ui->window,
                              onion,
                              onion_rotation,
                              onion_alpha);
    }
}

static void skin_ui_switch_to_layout4Dual(SkinUI* ui, SkinLayout* layout) {
    ui->layout = layout;
    skin_window_reset(ui->window, layout);
    SkinRotation rotation =layout->orientation;

    if (ui->keyboard) {
        skin_keyboard_set_rotation(ui->keyboard, rotation);
    }

    if (ui->trackball) {
        skin_trackball_set_rotation(ui->trackball, rotation);
        skin_window_set_trackball(ui->window, ui->trackball);
        skin_window_show_trackball(ui->window, ui->show_trackball);
    }
    skin_window_set_lcd_brightness(ui->window, ui->lcd_brightness);
    ui->ui_funcs->framebuffer_invalidate();
}
static void skin_ui_switch_to_layout(SkinUI* ui, SkinLayout* layout) {
    ui->layout = layout;
    skin_window_reset(ui->window, layout);
    SkinRotation rotation = skin_layout_get_dpad_rotation(layout);

    if (ui->keyboard) {
        skin_keyboard_set_rotation(ui->keyboard, rotation);
    }

    if (ui->trackball) {
        skin_trackball_set_rotation(ui->trackball, rotation);
        skin_window_set_trackball(ui->window, ui->trackball);
        skin_window_show_trackball(ui->window, ui->show_trackball);
    }
    skin_window_set_lcd_brightness(ui->window, ui->lcd_brightness);
    ui->ui_funcs->framebuffer_invalidate();
}

static void _skin_ui_handle_rotate_key_command(SkinUI* ui, bool next) {
    SkinLayout* layout = NULL;

    if (next) {
        layout = ui->layout->next;
        if (layout == NULL)
            layout = ui->layout_file->layouts;
    } else {
        layout = ui->layout_file->layouts;
        while (layout->next && layout->next != ui->layout)
            layout = layout->next;
    }
    if (layout != NULL) {
        skin_ui_switch_to_layout(ui, layout);
    }
}

void skin_ui_select_next_layout(SkinUI* ui) {
    _skin_ui_handle_rotate_key_command(ui, true);
}

void skin_ui_update_rotation(SkinUI* ui, SkinRotation rotation) {
    skin_window_update_rotation(ui->window, rotation);
}

bool skin_ui_rotate(SkinUI* ui, SkinRotation rotation) {
    SkinLayout* l;
    for (l = ui->layout_file->layouts;
         l;
         l = l->next) {
        if (l->orientation == rotation) {
            skin_ui_switch_to_layout(ui, l);
            return true;
        }
    }
    return false;
}

bool skin_ui_update_and_rotate(SkinUI* ui,
                               SkinFile* layout_file,
                               SkinRotation rotation) {
    ui->layout_file = layout_file;
    skin_ui_rotate(ui, rotation);
    return true;
}

bool skin_ui_process_events(SkinUI* ui) {
    SkinEvent ev;

    // If a scrolled window is zoomed or resized while the scroll bars
    // are moved, Qt window scroll events are created as the window resizes.
    // They will be in the event queue behind the set-scale or set-zoom. Because
    // scroll events work by "moving" the GL sub-window when using host GPU and
    // finding its intersection with the Qt window, scroll events produced by a
    // resize should be ignored, since they may move the GL sub-window far enough
    // that it no longer intersects the Qt window at its current size.
    bool ignoreScroll = false;

    // Enable mouse tracking so we can change the cursor when it's at a corner
    skin_enable_mouse_tracking(true);

    while(skin_event_poll(&ev)) {
        switch(ev.type) {
        case kEventForceRedraw:
            DE("EVENT: kEventForceRedraw\n");
            skin_window_redraw(ui->window, NULL);
            break;

        case kEventKeyDown:
            DE("EVENT: kEventKeyDown scancode=%d mod=0x%x\n",
               ev.u.key.keycode, ev.u.key.mod);
            skin_keyboard_process_event(ui->keyboard, &ev, 1);
            break;

        case kEventKeyUp:
            DE("EVENT: kEventKeyUp scancode=%d mod=0x%x\n",
               ev.u.key.keycode, ev.u.key.mod);
            skin_keyboard_process_event(ui->keyboard, &ev, 0);
            break;

        case kEventGeneric:
            DE("EVENT: kEventGeneric type=0x%02x code=0x%03x val=%x\n",
               ev.u.generic_event.type, ev.u.generic_event.code,
               ev.u.generic_event.value);
            skin_generic_event_process_event(ui->generic_events, &ev);
            break;

        case kEventTextInput:
            DE("EVENT: kEventTextInput text=[%s] down=%s\n",
               ev.u.text.text, ev.u.text.down ? "true" : "false");
            skin_keyboard_process_event(ui->keyboard, &ev, ev.u.text.down);
            break;

        case kEventMouseMotion:
            DE("EVENT: kEventMouseMotion x=%d y=%d xrel=%d yrel=%d button=%d\n",
               ev.u.mouse.x, ev.u.mouse.y, ev.u.mouse.xrel, ev.u.mouse.yrel,
               ev.u.mouse.button);
            skin_window_process_event(ui->window, &ev);
            break;
        case kEventLayoutRotate:
            DE("EVENT: kEventLayoutRotate orientation=%d\n",
                ev.u.layout_rotation.rotation);
            skin_ui_rotate(ui, ev.u.layout_rotation.rotation);
            break;
        case kEventMouseButtonDown:
        case kEventMouseButtonUp:
            DE("EVENT: kEventMouseButton x=%d y=%d xrel=%d yrel=%d button=%d\n",
               ev.u.mouse.x, ev.u.mouse.y, ev.u.mouse.xrel, ev.u.mouse.yrel,
               ev.u.mouse.button);
            if (ev.u.mouse.button == kMouseButtonLeft ||
                ev.u.mouse.button == kMouseButtonSecondaryTouch) {
                skin_window_process_event(ui->window, &ev);
            }
            break;

        case kEventScrollBarChanged:
            DE("EVENT: kEventScrollBarChanged x=%d xmax=%d y=%d ymax=%d ignored=%d\n",
               ev.u.scroll.x, ev.u.scroll.xmax, ev.u.scroll.y, ev.u.scroll.ymax, ignoreScroll);
            if (!ignoreScroll) {
                skin_window_scroll_updated(ui->window, ev.u.scroll.x, ev.u.scroll.xmax,
                                                       ev.u.scroll.y, ev.u.scroll.ymax);
            }
            break;
        case kEventRotaryInput:
            DE("EVENT: kEventRotaryInput delta=%d\n",
               ev.u.rotary_input.delta);
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventSetDisplayRegion:
            DE("EVENT: kEventSetDisplayRegion (%d, %d) %d x %d\n",
               ev.u.display_region.xOffset, ev.u.display_region.yOffset,
               ev.u.display_region.width, ev.u.display_region.height);

            skin_window_set_layout_region(ui->window,
                                          ev.u.display_region.xOffset, ev.u.display_region.yOffset,
                                          ev.u.display_region.width,   ev.u.display_region.height);
            break;

        case kEventSetDisplayRegionAndUpdate:
            DE("EVENT: kEventSetDisplayRegionAndUpdate (%d, %d) %d x %d\n",
               ev.u.display_region.xOffset, ev.u.display_region.yOffset,
               ev.u.display_region.width, ev.u.display_region.height);
            skin_window_set_display_region_and_update(ui->window,
                                                      ev.u.display_region.xOffset,
                                                      ev.u.display_region.yOffset,
                                                      ev.u.display_region.width,
                                                      ev.u.display_region.height);
            break;

        case kEventSetMultiDisplay:
            DE("EVENT: kEventSetMultiDisplay %d (%d, %d) %d x %d %s\n",
               ev.u.multi_display.id, ev.u.multi_display.xOffset, ev.u.multi_display.yOffset,
               ev.u.multi_display.width, ev.u.multi_display.height,
               ev.u.multi_display.add ? "add" : "delete");
            skin_window_set_multi_display(ui->window,
                                          ev.u.multi_display.id,
                                          ev.u.multi_display.xOffset,
                                          ev.u.multi_display.yOffset,
                                          ev.u.multi_display.width,
                                          ev.u.multi_display.height,
                                          ev.u.multi_display.add);
            break;

        case kEventSetScale:
            DE("EVENT: kEventSetScale scale=%f\n", ev.u.window.scale);
            ignoreScroll = true;
            skin_window_set_scale(ui->window, ev.u.window.scale);
            break;

        case kEventSetZoom:
            DE("EVENT: kEventSetZoom x=%d y=%d zoom=%f scroll_h=%d\n",
               ev.u.window.x, ev.u.window.y, ev.u.window.scale, ev.u.window.scroll_h);
            skin_window_set_zoom(ui->window, ev.u.window.scale, ev.u.window.x, ev.u.window.y,
                                             ev.u.window.scroll_h);
            break;

        case kEventQuit:
            DE("EVENT: kEventQuit\n");
            /* only save emulator config through clean exit */
            return true;

        case kEventWindowMoved:
            DE("EVENT: kEventWindowMoved x=%d y=%d\n", ev.u.window.x, ev.u.window.y);
            skin_window_position_changed(ui->window, ev.u.window.x, ev.u.window.y);
            break;

        case kEventScreenChanged:
            DE("EVENT: kEventScreenChanged\n");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventWindowChanged:
            DE("EVENT: kEventWindowChanged\n");
            skin_window_process_event(ui->window, &ev);
            break;

        case kEventZoomedWindowResized:
            DE("EVENT: kEventZoomedWindowResized dx=%d dy=%d w=%d h=%d\n",
               ev.u.scroll.x, ev.u.scroll.y, ev.u.scroll.xmax, ev.u.scroll.ymax);
            skin_window_zoomed_window_resized(ui->window, ev.u.scroll.x, ev.u.scroll.y,
                                                          ev.u.scroll.xmax, ev.u.scroll.ymax,
                                                          ev.u.scroll.scroll_h);
            break;
        case kEventToggleTrackball:
            if (ui->ui_params.enable_trackball) {
                ui->show_trackball = !ui->show_trackball;
                skin_window_show_trackball(ui->window, ui->show_trackball);
                skin_ui_reset_title(ui);
            }
            break;
        case kEventSetNoSkin:
            emulator_window_set_no_skin();
            break;
        case kEventRestoreSkin:
            emulator_window_restore_skin();
            break;
        case kEventSetDualSkin:
            DE("EVENT: kEventSetDualSkin =%d\n", ev.u.dual_skin.mode);
            skin_window_update_dualskin(ev.u.dual_skin.mode);
            break;
        case kEventSetFrameMode:
            DE("EVENT: kEventSetFrameMode =%d\n", ev.u.frame_mode.mode);
            skin_window_update_framemode(ev.u.frame_mode.mode);
            break;
        case kEventReconfigureLayout:
            skin_reconfigure_layout(ev.u.reconfigure.w,ev.u.reconfigure.h,ev.u.reconfigure.dual);
            break;

        }
    }

    skin_keyboard_flush(ui->keyboard);
    return false;
}

// to support multi display rotation & skins
void skin_reconfigure_layout(uint32_t w,uint32_t h, bool dual){
static int save_off=0,save_offX = 0,save_offY = 0;
static int save_off_x = 0,save_off_y = 0;
    int i,rot;
    EmulatorWindow* emulator = emulator_window_get();
    SkinPart* part = emulator->layout_file->parts;
    SkinDisplay* display;

    DD("Reconfigure Layout w,h[%d %d] dual %d",w,h,dual);
    // no skin case
    if ( (! part->name ) || (!strcmp(part->name,"")) ) {
        DD("No part name " );
        display = part->display;
        DD("Set w,h[%d %d]-> [%d %d]",display->rect.size.w,display->rect.size.h,w,h);
        if ( (display->rect.size.w == w ) && ( display->rect.size.h == h )) {
                DD("Same no need resize");
                return;
        }
        // display surface resize
        display->rect.size.w = w;
        display->rect.size.h = h;

        skin_resizeLayout(w,h,0,0);
        return;
    }
    // skin case
    for(i=0;i<20;i++) {
        if ( ! part ) break;
        display = part->display;
        DD("part-name %s",part->name);
        if ( !strcmp(part->name,"device")) {
                //skin
                display = part->display;
                rot = display->rotation;
                if ( (rot %2 )) {
                    DD("name %s rot %d rect[%d %d]->[%d %d]",part->name,rot,display->rect.size.w,display->rect.size.h,h,w);
                    display->rect.size.w = h;
                    display->rect.size.h = w;
                    display->rotation = (SkinRotation)rot;
                }
                else  {
                    DD("name %s rot %d rect[%d %d]->[%d %d]",part->name,rot,display->rect.size.w,display->rect.size.h,w,h);
                    display->rect.size.w = w;
                    display->rect.size.h = h;
                    display->rotation = (SkinRotation)rot;
                }
        }
        part = part->next;
    }

    SkinLayout* eLayouts = emulator->layout_file->layouts;
    SkinLocation* eSkinLocation = (SkinLocation *)0;
DD("save_off %d",save_off);
    if ( ! save_off ) {
      for(;;) {
        if ( !eLayouts ) {
                break;
        }

        rot = eLayouts->orientation;
        eSkinLocation = eLayouts->locations;
        for(;;) {
                if ( !eSkinLocation ) break;
                DD("SkinLocation part %s",eSkinLocation->part->name);
                if ( !strcmp(eSkinLocation->part->name,"device" ) ){
                        break;
                }
                eSkinLocation = eSkinLocation->next;
        }
        if ( !eSkinLocation ) break;

        if ( rot == 0 ) {
                save_offX = eSkinLocation->anchor.x ;
                save_offY = eSkinLocation->anchor.y ;
        }

        if ( rot == 2 ) {
                if ( !save_off_x )
                save_off_x = eSkinLocation->anchor.x ;
        }
        if ( rot == 3 ) {
                if ( !save_off_y )
                save_off_y = eSkinLocation->anchor.y ;
        }
        eLayouts = eLayouts->next;
      }
      save_off = 1;
    }

    eLayouts = emulator->layout_file->layouts;
    eSkinLocation = (SkinLocation *)0;

    DD("eLayout w,h[%d %d]",eLayouts->size.w,eLayouts->size.h);
    int skin_w = eLayouts->size.w;
    int skin_h = eLayouts->size.h;
    int offX = skin_w - w - save_offX;
    int offY = skin_h - h - save_offY;

    for(;;) {
        if ( !eLayouts ) break;

        rot = eLayouts->orientation;
        eSkinLocation = eLayouts->locations;
        for(;;) {
                if ( !eSkinLocation ) break;
                DD("SkinLocation name  %s",eSkinLocation->part->name);
                if ( !strcmp(eSkinLocation->part->name,"device" ) ){
                        break;
                }
                eSkinLocation = eSkinLocation->next;
        }
        if ( !eSkinLocation ) break;

        DD("Dual status %d",dual);
        if ( dual ) {
            if ( rot == 1 ) {
                eSkinLocation->anchor.x = offY;
                eSkinLocation->anchor.y = save_offX;
            }
            else if ( rot == 2 ) {
                eSkinLocation->anchor.x = offX;
                eSkinLocation->anchor.y = offY;
            }
            else if ( rot == 3 ) {
                eSkinLocation->anchor.x = save_offY;
                eSkinLocation->anchor.y = offX;
            }
            else {
                eSkinLocation->anchor.x = save_offX;
                eSkinLocation->anchor.y = save_offY;
            }
        }
        else {
            if ( rot == 1 ) {
                eSkinLocation->anchor.x = save_offY;
                eSkinLocation->anchor.y = save_offX;
            }
            else if ( rot == 2 ) {
                eSkinLocation->anchor.x = save_off_x;
                eSkinLocation->anchor.y = save_offY;
            }
            else if ( rot == 3 ) {
                eSkinLocation->anchor.x = save_offY ;
                eSkinLocation->anchor.y = save_off_y;
            }
        }
        eLayouts = eLayouts->next;
    }
}

void skin_resizeLayout(uint32_t w, uint32_t h,uint32_t offX, uint32_t offY) {
        DD("resizeLayout w,h[%d %d],offsetx,y[%d %d]",w,h,offX,offY);
        int rot;
        SkinLocation* eSkinLocation ;
        EmulatorWindow* emulator = emulator_window_get();
        SkinFile* eLayout_file = emulator->layout_file;
        SkinLayout* eLayouts = eLayout_file->layouts;
        for(;;) {
            if ( !eLayouts ) break;
            if ( !eLayouts->name ) { // No Skin Case
                eSkinLocation = eLayouts->locations;

                eLayouts->event_type = 5;
                eLayouts->event_code = 0;
                eLayouts->event_value = 1;
                eLayouts->color = 0xff808080;
                eLayouts->has_dpad_rotation = (int)0;
                eLayouts->dpad_rotation = (SkinRotation)0;

                rot = eLayouts->orientation;
                if ( rot == 0 ) {
                        eLayouts->size.w = w;
                        eLayouts->size.h = h;
                        eSkinLocation->anchor.x = offX;
                        eSkinLocation->anchor.y = offY;
                        eSkinLocation->rotation = (SkinRotation)0;
                }
                else if ( rot == 1 ) {
                        eLayouts->size.w = h;
                        eLayouts->size.h = w;
                        eSkinLocation->anchor.x = offX + h;
                        eSkinLocation->anchor.y = offY ;
                        eSkinLocation->rotation = (SkinRotation)1;
                }
                else if ( rot == 2 ) {
                        eLayouts->size.w = w;
                        eLayouts->size.h = h;
                        eSkinLocation->anchor.x = offX + w;
                        eSkinLocation->anchor.y = offY + h;
                        eSkinLocation->rotation = (SkinRotation)2;
                }
                else  {
                        eLayouts->size.w = h;
                        eLayouts->size.h = w;
                        eSkinLocation->anchor.x = offX ;
                        eSkinLocation->anchor.y = offY + w;
                        eSkinLocation->rotation = (SkinRotation)3;
                }
                DD("Updated Layout w,h[%d %d], anchor x,y[%d %d] rot %d",
                        eLayouts->size.w,eLayouts->size.h,
                        eSkinLocation->anchor.x,eSkinLocation->anchor.y,
                        eSkinLocation->rotation);

                eSkinLocation = eSkinLocation->next;
                if ( eSkinLocation ) {
                        DD("Have next SkinLocation");
                }
            }
            eLayouts = eLayouts->next;
        }
}

void skin_ui_update_display(SkinUI* ui, int x, int y, int w, int h) {
    if (ui->window) {
        skin_window_update_display(ui->window, x, y, w, h);
    }
}

void skin_ui_update_gpu_frame(SkinUI* ui, int w, int h, const void* pixels) {
    if (ui->window) {
        skin_window_update_gpu_frame(ui->window, w, h, pixels);
    }
}

SkinLayout* skin_ui_get_current_layout(const SkinUI* ui) {
    return ui->layout;
}

SkinLayout* skin_ui_get_next_layout(const SkinUI* ui) {
    return ui->layout->next ? ui->layout->next : ui->layout_file->layouts;
}

SkinLayout* skin_ui_get_prev_layout(const SkinUI* ui) {
    SkinLayout* layout = ui->layout_file->layouts;
    while (layout->next && layout->next != ui->layout) {
        layout = layout->next;
    }
    return layout;
}

void skin_ui_set_name(SkinUI* ui, const char* name) {
    snprintf(ui->ui_params.window_name,
             sizeof(ui->ui_params.window_name),
             "%s",
             name);
    skin_ui_reset_title(ui);
}

bool skin_ui_is_trackball_active(SkinUI* ui) {
    return (ui && ui->ui_params.enable_trackball && ui->show_trackball);
}
