#include "examples/animationclip/debug_draw.h"

int _main_(int _argc, char **_argv)
{
    DebugDrawSample *app = new DebugDrawSample("Hello World", "Hello World Demo.");
    return entry::runApp(app, _argc, _argv);
}