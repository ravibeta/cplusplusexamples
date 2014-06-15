



#include "TracerProcessor.h"
#include "PipelineData.h"
#include "Processor.h"
#include "SHA256Hash.h"
#include "Base64.h"
#include "Logger.h"
#include "SkipThisProcessorException.h"
#include "PropertiesMapConfig.h"
#include "LicenseMgr.h"

static Logger gLogger("TracerProcessor");

static const Str TRACING_INDEX = "_internal";
static const char TRACING_STANZA[] = "tracer";


TracerProcessor::TracerProcessor()
    {}  
TracerProcessor::~TracerProcessor()
    {}  

void TracerProcessor::init (const XmlNode&)
{
        gLogger.info( "Initializing" );
        // find host name and timestamp
}

Processor::EProcessorReturn TracerProcessor::execute(CowPipelineData& pData)
{
    // First, if we aren't enabled, exit right away.  Also if there's no
    // _raw key there's nothing for us to do
    if (!pData.hasRaw() ) { 
       return Processor::eTrue;
    }   
    const StrSegment& pRawData = pData.getRaw();
    const Str *pIndex = pData.getIfAvail(PipelineData::indexKey());

    Str host;
    host = ServerConfig::instance()->getHostname();

    TimeFormat fmt("%Y-%m-%d.%X");
    CurrentLocalStructTm now;
    Str time;
    now.appendFtime(time, fmt);

    Str newData;
    newdata.appendFormat("%s\n%s:%s", pData.getRaw().c_str(), time.c_str(), host.c_str());
    pData.getWriter()->setRawDestructive(newData);
    return Processor::eTrue;
}

DEFINE_PLUGIN(tracerprocessor, TracerProcessor)
