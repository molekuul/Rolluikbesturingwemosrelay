#include <ESP8266WiFi.h>
#include <Ticker.h>

enum requestType { ROPEN, RCLOSE, RNONE };

const char* ssid = "<PLACE SSID HERE>";
const char* password = "<PLACE WIFI PASSWORD HERE>";
// Define pins
const int dirpin = D1; //connect the direction relay to this pin
const int powpin = D2; //connect the power relay to this pin
const int openpin = D3; //connect the pulseswitch for open to this pin
const int closepin = D4; //connect the pulseswitch for close to this pin
const int tcrtmout = 30; //Timeout after what time the power of the blinds must be released ( this must be tuned )
  
//Define global vars
char* newdir = "nonone";
char* olddir = "none"; 

// Define ticker
Ticker ticker;
  
WiFiServer server(80);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

  pinMode(dirpin, OUTPUT);
  pinMode(powpin, OUTPUT);
  digitalWrite(powpin, HIGH); // The value for the relays not activated is HIGH, when LOW relay is activated
  digitalWrite(dirpin, HIGH); // The value for the relays not activated is HIGH, when LOW relay is activated
  pinMode(openpin, INPUT_PULLUP);
  pinMode(closepin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(openpin),handleOpenInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(closepin),handleCloseInterrupt, FALLING);
}

void resetRolluik(){
  olddir = "none";
  digitalWrite(powpin, HIGH);
  Serial.println("reset Rolluik");
}

void handleOpenInterrupt(){
  doRolluik("open");
}

void handleCloseInterrupt(){
  doRolluik("close");
}

void doRolluik(char* newdir)
{
    
  if ( olddir == newdir )
  {
	  digitalWrite(powpin, !digitalRead(powpin));
  }
  else
    {
      if ( newdir == "open" )
      {
        digitalWrite(dirpin, HIGH);
      }
      else if ( newdir == "close" )
      {
        digitalWrite(dirpin, LOW);
      }
      olddir = newdir;
    ticker.once(tcrtmout, resetRolluik );
	  digitalWrite(powpin, HIGH);
	  delay(1000);
	  digitalWrite(powpin, LOW);
  }
  Serial.printf("change rolluik to %s \n", newdir);
 }


void loop()
{
  WiFiClient client = server.available();
  // wait for a client (web browser) to connect
  if (client)
  {
    requestType command = RNONE;
    Serial.println("\n[Client connected]");
    while (client.connected())
    {
      // read line by line what the client (web browser) is requesting
      if (client.available())
      {
        String line = client.readStringUntil('\r');
        if (line.indexOf("/openSecretString") != -1)
            command = ROPEN;
        if (line.indexOf("/closeSecretString") != -1)
            command = RCLOSE;
        Serial.print(line);
       // wait for end of client's request, that is marked with an empty line
        
        if (line.length() == 1 && line[0] == '\n')
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("");
          switch(command)
          {
            case RNONE: 
			  client.println("Invalid Command"); 
			  break;
            case ROPEN: 
			  client.println("openRolluik");
              doRolluik("open");
			  break;
            case RCLOSE: 
			  client.println("closeRolluik");
			  doRolluik("close"); 
			  break;
          }
          break;
        }
      }
    }
    delay(200); // give the web browser time to receive the data
    // close the connection:
    client.stop();
    Serial.println("[Client disonnected]");
  }
}

