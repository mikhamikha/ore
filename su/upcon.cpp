#include "upcon.h"

using namespace std;
 
int arrivedcount = 0;


MQTTAsync_connectOptions _conn_opts = MQTTAsync_connectOptions_initializer;

upconnections upc;

void onDisconnect(void* context, MQTTAsync_successData* response)
{
//	disconnected = 1;
}


void onSubscribe(void* context, MQTTAsync_successData* response)
{
//	subscribed = 1;
}


void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
//	printf("Subscribe failed, rc %d\n", response->code);
//	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
//	printf("Connect failed, rc %d\n", response->code);
//	finished = 1;
}

void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions ropts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;
/*
	if (opts.showtopics)
		printf("Subscribing to topic %s with client %s at QoS %d\n", topic, opts.clientid, opts.qos);

	ropts.onSuccess = onSubscribe;
	ropts.onFailure = onSubscribeFailure;
	ropts.context = client;
	if ((rc = MQTTAsync_subscribe(client, topic, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
*/
}

void connectionLost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	int rc;
/*
	printf("connectionLost called\n");
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start reconnect, return code %d\n", rc);
		finished = 1;
	}
*/
}

int32_t messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    std::string::size_type  found;
    std::string             top(topicName, strlen(topicName));
    std::string             val((char*)message->payload, message->payloadlen);
    upconnections::iterator ib, ie;
    int32_t                 rc = EXIT_FAILURE;
    
//    cout<<"Received: "<< top <<" value "<< val << " set ";
    
    ib = upc.begin(); ie = upc.end(); 
       
    while( ib != ie ) {
        if((*ib)->handle() == uint32_t(context)) {
            taskparam( top, val );
            break;
        }
        ++ib;
    }
    
//    cout<<endl;
    
    MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
    return rc;
}

void onPublishFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Publish failed, rc %d\n", response ? -1 : response->code);
//	published = -1; 
}


void onPublish(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;

//	published = 1;
}

upcon::upcon() 
{ 
    conn_opts = new MQTTAsync_connectOptions();
    memcpy(conn_opts, &_conn_opts, sizeof(conn_opts));      
}

upcon::~upcon() {
    int16_t rc;

    rc = disconnect();
	MQTTAsync_destroy(&m_client);
    delete []conn_opts->password;
    delete []conn_opts->username;
    delete conn_opts;
}

int16_t upcon::disconnect() {
    int16_t rc;

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    
    disc_opts.onSuccess = onDisconnect;
	
    if ((rc = MQTTAsync_disconnect(m_client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}    
    
    return EXIT_SUCCESS;
}

int16_t upcon::connect() {
    int16_t         rc;
    int32_t         nv, nq;
    std::string     si, sport, su, sp, sc, sq;
    stringstream    url;

    rc =  (getproperty("ip", si) == EXIT_SUCCESS) && \
          (getproperty("port", sport) == EXIT_SUCCESS) && \
          (getproperty("version", nv) == EXIT_SUCCESS) && \
          (getproperty("user", su) == EXIT_SUCCESS) && \
          (getproperty("pwd", sp) == EXIT_SUCCESS) && \
          (getproperty("qos", nq) == EXIT_SUCCESS) && \
          (getproperty("clientid", sc) == EXIT_SUCCESS);
    
    if(rc) {
//        mqtt_create_options opts = \
            { sc.c_str(), 1, '\n', nq, su.c_str(), sp.c_str(), si.c_str(), sport.c_str(), 0, 10 };
        url<<si<<":"<<sport;
        rc = MQTTAsync_create(&m_client, url.str().c_str(), \
                                    sc.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL); 
        
        MQTTAsync_setCallbacks(m_client, m_client, connectionLost, messageArrived, NULL);
        
//        conn_opts.keepAliveInterval = opts.keepalive;
        conn_opts->cleansession = 1;
        conn_opts->username = new char[su.length()+1];
        strcpy((char*)conn_opts->username, su.c_str());
        conn_opts->password = new char[sp.length()+1];
        strcpy((char*)conn_opts->password, (char*)sp.c_str());
        conn_opts->onSuccess = onConnect;
        conn_opts->onFailure = onConnectFailure;
        conn_opts->context = m_client;
        if ((rc = MQTTAsync_connect(m_client, conn_opts)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to start connect, return code %d\n", rc);
            rc = EXIT_FAILURE;
        }
    }
    else rc = EXIT_FAILURE;

    if(rc == EXIT_FAILURE) {
        m_status = INIT_ERR;
    }
    else {
        m_status = INITIALIZED;
    }
    
    return rc;
}

int16_t upcon::subscribe(cparam &tag) {    
    int16_t                     rc = EXIT_FAILURE;
	MQTTAsync_responseOptions   ropts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message           pubmsg = MQTTAsync_message_initializer;
    string                      topic;
    string                      name;
    string                      sf;

    rc = getproperty("subf", sf) == EXIT_SUCCESS && \
         tag.getproperty("name", name)    == EXIT_SUCCESS && \
         tag.getproperty("topic", topic)  == EXIT_SUCCESS;

    if(rc==0) {
        cout << "cfg: prop not found" << endl; 
    }
    else {
        vector<string>  subtop;
        string          sztop;
        strsplit(sf, ';', subtop);                      // get fields for subscribe
        while(subtop.size()) {
            sf = subtop.back();
            subtop.pop_back();
            tag.setproperty( sf, double(0) );           // add subscribing fields to tag properties
//            if(subtop.size()==1) {                      // subscribe for all fields of tag
//                sztop = topic+"/"+name+"/#";;
                sztop = topic+"/"+name+"/"+sf;
                ropts.onSuccess = onSubscribe;
                ropts.onFailure = onSubscribeFailure;
                ropts.context   = m_client;
                if ((rc = MQTTAsync_subscribe(m_client, sztop.c_str(), 1, &ropts)) != MQTTASYNC_SUCCESS)
                {
                    printf("Failed to start subscribe, return code %d\n", rc);
                }
                cout<<"Subscribe "<<sztop<<" rc="<<rc<<endl;
//            }
        }
    }
    
    return rc;
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
         tag.getproperty("sec", sec) == EXIT_SUCCESS && \
         tag.getproperty("msec", msec) == EXIT_SUCCESS;
         
    if(rc==0) {
        cout << "cfg: prop not found" << endl; 
    }
    else {
        ts = time2string(sec)+"."+to_string(msec);        
        replaceString(sf, "value", val);
        replaceString(sf, "quality", kval);
        replaceString(sf, "timestamp", ts);

        if(pubs.size()<_pub_buf_max) {
            pubs.push_back(make_pair(topic+"/"+name, sf));
            res = EXIT_SUCCESS;
        }

//        cout<<setfill(' ')<<setw(12)<<left<<topic+"/"+name<<" | "<<sf<<endl;
        tag.acceptnewvalue();
    }
    return res;
}

//
// queue of message for pub processing
//
int16_t upcon::pubdataproc() {
    string                      topic, val;
    MQTTAsync_responseOptions   pub_opts = MQTTAsync_responseOptions_initializer;
    int16_t                     rc = EXIT_FAILURE;

    if( !pubs.empty() ) {
        topic = pubs.front().first;
        val = pubs.front().second;
        pub_opts.onSuccess = onPublish;
		pub_opts.onFailure = onPublishFailure;

        rc = MQTTAsync_send(m_client, topic.c_str(), val.length()+1, (char*)val.c_str(), 1, 0, &pub_opts);
				
        if(rc == MQTTASYNC_SUCCESS) {
            pubs.erase(pubs.begin());
            rc=EXIT_SUCCESS;
        }
    }
    return rc;
}

//
// поток обработки обмена с верхним уровнем 
//
void* upProcessing(void *args) {
    upcon *up = (upcon *)(args);
    if(up->getstatus() == INITIALIZED) {
        while(up->getstatus()!=TERMINATE) {
            up->pubdataproc();
            usleep(1000);
        }
    }

    cout << "\nend up connection " << up->m_id << endl;
}


