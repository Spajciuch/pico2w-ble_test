#include <Arduino.h>
#include <BLE.h>

String test_convert(String s);

class BLECustomService : public BLEService, public BLECharacteristicCallbacks, public BLEServerCallbacks
{
public:
  BLECustomService() : BLEService(BLEUUID("e06e1f61-3893-4bab-8711-42eedbbcb017"))
  {
    status_c = new BLECharacteristic(BLEUUID("01d3e07d-515c-428f-9743-f1fd718a275b"), BLERead | BLENotify, "Status of the radio player");
    instruction_c = new BLECharacteristic(BLEUUID("d0451060-d333-461e-9ecc-1ad71396c6a5"), BLEWrite, "Instructions");
    // Have the characteristic call this class when something happens
    status_c->setCallbacks(this);
    instruction_c->setCallbacks(this);
    // Give some default values
    status_c->setValue(String("Radio status"));
    instruction_c->setValue(String("Instructions"));
    // Add them to the service
    addCharacteristic(status_c);
    addCharacteristic(instruction_c);
  }

  void out(String str)
  {
    status_c->setValue(str);
  }

  bool connected = false;

private:
  void onWrite(BLECharacteristic *c)
  {
    if (c == instruction_c)
    {
    }
    instruction_c->setValue((const uint8_t *)c->valueData(), c->valueLen());
  }
  // We could also implement onRead()

  // BLEServerCallbacks
  void onConnect(BLEServer *s)
  {
    (void)s;
    connected = true;
  }

  void onDisconnect(BLEServer *s)
  {
    (void)s;
    connected = false;
  }

  BLECharacteristic *status_c;
  BLECharacteristic *instruction_c;
};

// Actual instance of the service
BLECustomService svc;

void setup()
{
  Serial.begin(115200);
  BLE.begin("RadioPico");
  BLE.server()->addService(&svc);
  BLE.startAdvertising(false);
}

void loop()
{
  delay(1000);
  svc.out("OK");
}