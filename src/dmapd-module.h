/* Copyright (c) 2006 Imendio AB
 * Author: Michael Natterer <mitch@imendio.com>
 * Modified by: W. Michael Petullo <mike@flyn.org>
 *
 * Originaly based on foo-module.h from foo-editor. Foo-editor was an
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

#ifndef __DMAPD_MODULE_H__
#define __DMAPD_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS


#define DMAPD_TYPE_MODULE         (dmapd_module_get_type ())
#define DMAPD_MODULE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                 DMAPD_TYPE_MODULE, DmapdModule))
#define DMAPD_MODULE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), \
                                 DMAPD_TYPE_MODULE, DmapdModuleClass))
#define DMAPD_IS_MODULE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                 DMAPD_TYPE_MODULE))
#define DMAPD_IS_MODULE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                 DMAPD_TYPE_MODULE))
#define DMAPD_MODULE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                 DMAPD_TYPE_MODULE, DmapdModuleClass))


typedef struct _DmapdModule      DmapdModule;
typedef struct _DmapdModuleClass DmapdModuleClass;

struct _DmapdModule
{
  GTypeModule  parent_instance;

  gchar       *filename;
  GModule     *library;

  /*  module symbols  */
  void (* load)   (DmapdModule *module);
  void (* unload) (DmapdModule *module);
};

struct _DmapdModuleClass
{
  GTypeModuleClass  parent_class;
};


GType       dmapd_module_get_type (void) G_GNUC_CONST;

DmapdModule * dmapd_module_new      (const gchar *filename);


/* API for the modules to implement */

gboolean    dmapd_module_load     (GTypeModule *module);
gboolean    dmapd_module_unload   (GTypeModule *module);


G_END_DECLS

#endif /* __DMAPD_MODULE_H_ */
