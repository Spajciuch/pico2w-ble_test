#include <Arduino.h>
#include <BLE.h>
#include <queue.h>

QueueHandle_t instruction_queue;
QueueHandle_t status_queue;

enum instruction_code : uint8_t {
    VOL_UP = 1,
    VOL_DWN = 2,
    VOL_SET = 3,
    FREQ_UP = 4,
    FREQ_DWN = 5,
    FREQ_SET = 6,
    FREQ_PRESET = 7,
    FREQ_SET_PRESET = 8,
    FREQ_SCAN_UP = 9,
    FREQ_SCAN_DWN = 10,
    GET_RSSI = 11,
    GET_RDS = 12,
    GET_FREQ = 13,
    GET_RSSI_RDS = 14,
    TOGGLE_OLED = 15,
    ERROR = 16
};

struct Instruction {
    instruction_code id;
    uint16_t freq;
    uint8_t memory_slot;
    uint8_t volume;
};

struct RadioStatus {
    instruction_code id;
    uint8_t volume;
    uint8_t rssi;
    uint16_t frequency;
    char RDS[8];
};

struct ble_header {
    uint8_t version;
    uint8_t code;
    uint8_t length;
};

ble_header decode_header(uint8_t *header);
void handle_instruction(ble_header header, const uint8_t *payload);

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

        uint8_t header_bytes[2]; // 2 bajty
        const uint8_t *payload = data + 2;

        for (size_t i = 0; i < len; i++) {
            if (i < 2) {
                header_bytes[i] = data[i];
            }

            if (data[i] < 0x10)
                Serial.print("0"); // leading zero
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }

        ble_header header = decode_header(header_bytes);

        Serial.println();

        Serial.print("VERSION: ");
        Serial.println(header.version);
        Serial.print("CODE: ");
        Serial.println(header.code);
        Serial.print("LEN: ");
        Serial.println(header.length);

        handle_instruction(header, payload);
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

void setup1() {
    instruction_queue = xQueueCreate(16, sizeof(Instruction));
    status_queue = xQueueCreate(16, sizeof(RadioStatus));
}

void loop() {
    RadioStatus received_stauts;

    if (xQueueReceive(status_queue, &received_stauts, 0) == pdTRUE) {

        switch (received_stauts.id) {
        case GET_RSSI:
            Serial.print("Wysyłam: ");
            Serial.println(received_stauts.rssi);
            break;
        }
    }
}

void loop1() {
    Instruction received_instruction;
    if (xQueueReceive(instruction_queue, &received_instruction, 0) == pdTRUE) {
        RadioStatus status;

        switch (received_instruction.id) {
        case GET_RSSI: 
            status.id = GET_RSSI;
            status.rssi = 70;

            xQueueSend(status_queue, &status, 0);
            break;
        
        case GET_RDS:
            status.RDS = "RADIOPIK";
            break;
        }
    }
}

void handle_instruction(ble_header header, const uint8_t *payload) {
    switch (static_cast<instruction_code>(header.code)) {
    case VOL_UP:
            Serial.println("GŁOŚNOŚĆ W GÓRE");
            break;
        case VOL_DWN:
            Serial.println("GŁOŚĆNOŚĆ W DÓŁ");
            break;
        case VOL_SET: {
            uint8_t volume = payload[0];
            Serial.print("USTAW GŁOŚNOŚĆ NA: ");
            Serial.println(volume);
            break;
        }
        case FREQ_UP:
            Serial.println("GŁOŚNOŚĆ W GÓRĘ");
            break;
        case FREQ_DWN:
            Serial.println("GŁOŚNOŚĆ W DÓŁ");
            break;
        case FREQ_SET: {
            uint16_t frequency = ((payload[0] << 8) | payload[1]) * 10;
            Serial.print("Ustaw częstotliwość na: ");
            Serial.println(frequency);
            break;
        }
        case FREQ_PRESET: {
            uint8_t preset = payload[0];
            Serial.print("Ustaw preset: ");
            Serial.println(preset);
            break;
        }
        case FREQ_SET_PRESET: {
            uint16_t frequency = (((payload[0] & 0x7) << 8) | payload[1]) * 10;
            uint8_t preset = payload[0] >> 3;

            Serial.print("USTAW ");
            Serial.print(frequency);
            Serial.print(" NA PRESECIE: ");
            Serial.println(preset);
            break;
        }
        case FREQ_SCAN_UP:
            Serial.println("SKANUJ W GÓRĘ");
            break;
        case FREQ_SCAN_DWN:
            Serial.println("SKANUJ W DÓŁ");
            break;
        case GET_RSSI:
            Serial.println("ZAŻĄDANO RSSI");
            Instruction rssi_request;
            rssi_request.id = GET_RSSI;
            xQueueSend(instruction_queue, &rssi_request, 0);
            break;
        case GET_RDS:
            Serial.println("ZAŻĄDANO RDS");
            break;
        case GET_FREQ:
            Serial.println("ZAŻĄDANO CZĘSTOTLIWOŚCI");
            break;
        case GET_RSSI_RDS:
            Serial.println("ZAŻĄDANO CZĘSTOTLIWOŚCI I RDS'a");
        }
    }

    ble_header decode_header(uint8_t *header) {
        ble_header decoded;

        decoded.version = (header[0] >> 5) & 0x07;
        decoded.code = header[0] & 0x1F;
        decoded.length = header[1];

        return decoded;
    }