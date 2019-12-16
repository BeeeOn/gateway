#include "conrad/ConradEvent.h"

using namespace BeeeOn;
using namespace std;

ConradEvent::ConradEvent()
{
}

ConradEvent ConradEvent::parse(
    DeviceID const &deviceID,
    const Poco::JSON::Object::Ptr message)
{
    ConradEvent event;

    event.m_id = deviceID;

    if (message->has("event"))
        event.m_event = message->getValue<string>("event");
    else
        throw Poco::IllegalStateException("Invalid message format");

    if (message->has("rssi"))
        event.m_rssi = message->getValue<double>("rssi");
    else
        event.m_rssi = -1;

    if (message->has("raw"))
        event.m_raw = message->getValue<string>("raw");

    if (message->has("type"))
        event.m_type = message->getValue<string>("type");

    if (message->has("channels")){
        event.m_channels = message->getValue<string>("channels");

        if (message->getObject("channels")->has("Main"))
            event.m_protState = message->getObject("channels")->getValue<string>("Main");
    }

    return event;
}


DeviceID ConradEvent::id() const
{
    return m_id;
}

double ConradEvent::rssi() const
{
    return m_rssi;
}

string ConradEvent::raw() const
{
    return m_raw;
}

string ConradEvent::type() const
{
    return m_type;
}

string ConradEvent::channels() const
{
    return m_channels;
}

string ConradEvent::event() const
{
    return m_event;
}

string ConradEvent::protState() const
{
    return m_protState;
}
