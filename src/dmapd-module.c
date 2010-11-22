/* Copyright (c) 2006 Imendio AB
 * Author: Michael Natterer <mitch@imendio.com>
 * Modified by: W. Michael Petullo <mike@flyn.org>
 *
 * Originaly based on foo-module.c from foo-editor. Foo-editor was an
 * example application distributed with a presentation titled, "Creating
 * a Plugin System using GTypeModule."
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <config.h>
#include <gmodule.h>

#include "dmapd-module.h"

enum
{
  PROP_0,
  PROP_FILENAME
};

static void      dmapd_module_finalize      (GObject      *object);
static void      dmapd_module_get_property  (GObject      *object,
                                           guint         param_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);
static void      dmapd_module_set_property  (GObject      *object,
                                           guint         param_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static gboolean  dmapd_module_load_module   (GTypeModule  *gmodule);
static void      dmapd_module_unload_module (GTypeModule  *gmodule);

G_DEFINE_TYPE (DmapdModule, dmapd_module, G_TYPE_TYPE_MODULE);

static GHashTable *modules = NULL;

static void
dmapd_module_class_init (DmapdModuleClass *class)
{
  GObjectClass     *object_class      = G_OBJECT_CLASS (class);
  GTypeModuleClass *type_module_class = G_TYPE_MODULE_CLASS (class);

  object_class->finalize     = dmapd_module_finalize;
  object_class->get_property = dmapd_module_get_property;
  object_class->set_property = dmapd_module_set_property;

  type_module_class->load    = dmapd_module_load_module;
  type_module_class->unload  = dmapd_module_unload_module;

  g_object_class_install_property (object_class, PROP_FILENAME,
                                   g_param_spec_string ("filename",
                                                        "Filename",
                                                        "The filaname of the module",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
dmapd_module_init (DmapdModule *module)
{
  module->filename = NULL;
  module->library  = NULL;
  module->load     = NULL;
  module->unload   = NULL;
}

static void
dmapd_module_finalize (GObject *object)
{
  DmapdModule *module = DMAPD_MODULE (object);

  g_free (module->filename);

  G_OBJECT_CLASS (dmapd_module_parent_class)->finalize (object);
}

static void
dmapd_module_get_property (GObject    *object,
                         guint       param_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  DmapdModule *module = DMAPD_MODULE (object);

  switch (param_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, module->filename);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
dmapd_module_set_property (GObject      *object,
                         guint         param_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DmapdModule *module = DMAPD_MODULE (object);

  switch (param_id)
    {
    case PROP_FILENAME:
      g_free (module->filename);
      module->filename = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static gboolean
dmapd_module_load_module (GTypeModule *gmodule)
{
  DmapdModule *module = DMAPD_MODULE (gmodule);

  if (!module->filename)
    {
      g_warning ("Module path not set");
      return FALSE;
    }

  module->library = g_module_open (module->filename, 0);

  if (!module->library)
    {
      g_printerr ("%s\n", g_module_error ());
      return FALSE;
    }

  /* Make sure that the loaded library contains the required methods */
  if (! g_module_symbol (module->library,
                         "dmapd_module_load",
                         (gpointer *) &module->load) ||
      ! g_module_symbol (module->library,
                         "dmapd_module_unload",
                         (gpointer *) &module->unload))
    {
      g_printerr ("%s\n", g_module_error ());
      g_module_close (module->library);

      return FALSE;
    }

  /* Initialize the loaded module */
  module->load (module);

  return TRUE;
}

static void
dmapd_module_unload_module (GTypeModule *gmodule)
{
  DmapdModule *module = DMAPD_MODULE (gmodule);

  module->unload (module);

  g_module_close (module->library);
  module->library = NULL;

  module->load   = NULL;
  module->unload = NULL;
}

DmapdModule *
dmapd_module_new (const gchar *filename)
{
  DmapdModule *module;

  g_return_val_if_fail (filename != NULL, NULL);

  if (modules == NULL) {
    /* FIXME: elements should be free'd on exit/unload: */
    modules = g_hash_table_new (g_str_hash, g_str_equal);
  }

  if (! (module = g_hash_table_lookup (modules, filename))) {
    g_debug ("Loading %s.", filename);
    module = g_object_new (DMAPD_TYPE_MODULE,
                           "filename", filename,
                           NULL);
    g_hash_table_insert (modules, g_strdup(filename), module);
  } else {
    g_debug ("Module %s was previously loaded", filename);
  }

  return module;
}
