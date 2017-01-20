#include "upcon.h"

using namespace std;
 
int arrivedcount = 0;
upconnections upc;


void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;

	printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
		++arrivedcount, message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
}

upcon::upcon() { 
    m_ipstack = IPStack();
    m_client = new MQTT::Client<IPStack, Countdown>(m_ipstack);        
}

upcon::~upcon() {
    m_client->disconnect();
    m_ipstack.disconnect();
    delete m_client;
}

int16_t upcon::connect() {
    int16_t     res;
    int32_t     np, nv;
    std::string s, su, sp, sc;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;   

    res = (getproperty("ip", s) == EXIT_SUCCESS) && \
          (getproperty("port", np) == EXIT_SUCCESS) && \
          (getproperty("version", nv) == EXIT_SUCCESS) && \
          (getproperty("user", su) == EXIT_SUCCESS) && \
          (getproperty("pwd", sp) == EXIT_SUCCESS) && \
          (getproperty("clientid", sc) == EXIT_SUCCESS);
    if(res) { 
        res = m_ipstack.connect(s.c_str(), np);
        if(res==0) {
//            memcpy(&m_data, &dataInit, sizeof(m_data));       
            data.MQTTVersion = nv;
            data.clientID.cstring = (char *)sc.c_str();
            data.username.cstring = (char *)su.c_str();
            data.password.cstring = (char *)sp.c_str();
            res = m_client->connect(data);    
        }
    }
    else res = EXIT_FAILURE;

    if(res == EXIT_FAILURE) {
        m_status = INIT_ERR;
    }
    else {
        m_status = INITIALIZED;
    }
    
    return res;
}
int16_t upcon::publish(cparam &tag) {    
    int16_t         res = EXIT_FAILURE;
    string          topic;
    string          name;
    string          val;
    string          kval;
    string          ts;
    string          sf;
    int16_t         rc;
    int32_t         sec, msec;
   
    rc = getproperty("pubf", sf) == EXIT_SUCCESS && \
         tag.getproperty("name", name)    == EXIT_SUCCESS;
         tag.getproperty("topic", topic)  == EXIT_SUCCESS && \
         tag.getproperty("value", val)    == EXIT_SUCCESS && \
         tag.getproperty("quality", kval) == EXIT_SUCCESS && \
         tag.getproperty("sec", sec) == EXIT_SUCCESS;
         tag.getproperty("msec", msec) == EXIT_SUCCESS;
         
    if(rc==0) {
        cout << "cfg: prop not found" << endl; 
    }
    else {
        ts = time2string(sec)+"."+to_string(msec);        
        sf=replaceString(sf, "value", val);
        sf=replaceString(sf, "quality", kval);
        sf=replaceString(sf, "timestamp", ts);

        if(pubs.size()<_pub_buf_max) {
            pubs.push_back(make_pair(topic+"/"+name, sf));
            res = EXIT_SUCCESS;
        }

        cout<<setfill(' ')<<setw(12)<<left<<topic+"/"+name<<" | "<<sf<<endl;
        tag.acceptnewvalue();
    }
    return res;
}

//
// queue of message for pub processing
//
int16_t upcon::pubdataproc() {
    string          topic, val;
    MQTT::Message   message;
    int16_t         res = EXIT_FAILURE, rc;

    if( !pubs.empty() ) {
        topic = pubs.front().first;
        val = pubs.front().second;
        message.qos = MQTT::QOS1;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)val.c_str();
        message.payloadlen = val.length()+1;
        rc = m_client->publish(topic.c_str(), message);
        if(!rc) {
            pubs.erase(pubs.begin());
            res=EXIT_SUCCESS;
        }
    }
    return res;
}

//
// поток обработки обмена с верхним уровнем 
//
void* upProcessing(void *args) {
    upcon *up = (upcon *)(args);
    if(up->connect() == INITIALIZED) {
        while(up->getStatus()!=TERMINATE) {
            up->pubdataproc();
            usleep(1000);
        }
    }

    cout << "\nend up connection " << up->m_id << endl;
}


