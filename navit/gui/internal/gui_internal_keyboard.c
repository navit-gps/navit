#include <glib.h>
#include <stdlib.h>
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"

/**
 * @brief Switch keyboard mode to uppercase if it's in lowercase mode and  {@code VKBD_MODE_2} is set.
 *
 * Called when there's no input left in the input field.
 *
 * @param this The internal GUI instance
 */
void gui_internal_keyboard_to_upper_case(struct gui_priv *this) {
    struct menu_data *md;

    if (!this->keyboard)
        return;

    md=gui_internal_menu_data(this);

    if (md->keyboard_mode == (VKBD_LATIN_LOWER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_LATIN_UPPER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_UMLAUT_LOWER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_UMLAUT_UPPER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_CYRILLIC_LOWER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_CYRILLIC_UPPER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_GREEK_LOWER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_GREEK_UPPER | VKBD_FLAG_2);
}

/**
 * @brief Switch keyboard mode to lowercase if it's in uppercase mode and  {@code VKBD_MODE_2} is set.
 *
 * Called on each alphanumeric input.
 *
 * @param this The internal GUI instance
 */
void gui_internal_keyboard_to_lower_case(struct gui_priv *this) {
    struct menu_data *md;

    if (!this->keyboard)
        return;

    md=gui_internal_menu_data(this);

    if (md->keyboard_mode == (VKBD_LATIN_UPPER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_LATIN_LOWER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_UMLAUT_UPPER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_UMLAUT_LOWER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_CYRILLIC_UPPER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_CYRILLIC_LOWER | VKBD_FLAG_2);
    if (md->keyboard_mode == (VKBD_GREEK_UPPER | VKBD_FLAG_2))
        gui_internal_keyboard_do(this, md->keyboard, VKBD_GREEK_LOWER | VKBD_FLAG_2);
}

/**
 * @brief Processes a key press on the internal GUI keyboard
 *
 * If the keyboard is currently in uppercase mode and {@code VKBD_MODE_2} is set, it is tswitched to
 * the corresponding lowercase mode in {@code gui_internal_keypress_do}.
 *
 * @param this The internal GUI instance
 * @param wm
 * @param data Not used
 */
static void gui_internal_cmd_keypress(struct gui_priv *this, struct widget *wm, void *data) {
    gui_internal_keypress_do(this, (char *) wm->data);
}

static struct widget *gui_internal_keyboard_key_data(struct gui_priv *this, struct widget *wkbd, char *text, int font,
        void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data, void (*data_free)(void *data), int w,
        int h) {
    struct widget *wk;
    gui_internal_widget_append(wkbd, wk=gui_internal_button_font_new_with_callback(this, text, font,
                                        NULL, gravity_center|orientation_vertical, func, data));
    wk->data_free=data_free;
    wk->background=this->background;
    wk->bl=0;
    wk->br=0;
    wk->bt=0;
    wk->bb=0;
    wk->w=w;
    wk->h=h;
    return wk;
}

static struct widget *gui_internal_keyboard_key(struct gui_priv *this, struct widget *wkbd, char *text, char *key,
        int w, int h) {
    return gui_internal_keyboard_key_data(this, wkbd, text, 0, gui_internal_cmd_keypress, g_strdup(key), g_free_func,w,h);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data);


/**
 * @struct gui_internal_keyb_mode
 * @brief Describes a keyboard mode
 */
/**
 * @var gui_internal_keyb_modes
 * @brief A list of all available keyboard modes
 */
struct gui_internal_keyb_mode {
    char title[16];		/**< Label to be displayed on keys that switch to it */
    int font;			/**< Font size of label */
    int case_mode;		/**< Mode to switch to when case CHANGE() key is pressed. */
    int umlaut_mode;	/**< Mode to switch to when UMLAUT() key is pressed. */
} gui_internal_keyb_modes[]= {
    /* 0: VKBD_LATIN_UPPER   */ {"ABC", 2, VKBD_LATIN_LOWER,    VKBD_UMLAUT_UPPER},
    /* 8: VKBD_LATIN_LOWER   */ {"abc", 2, VKBD_LATIN_UPPER,    VKBD_UMLAUT_LOWER},
    /*16: VKBD_NUMERIC       */ {"123", 2, VKBD_LATIN_UPPER,    VKBD_UMLAUT_UPPER},
    /*24: VKBD_UMLAUT_UPPER  */ {"ÄÖÜ", 2, VKBD_UMLAUT_LOWER,   VKBD_LATIN_UPPER},
    /*32: VKBD_UMLAUT_LOWER  */ {"äöü", 2, VKBD_UMLAUT_UPPER,   VKBD_LATIN_LOWER},
    /*40: VKBD_CYRILLIC_UPPER*/ {"АБВ", 2, VKBD_CYRILLIC_LOWER, VKBD_LATIN_UPPER},
    /*48: VKBD_CYRILLIC_LOWER*/ {"абв", 2, VKBD_CYRILLIC_UPPER, VKBD_LATIN_LOWER},
    /*56: VKBD_DEGREE        */ {"DEG", 2, VKBD_FLAG_2,         VKBD_FLAG_2},
    /*64: VKBD_GREEK_UPPER   */ {"ABΓ", 2, VKBD_GREEK_LOWER,    VKBD_LATIN_UPPER},
    /*72: VKBD_GREEK_LOWER   */ {"abγ", 2, VKBD_GREEK_UPPER,    VKBD_LATIN_LOWER}
};


// Some macros that make the keyboard layout easier to visualise in
// the source code. The macros are #undef'd after this function.
#define KEY(x) gui_internal_keyboard_key(this, wkbd, (x), (x), max_w, max_h)
#define SPACER() gui_internal_keyboard_key_data(this, wkbd, "", 0, NULL, NULL, NULL,max_w,max_h)
#define MODE(x) gui_internal_keyboard_key_data(this, wkbd, \
		gui_internal_keyb_modes[(x)/8].title, \
		gui_internal_keyb_modes[(x)/8].font, \
		gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h) \
			-> datai = (mode & VKBD_MASK_7) | ((x) & VKBD_LAYOUT_MASK)
#define SWCASE() MODE(gui_internal_keyb_modes[mode/8].case_mode)
#define UMLAUT() MODE(gui_internal_keyb_modes[mode/8].umlaut_mode)


static void gui_internal_keyboard_topbox_resize(struct gui_priv *this, struct widget *w, void *data,
        int neww, int newh) {
    struct menu_data *md=gui_internal_menu_data(this);
    struct widget *old_wkbdb = md->keyboard;

    dbg(lvl_debug, "resize called for keyboard widget %p with w=%d, h=%d", w, neww, newh);
    gui_internal_keyboard_do(this, old_wkbdb, md->keyboard_mode);
}

/**
 * @brief Creates a new keyboard widget or switches the layout of an existing widget
 *
 * This is an internal helper function that is not normally called directly. To create a new keyboard
 * widget, GUI widgets should call {@link gui_internal_keyboard(struct gui_priv *, struct widget *, int)}.
 *
 * @param this The internal GUI instance
 * @param wkbdb The existing keyboard widget whose layout is to be switched, or {@code NULL} to create a
 * new keyboard widget
 * @param mode The new keyboard mode, see {@link gui_internal_keyboard(struct gui_priv *, struct widget *, int)}
 * for a description of possible values
 *
 * @return {@code wkbdb} if a non-NULL value was passed, else a new keyboard widget will be returned.
 */
struct widget *
gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode) {
    struct widget *wkbd,*wk;
    struct menu_data *md=gui_internal_menu_data(this);
    int i, max_w=this->root.w, max_h=this->root.h;
    int render=0;
    char *space="_";
    char *backspace="←";
    char *hide="▼";
    char *unhide="▲";

    if (wkbdb) {
        this->current.x=-1;
        this->current.y=-1;
        gui_internal_highlight(this);
        if (md->keyboard_mode & VKBD_FLAG_1024)
            render=2;
        else
            render=1;
        gui_internal_widget_children_destroy(this, wkbdb);
        gui_internal_widget_reset_pack(this, wkbdb);
        gui_internal_widget_pack(this, wkbdb);
    } else
        wkbdb=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
    md->keyboard=wkbdb;
    md->keyboard_mode=mode;
    wkbd=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
    wkbd->background=this->background;
    wkbd->cols=8;
    wkbd->spx=0;
    wkbd->spy=0;
    wkbd->on_resize=gui_internal_keyboard_topbox_resize;
    max_w=max_w/8;
    max_h=max_h/8; // Allows 3 results in the list when searching for Towns
    wkbd->p.y=max_h*2;
    if (((mode & VKBD_LAYOUT_MASK) == VKBD_CYRILLIC_UPPER)
            || ((mode & VKBD_LAYOUT_MASK) == VKBD_CYRILLIC_LOWER)
            || ((mode & VKBD_LAYOUT_MASK) == VKBD_GREEK_UPPER)
            || ((mode & VKBD_LAYOUT_MASK) == VKBD_GREEK_LOWER)) { // Russian/Ukrainian/Belarussian/Greek layout needs more space...
        max_h=max_h*4/5;
        max_w=max_w*8/9;
        wkbd->cols=9;
    }

    if ((mode & VKBD_LAYOUT_MASK) == VKBD_LATIN_UPPER) {
        for (i = 0 ; i < 26 ; i++) {
            char text[]= {'A'+i,'\0'};
            KEY(text);
        }
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
        if (!(mode & VKBD_MASK_7)) {
            KEY("-");
            KEY("'");
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
        } else {
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
            SWCASE();
            MODE(VKBD_NUMERIC);

        }
        UMLAUT();
        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_LATIN_LOWER) {
        for (i = 0 ; i < 26 ; i++) {
            char text[]= {'a'+i,'\0'};
            KEY(text);
        }
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
        if (!(mode & VKBD_MASK_7)) {
            KEY("-");
            KEY("'");
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
        } else {
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
            SWCASE();

            MODE(VKBD_NUMERIC);
        }
        UMLAUT();
        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_NUMERIC) {
        for (i = 0 ; i < 10 ; i++) {
            char text[]= {'0'+i,'\0'};
            KEY(text);
        }
        /* ("8")     ("9")*/KEY(".");
        KEY("°");
        KEY("'");
        KEY("\"");
        KEY("-");
        KEY("+");
        KEY("*");
        KEY("/");
        KEY("(");
        KEY(")");
        KEY("=");
        KEY("?");
        KEY(":");
        KEY("_");



        if (!(mode & VKBD_MASK_7)) {
            MODE(VKBD_GREEK_UPPER);
            KEY("-");
            KEY("'");
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
            SPACER();
            SPACER();
        } else {
            if (mode == VKBD_GREEK_UPPER)
                MODE(VKBD_GREEK_LOWER);
            else
                MODE(VKBD_GREEK_UPPER);
            MODE(VKBD_CYRILLIC_UPPER);
            MODE(VKBD_CYRILLIC_LOWER);
            wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
            wk->datai = mode | VKBD_FLAG_1024;
            MODE(VKBD_LATIN_UPPER);
            MODE(VKBD_LATIN_LOWER);
        }
        UMLAUT();
        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_UMLAUT_UPPER) {
        KEY("Ä");
        KEY("Ë");
        KEY("Ï");
        KEY("Ö");
        KEY("Ü");
        KEY("Æ");
        KEY("Ø");
        KEY("Å");
        KEY("Á");
        KEY("É");
        KEY("Í");
        KEY("Ó");
        KEY("Ú");
        KEY("Š");
        KEY("Č");
        KEY("Ž");
        KEY("À");
        KEY("È");
        KEY("Ì");
        KEY("Ò");
        KEY("Ù");
        KEY("Ś");
        KEY("Ć");
        KEY("Ź");
        KEY("Â");
        KEY("Ê");
        KEY("Î");
        KEY("Ô");
        KEY("Û");
        SPACER();

        UMLAUT();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_UMLAUT_LOWER) {
        KEY("ä");
        KEY("ë");
        KEY("ï");
        KEY("ö");
        KEY("ü");
        KEY("æ");
        KEY("ø");
        KEY("å");
        KEY("á");
        KEY("é");
        KEY("í");
        KEY("ó");
        KEY("ú");
        KEY("š");
        KEY("č");
        KEY("ž");
        KEY("à");
        KEY("è");
        KEY("ì");
        KEY("ò");
        KEY("ù");
        KEY("ś");
        KEY("ć");
        KEY("ź");
        KEY("â");
        KEY("ê");
        KEY("î");
        KEY("ô");
        KEY("û");
        KEY("ß");

        UMLAUT();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_CYRILLIC_UPPER) {
        KEY("А");
        KEY("Б");
        KEY("В");
        KEY("Г");
        KEY("Д");
        KEY("Е");
        KEY("Ж");
        KEY("З");
        KEY("И");
        KEY("Й");
        KEY("К");
        KEY("Л");
        KEY("М");
        KEY("Н");
        KEY("О");
        KEY("П");
        KEY("Р");
        KEY("С");
        KEY("Т");
        KEY("У");
        KEY("Ф");
        KEY("Х");
        KEY("Ц");
        KEY("Ч");
        KEY("Ш");
        KEY("Щ");
        KEY("Ъ");
        KEY("Ы");
        KEY("Ь");
        KEY("Э");
        KEY("Ю");
        KEY("Я");
        KEY("Ё");
        KEY("І");
        KEY("Ї");
        KEY("Ў");
        SPACER();
        SPACER();
        SPACER();
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

        wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
        wk->datai = mode | VKBD_FLAG_1024;

        SWCASE();

        MODE(VKBD_NUMERIC);

        SPACER();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_CYRILLIC_LOWER) {
        KEY("а");
        KEY("б");
        KEY("в");
        KEY("г");
        KEY("д");
        KEY("е");
        KEY("ж");
        KEY("з");
        KEY("и");
        KEY("й");
        KEY("к");
        KEY("л");
        KEY("м");
        KEY("н");
        KEY("о");
        KEY("п");
        KEY("р");
        KEY("с");
        KEY("т");
        KEY("у");
        KEY("ф");
        KEY("х");
        KEY("ц");
        KEY("ч");
        KEY("ш");
        KEY("щ");
        KEY("ъ");
        KEY("ы");
        KEY("ь");
        KEY("э");
        KEY("ю");
        KEY("я");
        KEY("ё");
        KEY("і");
        KEY("ї");
        KEY("ў");
        SPACER();
        SPACER();
        SPACER();
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

        wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
        wk->datai = mode | VKBD_FLAG_1024;

        SWCASE();

        MODE(VKBD_NUMERIC);

        SPACER();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_GREEK_UPPER) {
        KEY("Α");
        KEY("Β");
        KEY("Γ");
        KEY("Δ");
        KEY("Ε");
        KEY("Ζ");
        KEY("Η");
        KEY("Θ");
        KEY("Ι");
        KEY("Κ");
        KEY("Λ");
        KEY("Μ");
        KEY("Ν");
        KEY("Ξ");
        KEY("Ο");
        KEY("Π");
        KEY("Ρ");
        KEY("Σ");
        KEY("Τ");
        KEY("Υ");
        KEY("Φ");
        KEY("Χ");
        KEY("Ψ");
        KEY("Ω");
        KEY("Ή");
        KEY("Ά");
        KEY("Ό");
        KEY("Ί");
        KEY("Ώ");
        KEY("Έ");
        KEY("Ύ");
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

        wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
        wk->datai = mode | VKBD_FLAG_1024;

        SWCASE();

        MODE(VKBD_NUMERIC);

        SPACER();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }
    if ((mode & VKBD_LAYOUT_MASK) == VKBD_GREEK_LOWER) {
        KEY("α");
        KEY("β");
        KEY("γ");
        KEY("δ");
        KEY("ε");
        KEY("ζ");
        KEY("η");
        KEY("θ");
        KEY("ι");
        KEY("κ");
        KEY("λ");
        KEY("μ");
        KEY("ν");
        KEY("ξ");
        KEY("ο");
        KEY("π");
        KEY("ρ");
        KEY("σ");
        KEY("τ");
        KEY("υ");
        KEY("φ");
        KEY("χ");
        KEY("ψ");
        KEY("ω");
        KEY("ή");
        KEY("ά");
        KEY("ό");
        KEY("ί");
        KEY("ώ");
        KEY("έ");
        KEY("ύ");
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        SPACER();
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

        wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
        wk->datai = mode | VKBD_FLAG_1024;

        SWCASE();

        MODE(VKBD_NUMERIC);

        SPACER();

        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }


    if(md->search_list && md->search_list->type==widget_table) {
        struct table_data *td=(struct table_data*)(md->search_list->data);
        td->scroll_buttons.button_box_hide = !(mode & VKBD_FLAG_1024);
    }

    if ((mode & VKBD_LAYOUT_MASK) == VKBD_DEGREE) { /* special case for coordinates input screen (enter_coord) */
        KEY("0");
        KEY("1");
        KEY("2");
        KEY("3");
        KEY("4");
        SPACER();
        KEY("N");
        KEY("S");
        KEY("5");
        KEY("6");
        KEY("7");
        KEY("8");
        KEY("9");
        SPACER();
        KEY("E");
        KEY("W");
        KEY("°");
        KEY(".");
        KEY("'");
        gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
        SPACER();

        wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
        wk->datai = mode | VKBD_FLAG_1024;

        SPACER();
        gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
    }

    if (mode & VKBD_FLAG_1024) {
        char *text=NULL;
        int font=0;
        struct widget *wkl;
        mode &= ~VKBD_FLAG_1024;
        text=gui_internal_keyb_modes[mode/8].title;
        font=gui_internal_keyb_modes[mode/8].font;
        wk=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_fill);
        wk->func=gui_internal_keyboard_change;
        wk->data=wkbdb;
        wk->background=this->background;
        wk->bl=0;
        wk->br=0;
        wk->bt=0;
        wk->bb=0;
        wk->w=max_w;
        wk->h=max_h;
        wk->datai=mode;
        wk->state |= STATE_SENSITIVE;
        gui_internal_widget_append(wk, wkl=gui_internal_label_new(this, unhide));
        wkl->background=NULL;
        gui_internal_widget_append(wk, wkl=gui_internal_label_font_new(this, text, font));
        wkl->background=NULL;
        gui_internal_widget_append(wkbd, wk);
        if (render)
            render=2;
    }
    gui_internal_widget_append(wkbdb, wkbd);
    if (render == 1) {
        gui_internal_widget_pack(this, wkbdb);
        gui_internal_widget_render(this, wkbdb);
    } else if (render == 2) {
        gui_internal_menu_reset_pack(this);
        gui_internal_menu_render(this);
    }
    return wkbdb;
}
#undef KEY
#undef SPACER
#undef SWCASE
#undef UMLAUT
#undef MODE

/**
 * @brief Creates a keyboard widget.
 *
 * This function creates a widget to display the internal GUI keyboard.
 *
 * The {@code mode} argument specifies the type of keyboard which should initially be displayed. Refer
 * to {@link enum vkbd_mode} for a list of possible modes and their meaning.
 *
 * @param this The internal GUI instance
 * @param mode The mode for the keyboard
 *
 * @return A new keyboard widget
 */
struct widget *
gui_internal_keyboard(struct gui_priv *this, int mode) {
    if (! this->keyboard)
        return NULL;
    return gui_internal_keyboard_do(this, NULL, mode);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data) {
    gui_internal_keyboard_do(this, key->data, key->datai);
}

/**
 * @brief Returns the default keyboard mode for a country.
 *
 * The return value can be passed to {@link gui_internal_keyboard(struct gui_priv *, int)} and related
 * functions.
 *
 * @param lang The two-letter country code
 *
 * @return {@code VKBD_CYRILLIC_UPPER} for countries using the Cyrillic alphabet,
 * {@code VKBD_LATIN_UPPER} otherwise
 */
int gui_internal_keyboard_init_mode(char *lang) {
    int ret=0;
    /* do not crash if lang is NULL, which may be returned by getenv*/
    if(lang == NULL)
        return VKBD_LATIN_UPPER;

    /* Converting to upper case here is required for Android */
    lang=g_ascii_strup(lang,-1);
    /*
    * Set cyrillic keyboard for countries using Cyrillic alphabet
    */
    if (strstr(lang,"RU"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"UA"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"BY"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"RS"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"BG"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"MK"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"KZ"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"KG"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"TJ"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"MN"))
        ret = VKBD_CYRILLIC_UPPER;
    else if (strstr(lang,"GR"))
        ret = VKBD_GREEK_UPPER;
    g_free(lang);
    return ret;
}


/**
 * @brief Hides the platform's native on-screen keyboard or other input method
 *
 * This function is called as the {@code wfree} method of the placeholder widget for the platform's
 * native on-screen keyboard. It is a wrapper around the corresponding method of the graphics plugin,
 * which takes care of all platform-specific actions to hide the on-screen input method it previously
 * displayed.
 *
 * A call to this function indicates that Navit no longer needs the input method and is about to destroy
 * its placeholder widget. Navit will subsequently reclaim any screen real estate it may have previously
 * reserved for the input method.
 *
 * This function will free the {@code struct graphics_keyboard} pointed to by {@code w->data}
 *
 * @param this The internal GUI instance
 * @param w The placeholder widget
 */
static void gui_internal_keyboard_hide_native(struct gui_priv *this_, struct widget *w) {
    struct graphics_keyboard *kbd = (struct graphics_keyboard *) w->data;

    if (kbd) {
        graphics_hide_native_keyboard(this_->gra, kbd);
        g_free(kbd->lang);
        g_free(kbd->gui_priv);
    } else
        dbg(lvl_warning, "no graphics_keyboard found, cleanup failed");
    g_free(w);
}


/**
 * @brief Shows the platform's native on-screen keyboard or other input method
 *
 * This method is a wrapper around the corresponding method of the graphics plugin, which takes care of
 * all platform-specific details. In particular, it is up to the graphics plugin to determine how to
 * handle the request: it may show its on-screen keyboard or another input method (such as stroke
 * recognition). It may choose to simply ignore the request, which will typically occur when a hardware
 * keyboard (or other hardware input) is available.
 *
 * The platform's native input method may obstruct parts of Navit's UI. To prevent parts of the UI from
 * becoming unreachable, this method will insert an empty box widget in the appropriate size at the
 * appropriate position, provided the graphics plugin reports the correct values. Otherwise a zero-size
 * widget is inserted. If the graphics driver decides not to display an on-screen input method, no
 * widget will be created and the return value will be {@code NULL}.
 *
 * The widget's {@code wfree} function, to be called when the widget is destroyed, will be used to hide
 * the platform keyboard when it is no longer needed.
 *
 * @param this The internal GUI instance
 * @param w The parent of the widget requiring text input
 * @param mode The requested keyboard mode
 * @param lang The language for text input, used to select a keyboard layout
 *
 * @return The placeholder widget for the on-screen keyboard, may be {@code NULL}
 */
struct widget * gui_internal_keyboard_show_native(struct gui_priv *this, struct widget *w, int mode, char *lang) {
    struct widget *ret = NULL;
    struct menu_data *md = gui_internal_menu_data(this);
    struct graphics_keyboard *kbd = g_new0(struct graphics_keyboard, 1);
    int res;

    kbd->mode = mode;
    if (lang)
        kbd->lang = g_strdup(lang);
    res = graphics_show_native_keyboard(this->gra, kbd);

    switch(res) {
    case -1:
        dbg(lvl_error, "graphics has no show_native_keyboard method, cannot display keyboard");
    /* fall through */
    case 0:
        g_free(kbd);
        return NULL;
    }

    ret = gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
    md->keyboard = ret;
    md->keyboard_mode=mode;
    ret->wfree = gui_internal_keyboard_hide_native;
    if (kbd->h < 0) {
        ret->h = w->h;
        ret->hmin = w->hmin;
    } else
        ret->h = kbd->h;
    if (kbd->w < 0) {
        ret->w = w->w;
        ret->wmin = w->wmin;
    } else
        ret->w = kbd->w;
    dbg(lvl_error, "ret->w=%d, ret->h=%d", ret->w, ret->h);
    ret->data = (void *) kbd;
    gui_internal_widget_append(w, ret);
    dbg(lvl_error, "return");
    return ret;
}
