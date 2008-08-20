#include <clutter/clutter.h>
#include <stdlib.h>


#define FOO_TYPE_ACTOR            (foo_actor_get_type ())
#define FOO_ACTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FOO_TYPE_ACTOR, FooActor))
#define FOO_IS_ACTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FOO_TYPE_ACTOR))
#define FOO_ACTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FOO_TYPE_ACTOR, FooActorClass))
#define FOO_IS_ACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FOO_TYPE_ACTOR))
#define FOO_ACTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FOO_TYPE_ACTOR, FooActorClass))

typedef struct _FooActor
{
  ClutterActor parent_instance;
} FooActor;

typedef struct _FooActorClass
{
  ClutterActorClass parent_class;
} FooActorClass;

ClutterActor* foo_actor_new (void);