/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <FreeImage.h>
#include <time.h>

#include "item.h"
#include "attr.h"
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "event.h"
#include "debug.h"
#include "callback.h"
#include "keys.h"
#include "window.h"
#include "navit/font/freetype/font_freetype.h"
#include <GL/glc.h>

#if defined(WINDOWS) || defined(WIN32)
#include <windows.h>
# define sleep(i) Sleep(i * 1000)
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>		/* glut.h includes gl.h and glu.h */
#endif

#define SCREEN_WIDTH 700
#define SCREEN_HEIGHT 700

//#define MIRRORED_VIEW 1

struct graphics_gc_priv
{
  struct graphics_priv *gr;
  float fr, fg, fb, fa;
  float br, bg, bb, ba;
  int linewidth;
  unsigned char *dash_list;
  int dash_count;
  int dash_mask;
} graphics_gc_priv;

struct graphics_priv
{
  int button_timeout;
  struct point p;
  int width;
  int height;
  int library_init;
  int visible;
  int overlay_enabled;
  int overlay_autodisabled;
  int wraparound;
  struct graphics_priv *parent;
  struct graphics_priv *overlays;
  struct graphics_priv *next;
  struct graphics_gc_priv *background_gc;
  enum draw_mode_num mode;
  void (*resize_callback) (void *data, int w, int h);
  void *resize_callback_data;
  void (*motion_callback) (void *data, struct point * p);
  void *motion_callback_data;
  void (*button_callback) (void *data, int press, int button,
			   struct point * p);
  void *button_callback_data;
  GLuint DLid;
  struct callback_list *cbl;
  struct font_freetype_methods freetype_methods;
  struct navit *nav;
  int timeout;
  int delay;
  struct window window;
  int dirty;                 //display needs to be redrawn (draw on root graphics or overlay is done)
  int force_redraw;                 //display needs to be redrawn (draw on root graphics or overlay is done)
  time_t last_refresh_time;  //last display refresh time
};

static struct graphics_priv *graphics_priv_root;
struct graphics_image_priv
{
  int w;
  int h;
  int hot_x;
  int hot_y;
  unsigned char *data;
  char *path;
} graphics_image_priv;

struct mouse_event_queue_element {
  int button;
  int state;
  int x;
  int y;
};

static const int mouse_event_queue_size = 100;
static int mouse_event_queue_begin_idx = 0;
static int mouse_event_queue_end_idx = 0;
static struct mouse_event_queue_element mouse_queue[100];

//hastable for uncompressed image data
static GHashTable *hImageData;

/*  prototypes */
void CALLBACK tessBeginCB (GLenum which);
void CALLBACK tessEndCB ();
void CALLBACK tessErrorCB (GLenum errorCode);
void CALLBACK tessVertexCB (const GLvoid * data);
void CALLBACK tessVertexCB2 (const GLvoid * data);
void tessCombineCB (GLdouble c[3], void *d[4], GLfloat w[4], void **out);

static struct graphics_priv *graphics_opengl_new_helper (struct
							 graphics_methods
							 *meth);
static void display (void);
static void resize_callback (int w, int h);
static void glut_close ();
const char *getPrimitiveType (GLenum type);

static void
graphics_destroy (struct graphics_priv *gr)
{
  /*FIXME graphics_destroy is never called*/
  /*TODO add destroy code for image cache(delete entries in hImageData)*/
  g_free (gr);
  gr = NULL;
}

static void
gc_destroy (struct graphics_gc_priv *gc)
{
  g_free (gc);
  gc = NULL;
}

static void
gc_set_linewidth (struct graphics_gc_priv *gc, int w)
{
  gc->linewidth = w;
}

static void
gc_set_dashes (struct graphics_gc_priv *gc, int width, int offset,
	       unsigned char *dash_list, int n)
{
  int i;
  const int cOpenglMaskBits = 16;
  gc->dash_count = n;
  if (1 == n)
    {
      gc->dash_mask = 0;
      for (i = 0; i < cOpenglMaskBits; ++i)
	{
	  gc->dash_mask <<= 1;
	  gc->dash_mask |= (i / n) % 2;
	}
    }
  else if (1 < n)
    {
      unsigned char *curr = dash_list;
      int cnt = 0;		//dot counter
      int dcnt = 0;		//dash element counter
      int sum_dash = 0;
      gc->dash_mask = 0;

      for (i = 0; i < n; ++i)
	{
	  sum_dash += dash_list[i];
	}

      //scale dashlist elements to max size
      if (sum_dash > cOpenglMaskBits)
	{
	  int num_error[2] = { 0, 0 };	//count elements rounded to 0 for odd(drawn) and even(masked) for compensation
	  double factor = (1.0 * cOpenglMaskBits) / sum_dash;
	  for (i = 0; i < n; ++i)
	    {			//calculate dashlist max and largest common denomiator for scaling
	      dash_list[i] *= factor;
	      if (dash_list[i] == 0)
		{
		  ++dash_list[i];
		  ++num_error[i % 2];
		}
	      else if (0 < num_error[i % 2] && 2 < dash_list[i])
		{
		  ++dash_list[i];
		  --num_error[i % 2];
		}
	    }
	}

      //calculate mask
      for (i = 0; i < cOpenglMaskBits; ++i)
	{
	  gc->dash_mask <<= 1;
	  gc->dash_mask |= 1 - dcnt % 2;
	  ++cnt;
	  if (cnt == *curr)
	    {
	      cnt = 0;
	      ++curr;
	      ++dcnt;
	      if (dcnt == n)
		{
		  curr = dash_list;
		}
	    }
	}
    }
}


static void
gc_set_foreground (struct graphics_gc_priv *gc, struct color *c)
{
  gc->fr = c->r / 65535.0;
  gc->fg = c->g / 65535.0;
  gc->fb = c->b / 65535.0;
  gc->fa = c->a / 65535.0;
}

static void
gc_set_background (struct graphics_gc_priv *gc, struct color *c)
{
  gc->br = c->r / 65535.0;
  gc->bg = c->g / 65535.0;
  gc->bb = c->b / 65535.0;
  gc->ba = c->a / 65535.0;
}

static struct graphics_gc_methods gc_methods = {
  gc_destroy,
  gc_set_linewidth,
  gc_set_dashes,
  gc_set_foreground,
  gc_set_background
};

static struct graphics_gc_priv *
gc_new (struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
  struct graphics_gc_priv *gc = g_new0(struct graphics_gc_priv, 1);

  *meth = gc_methods;
  gc->gr = gr;
  gc->linewidth = 1;
  return gc;
}

static struct graphics_image_priv image_error;

static struct graphics_image_priv *
image_new (struct graphics_priv *gr, struct graphics_image_methods *meth,
	   char *path, int *w, int *h, struct point *hot, int rotation)
{
  FIBITMAP *image;
  RGBQUAD aPixel;
  unsigned char *data;
  int width, height, i, j;
  struct graphics_image_priv *gi;
  //check if image already exists in hashmap
  struct graphics_image_priv*curr_elem = g_hash_table_lookup(hImageData,path);
  if(curr_elem == &image_error) {
    //found but couldn't be loaded
    return NULL;
  }
  else if(curr_elem) {
    //found and OK -> use hastable entry
    *w = curr_elem->w;
    *h = curr_elem->h;
    hot->x = curr_elem->w / 2 - 1;
    hot->y = curr_elem->h / 2 - 1;
    return curr_elem;
  }
  else {
  if (strlen (path) < 4)
    {
      g_hash_table_insert(hImageData,g_strdup(path),&image_error);
      return NULL;
    }
  char *ext_str = path + strlen (path) - 3;
  if (strstr (ext_str, "png") || strstr (path, "PNG"))
    {
      if ((image = FreeImage_Load (FIF_PNG, path, 0)) == NULL)
	{
          g_hash_table_insert(hImageData,g_strdup(path),&image_error);
	  return NULL;
	}
    }
  else if (strstr (ext_str, "xpm") || strstr (path, "XPM"))
    {
      if ((image = FreeImage_Load (FIF_XPM, path, 0)) == NULL)
	{
          g_hash_table_insert(hImageData,g_strdup(path),&image_error);
	  return NULL;
	}
    }
  else if (strstr (ext_str, "svg") || strstr (path, "SVG"))
    {
      char path_new[256];
      snprintf (path_new, strlen (path) - 3, "%s", path);
      strcat (path_new, "_48_48.png");

      if ((image = FreeImage_Load (FIF_PNG, path_new, 0)) == NULL)
	{
          g_hash_table_insert(hImageData,g_strdup(path),&image_error);
	  return NULL;
	}
    }
  else
    {
      g_hash_table_insert(hImageData,g_strdup(path),&image_error);
      return NULL;
    }

    if (FreeImage_GetBPP (image) == 64) {
        FIBITMAP *image2; 
        image2 = FreeImage_ConvertTo32Bits(image);
        FreeImage_Unload(image);
        image = image2;
    }

#if FREEIMAGE_MAJOR_VERSION*100+FREEIMAGE_MINOR_VERSION  >= 313
    if(rotation) {
      FIBITMAP *image2; 
      image2 = FreeImage_Rotate(image, rotation, NULL);
      image = image2;
    }
#endif

  gi = g_new0 (struct graphics_image_priv, 1);

  width = FreeImage_GetWidth (image);
  height = FreeImage_GetHeight (image);

  if( (*w!=width || *h!=height) && 0<*w && 0<*h ) {
    FIBITMAP *image2;
    image2 = FreeImage_Rescale(image, *w, *h, NULL);
    FreeImage_Unload(image);
    image = image2;
    width = *w;
    height = *h;
  }

  data = (unsigned char *) malloc (width * height * 4);

  RGBQUAD *palette = NULL;
  if (FreeImage_GetBPP (image) == 8)
    {
      palette = FreeImage_GetPalette (image);
    }

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
	{
	  unsigned char idx;
	  if (FreeImage_GetBPP (image) == 8)
	    {
	      FreeImage_GetPixelIndex (image, j, height - i - 1, &idx);
	      data[4 * width * i + 4 * j + 0] = palette[idx].rgbRed;
	      data[4 * width * i + 4 * j + 1] = palette[idx].rgbGreen;
	      data[4 * width * i + 4 * j + 2] = palette[idx].rgbBlue;
	      data[4 * width * i + 4 * j + 3] = 255;
	    }
	  else if (FreeImage_GetBPP (image) == 16
		   || FreeImage_GetBPP (image) == 24
		   || FreeImage_GetBPP (image) == 32)
	    {
	      FreeImage_GetPixelColor (image, j, height - i - 1, &aPixel);
	      int transparent = (aPixel.rgbRed == 0 && aPixel.rgbBlue == 0
				 && aPixel.rgbGreen == 0);
	      data[4 * width * i + 4 * j + 0] =
		transparent ? 0 : (aPixel.rgbRed);
	      data[4 * width * i + 4 * j + 1] = (aPixel.rgbGreen);
	      data[4 * width * i + 4 * j + 2] =
		transparent ? 0 : (aPixel.rgbBlue);
	      data[4 * width * i + 4 * j + 3] = transparent ? 0 : 255;

	    }
	}
    }

  FreeImage_Unload (image);

  *w = width;
  *h = height;
  gi->w = width;
  gi->h = height;
  gi->hot_x = width / 2 - 1;
  gi->hot_y = height / 2 - 1;
  hot->x = width / 2 - 1;
  hot->y = height / 2 - 1;
  gi->data = data;
  gi->path = path;
  //add to hashtable
  g_hash_table_insert(hImageData,g_strdup(path),gi);
  return gi;
  }
}

const char *
getPrimitiveType (GLenum type)
{
  char *ret = "";

  switch (type)
    {
    case 0x0000:
      ret = "GL_POINTS";
      break;
    case 0x0001:
      ret = "GL_LINES";
      break;
    case 0x0002:
      ret = "GL_LINE_LOOP";
      break;
    case 0x0003:
      ret = "GL_LINE_STRIP";
      break;
    case 0x0004:
      ret = "GL_TRIANGLES";
      break;
    case 0x0005:
      ret = "GL_TRIANGLE_STRIP";
      break;
    case 0x0006:
      ret = "GL_TRIANGLE_FAN";
      break;
    case 0x0007:
      ret = "GL_QUADS";
      break;
    case 0x0008:
      ret = "GL_QUAD_STRIP";
      break;
    case 0x0009:
      ret = "GL_POLYGON";
      break;
    }
  return ret;
}

void CALLBACK
tessBeginCB (GLenum which)
{
  glBegin (which);

  dbg (1, "glBegin( %s );\n", getPrimitiveType (which));
}



void CALLBACK
tessEndCB ()
{
  glEnd ();

  dbg (1, "glEnd();\n");
}



void CALLBACK
tessVertexCB (const GLvoid * data)
{
  // cast back to double type
  const GLdouble *ptr = (const GLdouble *) data;

  glVertex3dv (ptr);

  dbg (1, "  glVertex3d();\n");
}

static void
get_overlay_pos (struct graphics_priv *gr, struct point *point_out)
{
  if (gr->parent == NULL)
    {
      point_out->x = 0;
      point_out->y = 0;
      return;
    }
  point_out->x = gr->p.x;
  if (point_out->x < 0)
    {
      point_out->x += gr->parent->width;
    }

  point_out->y = gr->p.y;
  if (point_out->y < 0)
    {
      point_out->y += gr->parent->height;
    }
}

static void
draw_lines (struct graphics_priv *gr, struct graphics_gc_priv *gc,
	    struct point *p, int count)
{
  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  graphics_priv_root->dirty = 1;

  glColor4f (gc->fr, gc->fg, gc->fb, gc->fa);
  glLineWidth (gc->linewidth);
  if (!gr->parent && 0 < gc->dash_count)
    {
      glLineStipple (1, gc->dash_mask);
      glEnable (GL_LINE_STIPPLE);
    }
  glBegin (GL_LINE_STRIP);
  int i;
  for (i = 0; i < count; i++)
    {
      struct point p_eff;
      p_eff.x = p[i].x;
      p_eff.y = p[i].y;
      glVertex2f (p_eff.x, p_eff.y);
    }
  glEnd ();
  if (!gr->parent && 0 < gc->dash_count)
    {
      glDisable (GL_LINE_STIPPLE);
    }
}

void tessCombineCB (GLdouble c[3], void *d[4], GLfloat w[4], void **out) 
{ 
  GLdouble *nv = (GLdouble *) malloc(sizeof(GLdouble)*3); 
  nv[0] = c[0]; 
  nv[1] = c[1]; 
  nv[2] = c[2]; 
  *out = nv;  
}


static void
draw_polygon (struct graphics_priv *gr, struct graphics_gc_priv *gc,
	      struct point *p, int count)
{
  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  graphics_priv_root->dirty = 1;

  int i;
  GLUtesselator *tess = gluNewTess ();	// create a tessellator
  if (!tess)
    return;			// failed to create tessellation object, return 0

  GLdouble quad1[count][3];
  for (i = 0; i < count; i++)
    {
      quad1[i][0] = (GLdouble) (p[i].x);
      quad1[i][1] = (GLdouble) (p[i].y);
      quad1[i][2] = 0;
    }


  // register callback functions
  gluTessCallback (tess, GLU_TESS_BEGIN, (void (*)(void)) tessBeginCB);
  gluTessCallback (tess, GLU_TESS_END, (void (*)(void)) tessEndCB);
  //     gluTessCallback(tess, GLU_TESS_ERROR, (void (*)(void))tessErrorCB);
  gluTessCallback (tess, GLU_TESS_VERTEX, (void (*)(void)) tessVertexCB);
  gluTessCallback (tess, GLU_TESS_COMBINE, (void (*)(void)) tessCombineCB);

  // tessellate and compile a concave quad into display list
  // gluTessVertex() takes 3 params: tess object, pointer to vertex coords,
  // and pointer to vertex data to be passed to vertex callback.
  // The second param is used only to perform tessellation, and the third
  // param is the actual vertex data to draw. It is usually same as the second
  // param, but It can be more than vertex coord, for example, color, normal
  // and UV coords which are needed for actual drawing.
  // Here, we are looking at only vertex coods, so the 2nd and 3rd params are
  // pointing same address.
  glColor4f (gc->fr, gc->fg, gc->fb, gc->fa);
  gluTessBeginPolygon (tess, 0);	// with NULL data
  gluTessBeginContour (tess);
  for (i = 0; i < count; i++)
    {
      gluTessVertex (tess, quad1[i], quad1[i]);
    }
  gluTessEndContour (tess);
  gluTessEndPolygon (tess);

  gluDeleteTess (tess);		// delete after tessellation
}

static void
draw_rectangle (struct graphics_priv *gr, struct graphics_gc_priv *gc,
		struct point *p, int w, int h)
{
  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  graphics_priv_root->dirty = 1;

  struct point p_eff;
  p_eff.x = p->x;
  p_eff.y = p->y;

  glColor4f (gc->fr, gc->fg, gc->fb, gc->fa);
  glBegin (GL_POLYGON);
  glVertex2f (p_eff.x, p_eff.y);
  glVertex2f (p_eff.x + w, p_eff.y);
  glVertex2f (p_eff.x + w, p_eff.y + h);
  glVertex2f (p_eff.x, p_eff.y + h);
  glEnd ();
}

static void
draw_circle (struct graphics_priv *gr, struct graphics_gc_priv *gc,
	     struct point *p, int r)
{

  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  graphics_priv_root->dirty = 1;

  /* FIXME: does not quite match gtk */
  /* hack for osd compass.. why is this needed!? */
  if (gr->parent)
    {
      r = r / 2;
    }

  struct point p_eff;
  p_eff.x = p->x;
  p_eff.y = p->y;

  GLUquadricObj *quadratic;
  quadratic=gluNewQuadric();
  glPushMatrix ();
  glTranslatef (p_eff.x, p_eff.y, 0);
  glColor4f (gc->fr, gc->fg, gc->fb, gc->fa);
  gluDisk(quadratic, r-(gc->linewidth/2)-2,r+(gc->linewidth/2), 10+r/5,10+r/5);
  glPopMatrix ();
  gluDeleteQuadric(quadratic);

  return;
}

static void
display_text_draw (struct font_freetype_text *text, struct graphics_priv *gr,
		   struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
		   int color, struct point *p)
{
  int i, x, y, stride;
  struct font_freetype_glyph *g, **gp;
  unsigned char *shadow, *glyph;
  struct color transparent = { 0x0000, 0x0000, 0x0000, 0x0000 };
  struct color black =
    { fg->fr * 65535, fg->fg * 65535, fg->fb * 65535, fg->fa * 65535 };
  struct color white = { 0xffff, 0xffff, 0xffff, 0xffff };

  if (bg)
    {
      if(COLOR_IS_WHITE(black) && COLOR_IS_BLACK(white)) {
        black.r = 65535;
        black.g = 65535;
        black.b = 65535;
        black.a = 65535;

        white.r = 0;
        white.g = 0;
        white.b = 0;
        white.a = 65535;
       } 
      else if(COLOR_IS_BLACK(black) && COLOR_IS_WHITE(white)) {
        white.r = 65535;
        white.g = 65535;
        white.b = 65535;
        white.a = 65535;

        black.r = 0;
        black.g = 0;
        black.b = 0;
        black.a = 65535;
      } 
      else {
        white.r = bg->fr;
        white.g = bg->fg;
        white.b = bg->fb;
        white.a = bg->fa;
      }
    }
  else {
    white.r = 0;
    white.g = 0;
    white.b = 0;
    white.a = 0;
  }

  gp = text->glyph;
  i = text->glyph_count;
  x = p->x << 6;
  y = p->y << 6;
  while (i-- > 0)
    {
      g = *gp++;
      if (g->w && g->h && bg)
	{
	  stride = (g->w + 2) * 4;
	  if (color)
	    {
	      shadow = g_malloc (stride * (g->h + 2));
	      gr->freetype_methods.get_shadow (g, shadow, 32, stride, &white,
					       &transparent);
#ifdef MIRRORED_VIEW
	      glPixelZoom (-1.0, -1.0); //mirrored mode
#else
	      glPixelZoom (1.0, -1.0);
#endif
	      glRasterPos2d ((x + g->x) >> 6, (y + g->y) >> 6);
	      glDrawPixels (g->w + 2, g->h + 2, GL_BGRA, GL_UNSIGNED_BYTE,
			    shadow);
	      g_free (shadow);
	    }
	}
      x += g->dx;
      y += g->dy;
    }

  x = p->x << 6;
  y = p->y << 6;
  gp = text->glyph;
  i = text->glyph_count;
  while (i-- > 0)
    {
      g = *gp++;
      if (g->w && g->h)
	{
	  if (color)
	    {
	      stride = g->w;
	      if (bg)
		{
		  glyph = g_malloc (stride * g->h * 4);
		  gr->freetype_methods.get_glyph (g, glyph, 32, stride * 4,
						  &black, &white, &transparent);
#ifdef MIRRORED_VIEW
	      glPixelZoom (-1.0, -1.0); //mirrored mode
#else
	      glPixelZoom (1.0, -1.0);
#endif
	      glRasterPos2d ((x + g->x) >> 6, (y + g->y) >> 6);
	      glDrawPixels (g->w, g->h, GL_BGRA, GL_UNSIGNED_BYTE, glyph);

		  g_free (glyph);
		}
	      stride *= 4;
	      glyph = g_malloc (stride * g->h);
	      gr->freetype_methods.get_glyph (g, glyph, 32, stride, &black,
					      &white, &transparent);

#ifdef MIRRORED_VIEW
	      glPixelZoom (-1.0, -1.0); //mirrored mode
#else
	      glPixelZoom (1.0, -1.0);
#endif
	      glRasterPos2d ((x + g->x) >> 6, (y + g->y) >> 6);
	      glDrawPixels (g->w, g->h, GL_BGRA, GL_UNSIGNED_BYTE, glyph);
	      g_free (glyph);
	    }
	}
      x += g->dx;
      y += g->dy;
    }
}

static void
draw_text (struct graphics_priv *gr, struct graphics_gc_priv *fg,
	   struct graphics_gc_priv *bg, struct graphics_font_priv *font,
	   char *text, struct point *p, int dx, int dy)
{
  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  struct font_freetype_text *t;
  int color = 1;

  if (!font)
    {
      dbg (0, "no font, returning\n");
      return;
    }

  graphics_priv_root->dirty = 1;

  t =
    gr->freetype_methods.text_new (text, (struct font_freetype_font *) font,
				   dx, dy);

  struct point p_eff;
  p_eff.x = p->x;
  p_eff.y = p->y;

  display_text_draw (t, gr, fg, bg, color, &p_eff);
  gr->freetype_methods.text_destroy (t);
}


static void
draw_image (struct graphics_priv *gr, struct graphics_gc_priv *fg,
	    struct point *p, struct graphics_image_priv *img)
{
  if ((gr->parent && !gr->parent->overlay_enabled) || (gr->parent && gr->parent->overlay_enabled && !gr->overlay_enabled) )
    {
      return;
    }

  if (!img || !img->data)
    {
      return;
    }

  graphics_priv_root->dirty = 1;

  struct point p_eff;
  p_eff.x = p->x + img->hot_x;
  p_eff.y = p->y + img->hot_y;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glRasterPos2d (p_eff.x - img->hot_x, p_eff.y - img->hot_y);
  glDrawPixels (img->w, img->h, GL_RGBA, GL_UNSIGNED_BYTE, img->data);

}

static void
draw_image_warp (struct graphics_priv *gr, struct graphics_gc_priv *fg,
		 struct point *p, int count, char *data)
{
}

static void
draw_restore (struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void
draw_drag (struct graphics_priv *gr, struct point *p)
{

  if (p)
    {
      gr->p.x = p->x;
      gr->p.y = p->y;
    }
}

static void
background_gc (struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
  gr->background_gc = gc;
}

static void handle_mouse_queue() 
{
  static locked = 0;
  if(!locked) {
    locked = 1;
  }
  else {
    return;
  }

  if(mouse_event_queue_begin_idx < mouse_event_queue_end_idx) {
    if (mouse_queue[mouse_event_queue_begin_idx].button == GLUT_LEFT_BUTTON && mouse_queue[mouse_event_queue_begin_idx].state == GLUT_UP)
      {
        struct point p;
        p.x = mouse_queue[mouse_event_queue_begin_idx%mouse_event_queue_size].x;
        p.y = mouse_queue[mouse_event_queue_begin_idx%mouse_event_queue_size].y;
        graphics_priv_root->force_redraw = 1;
        callback_list_call_attr_3 (graphics_priv_root->cbl, attr_button,
				 (void *) 0, 1, (void *) &p);
      }
    else if (mouse_queue[mouse_event_queue_begin_idx].button == GLUT_LEFT_BUTTON && mouse_queue[mouse_event_queue_begin_idx].state == GLUT_DOWN)
      {
        struct point p;
        p.x = mouse_queue[mouse_event_queue_begin_idx%mouse_event_queue_size].x;
        p.y = mouse_queue[mouse_event_queue_begin_idx%mouse_event_queue_size].y;
        graphics_priv_root->force_redraw = 1;
        callback_list_call_attr_3 (graphics_priv_root->cbl, attr_button,
  				 (void *) 1, 1, (void *) &p);
      }
    ++mouse_event_queue_begin_idx;
  }
  locked = 0;
}


/*draws root graphics and its overlays*/
static int
redraw_screen (struct graphics_priv *gr)
{
  time_t curr_time = time(0);
  graphics_priv_root->dirty = 0;

  glCallList (gr->DLid);
  //display overlays display list
  struct graphics_priv *overlay;
  overlay = gr->overlays;
  while (gr->overlay_enabled && overlay)
    {
      if(overlay->overlay_enabled) {
      glPushMatrix ();
      struct point p_eff;
      get_overlay_pos (overlay, &p_eff);
      glTranslatef (p_eff.x, p_eff.y, 1);
      glCallList (overlay->DLid);
      glPopMatrix ();
      }
      overlay = overlay->next;
    }
  glutSwapBuffers ();

  return TRUE;
}


/*filters call to redraw in overlay enabled(map) mode*/
static void
redraw_filter (struct graphics_priv *gr)
{
  if(gr->overlay_enabled && gr->dirty) {
    redraw_screen(gr);
  }
}



static void
draw_mode (struct graphics_priv *gr, enum draw_mode_num mode)
{
  if (gr->parent)
    {				//overlay
      if (mode == draw_mode_begin)
	{
	  glNewList (gr->DLid, GL_COMPILE);
	}

      if (mode == draw_mode_end || mode == draw_mode_end_lazy)
	{
	  glEndList ();
	}
    }
  else
    {				//root graphics
      if (mode == draw_mode_begin)
	{
	  glNewList (gr->DLid, GL_COMPILE);
	}

      if (mode == draw_mode_end)
	{
	  glEndList ();
          gr->force_redraw = 1;
          if(!gr->overlay_enabled || gr->force_redraw ) {
            redraw_screen (gr);
          }
	}
    }
  gr->mode = mode;
}

static struct graphics_priv *overlay_new (struct graphics_priv *gr,
					  struct graphics_methods *meth,
					  struct point *p, int w, int h,
					  int alpha, int wraparound);

static int
graphics_opengl_fullscreen (struct window *w, int on)
{
  return 1;
}

static void
graphics_opengl_disable_suspend (struct window *w)
{
}


static void *
get_data (struct graphics_priv *this, char *type)
{
  /*TODO initialize gtkglext context when type=="gtk_widget" */
  if (!strcmp(type,"gtk_widget")) {
    fprintf(stderr, "Currently GTK gui is not yet supported with opengl graphics driver\n");
    exit(-1);
  }
  if (strcmp (type, "window") == 0)
    {
      struct window *win;
      win = g_new0 (struct window, 1);
      win->priv = this;
      win->fullscreen = graphics_opengl_fullscreen;
      win->disable_suspend = graphics_opengl_disable_suspend;
      return win;
    }
  else
    {
      return &this->DLid;
    }


}

static void
image_free (struct graphics_priv *gr, struct graphics_image_priv *priv)
{
//TODO free image data in hashtable when graphics is destroyed
//currently graphics destroy is not called !!!
/*
  g_free(priv->data);
  priv->data = NULL;
  g_free(priv); 
  priv = NULL;
*/
}

static void
overlay_disable (struct graphics_priv *gr, int disable)
{
  gr->overlay_enabled = !disable;
  gr->force_redraw = 1;
  draw_mode(gr,draw_mode_end);
}

static void
overlay_resize (struct graphics_priv *gr, struct point *p, int w, int h,
		int alpha, int wraparound)
{
  int changed = 0;
  int w2, h2;

  if (w == 0)
    {
      w2 = 1;
    }
  else
    {
      w2 = w;
    }

  if (h == 0)
    {
      h2 = 1;
    }
  else
    {
      h2 = h;
    }

  gr->p = *p;
  if (gr->width != w2)
    {
      gr->width = w2;
      changed = 1;
    }

  if (gr->height != h2)
    {
      gr->height = h2;
      changed = 1;
    }

  gr->wraparound = wraparound;

  if (changed)
    {
      if ((w == 0) || (h == 0))
	{
	  gr->overlay_autodisabled = 1;
	}
      else
	{
	  gr->overlay_autodisabled = 0;
	}

      callback_list_call_attr_2 (gr->cbl, attr_resize,
				 GINT_TO_POINTER (gr->width),
				 GINT_TO_POINTER (gr->height));
    }
}

static struct graphics_methods graphics_methods = {
  graphics_destroy,
  draw_mode,
  draw_lines,
  draw_polygon,
  draw_rectangle,
  draw_circle,
  draw_text,
  draw_image,
  draw_image_warp,
  draw_restore,
  draw_drag,
  NULL,
  gc_new,
  background_gc,
  overlay_new,
  image_new,
  get_data,
  image_free,
  NULL,
  overlay_disable,
  overlay_resize,
};

static struct graphics_priv *
graphics_opengl_new_helper (struct graphics_methods *meth)
{
  struct font_priv *(*font_freetype_new) (void *meth);
  font_freetype_new = plugin_get_font_type ("freetype");

  if (!font_freetype_new)
    {
      return NULL;
    }

  struct graphics_priv *this = g_new0 (struct graphics_priv, 1);

  font_freetype_new (&this->freetype_methods);
  *meth = graphics_methods;

  meth->font_new =
    (struct graphics_font_priv *
     (*)(struct graphics_priv *, struct graphics_font_methods *, char *, int,
	 int)) this->freetype_methods.font_new;
  meth->get_text_bbox = this->freetype_methods.get_text_bbox;

  return this;
}

static struct graphics_priv *
overlay_new (struct graphics_priv *gr, struct graphics_methods *meth,
	     struct point *p, int w, int h, int alpha, int wraparound)
{
  int w2, h2;
  struct graphics_priv *this = graphics_opengl_new_helper (meth);
  this->p = *p;
  this->width = w;
  this->height = h;
  this->parent = gr;

  /* If either height or width is 0, we set it to 1 to avoid warnings, and
   * disable the overlay. */
  if (h == 0)
    {
      h2 = 1;
    }
  else
    {
      h2 = h;
    }

  if (w == 0)
    {
      w2 = 1;
    }
  else
    {
      w2 = w;
    }

  if ((w == 0) || (h == 0))
    {
      this->overlay_autodisabled = 1;
    }
  else
    {
      this->overlay_autodisabled = 0;
    }
  this->overlay_enabled = 1;
  this->overlay_autodisabled = 0;

  this->next = gr->overlays;
  gr->overlays = this;
  this->DLid = glGenLists (1);
  return this;
}


static void
click_notify (int button, int state, int x, int y)
{
  mouse_queue[mouse_event_queue_end_idx%mouse_event_queue_size].button = button;
  mouse_queue[mouse_event_queue_end_idx%mouse_event_queue_size].state = state;
#ifdef MIRRORED_VIEW
  mouse_queue[mouse_event_queue_end_idx%mouse_event_queue_size].x = graphics_priv_root->width-x;
#else
  mouse_queue[mouse_event_queue_end_idx%mouse_event_queue_size].x = x;
#endif
  mouse_queue[mouse_event_queue_end_idx%mouse_event_queue_size].y = y;
  ++mouse_event_queue_end_idx;
}

static void
motion_notify (int x, int y)
{
  struct point p;
#ifdef MIRRORED_VIEW
  p.x = graphics_priv_root->width-x;
#else
  p.x = x;
#endif
  p.y = y;
  callback_list_call_attr_1 (graphics_priv_root->cbl, attr_motion,
			     (void *) &p);
  return;
}

static gboolean
graphics_opengl_idle (void *data)
{
  static int opengl_init_ok = 0;
  if (!opengl_init_ok)
    {
      callback_list_call_attr_2 (graphics_priv_root->cbl, attr_resize,
				 GINT_TO_POINTER (graphics_priv_root->width),
				 GINT_TO_POINTER
				 (graphics_priv_root->height));
      opengl_init_ok = 1;
    }
  else
    {
      glutMainLoopEvent ();
      handle_mouse_queue();
    }
  return TRUE;
}

static void
ProcessNormalKeys (unsigned char key_in, int x, int y)
{
  int key = 0;
  char keybuf[2];

  switch (key_in)
    {
    case 13:
      key = NAVIT_KEY_RETURN;
      break;
    default:
      key = key_in;
      break;
    }
  keybuf[0] = key;
  keybuf[1] = '\0';
  graphics_priv_root->force_redraw = 1;
  callback_list_call_attr_1 (graphics_priv_root->cbl, attr_keypress,
			     (void *) keybuf);
}

static void
ProcessSpecialKeys (int key_in, int x, int y)
{
  int key = 0;
  char keybuf[2];

  switch (key_in)
    {
    case 102:
      key = NAVIT_KEY_RIGHT;
      break;
    case 100:
      key = NAVIT_KEY_LEFT;
      break;
    case 103:
      key = NAVIT_KEY_DOWN;
      break;
    case 101:
      key = NAVIT_KEY_UP;
      break;
    case 104:
      key = NAVIT_KEY_ZOOM_OUT;
      break;
    case 105:
      key = NAVIT_KEY_ZOOM_IN;
      break;
    default:
      break;
    }				//switch

  graphics_priv_root->force_redraw = 1;
  keybuf[0] = key;
  keybuf[1] = '\0';
  callback_list_call_attr_1 (graphics_priv_root->cbl, attr_keypress,
			     (void *) keybuf);
}

static void
resize_callback (int w, int h)
{
  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
#ifdef MIRRORED_VIEW
  gluOrtho2D ( w, 0, h, 0.0); //mirrored mode
#else
  gluOrtho2D ( 0, w, h, 0.0);
#endif

  graphics_priv_root->width = w;
  graphics_priv_root->height = h;

  callback_list_call_attr_2 (graphics_priv_root->cbl, attr_resize,
			     GINT_TO_POINTER (w), GINT_TO_POINTER (h));
}

static void
display (void)
{
  graphics_priv_root->force_redraw = 1;
  redraw_screen(graphics_priv_root);
  resize_callback (graphics_priv_root->width, graphics_priv_root->height);
}

static void
glut_close (void)
{
  callback_list_call_attr_0(graphics_priv_root->cbl, attr_window_closed);
}


static struct graphics_priv *
graphics_opengl_new (struct navit *nav, struct graphics_methods *meth,
		     struct attr **attrs, struct callback_list *cbl)
{
  struct attr *attr;

  if (!event_request_system ("glib", "graphics_opengl_new"))
    return NULL;

  struct graphics_priv *this = graphics_opengl_new_helper (meth);
  graphics_priv_root = this;

  this->nav = nav;
  this->parent = NULL;
  this->overlay_enabled = 1;

  this->width = SCREEN_WIDTH;
  if ((attr = attr_search (attrs, NULL, attr_w)))
    this->width = attr->u.num;
  this->height = SCREEN_HEIGHT;
  if ((attr = attr_search (attrs, NULL, attr_h)))
    this->height = attr->u.num;
  this->timeout = 100;
  if ((attr = attr_search (attrs, NULL, attr_timeout)))
    this->timeout = attr->u.num;
  this->delay = 0;
  if ((attr = attr_search (attrs, NULL, attr_delay)))
    this->delay = attr->u.num;
  this->cbl = cbl;

  char *cmdline = "";
  int argc = 0;
  glutInit (&argc, &cmdline);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA);

  glutInitWindowSize (this->width, this->height);
  glutInitWindowPosition (0, 0);
  glutCreateWindow ("Navit opengl window");

  glutDisplayFunc (display);
  glutReshapeFunc (resize_callback);
  resize_callback (this->width, this->height);

  graphics_priv_root->cbl = cbl;
  graphics_priv_root->width = this->width;
  graphics_priv_root->height = this->height;

  glutMotionFunc (motion_notify);
  glutPassiveMotionFunc (motion_notify);
  glutMouseFunc (click_notify);
  glutKeyboardFunc (ProcessNormalKeys);
  glutSpecialFunc (ProcessSpecialKeys);
  glutCloseFunc (glut_close);

  this->DLid = glGenLists (1);

  g_timeout_add (G_PRIORITY_DEFAULT + 10, graphics_opengl_idle, NULL);
  
  /*this will only refresh screen in map(overlay enabled) mode*/
  g_timeout_add (G_PRIORITY_DEFAULT + 1000, redraw_filter, this);

  //create hash table for uncompressed image data
  hImageData = g_hash_table_new(g_str_hash, g_str_equal);
  return this;
}


static void
event_opengl_main_loop_run (void)
{
  dbg (0, "enter\n");
}

static void
event_opengl_main_loop_quit (void)
{
  dbg (0, "enter\n");
}

static struct event_watch *
event_opengl_add_watch (void *h, enum event_watch_cond cond,
			struct callback *cb)
{
  dbg (0, "enter\n");
  return NULL;
}

static void
event_opengl_remove_watch (struct event_watch *ev)
{
  dbg (0, "enter\n");
}


static struct event_timeout *
event_opengl_add_timeout (int timeout, int multi, struct callback *cb)
{
  dbg (0, "enter\n");
  return NULL;
}

static void
event_opengl_remove_timeout (struct event_timeout *to)
{
  dbg (0, "enter\n");
}


static struct event_idle *
event_opengl_add_idle (int priority, struct callback *cb)
{
  dbg (0, "enter\n");
  return NULL;
}

static void
event_opengl_remove_idle (struct event_idle *ev)
{
  dbg (0, "enter\n");
}

static void
event_opengl_call_callback (struct callback_list *cb)
{
  dbg (0, "enter\n");
}

static struct event_methods event_opengl_methods = {
  event_opengl_main_loop_run,
  event_opengl_main_loop_quit,
  event_opengl_add_watch,
  event_opengl_remove_watch,
  event_opengl_add_timeout,
  event_opengl_remove_timeout,
  event_opengl_add_idle,
  event_opengl_remove_idle,
  event_opengl_call_callback,
};

static struct event_priv *
event_opengl_new (struct event_methods *meth)
{
  *meth = event_opengl_methods;
  return NULL;
}

void
plugin_init (void)
{
  plugin_register_graphics_type ("opengl", graphics_opengl_new);
  plugin_register_event_type ("opengl", event_opengl_new);
}
