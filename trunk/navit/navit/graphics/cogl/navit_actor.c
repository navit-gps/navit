
// ---------------------------------------------------------
#include "navit_actor.h"
#include "debug.h"


G_DEFINE_TYPE (FooActor, foo_actor, CLUTTER_TYPE_ACTOR);


static void
test_coglbox_paint(ClutterActor *self)
{
	dbg(0,"low level drawing\n");
  ClutterColor cfill;
  ClutterColor cstroke;
  
  cfill.red    = 0;
  cfill.green  = 160;
  cfill.blue   = 0;
  cfill.alpha  = 255;
  
  cstroke.red    = 200;
  cstroke.green  = 0;
  cstroke.blue   = 0;
  cstroke.alpha  = 255;
  
  cogl_push_matrix ();
  
  cogl_path_round_rectangle (CLUTTER_INT_TO_FIXED (-50),
                             CLUTTER_INT_TO_FIXED (-25),
                             CLUTTER_INT_TO_FIXED (100),
                             CLUTTER_INT_TO_FIXED (50),
                             CLUTTER_INT_TO_FIXED (10),
                             5);	
  
  cogl_translate (100,100,0);
  cogl_color (&cstroke);
  cogl_path_stroke ();
  
  cogl_translate (150,0,0);
  cogl_color (&cfill);
  cogl_path_fill ();
  
  cogl_pop_matrix();
}

static void
foo_actor_class_init (FooActorClass *klass)
{
	ClutterActorClass *actor_class   = CLUTTER_ACTOR_CLASS (klass);
	actor_class->paint          = test_coglbox_paint;
}

static void
foo_actor_init (FooActor *actor)
{

}

ClutterActor*
foo_actor_new (void)
{
	dbg(0,"New canvas for cogl created\n");
	return g_object_new (FOO_TYPE_ACTOR, NULL);
}

// ---------------------------------------------------------