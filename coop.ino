/* 
 * ===============================
 * Automated Chicken Coop
 * ===============================
 */

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#include <Process.h>

BridgeServer server;
Process picture;

// Door
bool doorIsOpen = false;

// LED PIN
const int led = 8;

// LDR PIN
const int ldrPin1 = A0;
const int ldrPin2 = A1;
int ldrValue1 = 0;
int ldrValue2 = 0;

// Override
bool inOverrideMode = false;

// RELAY pins to control Linear Actuator
const int relay1 = 2;
const int relay2 = 3;
int relayValue1;
int relayValue2;

// Time
unsigned long interval = 300000; // 5 min
unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long closedTime = 0;
unsigned long openedTime = 0;

// LIGHT THRESHOLDS
const int daytime = 250;
const int nighttime = 15;

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
        doorIsOpen = true;
        openedTime = currentTime;

        // Open
        setRelayStatus(HIGH, LOW);
      }
      else
      {
        doorIsOpen = false;
        closedTime = currentTime;

        // Close
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
    getTimeResponse(action, client, currentTime, previousTime);
  }
  else if (action == "openclose")
  {
    getTimeResponse(action, client, openedTime, closedTime);
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
 * COOP Controls (LED, LDR, RHELAY)
 * ===============================
 */

int checkDoor()
{
  int rv1 = relayValue1;
  int rv2 = relayValue2;

  // Door is open
  if (rv1 == HIGH && rv2 == LOW)
  {
    return 1;
  }

  // Door is closed
  if (rv1 == LOW && rv2 == HIGH)
  {
    return 0;
  }

  return 2;
}

int checkLight()
{
  int ldrAverage = (ldrValue1 + ldrValue2) / 2;
  if (ldrAverage >= daytime)
  {
    return 1; // Daytime
  }
  else if (ldrAverage <= nighttime)
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

void getTimeResponse(String action, BridgeClient client, unsigned long currentTime, unsigned long previousTime)
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

void setRelayStatus(int status1, int status2)
{
  digitalWrite(relay1, status1);
  digitalWrite(relay2, status2);
}

void runCoop()
{
  ldrValue1 = analogRead(ldrPin1);
  ldrValue2 = analogRead(ldrPin2);
  relayValue1 = digitalRead(relay1);
  relayValue2 = digitalRead(relay2);

  currentTime = millis();

  // Check light values and switch LED / RELAY
  if (currentTime - previousTime > interval)
  {
    if (inOverrideMode == false)
    {
      int doorValue = checkDoor();
      int lightValue = checkLight();

      if (lightValue != 2) // Not Twilight
      {
        // Is Daytime
        if (lightValue == 1)
        {

          digitalWrite(led, HIGH);
          setRelayStatus(HIGH, LOW);
        }

        // Is Nighttime
        if (lightValue == 0)
        {

          digitalWrite(led, LOW);
          setRelayStatus(LOW, HIGH);
        }
      }

      // Door is open - set state
      if (doorValue == 1 && doorIsOpen == false)
      {
        doorIsOpen = true;
        openedTime = currentTime;
      }

      // Door closed
      if (doorValue == 0 && doorIsOpen == true)
      {
        doorIsOpen = false;
        closedTime = currentTime;
      }
    }

    previousTime = currentTime;
  }
}

void runCoopCam()
{
  while (!picture.running())
  {
    picture.runShellCommandAsynchronously("fswebcam -S 15 --jpeg 65 --no-banner -r 1280x720 /mnt/sda1/arduino/www/coop/images/coopcam.jpg");
  }
}

/* 
 * ===============================
 * Arduino Methods
 * ===============================
 */

void setup()
{
  pinMode(ldrPin1, INPUT);
  pinMode(ldrPin2, INPUT);
  pinMode(led, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  // Turn off relay by default
  setRelayStatus(HIGH, HIGH);

  // Set delay in case of reboot
  // to allow time for Bridge to reconnect
  delay(60000);
  Bridge.begin();
  server.noListenOnLocalhost();
  server.begin();
}

void loop()
{
  runClient();
  runCoop();
  runCoopCam();
}
