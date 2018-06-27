#include "upcon.h"
#include "tagdirector.h"
#include "hist.h"

using namespace std;
 
int     arrivedcount = 0;
pubdata pubs;

//MQTTAsync_connectOptions g_conn_opts = MQTTAsync_connectOptions_initializer;

upconnections upc;

void onDisconnect(void* context, MQTTAsync_successData* response)
{
//	disconnected = 1;
    upcon* _upc = (upcon *)context;    

    _upc->connected( false );
    printf("onDisconnect \n");//, response->code);
}


void onSubscribe(void* context, MQTTAsync_successData* response)
{
//	subscribed = 1;
}


void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
//	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    upcon* _upc = (upcon *)context;    

    _upc->connected( false );
   
	printf("Connect failed, rc %d\n", response->code);
//	finished = 1;
}

void onConnect(void* context, MQTTAsync_successData* response)
{
    upcon* _upc = (upcon *)context;    

    _upc->connected( true );
	printf("onConnect \n");
   
	//MQTTAsync client = (MQTTAsync)context;
	//MQTTAsync_responseOptions ropts = MQTTAsync_responseOptions_initializer;
	//MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	//int rc;
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
    upcon* _upc = (upcon *)context;    

    _upc->connected( false );
//	MQTTAsync client = (MQTTAsync)context;
//	int rc;

	printf("connectionLost called\n");/*
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start reconnect, return code %d\n", rc);
		finished = 1;
	}
*/
}

int32_t messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
//    std::string::size_type  found;
    std::string             top(topicName, strlen(topicName));
    std::string             val((char*)message->payload, message->payloadlen);
//    upconnections::iterator ib, ie;
    int32_t                 rc = EXIT_FAILURE;
    upcon*                  _upc = (upcon *)context;    

    _upc->connected( true );
//    cout<<"Received: "<< top <<" value "<< val << " set ";
/*    
    ib = upc.begin(); ie = upc.end(); 
       
    while( ib != ie ) {
        if((*ib)->handle() == uint32_t(context)) {
            tagdir.tasktag( top, val );
            break;
        }
        ++ib;
    }
*/    
//    cout<<endl;
    tagdir.tasktag( top, val );
    
    MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

    return rc;
}

void onPublishFailure(void* context, MQTTAsync_failureData* response)
{
    upcon* _upc = (upcon *)context;    

    _upc->connected( false );

	cout<<"Publish failed<<, rc "<< (response ? -1 : response->code)<<endl;
    //	published = -1; 
}


void onPublish(void* context, MQTTAsync_successData* response)
{
    upcon* _upc = (upcon *)context;    

    _upc->connected( true );
//    cout<<"onPublish\n";
    //MQTTAsync client = (MQTTAsync)context;
    //	published = 1;
}

upcon::upcon() 
{ 
//    conn_opts = new MQTTAsync_connectOptions();
//    memcpy(conn_opts, &g_conn_opts, sizeof(conn_opts));  
    m_connected = false;
}

upcon::~upcon() {
//    int16_t rc;

    /*rc = */disconnect();
	MQTTAsync_destroy(&m_client);
//    delete []conn_opts->password;
//    delete []conn_opts->username;
//    delete conn_opts;
}

int16_t upcon::disconnect() {
    int16_t rc;

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    
    disc_opts.onSuccess = onDisconnect;
	disc_opts.context = this;
    
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
    int32_t         nkeepalive;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    
    cout<<"upcon::connect "<<hex<<long(this)<<dec<<endl;
    rc =  (getproperty("ip", si) == EXIT_SUCCESS) && \
          (getproperty("port", sport) == EXIT_SUCCESS) && \
          (getproperty("version", nv) == EXIT_SUCCESS) && \
          (getproperty("user", su) == EXIT_SUCCESS) && \
          (getproperty("pwd", sp) == EXIT_SUCCESS) && \
          (getproperty("qos", nq) == EXIT_SUCCESS) && \
          (getproperty("keepAliveInterval", nkeepalive) == EXIT_SUCCESS) && \
          (getproperty("clientid", sc) == EXIT_SUCCESS);
    
    if(rc) {
//        mqtt_create_options opts = 
//            { sc.c_str(), 1, '\n', nq, su.c_str(), sp.c_str(), si.c_str(), sport.c_str(), 0, 10 };
        url<<si<<":"<<sport;
        rc = MQTTAsync_create( &m_client, url.str().c_str(), \
                                    sc.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL ); 
        
        MQTTAsync_setCallbacks( m_client, this/*m_client*/, connectionLost, messageArrived, NULL );
        
        conn_opts.keepAliveInterval = nkeepalive;
        conn_opts.cleansession = 1;
        conn_opts.username = new char[su.length()+1];
        strcpy((char*)conn_opts.username, su.c_str());
        conn_opts.password = new char[sp.length()+1];
        strcpy((char*)conn_opts.password, (char*)sp.c_str());
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = this;//m_client;
        if ((rc = MQTTAsync_connect(m_client, &conn_opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start connect, return code %d\n", rc);
            rc = EXIT_FAILURE;
        }
    }
    else rc = EXIT_FAILURE;

    if(rc == EXIT_FAILURE) {
        m_status = INIT_ERR;
        cout << "нет удаленного подключения" << endl;
    }
    else {
        m_status = INITIALIZED;
    }
    
    return rc;
}

bool upcon::connected() { 
    return m_connected; 
}

void upcon::connected(bool v) { 
    m_connected = v; 
//    cout<<"upcon "<<(v?"connected":"disconnected")<<endl;
}

int16_t upcon::subscribe(void* vtag) {    
    int16_t                     rc = _exFail;
	MQTTAsync_responseOptions   ropts = MQTTAsync_responseOptions_initializer;
//	MQTTAsync_message           pubmsg = MQTTAsync_message_initializer;
    string                      topic;
    string                      name;
    string                      sf;
    ctag*                       tag = (ctag*)vtag;

    rc = getproperty("subf", sf) == _exOK && \
         tag->getproperty("name", name)    == _exOK && \
         tag->getproperty("topic", topic)  == _exOK;

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
            double d;
            if(tag->getproperty(sf, d)==_exOK) {
//            tag->setproperty( sf, double(0) );         // add subscribing fields to tag properties
//            if(subtop.size()==1) {                      // subscribe for all fields of tag
//                sztop = topic+"/"+name+"/#";;
                sztop = topic+"/"+name+"/"+sf;
                ropts.onSuccess = onSubscribe;
                ropts.onFailure = onSubscribeFailure;
                ropts.context   = this;//m_client;
                if((rc = MQTTAsync_subscribe(m_client, sztop.c_str(), 1, &ropts)) != MQTTASYNC_SUCCESS) {
                    printf("Failed to start subscribe, return code %d\n", rc);
                }
                else
                cout<<"Subscribed "<<sztop<<" rc="<<rc<<endl;
            }
        }
    }
    return rc;
}

//
// queue of message for pub processing
//
int16_t upcon::pubdataproc() {
    string                      topic;
    MQTTAsync_responseOptions   pub_opts = MQTTAsync_responseOptions_initializer;
    int16_t                     rc = _exFail;
    string                      sPubData;
    
    if( getproperty("pubf", sPubData) == EXIT_SUCCESS && 1 ) {
//    if( !pubs.empty() ) {
        // публикация через буфер
        histCellBody    _hcb;
        ctag*           _pt = NULL; 
        pthread_mutex_lock( &mutex_pub );
        bool _fRes = ( connected() && hist.pull( topic, _hcb )==_exOK );
        pthread_mutex_unlock( &mutex_pub );
        /*  
        // публикация через простую очередь
        topic = pubs.front().first;
        val = pubs.front().second;
        */
        if(_fRes)
//            cout<<"pubdataproc name="<<topic<<" pv="<<_hcb.m_value<<" q=0x"<<hex<<int(_hcb.m_qual)
//                <<" ts="<<dec<<_hcb.m_ts<<" id="<<_hcb.m_id;
        if( _fRes && ((_pt=getaddr(topic))!=NULL) ){
//            cout<<"pubdataproc:\t";
            replaceString( sPubData, "value", to_string( _hcb.m_value ) );
            replaceString( sPubData, "quality", to_string( _hcb.m_qual ) );
            replaceString( sPubData, "timestamp", to_string( _hcb.m_ts ) );
            
            pub_opts.onSuccess = onPublish;
            pub_opts.onFailure = onPublishFailure;
            pub_opts.context   = this;//m_client;

            _pt->getfullname( topic );
            rc = MQTTAsync_send( m_client, topic.c_str(), sPubData.length()+1, (char*)sPubData.c_str(), 1, 0, &pub_opts );
                    
            if(rc == MQTTASYNC_SUCCESS) {
    //            pubs.erase(pubs.begin());
                rc=_exOK;
            }
//            cout<<" pt="<<hex<<long(_pt)<<dec<<" topic="<<topic;        
        }
//        if(_fRes) cout<<" rc="<<rc<<endl;
    }
    return rc;
}

void upcon::valueChanged( ctag& tag ) {
    pthread_mutex_lock( &mutex_pub );
    string s = tag.getname();
    hist.push( s, tag.getvalue(), tag.getquality(), tag.getmsec() );
    pthread_mutex_unlock( &mutex_pub );
}

//
// поток обработки обмена с верхним уровнем 
//
void upcon::run() {
    cout << "\nstart up connection " << m_id << endl;
    static int nCntTO=0;

    while(getstatus()!=TERMINATE) {
        if( connected() ) pubdataproc();
        else if( !(nCntTO++%5000) ) connect();
        usleep(1000);
    }
    if( connected() ) disconnect();
    cout << "\nend up connection " << m_id << endl;
}


