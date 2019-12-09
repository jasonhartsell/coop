// Chicken coop project

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

// Listen to the default port 5555, the Yún webserver
// will forward there all the HTTP requests you send
BridgeServer server;

// LED PIN
const int led = 8;

// RELAY PINS to control Linear Actuator
const int relay1 = 2;
const int relay2 = 3;

// LDR PIN
const int ldrPin = A0;
int ldrValue = 0;

// Override
bool inOverrideMode = false;

// Time
unsigned long currentTime = 0;
unsigned long interval = 240000; // 4 min
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

    // Only switch relay based on LED Pin
    if (pin == led)
    {
      int lightValue = digitalRead(pin);
      if (lightValue == 1)
      {
        setRelayStatus(HIGH, LOW);
      }
      else
      {
        setRelayStatus(LOW, HIGH);
      }
    }
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

int checkLight()
{
  if (ldrValue >= OPENLDR)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void setRelayStatus(int status1, int status2)
{
  digitalWrite(relay1, status1);
  digitalWrite(relay2, status2);
}

void relayInit()
{
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  setRelayStatus(HIGH, HIGH);
}

// Arduino methods

void setup()
{
  pinMode(ldrPin, INPUT);
  pinMode(led, OUTPUT);

  relayInit();

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

  if (currentTime - lastCheck > interval)
  {
    if (inOverrideMode == false)
    {
      int lightValue = checkLight();
      if (lightValue == 1)
      {
        digitalWrite(led, HIGH);
        setRelayStatus(HIGH, LOW);
      }
      else
      {
        digitalWrite(led, LOW);
        setRelayStatus(LOW, HIGH);
      }
    }

    lastCheck = currentTime;
  }

  Serial.println("Current Time:");
  Serial.println(currentTime);

  Serial.println("Last Check:");
  Serial.println(lastCheck);

  Serial.println("LED ON/OFF:");
  Serial.println(digitalRead(led));
}
