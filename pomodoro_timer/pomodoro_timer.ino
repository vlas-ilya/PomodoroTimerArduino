#include <DHT.h>

const long MINUTE = 60l * 1000l;
const long TIMER_INTERVAL = 5l * MINUTE / 2l;

const int clockPin = 2;  // SH_SP
const int latchPin = 3;  // ST_SP
const int dataPin = 4;   // DS

const int settingsButton = 5;
const int actionButton = 6;
const int ledPin = 13;
const int tempPin = 7;
const int tempButtonPin = 8;

const int vibratorPin = 10;

const int volumePotentiometer = A6;
const int minVolumePotentiometer = 10;
const int maxVolumePotentiometer = 970;
const int volumePotentiometerIntervals = 20;
const int volumePotentiometerInterval = (maxVolumePotentiometer - minVolumePotentiometer) / volumePotentiometerIntervals;

const int leftSettingLed[] = { 0b00000001, 0b00000000 };
const int rightSettingLed[] = { 0b00000000, 0b00001000 };
const int notificationLed[] = { 0b00000000, 0b00010000 };
const int progressLeds[10][2] = {
  { 0b00000010, 0b00000000 },
  { 0b00000100, 0b00000000 },
  { 0b00001000, 0b00000000 },
  { 0b00010000, 0b00000000 },
  { 0b00100000, 0b00000000 },
  { 0b01000000, 0b00000000 },
  { 0b10000000, 0b00000000 },
  { 0b00000000, 0b00000001 },
  { 0b00000000, 0b00000010 },
  { 0b00000000, 0b00000100 }
};

DHT dht(tempPin, DHT22);

class AppState {
public:
  static const int SET_FIRST_TIMER = 1;
  static const int SET_SECOND_TIMER = 2;
  static const int RUN_TIMER = 3;
};

class TempState {
public:
  static const int app = 1;
  static const int humidity = 2;
  static const int temperature = 3;
};

class TimerState {
public:
  static const int WAITING_FOR_START = 0;
  static const int STARTED = 1;
};

class CurrentTimer {
public:
  static const int FIRST_TIMER = 1;
  static const int SECOND_TIMER = 2;
};

class App {
  private:
    int appState = AppState::RUN_TIMER;
    int tempState = TempState::app;
    int ledState[2] = { 0b00000000, 0b00000000 };
    long timersValue[2] = { 25l * MINUTE, 5l * MINUTE };
    int timersState = TimerState::WAITING_FOR_START;
    int currentTimer = CurrentTimer::FIRST_TIMER;
    long potentiometerValue = 0;
    long currentTimerStartTime = 0;
    int settingsButtonState = LOW;
    int actionButtonState = LOW;
    int temperatureButtonState = LOW;
    int firstTemperatureShow = true;
    int ledPinState = LOW;
    int alarm = false;
    int temperature = 0;
    int humidity = 0;
    long vibratorTime = 0;
    
  
    long getPotentiometerValue() {
      long value = maxVolumePotentiometer - analogRead(volumePotentiometer);
      long intervals = min(value / volumePotentiometerInterval + 1, volumePotentiometerIntervals);
      long intervalsInMilliseconds = intervals * TIMER_INTERVAL;
      return intervalsInMilliseconds;
    }
  
    void initNewLedState(int* newLedState) {
      if (this->appState == AppState::SET_FIRST_TIMER) {
        newLedState[0] = leftSettingLed[0];
        newLedState[1] = leftSettingLed[1];
      } else if (this->appState == AppState::SET_SECOND_TIMER) {
        newLedState[0] = rightSettingLed[0];
        newLedState[1] = rightSettingLed[1];
      } else {
        newLedState[0] = 0b00000000;
        newLedState[1] = 0b00000000;
      }
    }
    
    void show(int newLedState[2]) {
      if (newLedState[0] != this->ledState[0] || newLedState[1] != this->ledState[1]) {
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, LSBFIRST, newLedState[0]);
        shiftOut(dataPin, clockPin, LSBFIRST, newLedState[1]);
        digitalWrite(latchPin, HIGH);
        this->ledState[0] = newLedState[0];
        this->ledState[1] = newLedState[1];
      }
    };
    
  public:
    int getAppState() {
      return this->appState;
    }
    int getTempState() {
      return this->tempState;
    }
    
    void toggleAppState() {
      this->ledState[0] = 0b00000000;
      this->ledState[0] = 0b00000000;
      this->potentiometerValue = this->getPotentiometerValue();
      this->alarm = false;
      
      if (this->appState == AppState::RUN_TIMER) {
        this->appState = AppState::SET_FIRST_TIMER;
      } else if (this->appState == AppState::SET_FIRST_TIMER) {
        this->appState = AppState::SET_SECOND_TIMER;
      } else if (this->appState == AppState::SET_SECOND_TIMER) {
        this->appState = AppState::RUN_TIMER;
        this->timersState = TimerState::WAITING_FOR_START;
      } 
    };
  
    void toggleTimerState() {
      if (this->appState == AppState::RUN_TIMER) {
        this->timersState = this->timersState == TimerState::WAITING_FOR_START ? TimerState::STARTED : TimerState::WAITING_FOR_START;
      }
    }
  
    void toggleCurrentTimer() {
      if (this->appState == AppState::RUN_TIMER) {
        this->currentTimer = this->currentTimer == CurrentTimer::FIRST_TIMER ? CurrentTimer::SECOND_TIMER : CurrentTimer::FIRST_TIMER;
      }
    }

    void toggleTempState() {
      if (this->tempState == TempState::app) {
        this->tempState = TempState::humidity;
      } else if (this->tempState == TempState::humidity) {
        this->tempState = TempState::temperature;
      } else if (this->tempState == TempState::temperature) {
        this->tempState = TempState::app;
      }
      this->firstTemperatureShow=true;
    }
  
    bool tempButtonClicked() {
      int newTemperatureButtonState = digitalRead(tempButtonPin);
      bool result = temperatureButtonState == HIGH && newTemperatureButtonState == LOW;
      this->temperatureButtonState = newTemperatureButtonState;
      return result;
    }
  
    bool settingsButtonClicked() {
      int newSettingsButtonState = digitalRead(settingsButton);
      bool result = settingsButtonState == HIGH && newSettingsButtonState == LOW;
      this->settingsButtonState = newSettingsButtonState;
      return result;
    }
  
    bool actionButtonClicked() {
      int newActionButtonState = digitalRead(actionButton);
      bool result = actionButtonState == HIGH && newActionButtonState == LOW;
      this->actionButtonState = newActionButtonState;
      return result;
    }
  
    void settingTimer() {
      long newValue = this->getPotentiometerValue();
      long value = this->appState == AppState::SET_FIRST_TIMER ? this->timersValue[0] : this->timersValue[1];
      
      if (this->potentiometerValue != newValue) {
        this->potentiometerValue = newValue;
        value = newValue;
        if (this->appState == AppState::SET_FIRST_TIMER) {
          this->timersValue[0] = newValue;
        } else {
          this->timersValue[1] = newValue;
        }
      }
        
      value = value / TIMER_INTERVAL - 1;
      
      int newLedState[] = { 0b00000000, 0b00000000 };
      this->initNewLedState(newLedState);
  
      for (int i = 0; i <= value; i++) {
        if (i == value && value % 2 == 0) {
          long time = millis() / 100;
          if (time % 2 == 0) {
            break;
          }
        }
        newLedState[0] = newLedState[0] | progressLeds[i / 2][0];
        newLedState[1] = newLedState[1] | progressLeds[i / 2][1];
      }

      show(newLedState);
    }

    void startVibrator() {
      if (!this->alarm) {
        return;
      }
      if (vibratorTime == 0) {
        vibratorTime = millis();
      }
      if (millis() - vibratorTime < 3000) {
        long time = millis() / 100;
        if (time % 2 == 0) {
          digitalWrite(vibratorPin, HIGH);
        } else {
        digitalWrite(vibratorPin, LOW);
        }
      } else {
        digitalWrite(vibratorPin, LOW);
      }
    }

    void stopVibrator() {
      digitalWrite(vibratorPin, LOW);
      vibratorTime = 0;
    }
  
    void runTimer() {
      int newLedState[] = { 0b00000000, 0b00000000 };
      this->initNewLedState(newLedState);
      
      if (actionButtonClicked()) {
        this->toggleTimerState();
        if (this->timersState != TimerState::WAITING_FOR_START) {
          this->currentTimerStartTime = millis();
        }
      }
    
      if (this->timersState == TimerState::WAITING_FOR_START) {
        long time = millis() / 300;
        if (time % 2 == 0) {
          for (int i = 0; i < 10; i++) {
            newLedState[0] = newLedState[0] | progressLeds[i][0];
            newLedState[1] = newLedState[1] | progressLeds[i][1];
          }
          if (this->alarm) {
            newLedState[0] = newLedState[0] | notificationLed[0];
            newLedState[1] = newLedState[1] | notificationLed[1];
          }
        }
        this->startVibrator();
      } else if (this->timersState == TimerState::STARTED) {
        double spentTime = (millis() - this->currentTimerStartTime);
        double timerValue = this->currentTimer == CurrentTimer::FIRST_TIMER
          ? this->timersValue[0] 
          : this->timersValue[1];
    
        if (timerValue - spentTime < 0) {
          this->toggleTimerState();
          this->toggleCurrentTimer();
          this->alarm = true;
          this->vibratorTime = 0;
          return;
        }
        
        double intervalSize = timerValue / volumePotentiometerIntervals;
        double leftTime = timerValue - spentTime;
        double leftIntervals = leftTime / (intervalSize);
        
        for (int i = 0; i <= leftIntervals; i++) {
          newLedState[0] = newLedState[0] | progressLeds[i / 2][0];
          newLedState[1] = newLedState[1] | progressLeds[i / 2][1];
        }
      }
      
      this->show(newLedState);
    }

    void showTemperature() {
      int newLedState[] = { 0b00000000, 0b00000000 };
      newLedState[0] |= rightSettingLed[0];
      newLedState[1] |= rightSettingLed[1];
      newLedState[0] |= notificationLed[0];
      newLedState[1] |= notificationLed[1];

      if (this->firstTemperatureShow || millis() % 2000 == 0) {
        this->temperature = min(round(dht.readTemperature()) - 10, 20);
        Serial.println(dht.readTemperature());
      }

      long time = millis() / 300;
      for (int i = 0; i <= this->temperature; i++) {
        if (i == this->temperature && time % 2 == 0) {
          break;
        }
          
        newLedState[0] = newLedState[0] | progressLeds[i / 2][0];
        newLedState[1] = newLedState[1] | progressLeds[i / 2][1];
      }
      
      this->firstTemperatureShow = false;
      this->show(newLedState);
    }

    void showHumidity() {
      int newLedState[] = { 0b00000000, 0b00000000 };
      newLedState[0] |= leftSettingLed[0];
      newLedState[1] |= leftSettingLed[1];
      newLedState[0] |= notificationLed[0];
      newLedState[1] |= notificationLed[1];

      if (this->firstTemperatureShow || millis() % 2000 == 0) {
        this->humidity = round(dht.readHumidity() * 20 / 100);
        Serial.println(dht.readHumidity());
      }


      long time = millis() / 300;
      for (int i = 0; i <= this->humidity; i++) {
        if (i == this->humidity && time % 2 == 0) {
          break;
        }
        
        newLedState[0] = newLedState[0] | progressLeds[i / 2][0];
        newLedState[1] = newLedState[1] | progressLeds[i / 2][1];
      }
  
      this->firstTemperatureShow = false;
      this->show(newLedState);
    }
};

App app;

void setup() {
  Serial.begin(9600);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(vibratorPin, OUTPUT);

  pinMode(settingsButton, INPUT);
  pinMode(actionButton, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(volumePotentiometer, INPUT);
  pinMode(tempPin, INPUT);
  pinMode(tempButtonPin, INPUT);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, 0);
  shiftOut(dataPin, clockPin, LSBFIRST, 0);
  digitalWrite(latchPin, HIGH); 
}

void loop() {
  if (app.tempButtonClicked()) {
    app.stopVibrator();
    app.toggleTempState();
  }
  
  if (app.getTempState() == TempState::temperature) {
    app.showTemperature();
    return;
  }

  if (app.getTempState() == TempState::humidity) {
    app.showHumidity();
    return;
  }
  
  if (app.settingsButtonClicked()) {
    app.toggleAppState();
  }
  
  if (app.getAppState() == AppState::SET_FIRST_TIMER || app.getAppState() == AppState::SET_SECOND_TIMER ) {
    app.settingTimer();
  } else if (app.getAppState() == AppState::RUN_TIMER) {
    app.runTimer();
  }
}
