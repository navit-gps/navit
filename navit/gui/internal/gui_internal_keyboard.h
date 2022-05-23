/**
 * Modes for the on-screen keyboard
 */
enum vkbd_mode {
    /* layouts */
    VKBD_LATIN_UPPER = 0,		/*!< Latin uppercase characters */
    VKBD_LATIN_LOWER = 8,		/*!< Latin lowercase characters */
    VKBD_NUMERIC = 16,			/*!< Numeric keyboard */
    VKBD_UMLAUT_UPPER = 24,		/*!< Extended Latin uppercase characters */
    VKBD_UMLAUT_LOWER = 32,		/*!< Extended Latin lowercase characters */
    VKBD_CYRILLIC_UPPER = 40,	/*!< Cyrillic uppercase characters */
    VKBD_CYRILLIC_LOWER = 48,	/*!< Cyrillic lowercase characters */
    VKBD_DEGREE = 56,			/*!< Numeric keyboard with extra characters (NESW, degree, minute) for coordinate input */
    VKBD_GREEK_UPPER = 64,      /*!< Greek uppercase letters */
    VKBD_GREEK_LOWER = 72,      /*!< Greek lowercase letters */

    /* modifiers and masks */
    VKBD_FLAG_2 = 2,			/* FIXME seems to show alpha/num switch (VKBD_NUMERIC and VKBD_LATIN_* only) and switches to lowercase after first character */
    VKBD_MASK_7 = 7,			/* FIXME modifiers for layout? */
    VKBD_FLAG_1024 = 1024,		/* FIXME what is this for? Seems to have to do something with scroll box visibility */
    VKBD_LAYOUT_MASK = ~7,		/* when XORed with the mode, preserves only the layout FIXME document properly */
};

/* prototypes */
struct gui_priv;
struct widget;
struct widget *gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode);
struct widget *gui_internal_keyboard(struct gui_priv *this, int mode);
struct widget *gui_internal_keyboard_show_native(struct gui_priv *this, struct widget *w, int mode, char *lang);
int gui_internal_keyboard_init_mode(char *lang);
void gui_internal_keyboard_to_upper_case(struct gui_priv *this);
void gui_internal_keyboard_to_lower_case(struct gui_priv *this);
/* end of prototypes */
