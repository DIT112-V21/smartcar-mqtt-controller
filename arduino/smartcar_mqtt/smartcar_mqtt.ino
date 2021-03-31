#include <vector>
#include <Smartcar.h>
#include <MQTT.h>
#ifdef __SMCE__
#include <OV767X.h>
#endif
#include <WiFi.h>

#ifndef __SMCE__
WiFiClient net;
#endif
MQTTClient mqtt;

ArduinoRuntime arduinoRuntime;
BrushedMotor leftMotor(arduinoRuntime, smartcarlib::pins::v2::leftMotorPins);
BrushedMotor rightMotor(arduinoRuntime, smartcarlib::pins::v2::rightMotorPins);
DifferentialControl control(leftMotor, rightMotor);

SimpleCar car(control);

const auto oneSecond = 1000UL;
const auto triggerPin = 6;
const auto echoPin = 7;
const auto maxDistance = 400;
SR04 front(arduinoRuntime, triggerPin, echoPin, maxDistance);

std::vector<char> frameBuffer;

void setup() {
  Serial.begin(9600);
#ifdef __SMCE__
  Camera.begin(QVGA, RGB888, 15);
  frameBuffer.resize(Camera.width() * Camera.height() * Camera.bytesPerPixel());
  mqtt.begin("aerostun.dev", 1883, WiFi);
//mqtt.begin(WiFi); // Will connect to localhost
#else
  mqtt.begin(net);
#endif
  if (mqtt.connect("arduino", "public", "public")) {
    mqtt.subscribe("/smartcar/control/#", 1);
    mqtt.onMessage([](String topic, String message) {
      if (topic == "/smartcar/control/throttle") {
        car.setSpeed(message.toInt());
      } else if (topic == "/smartcar/control/steering") {
        car.setAngle(message.toInt());
      } else {
        Serial.println(topic + " " + message);
      }
    });
  }
}

void loop() {
  static unsigned long previousFrame = 0;
  if (mqtt.connected()) {
    mqtt.loop();
#ifdef __SMCE__
    const auto currentTime = millis();
    if(currentTime - previousFrame >= 65) {
      previousFrame = currentTime;
      Camera.readFrame(frameBuffer.data());
      mqtt.publish("/smartcar/camera", frameBuffer.data(), frameBuffer.size(), false, 0);
    }
#endif
  }

  static unsigned long previousTransmission = 0;
  const auto currentTime = millis();
  if (currentTime - previousTransmission >= oneSecond) {
    previousTransmission = currentTime;
    const auto distance = String(front.getDistance());
    mqtt.publish("/smartcar/ultrasound/front", distance);
  }
  delay(50);
}
