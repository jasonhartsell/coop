// Chicken coop project

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

// Listen to the default port 5555, the Yún webserver
// will forward there all the HTTP requests you send
BridgeServer server;

// Override
bool inOverrideMode = false;

// LDR
const int ldrPin = A0;
const int led = 8;
int ldrValue = 0;

// Time
unsigned long currentTime = 0;
unsigned long interval = 1000;
unsigned long lastCheck = 0;

// DOOR
const int OPENLDR = 450;

// BridgeServer client methods

void analogCommand(BridgeClient client)
{
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/')
  {
    // Read value and execute command
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    client.print("{\"value\":");
    client.print(value);
    client.print("}");

    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
  else
  {
    // Read analog pin
    value = analogRead(pin);

    // Send feedback to client
    client.print("{\"value\":");
    client.print(value);
    client.print("}");

    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
}

void digitalCommand(BridgeClient client)
{
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/')
  {
    value = client.parseInt();
    digitalWrite(pin, value);
  }
  else
  {
    value = digitalRead(pin);
  }

  // Send feedback to client
  client.print("{\"value\":");
  client.print(value);
  client.print("}");

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void overrideCommand(BridgeClient client) 
{
  // Set override
  inOverrideMode = true;
  digitalCommand(client);
}

void resetCommand(BridgeClient client) 
{
  int pin, value;

  // Unset override
  inOverrideMode = false;

  if (client.read() != '/')
  {
    value = digitalRead(pin);
  }

  // Send feedback to client
  client.print("{\"value\":");
  client.print(value);
  client.print("}");

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void process(BridgeClient client)
{
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital")
  {
    digitalCommand(client);
  }

  // is "analog" command?
  if (command == "analog")
  {
    analogCommand(client);
  }

  // is "override" command?
  if (command == "override")
  {
    currentTime = millis();
    if (currentTime >= interval)
    {
      overrideCommand(client);
    }
  }

  // is "reset override" command?
  if (command == "reset")
  {
    resetCommand(client);
  }
}

// Coop custom methods

void checkLight()
{
  if (ldrValue >= OPENLDR)
  {
    digitalWrite(led, HIGH);
  }
  else
  {
    digitalWrite(led, LOW);
  }
}

// Arduino methods

void setup()
{
  pinMode(ldrPin, INPUT);
  pinMode(led, OUTPUT);

  Bridge.begin();
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  Serial.begin(9600);
}

void loop()
{
  // Get clients coming from server
  BridgeClient client = server.accept();

  ldrValue = analogRead(ldrPin);
  currentTime = millis();

  // There is a new client?
  if (client)
  {
    if (client.connected())
    {
      if (client.available())
      {
        // Process request
        process(client);
      }
      // Close connection and free resources.
      client.stop();
    }
  }

  if (currentTime - lastCheck >= interval)
  {
    if (inOverrideMode == false) {
      checkLight();
    }

    lastCheck = currentTime;
  }
}
