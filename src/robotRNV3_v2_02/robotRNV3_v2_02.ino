#include <Arduino.h>
#include "config.h"
#include "robotGeometry.h"
#include "interpolation.h"
#include "RampsStepper.h"
#include "queue.h"
#include "command.h"
#include "equipment.h"
#include "endstop.h"
#include "logger.h"
#include "fanControl.h"
#include <Servo.h>
#include "pinout/pinout.h"

int targetServoA = -1;
int targetServoB = -1;
bool waitingForServo = false;
unsigned long servoMoveStartTime = 0;
int servoMoveDuration = 0;  // Waktu estimasi servo sampai target (ms)


RampsStepper stepperHigher(X_STEP_PIN, X_DIR_PIN, X_ENABLE_PIN, INVERSE_X_STEPPER, MAIN_GEAR_TEETH, MOTOR_GEAR_TEETH, MICROSTEPS, STEPS_PER_REV);
RampsStepper stepperLower(Y_STEP_PIN, Y_DIR_PIN, Y_ENABLE_PIN, INVERSE_Y_STEPPER, MAIN_GEAR_TEETH, MOTOR_GEAR_TEETH, MICROSTEPS, STEPS_PER_REV);
RampsStepper stepperRotate(Z_STEP_PIN, Z_DIR_PIN, Z_ENABLE_PIN, INVERSE_Z_STEPPER, MAIN_GEAR_TEETH, MOTOR_GEAR_TEETH, MICROSTEPS, STEPS_PER_REV);

#if RAIL
  RampsStepper stepperRail(E0_STEP_PIN, E0_DIR_PIN, E0_ENABLE_PIN, INVERSE_E0_STEPPER, MAIN_GEAR_TEETH, MOTOR_GEAR_TEETH, MICROSTEPS, STEPS_PER_REV);
  Endstop endstopE0(E0_MIN_PIN, E0_DIR_PIN, E0_STEP_PIN, E0_ENABLE_PIN, E0_MIN_INPUT, E0_HOME_STEPS, HOME_DWELL, false);
#endif

Endstop endstopX(X_MIN_PIN, X_DIR_PIN, X_STEP_PIN, X_ENABLE_PIN, X_MIN_INPUT, X_HOME_STEPS, HOME_DWELL, false);
Endstop endstopY(Y_MIN_PIN, Y_DIR_PIN, Y_STEP_PIN, Y_ENABLE_PIN, Y_MIN_INPUT, Y_HOME_STEPS, HOME_DWELL, false);
Endstop endstopZ(Z_MIN_PIN, Z_DIR_PIN, Z_STEP_PIN, Z_ENABLE_PIN, Z_MIN_INPUT, Z_HOME_STEPS, HOME_DWELL, false);


Equipment lg1(LG1_PIN);
Equipment lg2(LG2_PIN);
Equipment lg3(LG3_PIN);
Equipment led(LED_PIN);
FanControl fan(FAN_PIN, FAN_DELAY);

RobotGeometry geometry(END_EFFECTOR_OFFSET, LOW_SHANK_LENGTH, HIGH_SHANK_LENGTH);
Interpolation interpolator;
Queue<Cmd> queue(QUEUE_SIZE);
Command command;

int IO1Before = LOW;
int IO2Before = LOW;
int IO3Before = LOW;

Servo servoA;
Servo servoB;

static bool waitingForMotion = false;
const char startCmds[][48] PROGMEM = {
  "G0 X0.00 Y217.00 Z138.00 E355.00 F100.00",
  "G0 X0.00 Y243.00 Z-7.00 E355.00 F80.00",
  "G0 X0.00 Y243.00 Z-7.00 E0.00 F100.00",
  "G0 X215.00 Y215.00 Z-7.00 E0.00 F80.00",
  "G0 X0 Y217 Z138 E0 F80"
  ""
};
const char NWR0_CMD[] PROGMEM =  "G0 X0 Y217 Z138 E0 F80";
bool  startMode   = false;
uint8_t startIdx  = 0;
const uint8_t BTN_CNT = 5;
const uint8_t buttonPins[BTN_CNT] = {4, 29, 27, 31, 33};
const char*   buttonMsgs[BTN_CNT] = {"G28", "NWR0", "NWR1", "NWR99", "M114"};
bool          btnLast[BTN_CNT]    = {HIGH, HIGH, HIGH, HIGH, HIGH};


void setup() {
  Serial.begin(BAUD);
  stepperHigher.setPositionRad(PI / 2.0); 
  stepperLower.setPositionRad(0);
  stepperRotate.setPositionRad(0);
  #if RAIL
  stepperRail.setPosition(0);
  #endif

  Logger::logINFO("SETUP GRIPPER SERVO : MIN " + String(MIN_SERVO) + " MAX " + String(MAX_SERVO));
  delay(50);

  if (HOME_ON_BOOT) {
    homeSequence(); 
    Logger::logINFO("ROBOT ONLINE");
  } else {
    setStepperEnable(false);
    if (RAIL) {
      delay(100);
      Logger::logINFO("RAIL ON");
    } else {
      delay(100);  
      Logger::logINFO("RAIL OFF");
    }
    if (HOME_X_STEPPER && HOME_Y_STEPPER && !HOME_Z_STEPPER){
      Logger::logINFO("PUTAR ROBOT KE DEPAN TENGAH & KIRIM G28 UNTUK KALIBRASI");
    }
    if (HOME_X_STEPPER && HOME_Y_STEPPER && HOME_Z_STEPPER){
      delay(100);
      Logger::logINFO("READY CALIBRATION");
    }
    if (!HOME_X_STEPPER && !HOME_Y_STEPPER){
      Logger::logINFO("HOME ROBOT MANUALLY & SEND G28 TO CALIBRATE");
    }
  }
  interpolator.setInterpolation(INITIAL_X, INITIAL_Y, INITIAL_Z, INITIAL_E0, INITIAL_X, INITIAL_Y, INITIAL_Z, INITIAL_E0);
  
    pinMode(IO1_PIN, INPUT);
    pinMode(IO2_PIN, INPUT);
    pinMode(IO3_PIN, INPUT);
    lg1.cmdOff();
    lg2.cmdOff();
    lg3.cmdOff();
    servoA.attach(SERVO_PIN_A);
    servoB.attach(SERVO_PIN);
    servoA.write(90);
    servoB.write(MAX_SERVO);
    for (uint8_t i = 0; i < BTN_CNT; ++i) pinMode(buttonPins[i], INPUT_PULLUP);
}

void loop() {
  for (uint8_t i = 0; i < BTN_CNT; ++i) {
    bool now = digitalRead(buttonPins[i]);
    if (now == LOW && btnLast[i] == HIGH) {
      String msg(buttonMsgs[i]);
      if (msg == "NWR99") {
        startMode = false;
        while (!queue.isEmpty()) queue.pop();
        interpolator.abort();
        freezeSteppers();
        waitingForMotion = waitingForServo = false;
        Logger::logINFO("AUTO-SCRIPT ABORTED");
        Serial.println(PRINT_REPLY_MSG);
      }
      else if (command.processMessage(msg)) {
        queue.push(command.getCmd());
      } else {
        Logger::logERROR("BTN PARSE FAIL: " + msg);
      }
    }
    btnLast[i] = now;
  }

  static String inLine = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (inLine.length() > 0) {
        inLine.trim();
        inLine.toUpperCase();

        if (inLine == "NWR99") {
          startMode = false;
          while (!queue.isEmpty()) queue.pop();
          interpolator.abort();
          freezeSteppers();                // fungsi yg kita buat kemarin
          waitingForMotion = false;
          waitingForServo  = false;
          Logger::logINFO("AUTO-SCRIPT ABORTED");
          Serial.println(PRINT_REPLY_MSG); // kirim “ok”
        }
        else if (command.processMessage(inLine)) {
          queue.push(command.getCmd());
        }
        else {
          printErr();
        }
      }
      inLine = "";
    } else {
      inLine += c;
    }
  }


  interpolator.updateActualPosition();
  geometry.set(interpolator.getXPosmm(), interpolator.getYPosmm(), interpolator.getZPosmm());
  stepperRotate.stepToPositionRad(geometry.getRotRad());
  stepperLower.stepToPositionRad(geometry.getLowRad());
  stepperHigher.stepToPositionRad(geometry.getHighRad());
  #if RAIL
    stepperRail.stepToPositionMM(interpolator.getEPosmm(), STEPS_PER_MM_RAIL);
  #endif
  stepperRotate.update();
  stepperLower.update();
  stepperHigher.update();
  #if RAIL
    stepperRail.update();
  #endif
  fan.update();
  if (startMode && interpolator.isFinished() && !queue.isFull()) {

    char line[28];
    strcpy_P(line, startCmds[startIdx]);     // ambil 1 baris dari PROGMEM

    if (line[0] == '\0') {                   // sentinel, skrip selesai
        startMode = false;
    } else {
        if (command.processMessage(String(line))) {
            queue.push(command.getCmd());
            startIdx++;
        } else {
            Logger::logERROR("SCRIPT PARSE FAIL: " + String(line));
            startMode = false;
        }
    }
  }

  if (!queue.isEmpty() && interpolator.isFinished() && !waitingForMotion && !waitingForServo) {
      Cmd currentCmd = queue.pop();
      executeCommand(currentCmd);
      
      if (!(currentCmd.id == 'M' && currentCmd.num == 208)) {
          waitingForMotion = true;
      }
  }

  if (waitingForMotion && interpolator.isFinished()) {
    if (!waitingForServo) {
        if (PRINT_REPLY) {
          Serial.println(PRINT_REPLY_MSG);
        }
        waitingForMotion = false;
    }
}

if (waitingForServo && (millis() - servoMoveStartTime >= servoMoveDuration)) {
    waitingForServo = false;
    targetServoA = -1;
    targetServoB = -1;
}

  if (millis() % 500 < 250) {
    led.cmdOn();
  }
  else {
    led.cmdOff();
  }

  if (digitalRead(IO1_PIN) == HIGH && IO1Before == LOW) {
      Logger::logINFO("S1 ON");
      IO1Before = HIGH;
  } else if (digitalRead(IO1_PIN) == LOW && IO1Before == HIGH) {
      Logger::logINFO("S1 OFF");
      IO1Before = LOW;
  }

  if (digitalRead(IO2_PIN) == HIGH && IO2Before == LOW) {
      Logger::logINFO("S2 ON");
      IO2Before = HIGH;
  } else if (digitalRead(IO2_PIN) == LOW && IO2Before == HIGH) {
      Logger::logINFO("S2 OFF");
      IO2Before = LOW;
  }

  if (digitalRead(IO3_PIN) == HIGH && IO3Before == LOW) {
      Logger::logINFO("S3 ON");
      IO3Before = HIGH;
  } else if (digitalRead(IO3_PIN) == LOW && IO3Before == HIGH) {
      Logger::logINFO("S3 OFF");
      IO3Before = LOW;
  }
}


void executeCommand(Cmd cmd) {

  if (cmd.id == -1) {
    printErr();
    return;
  }

  if (cmd.id == 'G') {
    switch (cmd.num) {
    case 0:
    case 1:
            fan.enable(true);
            Point posoffset;
            posoffset = interpolator.getPosOffset();      
            cmdMove(cmd, interpolator.getPosmm(), posoffset, command.isRelativeCoord);
            interpolator.setInterpolation(cmd.valueX, cmd.valueY, cmd.valueZ, cmd.valueE, cmd.valueF);
            Logger::logINFO("LINEAR MOVE: [X:" + String(cmd.valueX-posoffset.xmm) + " Y:" + String(cmd.valueY-posoffset.ymm) + " Z:" + String(cmd.valueZ-posoffset.zmm) + " E:" + String(cmd.valueE-posoffset.emm)+"]");
    case 4: cmdDwell(cmd); break;
    case 28:  homeSequence(); break;  // Hapus pilihan board lain, hanya gunakan sequence untuk Mega
    case 90:  command.cmdToAbsolute(); break; // ABSOLUTE COORDINATE MODE
    case 91:  command.cmdToRelative(); break; // RELATIVE COORDINATE MODE
    case 92:  interpolator.resetPosOffset(); cmdMove(cmd, interpolator.getPosmm(), interpolator.getPosOffset(), false);
              interpolator.setPosOffset(cmd.valueX, cmd.valueY, cmd.valueZ, cmd.valueE); break;
    case 100: {  // G100: Gerakan Servo
              String logMessage = "SERVO MOVE: [";
              bool firstValue = true;

              float waktuPerDerajat = 5;
              float waktuA = 0, waktuB = 0;

              if (!isnan(cmd.valueA) && cmd.valueA >= 0 && cmd.valueA <= 180) {
                  float posisiSekarangA = servoA.read();
                  float deltaA = abs(cmd.valueA - posisiSekarangA);
                  waktuA = deltaA * waktuPerDerajat;

                  servoA.write(cmd.valueA);
                  logMessage += "A:" + String(cmd.valueA, 2);
                  firstValue = false;
                  targetServoA = cmd.valueA;  
              } else {
                  Serial.println("Nilai cmd.valueA di luar rentang yang diizinkan!");
              }

              if (!isnan(cmd.valueB) && cmd.valueB >= MIN_SERVO && cmd.valueB <= MAX_SERVO) {
                  float posisiSekarangB = servoB.read();
                  float deltaB = abs(cmd.valueB - posisiSekarangB);
                  waktuB = deltaB * waktuPerDerajat; 

                  servoB.write(cmd.valueB);
                  if (!firstValue) logMessage += " ";
                  logMessage += "B:" + String(cmd.valueB, 2);
                  targetServoB = cmd.valueB;  
              } else {
                  Serial.println("Nilai cmd.valueB di luar rentang yang diizinkan!");
              }

              logMessage += "]";
              Logger::logINFO(logMessage);

              waitingForServo = true;
              servoMoveStartTime = millis();

              // Pilih waktu terlama dari kedua servo agar replay "ok" sesuai
              servoMoveDuration = max(waktuA, waktuB);  

              break;
            }

    default: printErr();
    }
  }
  else if (cmd.id == 'M') {
    switch (cmd.num) {
    case 1: lg1.cmdOn(); break;
    case 2: lg1.cmdOff(); break;
    case 6: lg3.cmdOn(); break;
    case 7: lg3.cmdOff(); break;
    case 17: setStepperEnable(true); break;
    case 18: setStepperEnable(false); break;
    case 105: {
      // Read and report all sensor states
      String sensorStatus = "SENSOR STATUS: [S1:";
      sensorStatus += String(digitalRead(IO1_PIN) == HIGH ? "ON" : "OFF");
      sensorStatus += " S2:";
      sensorStatus += String(digitalRead(IO2_PIN) == HIGH ? "ON" : "OFF");
      sensorStatus += " S3:";
      sensorStatus += String(digitalRead(IO3_PIN) == HIGH ? "ON" : "OFF");
      sensorStatus += "]";
      Logger::logINFO(sensorStatus);
      break;
    }
    case 106: fan.enable(true); break;
    case 107: fan.enable(false); break;
    case 114: command.cmdGetPosition(interpolator.getPosmm(), interpolator.getPosOffset(), stepperHigher.getPosition(), stepperLower.getPosition(), stepperRotate.getPosition()); break;// Return the current positions of all axis 
    case 119: {
      String endstopMsg = "ENDSTOP: [X:";
      endstopMsg += String(endstopX.state());
      endstopMsg += " Y:";
      endstopMsg += String(endstopY.state());
      endstopMsg += " Z:";
      endstopMsg += String(endstopZ.state());
      #if RAIL
        endstopMsg += " E:";
        endstopMsg += String(endstopE0.state());
      #endif
      endstopMsg += "]";
      Logger::logINFO(endstopMsg);
      break;}
    case 205:
      interpolator.setSpeedProfile(cmd.valueS); 
      Logger::logINFO("SPEED PROFILE: [" + String(interpolator.speed_profile) + "]");
      break;
    case 206: lg2.cmdOn(); break;
    case 207: lg2.cmdOff(); break;

    case 208: lg2.cmdOff(); break;

    case 209: {
                lg3.cmdOn(); 
                lg2.cmdOff();
                delay(VACUM_DELAY_ON);
                break;
    }  

    case 230: {
                lg3.cmdOff(); 
                lg2.cmdOn();
                Logger::logINFO("Tunggu..");
                delay(VACUM_DELAY_OFF);
                lg2.cmdOff();
                break;
    }
    case 250:   // === START SCRIPT (NWR1) ===
      startIdx  = 0;        // mulai lagi dari baris pertama
      startMode = true;
      Logger::logINFO("AUTO-SCRIPT STARTED");
      break;

    case 251:   // === ABORT SCRIPT (NWR99) ===
      startMode = false;
      // Kosongkan semua perintah yang belum jalan
      while (!queue.isEmpty()) queue.pop();
      interpolator.abort();
      freezeSteppers();
      waitingForMotion = false;
      waitingForServo  = false;
      Logger::logINFO("AUTO-SCRIPT ABORTED");
      break;
    case 252: {
      char line[32];
      strcpy_P(line, NWR0_CMD);

      if (command.processMessage(String(line))) {
          queue.push(command.getCmd());
          Logger::logINFO("NWR0: MOVE TO SAFE POSE");
      } else {
          Logger::logERROR("NWR0 PARSE FAIL");
      }
      break;              // <-- WAJIB supaya tidak jatuh ke default
    }

    default: printErr();
    }
  }
  else {
    printErr();
  }
}

void setStepperEnable(bool enable){
  stepperRotate.enable(enable);
  stepperLower.enable(enable);
  stepperHigher.enable(enable);
  #if RAIL
    stepperRail.enable(enable);
  #endif
  fan.enable(enable);
}

void homeSequence(){
  setStepperEnable(false);
  fan.enable(true);
  if (HOME_Y_STEPPER && HOME_X_STEPPER){
    endstopY.home(!INVERSE_Y_STEPPER);
    endstopX.home(!INVERSE_X_STEPPER);
  } else {
    setStepperEnable(true);
    endstopY.homeOffset(!INVERSE_Y_STEPPER);
    endstopX.homeOffset(!INVERSE_X_STEPPER);
  }
  if (HOME_Z_STEPPER){
    endstopZ.home(INVERSE_Z_STEPPER);
  }
  #if RAIL
    if (HOME_E0_STEPPER){
      endstopE0.home(!INVERSE_E0_STEPPER);
    }
  #endif
  interpolator.setInterpolation(INITIAL_X, INITIAL_Y, INITIAL_Z, INITIAL_E0, INITIAL_X, INITIAL_Y, INITIAL_Z, INITIAL_E0);
  Logger::logINFO("HOMING COMPLETE");
}

void freezeSteppers() {
  stepperHigher.setPosition(stepperHigher.getPosition());
  stepperLower .setPosition(stepperLower .getPosition());
  stepperRotate.setPosition(stepperRotate.getPosition());
  #if RAIL
    stepperRail .setPosition(stepperRail .getPosition());
  #endif
}
