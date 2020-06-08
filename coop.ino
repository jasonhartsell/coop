/* 
 * ===============================
 * Automated Chicken Coop
 * ===============================
 */

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

BridgeServer server;

// LED PIN
const int led = 8;

// RELAY pins to control Linear Actuator
const int relay1 = 2;
const int relay2 = 3;

// LDR PIN
const int ldrPin = A0;
int ldrValue = 0;

// Override
bool inOverrideMode = false;

// Time
unsigned long interval = 300000; // 5 min
unsigned long currentTime = 0;
unsigned long previousTime = 0;

// LIGHT THRESHOLDS
const int daytime = 250;
const int nighttime = 50;

/* 
 * ===============================
 * BridgeServer client methods
 * ===============================
 */

void analogCommand(BridgeClient client)
{
  int pin, value;

  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/')
  {
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    client.print("{\"value\":");
    client.print(value);
    client.print("}");
  }
  else
  {
    value = analogRead(pin);

    // Send feedback to client
    client.print("{\"value\":");
    client.print(value);
    client.print("}");
  }

  // Update datastore
  String key = "A";
  key += pin;
  Bridge.put(key, String(value));
}

void digitalCommand(BridgeClient client)
{
  int pin, value;

  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/')
  {
    value = client.parseInt();
    digitalWrite(pin, value);

    // If Override:
    // Only switch RELAY based on LED pin
    if (pin == led && inOverrideMode == true)
    {
      if (value == 1)
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

  // Update datastore
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

void customCommand(BridgeClient client)
{
  String action = client.readStringUntil('\r');

  // Send feedback to client
  if (action == "reset")
  {
    // Unset override
    inOverrideMode = false;

    client.print("{\"value\":");
    client.print("\"");
    client.print(action);
    client.print("\"");
    client.print("}");
  }
  else if (action == "time")
  {
    client.print("{\"value\":");
    client.print("\"");
    client.print(action);
    client.print("\"");
    client.print(",\"currentTime\":\"");
    client.print(getHours(currentTime));
    client.print(" hr : ");
    client.print(getMinutes(currentTime));
    client.print(" min : ");
    client.print(getSeconds(currentTime));
    client.print(" sec\",\"previousTime\":\"");
    client.print(getHours(previousTime));
    client.print(" hr : ");
    client.print(getMinutes(previousTime));
    client.print(" min : ");
    client.print(getSeconds(previousTime));
    client.print(" sec\"}");
  }

  // Update datastore
  String key = "C";
  key += action;
  Bridge.put(key, String(action));
}

void process(BridgeClient client)
{
  String command = client.readStringUntil('/');

  if (command == "digital")
  {
    digitalCommand(client);
  }

  if (command == "analog")
  {
    analogCommand(client);
  }

  if (command == "override")
  {
    overrideCommand(client);
  }

  if (command == "custom")
  {
    customCommand(client);
  }
}

void runClient()
{
  BridgeClient client = server.accept();

  if (client)
  {
    if (client.connected())
    {
      if (client.available())
      {
        // Process request
        process(client);
      }
      // Close connection and free resources
      client.stop();
    }
  }
}

/* 
 * ===============================
 * COOP Controls (LED, LDR, RELAY)
 * ===============================
 */

int checkLight()
{
  if (ldrValue >= daytime)
  {
    return 1; // Daytime
  }
  else if (ldrValue <= nighttime)
  {
    return 0; // Nighttime
  }

  return 2; // Twilight
}

unsigned long getHours(unsigned long time)
{
  return ((time / 1000) / 60) / 60;
}

unsigned long getMinutes(unsigned long time)
{
  return (time / 1000) / 60;
}

unsigned long getSeconds(unsigned long time)
{
  return (time / 1000);
}

void setRelayStatus(int status1, int status2)
{
  digitalWrite(relay1, status1);
  digitalWrite(relay2, status2);
}

void runCoop()
{
  ldrValue = analogRead(ldrPin);
  currentTime = millis();

  // Check light values and switch LED / RELAY
  if (currentTime - previousTime > interval)
  {
    if (inOverrideMode == false)
    {
      int lightValue = checkLight();

      if (lightValue != 2) { // Not Twilight
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
    }

    previousTime = currentTime;
  }
}

/* 
 * ===============================
 * Arduino Methods
 * ===============================
 */

void setup()
{
  pinMode(ldrPin, INPUT);
  pinMode(led, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  setRelayStatus(HIGH, HIGH);

  // Set delay in case of reboot
  // to allow time for Bridge to reconnect
  delay(60000);
  Bridge.begin();
  server.noListenOnLocalhost();
  server.begin();

  Serial.begin(9600);
}

void loop()
{
  runClient();
  runCoop();
}
