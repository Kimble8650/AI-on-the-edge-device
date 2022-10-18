#include <sstream>
#include "ClassFlowMQTT.h"
#include "Helper.h"
#include "connect_wlan.h"

#include "time_sntp.h"
#include "interface_mqtt.h"
#include "ClassFlowPostProcessing.h"
#include "ClassLogFile.h"
#include "../jomjol_wlan/read_wlanini.h"

#include <time.h>

extern const char* libfive_git_version(void);
extern const char* libfive_git_revision(void);
extern const char* libfive_git_branch(void);

#define __HIDE_PASSWORD

void ClassFlowMQTT::SetInitialParameter(void)
{
    uri = "";
    topic = "";
    topicError = "";
    topicRate = "";
    topicTimeStamp = "";
    maintopic = "";
    mainerrortopic = ""; 

    topicUptime = "";
    topicFreeMem = "";
    clientname = "watermeter";
    OldValue = "";
    flowpostprocessing = NULL;  
    user = "";
    password = ""; 
    SetRetainFlag = 0;  
    previousElement = NULL;
    ListFlowControll = NULL; 
    disabled = false;
    MQTTenable = false;
    keepAlive = 600; // TODO This must be greater than the Flow Interval!
}       

ClassFlowMQTT::ClassFlowMQTT()
{
    SetInitialParameter();
}

ClassFlowMQTT::ClassFlowMQTT(std::vector<ClassFlow*>* lfc)
{
    SetInitialParameter();

    ListFlowControll = lfc;
    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
    }
}

ClassFlowMQTT::ClassFlowMQTT(std::vector<ClassFlow*>* lfc, ClassFlow *_prev)
{
    SetInitialParameter();

    previousElement = _prev;
    ListFlowControll = lfc;

    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
    }
}


bool ClassFlowMQTT::ReadParameter(FILE* pfile, string& aktparamgraph)
{
    std::vector<string> zerlegt;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (toUpper(aktparamgraph).compare("[MQTT]") != 0)       // Paragraph passt nich zu MakeImage
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        zerlegt = this->ZerlegeZeile(aktparamgraph);
        if ((toUpper(zerlegt[0]) == "USER") && (zerlegt.size() > 1))
        {
            this->user = zerlegt[1];
        }  
        if ((toUpper(zerlegt[0]) == "PASSWORD") && (zerlegt.size() > 1))
        {
            this->password = zerlegt[1];
        }               
        if ((toUpper(zerlegt[0]) == "URI") && (zerlegt.size() > 1))
        {
            this->uri = zerlegt[1];
        }
        if ((toUpper(zerlegt[0]) == "SETRETAINFLAG") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                SetRetainFlag = 1;  
        }


        if ((toUpper(zerlegt[0]) == "CLIENTID") && (zerlegt.size() > 1))
        {
            this->clientname = zerlegt[1];
        }

        if (((toUpper(zerlegt[0]) == "TOPIC") || (toUpper(zerlegt[0]) == "MAINTOPIC")) && (zerlegt.size() > 1))
        {
            maintopic = zerlegt[1];
        }
    }

#ifdef __HIDE_PASSWORD
    printf("Init Read with uri: %s, clientname: %s, user: %s, password: XXXXXX, maintopic: %s\n", uri.c_str(), clientname.c_str(), user.c_str(), mainerrortopic.c_str());
#else
    printf("Init Read with uri: %s, clientname: %s, user: %s, password: %s, maintopic: %s\n", uri.c_str(), clientname.c_str(), user.c_str(), password.c_str(), mainerrortopic.c_str());
#endif

    if (!MQTTisConnected() && (uri.length() > 0) && (maintopic.length() > 0)) 
    { 
        printf("InitMQTTInit\n");
        mainerrortopic = maintopic + "/connection";
#ifdef __HIDE_PASSWORD
        printf("Init MQTT with uri: %s, clientname: %s, user: %s, password: XXXXXXXX, maintopic: %s\n", uri.c_str(), clientname.c_str(), user.c_str(), mainerrortopic.c_str());
#else
        printf("Init MQTT with uri: %s, clientname: %s, user: %s, password: %s, maintopic: %s\n", uri.c_str(), clientname.c_str(), user.c_str(), password.c_str(), mainerrortopic.c_str());
#endif
        if (!MQTTInit(uri, clientname, user, password, mainerrortopic, keepAlive))
        { // Failed
            MQTTenable = false;
            return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
        }
    }

    // Try sending mainerrortopic. If it fails, re-run init
    if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    { // Failed
        LogFile.WriteToFile("MQTT - Re-running init...!");
        if (!MQTTInit(this->uri, this->clientname, this->user, this->password, this->mainerrortopic, keepAlive))
        { // Failed
            MQTTenable = false;
            return false;
        } 
    }

    // Try again and quit if it fails
    if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    { // Failed
        MQTTenable = false;
        return false;
    }



   
 /*   if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    { // Failed
        LogFile.WriteToFile("MQTT - Could not publish connection status!");
        MQTTenable = false;
        return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
    }*/

 /*   if(!MQTTPublish(_LWTContext, "", 1))
    {
        LogFile.WriteToFile("MQTT - Could not publish LWT!");
        MQTTenable = false;
        return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
    }*/


    MQTThomeassistantDiscovery();

    MQTTenable = true;
    return true;
}


string ClassFlowMQTT::GetMQTTMainTopic()
{
    return maintopic;
}


bool ClassFlowMQTT::doFlow(string zwtime)
{
    // Try sending mainerrortopic. If it fails, re-run init
    if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    { // Failed
        LogFile.WriteToFile("MQTT - Re-running init...!");
        if (!MQTTInit(this->uri, this->clientname, this->user, this->password, this->mainerrortopic, keepAlive))
        { // Failed
            MQTTenable = false;
            return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
        } 
    }

    // Try again and quit if it fails
    if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    { // Failed
        MQTTenable = false;
        return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
    }

    std::string result;
    std::string resulterror = "";
    std::string resultraw = "";
    std::string resultrate = "";
    std::string resulttimestamp = "";
    std::string resultchangabs = "";
    string zw = "";
    string namenumber = "";

    // if (!MQTTPublish(mainerrortopic, "connected", SetRetainFlag))
    //{ // Failed, skip other topics
    //    return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
    //}
    
    zw = maintopic + "/" + "uptime";
    char uptimeStr[11];
    sprintf(uptimeStr, "%ld", (long)getUpTime());
    MQTTPublish(zw, uptimeStr, SetRetainFlag);

    zw = maintopic + "/" + "freeMem";
    char freeheapmem[11];
    sprintf(freeheapmem, "%zu", esp_get_free_heap_size());
    if (!MQTTPublish(zw, freeheapmem, SetRetainFlag))
    { // Failed, skip other topics
        return true; // We need to return true despite we failed, else it will retry 5x and then reboot!
    }

    zw = maintopic + "/" + "wifiRSSI";
    char rssi[11];
    sprintf(rssi, "%d", get_WIFI_RSSI());
    MQTTPublish(zw, rssi, SetRetainFlag);

    zw = maintopic + "/" + "CPUtemp";
    std::string cputemp = std::to_string(temperatureRead());
    MQTTPublish(zw, cputemp, SetRetainFlag);

    if (flowpostprocessing)
    {
        std::vector<NumberPost*>* NUMBERS = flowpostprocessing->GetNumbers();

        for (int i = 0; i < (*NUMBERS).size(); ++i)
        {
            result =  (*NUMBERS)[i]->ReturnValue;
            resultraw =  (*NUMBERS)[i]->ReturnRawValue;
            resulterror = (*NUMBERS)[i]->ErrorMessageText;
            resultrate = (*NUMBERS)[i]->ReturnRateValue;
            resultchangabs = (*NUMBERS)[i]->ReturnChangeAbsolute;
            resulttimestamp = (*NUMBERS)[i]->timeStamp;

            namenumber = (*NUMBERS)[i]->name;
            if (namenumber == "default")
                namenumber = maintopic + "/";
            else
                namenumber = maintopic + "/" + namenumber + "/";

            zw = namenumber + "value"; 
            if (result.length() > 0)   
                MQTTPublish(zw, result, SetRetainFlag);

            zw = namenumber + "error"; 
            if (resulterror.length() > 0)  
                MQTTPublish(zw, resulterror, SetRetainFlag);

            zw = namenumber + "rate"; 
            if (resultrate.length() > 0)   
                MQTTPublish(zw, resultrate, SetRetainFlag);

            zw = namenumber + "changeabsolut"; 
            if (resultchangabs.length() > 0)   
                MQTTPublish(zw, resultchangabs, SetRetainFlag);

            zw = namenumber + "raw"; 
            if (resultraw.length() > 0)   
                MQTTPublish(zw, resultraw, SetRetainFlag);

            zw = namenumber + "timestamp";
            if (resulttimestamp.length() > 0)
                MQTTPublish(zw, resulttimestamp, SetRetainFlag);


            std::string json = "";
            
            if (result.length() > 0)
                json += "{\"value\":"+result;
            else
                json += "{\"value\":\"\"";

            json += ",\"raw\":\""+resultraw;
            json += "\",\"error\":\""+resulterror;
            if (resultrate.length() > 0)
                json += "\",\"rate\":"+resultrate;
            else
                json += "\",\"rate\":\"\"";
            json += ",\"timestamp\":\""+resulttimestamp+"\"}";

            zw = namenumber + "json";
            MQTTPublish(zw, json, SetRetainFlag);
        }
    }
    else
    {
        for (int i = 0; i < ListFlowControll->size(); ++i)
        {
            zw = (*ListFlowControll)[i]->getReadout();
            if (zw.length() > 0)
            {
                if (result.length() == 0)
                    result = zw;
                else
                    result = result + "\t" + zw;
            }
        }
        MQTTPublish(topic, result, SetRetainFlag);
    }
    
    OldValue = result;
    
    return true;
}

void ClassFlowMQTT::sendHomeAssistantDiscoveryTopic(std::string group, std::string field, std::string icon, std::string unit) {
    std::string version = std::string(libfive_git_version());

    if (version == "") {
        version = std::string(libfive_git_branch()) + " (" + std::string(libfive_git_revision()) + ")";
    }
    std::string deviceName = "AIOTED";

    char *ssid = NULL, *passwd = NULL, *hostname = NULL, *ip = NULL, *gateway = NULL, *netmask = NULL, *dns = NULL;
    LoadWlanFromFile("/sdcard/wlan.ini", ssid, passwd, hostname, ip, gateway, netmask, dns);

    if (hostname != NULL) {
        deviceName = hostname;
    }

    std::string topic;
    std::string topicT;
    std::string payload;
    std::string nl = "\n";

    if (group != "") {
        topic = group + "/" + field;
        topicT = group + "_" + field;
    }
    else {
        topic =  field;
        topicT = field;
    }
    
    topic = "homeassistant/sensor/" + deviceName + "-" + topicT + "/config";
    
    payload = "{" + nl +
        "\"~\": \"" + deviceName + "\"," + nl +
        "\"unique_id\": \"" + deviceName + "-" +topicT + "\"," + nl +
        "\"name\": \"" + topic + "\"," + nl +
        "\"icon\": \"mdi:" + icon + "\"," + nl +
        "\"unit_of_meas\": \"" + unit + "\"," + nl +
        "\"state_topic\": \"~/" + topic + "\"," + nl;
        
/* Enable once MQTT is stable */
/*    payload += 
        "\"availability_topic\": \"~/connection\"," + nl +
        "\"payload_available\": \"connected\"," + nl +
        "\"payload_not_available\": \"connection lost\"," + nl; */
    
    payload +=
    "\"device\": {" + nl +
        "\"identifiers\": [\"" + deviceName + "\"]," + nl +
        "\"name\": \"" + deviceName + "\"," + nl +
        "\"model\": \"HomeAssistant Discovery for AI on the Edge Device\"," + nl +
        "\"manufacturer\": \"AI on the Edge Device - https://github.com/jomjol/AI-on-the-edge-device\"," + nl +
        "\"sw_version\": \"" + version + "\"" + nl +
    "}" + nl +
    "}" + nl;
    
    MQTTPublish(topic, payload, true);
}

void ClassFlowMQTT::MQTThomeassistantDiscovery() {
    LogFile.WriteToFile("MQTT - Sending Homeassistant Discovery Topics...");

    sendHomeAssistantDiscoveryTopic("", "uptime",              "clock-time-eight-outline", "s");
    sendHomeAssistantDiscoveryTopic("", "freeMem",             "memory",                   "B");
    sendHomeAssistantDiscoveryTopic("", "wifiRSSI",            "file-question-outline",    "dBm");
    sendHomeAssistantDiscoveryTopic("", "CPUtemp",             "thermometer",              "°C");
    
    if (flowpostprocessing){
        std::vector<NumberPost*>* NUMBERS = flowpostprocessing->GetNumbers();

        for (int i = 0; i < (*NUMBERS).size(); ++i) {
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "value",           "gauge",                    "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "error",           "alert-circle-outline",     "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "rate",            "file-question-outline",    "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "changeabsolut",   "file-question-outline",    "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "raw",             "file-question-outline",    "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "timestamp",       "clock-time-eight-outline", "");
            sendHomeAssistantDiscoveryTopic((*NUMBERS)[i]->name, "json",            "code-json",                "");
        }
    }
}