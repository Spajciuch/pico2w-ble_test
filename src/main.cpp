#include <Arduino.h>
#include <BLE.h>

String test_convert(String s);

struct ble_header {
    uint8_t version;
    uint8_t code;
    uint8_t length;
};

ble_header decode_header(uint8_t * header);

class BLECustomService : public BLEService, public BLECharacteristicCallbacks, public BLEServerCallbacks {
  public:
    BLECustomService() : BLEService(BLEUUID("e06e1f61-3893-4bab-8711-42eedbbcb017")) {
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

    void out(String str) {
        status_c->setValue(str);
    }

    bool connected = false;

  private:
    void onWrite(BLECharacteristic *c) {
        if (c != instruction_c)
            return;

        const uint8_t *data = static_cast<const uint8_t *>(c->valueData());
        size_t len = c->valueLen();

        Serial.print("RX (");
        Serial.print(len);
        Serial.print(" B): ");

        uint8_t raw_header[2]; // 2 bajty

        for (size_t i = 0; i < len; i++) {
            if (i < 2) {
              raw_header[i] = data[i];
            }

            if (data[i] < 0x10)
                Serial.print("0"); // leading zero
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }

        ble_header header = decode_header(raw_header);

        Serial.println();

        Serial.print("VERSION: ");
        Serial.println(header.version);
        Serial.print("CODE: ");
        Serial.println(header.code);
        Serial.print("LEN: ");
        Serial.println(header.length);
    }

    void onConnect(BLEServer *s) {
        (void)s;
        connected = true;
    }

    void onDisconnect(BLEServer *s) {
        (void)s;
        connected = false;
    }

    BLECharacteristic *status_c;
    BLECharacteristic *instruction_c;
};

// Actual instance of the service
BLECustomService svc;

void setup() {
    Serial.begin(115200);
    BLE.begin("RadioPico");
    BLE.server()->addService(&svc);
    BLE.startAdvertising(false);
}

void loop() {
    // delay(1000);
    // svc.out("OK");
}

ble_header decode_header(uint8_t * header)
{
  ble_header decoded;

  decoded.version = (header[0] >> 5) & 0x07;
  decoded.code = header[0] & 0x1F;
  decoded.length = header[1];

  return decoded;
}