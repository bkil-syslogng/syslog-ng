#include "glib.h"

#if !defined(g_list_free_full)
  void
  g_list_free_full(GList *list, void(*destroy_fun)(gpointer s))
  {
    g_list_foreach(list, (GFunc) destroy_fun, NULL);
    g_list_free(list);
  }
#endif

