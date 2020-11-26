#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include "global_pre.hpp"
#include "app.hpp"
#include "engine.hpp"
#include "plugin.hpp"
#include "window.hpp"
#include "global.hpp"
#include "global_ui.hpp"

#ifdef __GNUC__
#include <fenv.h>
#endif

#ifdef USE_LOG_PRINTF
extern void log_printf(const char *logData, ...);
#undef Dprintf
#define Dprintf log_printf
#else
#define Dprintf printf
#endif // USE_LOG_PRINTF


namespace rack {

bool vst2_find_module_and_paramid_by_unique_paramid(int uniqueParamId, Module**retModule, int *retParamId);

float Light::getBrightness() {
	// LEDs are diodes, so don't allow reverse current.
	// For some reason, instead of the RMS, the sqrt of RMS looks better
	return powf(fmaxf(0.f, value), 0.25f);
}

void Light::setBrightnessSmooth(float brightness, float frames) {
	float v = (brightness > 0.f) ? brightness * brightness : 0.f;
	if (v < value) {
		// Fade out light with lambda = framerate
		value += (v - value) * global->engine.sampleTime * frames * 60.f;
	}
	else {
		// Immediately illuminate light
		value = v;
	}
}


void Wire::step() {
	float value = outputModule->outputs[outputId].value;
	inputModule->inputs[inputId].value = value;
}


void engineInit() {
	engineSetSampleRate(44100.0);
}

void engineDestroy() {
	// Make sure there are no wires or modules in the rack on destruction. This suggests that a module failed to remove itself before the WINDOW was destroyed.
	assert(global->gWires.empty());
	assert(global->gModules.empty());
}

static void engineStep() {
	// Param interpolation
	if (global->engine.smoothModule) {
		float value = global->engine.smoothModule->params[global->engine.smoothParamId].value;
		const float lambda = 60.0; // decay rate is 1 graphics frame
		float delta = global->engine.smoothValue - value;
		float newValue = value + delta * lambda * global->engine.sampleTime;
		if (value == newValue) {
			// Snap to actual smooth value if the value doesn't change enough (due to the granularity of floats)
			global->engine.smoothModule->params[global->engine.smoothParamId].value = global->engine.smoothValue;
			global->engine.smoothModule = NULL;
		}
		else {
			global->engine.smoothModule->params[global->engine.smoothParamId].value = newValue;
		}
	}

	// Step modules
	for (Module *module : global->gModules) {
		std::chrono::high_resolution_clock::time_point startTime;
		if (global->gPowerMeter) {
			startTime = std::chrono::high_resolution_clock::now();

			module->step();

			auto stopTime = std::chrono::high_resolution_clock::now();
			float cpuTime = std::chrono::duration<float>(stopTime - startTime).count() * global->engine.sampleRate;
			module->cpuTime += (cpuTime - module->cpuTime) * global->engine.sampleTime / 0.5f;
		}
		else {
			module->step();
		}

		// Step ports
		for (Input &input : module->inputs) {
			if (input.active) {
				float value = input.value / 5.f;
				input.plugLights[0].setBrightnessSmooth(value);
				input.plugLights[1].setBrightnessSmooth(-value);
			}
		}
		for (Output &output : module->outputs) {
			if (output.active) {
				float value = output.value / 5.f;
				output.plugLights[0].setBrightnessSmooth(value);
				output.plugLights[1].setBrightnessSmooth(-value);
			}
		}
	}

	// Step cables by moving their output values to inputs
	for (Wire *wire : global->gWires) {
		wire->step();
	}
}

#ifndef USE_VST2
static void engineRun() {
#ifdef _MSC_VER
	// Set CPU to flush-to-zero (FTZ) and denormals-are-zero (DAZ) mode
	// https://software.intel.com/en-us/node/682949
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif // _MSC_VER

#if defined(__GNUC__) && (defined(ARCH_X64) || defined(ARCH_X86))
   ::fesetround(FE_TOWARDZERO);
#endif // __GNUC__

	// Every time the engine waits and locks a mutex, it steps this many frames
	const int mutexSteps = 64;
	// Time in seconds that the engine is rushing ahead of the estimated clock time
	double ahead = 0.0;
	auto lastTime = std::chrono::high_resolution_clock::now();

	while (global->engine.running) {
		global->engine.vipMutex.wait();

		if (!global->gPaused) {
			std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
			for (int i = 0; i < mutexSteps; i++) {
				engineStep();
			}
		}

		double stepTime = mutexSteps * global->engine.sampleTime;
		ahead += stepTime;
		auto currTime = std::chrono::high_resolution_clock::now();
		const double aheadFactor = 2.0;
		ahead -= aheadFactor * std::chrono::duration<double>(currTime - lastTime).count();
		lastTime = currTime;
		ahead = fmaxf(ahead, 0.0);

		// Avoid pegging the CPU at 100% when there are no "blocking" modules like AudioInterface, but still step audio at a reasonable rate
		// The number of steps to wait before possibly sleeping
		const double aheadMax = 1.0; // seconds
		if (ahead > aheadMax) {
			std::this_thread::sleep_for(std::chrono::duration<double>(stepTime));
		}
	}
}

void engineStart() {
	global->engine.running = true;
	global->engine.thread = std::thread(engineRun);
}

void engineStop() {
	global->engine.running = false;
	global->engine.thread.join();
}
#endif // USE_VST2

void engineAddModule(Module *module) {
	assert(module);
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	// Check that the module is not already added
	auto it = std::find(global->gModules.begin(), global->gModules.end(), module);
	assert(it == global->gModules.end());
	global->gModules.push_back(module);
#ifdef USE_VST2
   module->vst2_unique_param_base_id = global->vst2.next_unique_param_base_id;
   global->vst2.next_unique_param_base_id += module->params.size() + 1;
#endif // USE_VST2
}

void engineRemoveModule(Module *module) {
	assert(module);
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	// If a param is being smoothed on this module, stop smoothing it immediately
	if (module == global->engine.smoothModule) {
		global->engine.smoothModule = NULL;
	}
	// Check that all wires are disconnected
	for (Wire *wire : global->gWires) {
		assert(wire->outputModule != module);
		assert(wire->inputModule != module);
	}
	// Check that the module actually exists
	auto it = std::find(global->gModules.begin(), global->gModules.end(), module);
	assert(it != global->gModules.end());
	// Remove it
	global->gModules.erase(it);
}

static void updateActive() {
	// Set everything to inactive
	for (Module *module : global->gModules) {
		for (Input &input : module->inputs) {
			input.active = false;
		}
		for (Output &output : module->outputs) {
			output.active = false;
		}
	}
	// Set inputs/outputs to active
	for (Wire *wire : global->gWires) {
		wire->outputModule->outputs[wire->outputId].active = true;
		wire->inputModule->inputs[wire->inputId].active = true;
	}
}

void engineAddWire(Wire *wire) {
	assert(wire);
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	// Check wire properties
	assert(wire->outputModule);
	assert(wire->inputModule);
	// Check that the wire is not already added, and that the input is not already used by another cable
	for (Wire *wire2 : global->gWires) {
		assert(wire2 != wire);
		assert(!(wire2->inputModule == wire->inputModule && wire2->inputId == wire->inputId));
	}
	// Add the wire
	global->gWires.push_back(wire);
	updateActive();
}

void engineRemoveWire(Wire *wire) {
	assert(wire);
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	// Check that the wire is already added
	auto it = std::find(global->gWires.begin(), global->gWires.end(), wire);
	assert(it != global->gWires.end());
	// Set input to 0V
	wire->inputModule->inputs[wire->inputId].value = 0.0;
	// Remove the wire
	global->gWires.erase(it);
	updateActive();
}

#ifdef USE_VST2
static int loc_vst2_find_unique_param_by_module_and_paramid(Module *module, int paramId) {
   return module->vst2_unique_param_base_id + paramId;
}

bool vst2_find_module_and_paramid_by_unique_paramid(int uniqueParamId, Module**retModule, int *retParamId) {
   if(uniqueParamId >= 0)
   {
      if(uniqueParamId < VST2_MAX_UNIQUE_PARAM_IDS)
      {
         // (todo) speed this up with a hashtable
         for(Module *module : global->gModules) {
            if( (uniqueParamId >= module->vst2_unique_param_base_id) &&
                (uniqueParamId < int(module->vst2_unique_param_base_id + module->params.size()))
                )
            {
               *retParamId = (uniqueParamId - module->vst2_unique_param_base_id);
               *retModule = module;
               return true;
            }
         }         
      }
   }
   return false;
}

#endif // USE_VST2

#ifdef USE_VST2
void engineSetParam(Module *module, int paramId, float value, bool bVSTAutomate) {
#else
void engineSetParam(Module *module, int paramId, float value) {
#endif
   if(module->params[paramId].value == value)
      return;
	module->params[paramId].value = value;

#ifdef USE_VST2
   if(bVSTAutomate && !global->vst2.b_patch_loading)
   {
      // (note) [bsp] this is usually called from the UI thread (see ParamWidget.cpp)
      // (note) [bsp] VST requires all parameters to be in the normalized 0..1 range
      // (note) [bsp] VCV Rack parameters however are arbitrary floats
      // printf("xxx vcvrack: paramId=%d value=%f (=> %f)\n", paramId, value, normValue);
      int uniqueParamId = loc_vst2_find_unique_param_by_module_and_paramid(module, paramId);
      if(-1 != uniqueParamId)
      {
         ModuleWidget *moduleWidget = global_ui->app.gRackWidget->findModuleWidgetByModule(module);
         if(NULL != moduleWidget)
         {
            // Find 
            ParamWidget *paramWidget = moduleWidget->findParamWidgetByParamId(paramId);
            if(NULL != paramWidget)
            {
               if(isfinite(paramWidget->minValue) && isfinite(paramWidget->maxValue))
               {
                  // Normalize parameter
                  float paramRange = (paramWidget->maxValue - paramWidget->minValue);
                  if(paramRange > 0.0f)
                  {
                     float normValue = (value - paramWidget->minValue) / paramRange;
                     // printf("xxx paramId=%d normValue=%f\n", paramId, normValue);

                     // Call host audioMasterAutomate
                     vst2_handle_ui_param(uniqueParamId, normValue);
                  }
               }
            }
         }
      }
   }
#endif // USE_VST2
}

#ifdef USE_VST2
}
using namespace rack;
void vst2_queue_param(int uniqueParamId, float value, bool bNormalized) {
   // Called from any thread via setParameter()
   //  (note) protected by caller mutex
   VST2QueuedParam qp;
   qp.unique_id = uniqueParamId;
   qp.value = value;
   qp.b_normalized = bNormalized;
   global->vst2.queued_params.push_back(qp);
}

void vst2_handle_queued_params(void) {
   // Called in processReplacing()
   //  (note) protected by caller mutex
   if(global->vst2.queued_params.size() > 0)
   {
      global_ui->app.mtx_param.lock();
      for(VST2QueuedParam qp : global->vst2.queued_params)
      {
         Module *module;
         int paramId;
         if(vst2_find_module_and_paramid_by_unique_paramid(qp.unique_id, &module, &paramId))
         {
            ModuleWidget *moduleWidget = global_ui->app.gRackWidget->findModuleWidgetByModule(module);
            if(NULL != moduleWidget)
            {
               // Find 
               ParamWidget *paramWidget = moduleWidget->findParamWidgetByParamId(paramId);
               if(NULL != paramWidget)
               {
                  if(isfinite(paramWidget->minValue) && isfinite(paramWidget->maxValue))
                  {
                     if(qp.b_normalized)
                     {
                        // De-Normalize parameter
                        global_ui->param_info.b_lock = true;
                        float paramRange = (paramWidget->maxValue - paramWidget->minValue);
                        if(paramRange > 0.0f)
                        {
                           float value = (qp.value * paramRange) + paramWidget->minValue;
                           // Dprintf("vst2_handle_queued_params: paramId=%d value=%f min=%f max=%f\n", paramId, value, paramWidget->minValue, paramWidget->maxValue);
                           engineSetParam(module, paramId, value, false/*bVSTAutomate*/);

                           // Update UI widget
                           paramWidget->setValue(value);
                        }
                        global_ui->param_info.b_lock = false;
                     }
                     else
                     {
                        float value = qp.value;
                        if(value < paramWidget->minValue)
                           value = paramWidget->minValue;
                        else if(value > paramWidget->maxValue)
                           value = paramWidget->maxValue;
                        engineSetParam(module, paramId, value, false/*bVSTAutomate*/);

                        // Update UI widget
                        paramWidget->setValue(qp.value);
                     }
                  }
               }
            }
         }
      }
      global->vst2.queued_params.clear();
      global_ui->app.mtx_param.unlock();
   }
}

float vst2_get_param(int uniqueParamId) {
   Module *module;
   int paramId;
   if(vst2_find_module_and_paramid_by_unique_paramid(uniqueParamId, &module, &paramId))
   {
      if(sUI(paramId) < sUI(module->params.size())) // paranoia
      {
         ModuleWidget *moduleWidget = global_ui->app.gRackWidget->findModuleWidgetByModule(module);
         if(NULL != moduleWidget)
         {
            // Find 
            ParamWidget *paramWidget = moduleWidget->findParamWidgetByParamId(paramId);
            if(NULL != paramWidget)
            {
               if(isfinite(paramWidget->minValue) && isfinite(paramWidget->maxValue))
               {
                  float value = module->params[paramId].value;
                  value = (value - paramWidget->minValue) / (paramWidget->maxValue - paramWidget->minValue);
                  return value;
               }
            }
         }
      }      
   }
   return 0.0f;
}

void vst2_get_param_name(int uniqueParamId, char *s, int sMaxLen) {
   Module *module;
   int paramId;
   if(vst2_find_module_and_paramid_by_unique_paramid(uniqueParamId, &module, &paramId))
   {
      if(sUI(paramId) < sUI(module->params.size())) // paranoia
      {
         // (note) VCV params do not have names
         ::snprintf(s, sMaxLen, "%4d: <param>", uniqueParamId);
         return;
      }      
   }
   ::snprintf(s, sMaxLen, "%4d: -", uniqueParamId);
}

namespace rack {
#endif // USE_VST2

void engineSetParamSmooth(Module *module, int paramId, float value) {
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	// Since only one param can be smoothed at a time, if another param is currently being smoothed, skip to its final state
	if (global->engine.smoothModule && !(global->engine.smoothModule == module && global->engine.smoothParamId == paramId)) {
		global->engine.smoothModule->params[global->engine.smoothParamId].value = global->engine.smoothValue;
	}
	global->engine.smoothModule = module;
	global->engine.smoothParamId = paramId;
	global->engine.smoothValue = value;
}

void engineSetSampleRate(float newSampleRate) {
	// // VIPLock vipLock(global->engine.vipMutex);
	std::lock_guard<std::recursive_mutex> lock(global->engine.mutex);
	global->engine.sampleRate = newSampleRate;
	global->engine.sampleTime = 1.0 / global->engine.sampleRate;
	// onSampleRateChange
	for (Module *module : global->gModules) {
      printf("xxx engineSetSampleRate: module=%p\n", module); fflush(stdout);
		module->onSampleRateChange();
	}
}

float engineGetSampleRate() {
	return global->engine.sampleRate;
}

float engineGetSampleTime() {
	return global->engine.sampleTime;
}

} // namespace rack


#ifdef USE_VST2
using namespace rack;
void vst2_set_samplerate(sF32 _rate) {
   rack::engineSetSampleRate(_rate);
}
void vst2_engine_process(float *const*_in, float **_out, unsigned int _numFrames) {
   global->vst2.inputs = _in;
   global->vst2.outputs = _out;
   for(global->vst2.frame_idx = 0u; global->vst2.frame_idx < _numFrames; global->vst2.frame_idx++)
   {
      engineStep();
   }
}
#endif // USE_VST2
