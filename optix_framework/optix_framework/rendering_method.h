#ifndef RENDERINGMETHOD_H
#define RENDERINGMETHOD_H

#include <optix_world.h>
#include <map>
#include <vector>
#include "folders.h"

class RenderingMethod 
{
public:
    virtual ~RenderingMethod() = default;
    RenderingMethod(optix::Context & context);
	virtual void init() = 0;
	virtual std::string get_suffix() const = 0;
	virtual void pre_trace() {}
	virtual void post_trace() {}

protected:
    optix::Context& context;
};

#endif // RENDERINGMETHOD_H
