#include "cf.hpp"

RACK_PLUGIN_MODEL_DECLARE(cf, ALGEBRA);
RACK_PLUGIN_MODEL_DECLARE(cf, BUFFER);
RACK_PLUGIN_MODEL_DECLARE(cf, CHOKE);
RACK_PLUGIN_MODEL_DECLARE(cf, CUBE);
RACK_PLUGIN_MODEL_DECLARE(cf, CUTS);
RACK_PLUGIN_MODEL_DECLARE(cf, DAVE);
RACK_PLUGIN_MODEL_DECLARE(cf, DISTO);
RACK_PLUGIN_MODEL_DECLARE(cf, EACH);
RACK_PLUGIN_MODEL_DECLARE(cf, FOUR);
RACK_PLUGIN_MODEL_DECLARE(cf, LEDS);
RACK_PLUGIN_MODEL_DECLARE(cf, L3DS3Q);
RACK_PLUGIN_MODEL_DECLARE(cf, LEDSEQ);
RACK_PLUGIN_MODEL_DECLARE(cf, MASTER);
RACK_PLUGIN_MODEL_DECLARE(cf, METRO);
RACK_PLUGIN_MODEL_DECLARE(cf, MONO);
RACK_PLUGIN_MODEL_DECLARE(cf, PATCH);
RACK_PLUGIN_MODEL_DECLARE(cf, PEAK);
RACK_PLUGIN_MODEL_DECLARE(cf, PLAY);
RACK_PLUGIN_MODEL_DECLARE(cf, PLAYER);
RACK_PLUGIN_MODEL_DECLARE(cf, SLIDERSEQ);
RACK_PLUGIN_MODEL_DECLARE(cf, STEPS);
RACK_PLUGIN_MODEL_DECLARE(cf, STEREO);
RACK_PLUGIN_MODEL_DECLARE(cf, SUB);
RACK_PLUGIN_MODEL_DECLARE(cf, trSEQ);
RACK_PLUGIN_MODEL_DECLARE(cf, VARIABLE);

RACK_PLUGIN_INIT(cf) {
   RACK_PLUGIN_INIT_ID();
   RACK_PLUGIN_INIT_VERSION("0.6.8");

   RACK_PLUGIN_INIT_WEBSITE("https://github.com/cfoulc/cf");
   RACK_PLUGIN_INIT_MANUAL("https://github.com/cfoulc/cf/blob/master/README.md");

   RACK_PLUGIN_MODEL_ADD(cf, ALGEBRA);
   RACK_PLUGIN_MODEL_ADD(cf, BUFFER);
   RACK_PLUGIN_MODEL_ADD(cf, CHOKE);
   RACK_PLUGIN_MODEL_ADD(cf, CUBE);
   RACK_PLUGIN_MODEL_ADD(cf, CUTS);
   RACK_PLUGIN_MODEL_ADD(cf, DAVE);
   RACK_PLUGIN_MODEL_ADD(cf, DISTO);
   RACK_PLUGIN_MODEL_ADD(cf, EACH);
   RACK_PLUGIN_MODEL_ADD(cf, FOUR);
   RACK_PLUGIN_MODEL_ADD(cf, LEDS);
   RACK_PLUGIN_MODEL_ADD(cf, L3DS3Q);
   RACK_PLUGIN_MODEL_ADD(cf, LEDSEQ);
   RACK_PLUGIN_MODEL_ADD(cf, MASTER);
   RACK_PLUGIN_MODEL_ADD(cf, METRO);
   RACK_PLUGIN_MODEL_ADD(cf, MONO);
   RACK_PLUGIN_MODEL_ADD(cf, PATCH);
   RACK_PLUGIN_MODEL_ADD(cf, PEAK);
   RACK_PLUGIN_MODEL_ADD(cf, PLAY);
   RACK_PLUGIN_MODEL_ADD(cf, PLAYER);
   RACK_PLUGIN_MODEL_ADD(cf, SLIDERSEQ);
   RACK_PLUGIN_MODEL_ADD(cf, STEPS);
   RACK_PLUGIN_MODEL_ADD(cf, STEREO);
   RACK_PLUGIN_MODEL_ADD(cf, SUB);
   RACK_PLUGIN_MODEL_ADD(cf, trSEQ);
   RACK_PLUGIN_MODEL_ADD(cf, VARIABLE);
}
