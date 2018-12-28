//************************************************************
// A simple ESP8266 painlessMesh application:
// - send MeshTopology to serial port. Received by the Python app on PC side to visualize the mesh topology
// - receive 'broadcast' or 'single' command from serial port, which is sent by the Python app on PC side 
// - read and write 2 parameters, i.e. 'timer' and 'brightness'
// 
// 
// Yoppy ~ Dec 2018
//
//************************************************************
#include <painlessMesh.h>

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Prototypes
//void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);


Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

// parameters to be set and queried by PC 
// instead of primitive data-type,  it is implemented as JSON object for easier and flexible usage 
StaticJsonBuffer<100> jsonBuffer;
JsonObject& params = jsonBuffer.createObject();

// uint32_t num_of_message_sent = 0;
//uint32_t prev_num_of_message_sent = 0;
bool calc_delay = true;
SimpleList<uint32_t> nodes;


void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 2  /*10000UL*/, TASK_FOREVER, &sendMessage ); // start with a one or x second(s) interval

// void printStatus();
// Task taskPrintStatus( TASK_SECOND * 1  /*10000UL*/, TASK_FOREVER, &printStatus ); // start with a one second interval

//float dimLevel[2];

void readSerial();
// serial speed 115.200 bps = 115.000/10bit/1000ms = 11.5 chars/ms.
// read serial every 20 ms. at most 20*11.5 = 230 chars accumulated in serial buffer each task run
Task taskReadSerial(TASK_MILLISECOND * 20, TASK_FOREVER, &readSerial);

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  //mesh.setDebugMsgTypes(S_TIME /*| DEBUG | CONNECTION*/);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

//   userScheduler.addTask( taskPrintStatus );
//   taskPrintStatus.enable();

  userScheduler.addTask(taskReadSerial);
  taskReadSerial.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
      // If on, switch off, else switch on
      if (onFlag)
        onFlag = false;
      else
        onFlag = true;
      blinkNoNodes.delay(BLINK_DURATION);

      if (blinkNoNodes.isLastIteration()) {
        // Finished blinking. Reset task for next run 
        // blink number of nodes (including this node) times
        blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
        // Calculate delay based on current mesh time and BLINK_PERIOD
        // This results in blinks between nodes being synced
        blinkNoNodes.enableDelayed(BLINK_PERIOD - 
            (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
      }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void sendMessage() {
     // String msg = "My node ID:  ";
     // msg += mesh.getNodeId();
     // msg += " myFreeMemory: " + String(ESP.getFreeHeap());
     // Serial.printf("%s\n", msg.c_str());     
     
     /* String msg = "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"                    
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"                    
                    "0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0"; */
     //mesh.sendBroadcast(msg);

     //num_of_message_sent += 1;

     String meshTopology = mesh.subConnectionJson();
     if (meshTopology != NULL)
          Serial.printf("MeshTopology: %s\n", meshTopology.c_str());
    
     nodes = mesh.getNodeList();
     // Serial.printf("Num nodes: %d\n", nodes.size());
  
     if (calc_delay) {
          SimpleList<uint32_t>::iterator node = nodes.begin();
          while (node != nodes.end()) {
               mesh.startDelayMeas(*node);        // send a delay measurement request
               node++;
          }
          calc_delay = false;                     // Comment out to do delay measurement repeatedly
     }

     //Serial.printf("Sending message: %s\n", msg.c_str());  
     //taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
}

void readSerial()
{
	  String data;
	  // whenever the first char is found, we continously waiting for subsequent chars until no more.
	  // exit waiting if 1000 ms of idle
	  if (Serial.available())
	  {
		  data = Serial.readStringUntil('\n');
		  Serial.println("Serial received:" + data);

		  DynamicJsonBuffer jsonBuffer(150);
		  JsonObject &dataJSON = jsonBuffer.parseObject(data);

          //if the data is of a JSON string, --> forward it to other nodes
          if (dataJSON.success())
          {
               Serial.println("[ESP] Serial prints: A valid JSON string");

               // because ArduinoJSON is based on 'null object pattern', check the key by reading the object
               // if the key is not available, return 0
               // "dest-id" is coupled to JSON string sent from PC. So should match exactly
               uint32_t id = dataJSON["dest-id"];

               // brodcast
               if (id == 1)
               {
                    Serial.println("[ESP] WiFi sending broadcast message");
                    mesh.sendBroadcast(data);				

                    return;
               }
               // single
               else if (id > 1)
               {
                    Serial.println("[ESP] WiFi sending single message");
                    mesh.sendSingle(id, data);
               }
               // no "id" key
               else if (id == 0)
               {
                    Serial.println("[ESP] id is not available.");
               }
          }
          // if it is a plain string     
          else if (data.equals("myFreeMemory-query"))
               Serial.printf( "myFreeMemory-reply: %d\n", ESP.getFreeHeap() );

	  }
}


void receivedCallback(uint32_t from, String &msg) 
{
  //Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str()
  //Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
	String meshTopology = mesh.subConnectionJson();
	if (meshTopology != NULL)
		Serial.printf("MeshTopology: %s\n", meshTopology.c_str());

	Serial.printf("[ESP] WiFi received: %s\n", msg.c_str());

	// added by yoppy
	DynamicJsonBuffer jsonBuffer(100);
	JsonObject &message = jsonBuffer.parseObject(msg);

	if (message.success())
	{
	
		if(message.containsKey("query")) {
			JsonArray& queryParam = message["query"];			
			Serial.print("[ESP] Number of query params:");
			Serial.println (queryParam.size());			
			
			StaticJsonBuffer<80> jsonBuffer;
			JsonObject& msg = jsonBuffer.createObject();
			
			//TODO: instead of all paramters, probably want to reply selective parameters
               String param_3 = "freeMem";
               params[param_3] = ESP.getFreeHeap();
			msg["query-reply"] = params;
			msg["src-id"] = mesh.getNodeId();
			String str;
			msg.printTo(str);
			Serial.println(str);			
			
			uint32_t dest = from;
			mesh.sendSingle( dest, str);

		}
		else if(message.containsKey("set")) {
			Serial.println("[ESP] <found a 'set' key.>");
						
			JsonObject& setParams = message["set"];

			String param_1 = "timer";
			String param_2 = "brightness";
			uint32_t dest = from;

			if( setParams.containsKey(param_1) & setParams.containsKey(param_2) ) {
				uint32_t timer = setParams[param_1];
				//Serial.printf( "TIMER going to be set: %d\n", timer );	
				params[param_1] = timer;

				uint32_t brightness = setParams[param_2];
                    //Serial.printf("Debug: de-JSON 'brightness': %d", brightness);
				//Serial.printf( "BRIGHTNESS going to be set: %d\n", brightness );
				params[param_2] = brightness;

				//Serial.println("Params after being set:");
				//params.printTo(Serial);
				String setReply = "{ \"set-reply\": \"success!\" }";
				mesh.sendSingle(dest, setReply);
			}
			else {
				String setReply = "{ \"set-reply\": \"failed!\" }";
				mesh.sendSingle(dest, setReply );				
			}
		}
		else if(message.containsKey("query-reply")){
			Serial.println("[ESP] <found a 'query-reply' key.>");
		}
		else if(message.containsKey("set-reply")){
			Serial.println("[ESP] <found a 'set-reply' key.>");
		}

		else Serial.println("[ESP] Keys not found.");
  }					
	  
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);

  calc_delay = true;
}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}


/* void printStatus () {
     String msg ;     
     msg += "My free memory: " + String(ESP.getFreeHeap()) + ". Messages sent:" + String(num_of_message_sent) + " .Messages sent/s:" + String(num_of_message_sent-prev_num_of_message_sent);
     Serial.printf("%s\n", msg.c_str());
     prev_num_of_message_sent = num_of_message_sent;
} */
