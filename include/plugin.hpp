#pragma once
#include "global_pre.hpp"
#include <string>
#include <stdio.h> // debug
#include <list>
#include "tags.hpp"
#include "util/common.hpp"

#define RACK_PLUGIN_INIT_ID_INTERNAL p->slug = TOSTRING(SLUG); p->version = TOSTRING(VERSION)

#ifdef USE_BEGIN_REDRAW_FXN
extern "C" {
#include <lglw.h>
}
#endif // USE_BEGIN_REDRAW_FXN

#ifdef USE_VST2

namespace rack {
   struct Plugin;

   typedef void (*InitCallback)(Plugin *);
}

typedef void (*vst2_handle_ui_param_fxn_t) (int uniqueParamId, float normValue);
typedef void (*vst2_queue_param_sync_fxn_t) (int uniqueParamId, float value, bool _bNormalized);
typedef void (*rack_set_tls_globals_fxn_t) (rack::Plugin *p);
#ifdef USE_BEGIN_REDRAW_FXN
typedef void (*rack_begin_redraw_fxn_t) (void);
#endif // USE_BEGIN_REDRAW_FXN

#ifdef RACK_HOST

// Rack host build:

extern void vst2_handle_ui_param (int uniqueParamId, float normValue);
extern void vst2_queue_param_sync (int _uniqueParamId, float _value, bool _bNormalized);

#define RACK_PLUGIN_DECLARE(pluginname) 
#ifdef _MSC_VER
#define RACK_PLUGIN_INIT(pluginname)  extern "C" void init_plugin_##pluginname##(rack::Plugin *p)
#else
#define RACK_PLUGIN_INIT(pluginname)  static void loc_init_plugin(rack::Plugin *p); extern "C" { void CONCAT(init_plugin_, pluginname) (rack::Plugin *p) { loc_init_plugin(p); } } static void loc_init_plugin(rack::Plugin *p)
#endif // _MSC_VER
#define RACK_PLUGIN_INIT_ID() RACK_PLUGIN_INIT_ID_INTERNAL

#else

// Plugin build:

#ifdef _MSC_VER
#ifdef RACK_PLUGIN_SHARED
 #define RACK_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
 #define RACK_PLUGIN_EXPORT extern "C"
#endif // RACK_PLUGIN_SHARED
 #define RACK_TLS __declspec(thread)
#else
 #define RACK_PLUGIN_EXPORT extern "C"
 #define RACK_TLS __thread
#endif // _MSC_VER

extern vst2_handle_ui_param_fxn_t vst2_handle_ui_param;
extern vst2_queue_param_sync_fxn_t vst2_queue_param_sync;

#ifndef RACK_PLUGIN_SHARED_LIB_BUILD
#ifdef RACK_PLUGIN_SHARED
 // Dynamically loaded plugin build
 #define RACK_PLUGIN_DECLARE(pluginname)  namespace rack { extern RACK_TLS Plugin *plugin; } extern void __rack_unused_symbol(void)
#ifdef ARCH_WIN
#define JSON_SEED_INIT_EXTERNAL extern "C" { extern long seed_initialized; }
#else
#define JSON_SEED_INIT_EXTERNAL extern "C" { extern volatile char seed_initialized; }
#endif

#ifdef USE_BEGIN_REDRAW_FXN
// a hack that rebinds the GL context in shared plugins (may fix shared module redraw issues on Linux)
#define BEGIN_REDRAW_FXN_SET \
   rack::plugin->begin_redraw_fxn = &rack::loc_begin_redraw;
#define BEGIN_REDRAW_FXN_IMP \
   static void loc_begin_redraw(void) {                 \
      lglw_glcontext_rebind(global_ui->window.lglw);    \
   }
#else
#define BEGIN_REDRAW_FXN_SET  
#define BEGIN_REDRAW_FXN_IMP  
#endif // USE_BEGIN_REDRAW_FXN

#define RACK_PLUGIN_INIT(pluginname)                    \
vst2_handle_ui_param_fxn_t vst2_handle_ui_param;        \
vst2_queue_param_sync_fxn_t vst2_queue_param_sync;      \
JSON_SEED_INIT_EXTERNAL                                 \
extern "C" { extern volatile uint32_t hashtable_seed; } \
namespace rack {                                        \
   RACK_TLS Plugin *plugin;                             \
   RACK_TLS Global *global;                             \
   RACK_TLS GlobalUI *global_ui;                        \
   static void loc_set_tls_globals(rack::Plugin *p) {   \
      plugin = p;                                       \
      global = plugin->global;                          \
      global_ui = plugin->global_ui;                    \
      /*printf("xxx plugin:loc_set_tls_globals: &global=%p global=%p\n", &global, plugin->global);*/ \
      hashtable_seed = p->json.hashtable_seed;          \
      seed_initialized = p->json.seed_initialized;      \
   }                                                    \
   BEGIN_REDRAW_FXN_IMP                                 \
}                                                       \
static void loc_init_plugin(rack::Plugin *p); extern "C" { RACK_PLUGIN_EXPORT void init_plugin(rack::Plugin *p) { loc_init_plugin(p); } } static void loc_init_plugin(rack::Plugin *p)
 #define RACK_PLUGIN_INIT_ID() \
   rack::plugin = p;                                                 \
   rack::plugin->set_tls_globals_fxn = &rack::loc_set_tls_globals;   \
   BEGIN_REDRAW_FXN_SET                                              \
   vst2_handle_ui_param = p->vst2_handle_ui_param_fxn;               \
   vst2_queue_param_sync = p->vst2_queue_param_sync_fxn;             \
   rack::global = p->global;                                         \
   rack::global_ui = p->global_ui;                                   \
   RACK_PLUGIN_INIT_ID_INTERNAL
#else
 // Statically linked plugin build
 #define RACK_PLUGIN_DECLARE(pluginname) 
 #ifdef _MSC_VER
  #define RACK_PLUGIN_INIT(pluginname)  extern "C" void init_plugin_##pluginname##(rack::Plugin *p)
 #else
  #define RACK_PLUGIN_INIT(pluginname)  static void loc_init_plugin(rack::Plugin *p); extern "C" { void CONCAT(init_plugin_, pluginname) (rack::Plugin *p) { loc_init_plugin(p); } } static void loc_init_plugin(rack::Plugin *p)
 #endif
 #define RACK_PLUGIN_INIT_ID() RACK_PLUGIN_INIT_ID_INTERNAL
#endif // RACK_PLUGIN_SHARED
#endif // RACK_PLUGIN_SHARED_LIB_BUILD

#endif // RACK_HOST


#else

#define RACK_PLUGIN_DECLARE(pluginname) extern Plugin *plugin
#define RACK_PLUGIN_INIT(pluginname)  extern "C" RACK_PLUGIN_EXPORT void init(rack::Plugin *p)
#define RACK_PLUGIN_INIT_ID() plugin = p; RACK_PLUGIN_INIT_ID_INTERNAL

#endif // USE_VST2

#define RACK_PLUGIN_INIT_WEBSITE(url) p->website = url
#define RACK_PLUGIN_INIT_MANUAL(url) p->manual = url
#define RACK_PLUGIN_INIT_VERSION(ver) p->version = ver

#ifdef _MSC_VER
#define RACK_PLUGIN_MODEL_DECLARE(pluginname, modelname) extern Model *create_model_##pluginname##_##modelname##(void)
#define RACK_PLUGIN_MODEL_INIT(pluginname, modelname) Model *create_model_##pluginname##_##modelname##(void)
#define RACK_PLUGIN_MODEL_ADD(pluginname, modelname) p->addModel(create_model_##pluginname##_##modelname##())
#else
#define CONCAT_USCORE_LITERAL(x, y) x ## _ ## y
#define CONCAT_USCORE(x, y) CONCAT_USCORE_LITERAL(x, y)
#define RACK_PLUGIN_MODEL_DECLARE(pluginname, modelname) extern Model *CONCAT(create_model_, CONCAT_USCORE(pluginname, modelname))(void)
#define RACK_PLUGIN_MODEL_INIT(pluginname, modelname) Model *CONCAT(create_model_, CONCAT_USCORE(pluginname, modelname))(void)
#define RACK_PLUGIN_MODEL_ADD(pluginname, modelname) p->addModel(CONCAT(create_model_, CONCAT_USCORE(pluginname, modelname))())
#endif // _MSC_VER


namespace rack {


struct ModuleWidget;
struct Module;
struct Model;


// Subclass this and return a pointer to a new one when init() is called
struct Plugin {
	/** A list of the models available by this plugin, add with addModel() */
	std::list<Model*> models;
	/** The file path of the plugin's directory */
	std::string path;
	/** OS-dependent library handle */
	void *handle = NULL;

	/** Must be unique. Used for patch files and the VCV store API.
	To guarantee uniqueness, it is a good idea to prefix the slug by your "company name" if available, e.g. "MyCompany-MyPlugin"
	*/
	std::string slug;

	/** The version of your plugin
	Plugins should follow the versioning scheme described at https://github.com/VCVRack/Rack/issues/266
	Do not include the "v" in "v1.0" for example.
	*/
	std::string version;

	/** Deprecated, do not use. */
	std::string website;
	std::string manual;

#ifdef USE_VST2
   //
   // Set by Rack host (before init_plugin()):
   //
   vst2_handle_ui_param_fxn_t vst2_handle_ui_param_fxn = NULL;
   vst2_queue_param_sync_fxn_t vst2_queue_param_sync_fxn = NULL;
   Global *global = NULL;
   GlobalUI *global_ui = NULL;

   // Set by Rack host immediately before set_tls_globals_fxn is called:
   //  (note) must be copied by the plugin or json import won't function properly
   struct {
      uint32_t hashtable_seed;
#ifdef ARCH_WIN
      long seed_initialized;
#else
      char seed_initialized;
#endif
   } json;

   //
   // Set by plugin:
   //  - in init_plugin()
   //  - called by Rack host in audio+UI threads
   //  - NULL if this is a statically linked add-on
   //
   rack_set_tls_globals_fxn_t set_tls_globals_fxn = NULL;

#ifdef USE_BEGIN_REDRAW_FXN
   // Called when redrawing the UI.
   //  An attempt to fix the GL issue with shared modules on Cameron's machine.
   rack_begin_redraw_fxn_t begin_redraw_fxn = NULL;
#endif // USE_BEGIN_REDRAW_FXN

#endif // USE_VST2

	virtual ~Plugin();
	void addModel(Model *model);
};


struct Model {
	Plugin *plugin = NULL;
	/** An identifier for the model, e.g. "VCO". Used for saving patches.
	The model slug must be unique in your plugin, but it doesn't need to be unique among different plugins.
	*/
	std::string slug;
	/** Human readable name for your model, e.g. "Voltage Controlled Oscillator" */
	std::string name;
	/** The author name of the module.
	This might be different than the plugin slug. For example, if you create multiple plugins but want them to be branded similarly, you may use the same author in multiple plugins.
	You may even have multiple authors in one plugin, although this property will be moved to Plugin for Rack 1.0.
	*/
	std::string author;
	/** List of tags representing the function(s) of the module (optional) */
	std::list<ModelTag> tags;

	virtual ~Model() {}
	/** Creates a headless Module */
	virtual Module *createModule() { return NULL; }
	/** Creates a ModuleWidget with a Module attached */
	virtual ModuleWidget *createModuleWidget() { return NULL; }
	/** Creates a ModuleWidget with no Module, useful for previews */
	virtual ModuleWidget *createModuleWidgetNull() { return NULL; }

	/** Create Model subclass which constructs a specific Module and ModuleWidget subclass */
	template <typename TModule, typename TModuleWidget, typename... Tags>
	static Model *create(std::string author, std::string slug, std::string name, Tags... tags) {
		struct TModel : Model {
			Module *createModule() override {
				TModule *module = new TModule();
				return module;
			}
			ModuleWidget *createModuleWidget() override {
            // printf("xxx createModuleWidget: ENTER\n");
				TModule *module = new TModule();
            // printf("xxx createModuleWidget: module=%p\n", module);
				TModuleWidget *moduleWidget = new TModuleWidget(module);
            // printf("xxx createModuleWidget: moduleWidget=%p\n", moduleWidget);
				moduleWidget->model = this;
            // printf("xxx createModuleWidget: LEAVE\n");
				return moduleWidget;
			}
			ModuleWidget *createModuleWidgetNull() override {
				TModuleWidget *moduleWidget = new TModuleWidget(NULL);
				moduleWidget->model = this;
				return moduleWidget;
			}
		};
		TModel *o = new TModel();
		o->author = author;
		o->slug = slug;
		o->name = name;
		o->tags = {tags...};
		return o;
	}
};


void pluginInit(bool devMode, bool _bFX);
void pluginDestroy();
void pluginLogIn(std::string email, std::string password);
void pluginLogOut();
/** Returns whether a new plugin is available, and downloads it unless doing a dry run */
bool pluginSync(bool dryRun);
void pluginCancelDownload();
bool pluginIsLoggedIn();
bool pluginIsDownloading();
float pluginGetDownloadProgress();
std::string pluginGetDownloadName();
std::string pluginGetLoginStatus();
Plugin *pluginGetPlugin(std::string pluginSlug);
Model *pluginGetModel(std::string pluginSlug, std::string modelSlug);


extern std::list<Plugin*> gPlugins;
extern std::string gToken;


} // namespace rack



// Access helpers for global UI vars
//
//  (note) these avoid accessing the global rack vars directly 
//          (global TLS vars cannot be exported to dynamically loaded plugins)
//
//  (note) please use the macros and do _not_ call the functions directly!
//
#include "global_pre.hpp"
#include "global_ui.hpp"

extern rack::RackScene *rack_plugin_ui_get_rackscene(void);
#define RACK_PLUGIN_UI_RACKSCENE rack_plugin_ui_get_rackscene()

extern rack::RackWidget *rack_plugin_ui_get_rackwidget(void);
#define RACK_PLUGIN_UI_RACKWIDGET rack_plugin_ui_get_rackwidget()

extern rack::Toolbar *rack_plugin_ui_get_toolbar(void);
#define RACK_PLUGIN_UI_TOOLBAR rack_plugin_ui_get_toolbar()

extern rack::Widget *rack_plugin_ui_get_hovered_widget(void);
#define RACK_PLUGIN_UI_HOVERED_WIDGET rack_plugin_ui_get_hovered_widget()

extern rack::Widget *rack_plugin_ui_get_dragged_widget(void);
#define RACK_PLUGIN_UI_DRAGGED_WIDGET rack_plugin_ui_get_dragged_widget()

extern void rack_plugin_ui_set_dragged_widget(rack::Widget *);
#define RACK_PLUGIN_UI_DRAGGED_WIDGET_SET(a) rack_plugin_ui_set_dragged_widget(a)

extern rack::Widget *rack_plugin_ui_get_draghovered_widget(void);
#define RACK_PLUGIN_UI_DRAGHOVERED_WIDGET rack_plugin_ui_get_draghovered_widget()

extern rack::Widget *rack_plugin_ui_get_focused_widget(void);
#define RACK_PLUGIN_UI_FOCUSED_WIDGET rack_plugin_ui_get_focused_widget()

extern void rack_plugin_ui_set_focused_widget(rack::Widget *);
#define RACK_PLUGIN_UI_FOCUSED_WIDGET_SET(a) rack_plugin_ui_set_focused_widget(a)
