#ifndef TRACERPROCESSOR_H
#define TRACERPROCESSOR_H

#include "Str.h"

#include "Processor.h"
#include "PipelineData.h"
#include "ProducerConsumerQueue.h"
#include "Filter.h"

class TracerProcessor : public Processor
{
    public:
        TracerProcessor();
        virtual ~TracerProcessor();
    
        virtual void init(const XmlNode& pluginInstanceConfig);
    
        virtual EProcessorReturn execute(CowPipelineData&
            pData);   

        private:
            FilterManager   _filterMgr;
};

#endif // TRACERPROCESSOR_H
