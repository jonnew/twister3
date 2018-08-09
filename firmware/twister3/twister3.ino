// This script is used to control the twister3. It is very shitty. It was
// written by JPN while mildly drunk.

#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <StepControl.h>
#include <Encoder.h>
#include <i2c_t3.h>

//#define DEBUG
//#include <Serial.h>

// Button read parameters
#define HOLD_MSEC 300

// Stepper parameters
#define DETENTS 200
#define USTEPS_PER_REV (DETENTS * 16)

// Stepper driver pins
#define MOT_DIR 17
#define MOT_STEP 32
#define MOT_CFG0_MISO 12
#define MOT_CFG1_MOSI 11
#define MOT_CFG2_SCLK 13
#define MOT_CFG3_TMCCS 31
#define MOT_CFG4 14
#define MOT_CFG5 30
#define MOT_CFG6_EN 26
#define MOT_SPI_MODE 15
#define MOT_SD_MODE 16
#define MOT_DIAG0 29
#define MOT_DIAG1 28

// I2C
#define SDA 18
#define SCL 19

// RGB Driver
#define IS31_ADDR 0x68
#define IS31_SHDN 33

// Rotary encoder pins
#define ENC_A 22
#define ENC_B 23
#define ENC_BUTT 21

// Settings address start byte.
#define SETTINGS_ADDR_START 0

// Rotary encoder state
long last_encoder_pos_ = 0;

// Device Mode
int mode_idx_ = 0;
const int num_modes_ = 2;
float mode_idx_inc_ = 10000.0;

// Device parameters (match number of modes)
// [0] for tt twisting
// [1] for spool winding
float forward_turns_[num_modes_] = {50, 100};
float back_turns_[num_modes_] = {0, 0};
float turn_speed_rpm_[num_modes_] = {700, 100};
const float knob_gain_[num_modes_] = {0.25, 1};

// Start trigger
volatile bool start_requested_ = false;

// Selected parameter state
enum SelectedParam { fwd_turns, bck_turns, turn_spd, sel_mode, num_params };
volatile SelectedParam selected_param_ = fwd_turns;

// Stepper motor
Stepper motor_(MOT_STEP, MOT_DIR);
StepControl<> controller_; // Use default settings

// Rotary encoder
Encoder encoder_(ENC_B, ENC_A);

// LCD
volatile bool disp_update_requested = false;
LiquidCrystal lcd(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

elapsedMillis sinceButtonChange;

// Setting state load/save
void loadSettings()
{
    int addr = SETTINGS_ADDR_START;

    byte good_settings = 0x0;
    EEPROM.get(addr, good_settings); // check good settings flag
    if (good_settings != 0x12) return;

    for (int i = 0; i < num_modes_; i++) {
        EEPROM.get(addr += sizeof(float), forward_turns_[i]);
        EEPROM.get(addr += sizeof(float), back_turns_[i]);
        EEPROM.get(addr += sizeof(float), turn_speed_rpm_[i]);
    }

    Serial.println("Fwd turns:");
    Serial.println(forward_turns_[0]);
}

void saveSettings()
{
    int addr = SETTINGS_ADDR_START;
    EEPROM.put(addr, 0x12); // good settings flag

    for (int i = 0; i < num_modes_; i++) {
        EEPROM.put(addr += sizeof(float), forward_turns_[i]);
        EEPROM.put(addr += sizeof(float), back_turns_[i]);
        EEPROM.put(addr += sizeof(float), turn_speed_rpm_[i]);
    }
}

// Return code
// -2 : Wrong value compared to desired_value
//  1 : Normal button press.
//  2 : Hold value maintained for longer than wait period.
// NOTE: Need interrupts enabled for this to work properly.
static int readButton(int pin,
                      int *value,
                      int desired_value = -1)
{
    // Log button change time
    sinceButtonChange = 0;

    // Make sure button state equals desired value
    *value = digitalReadFast(pin);
    if (desired_value != -1 && desired_value != *value)
        return -2;

    // See if the user keeps button held for HOLD_MSEC
    while (sinceButtonChange < HOLD_MSEC) {
        if (*value != digitalReadFast(pin))
            return 1;
    }

    return 2;
}

int executeTwist(void)
{
    int rc = 0;

    // Unsleep the driver
    digitalWriteFast(MOT_CFG6_EN, LOW);

    // Set speed
    auto max_speed = (float)USTEPS_PER_REV * turn_speed_rpm_[mode_idx_] / 60.0;
    motor_.setMaxSpeed(max_speed);
    motor_.setAcceleration(max_speed / 3);

    // Change RBG to red and tell user we are turning
    setRGBColor(0xFF, 0x00, 0x00);
    lcd.clear();
    lcd.print(F("Forward..."));

    // Forward motion
    motor_.setTargetRel((long)forward_turns_[mode_idx_] * (long)USTEPS_PER_REV);
    controller_.moveAsync(motor_);
    while (controller_.isRunning() && start_requested_) {
        delay(10);
    }

    lcd.clear();
    lcd.print(F("Backward..."));

    // Backward motion
    motor_.setTargetRel(-(long)back_turns_[mode_idx_] * (long)USTEPS_PER_REV);
    controller_.moveAsync(motor_);
    while (controller_.isRunning() && start_requested_) {
        delay(10);
    }

    if (!start_requested_) {
        controller_.emergencyStop();
        lcd.clear();
        lcd.print(F("Turn aborted."));
        rc = 1;
    } else {
        lcd.clear();
        lcd.print(F("Turn complete."));
    }

    // Sleep the driver
    digitalWriteFast(MOT_CFG6_EN, HIGH);

    // Reset RGB based on mode
    modeToRGB();

    // 0 = successful move
    // 1 = emergency stop
    return rc;
}

// Return difference in encoder position since last read
int getEncoderDiff()
{
    long curr = encoder_.read();
    int diff = curr - last_encoder_pos_;
    last_encoder_pos_ = curr;
    if (diff)
        disp_update_requested = true;
    return diff;
}

void setRGBColor(byte r, byte g, byte b)
{
    // Set PWM state
    Wire.beginTransmission(IS31_ADDR);
    Wire.write(0x04); 
    Wire.write(r); 
    Wire.write(g); 
    Wire.write(b); 
    Wire.endTransmission();

    // Update PWM
    Wire.beginTransmission(IS31_ADDR);
    Wire.write(0x07); 
    Wire.write(0x00); 
    Wire.endTransmission();

}

void modeToRGB() {

    switch (mode_idx_) {
    case 0:
        setRGBColor(0x33, 0x55, 0xFF);
        break;
    case 1:
        setRGBColor(0x00, 0xFF, 0x55);
        break;
    }
}

void setupRGB() 
{
    // I2C for RGB LED
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
    Wire.setDefaultTimeout(200000); // 200ms

    // Enable LED current driver
    pinMode(IS31_SHDN, OUTPUT);
    digitalWriteFast(IS31_SHDN, HIGH);
    delay(1);

    // Set max LED current 
    Wire.beginTransmission(IS31_ADDR);
    Wire.write(0x03); 
    Wire.write(0x08); // Set max current to 5 mA
    Wire.endTransmission();

    // Default color
    modeToRGB();

    // Enable current driver
    Wire.beginTransmission(IS31_ADDR);
    Wire.write(0x00); 
    Wire.write(0x20); // Enable current driver
    Wire.endTransmission();
}

void configMotor() 
{
    // TODO: use spi interface and get to work in silent mode
    pinMode(MOT_SPI_MODE, OUTPUT);
    pinMode(MOT_SPI_MODE, LOW);

    // Chopper off time
    pinMode(MOT_CFG0_MISO, OUTPUT);
    digitalWriteFast(MOT_CFG0_MISO, LOW);

    // 16-step spreadcycle: GFG2 = open, CFG 1 = gnd
    // TODO: the led is preventing cfg2 from being open, I think. Instead, I'm
    // using 16-ustep without interp...
    //pinMode(MOT_CFG1_MOSI, INPUT); 
    pinMode(MOT_CFG1_MOSI, OUTPUT);
    //digitalWriteFast(MOT_CFG1_MOSI, LOW); 
    digitalWriteFast(MOT_CFG1_MOSI, HIGH); 
    //pinMode(MOT_CFG2_SCLK, INPUT); 
    pinMode(MOT_CFG2_SCLK, OUTPUT); 
    digitalWriteFast(MOT_CFG2_SCLK, HIGH); 

    // Use external sense resistors
    pinMode(MOT_CFG3_TMCCS, INPUT);

    // Chopper hysteresis
    pinMode(MOT_CFG4, OUTPUT);
    digitalWriteFast(MOT_CFG4, LOW);

    // Chopper blank time 
    pinMode(MOT_CFG5, OUTPUT);
    digitalWriteFast(MOT_CFG5, HIGH);

    // Disable motor
    pinMode(MOT_CFG6_EN, OUTPUT);
    digitalWriteFast(MOT_CFG6_EN, HIGH); 
}

void toggleParam()
{
    int val;
    auto hold = readButton(ENC_BUTT, &val, HIGH);

    noInterrupts();
    switch (hold) {
        case 2: {
            start_requested_ = true;
            break;
        }
        case 1: {
            if (start_requested_) {
                start_requested_ = false;
                setRGBColor(0x00, 0xFF, 0x55);
            } else {
                int sm = (int)selected_param_;
                sm++;
                if (sm >= num_params)
                    sm = 0;
                selected_param_ = (SelectedParam)sm;
            }
            break;
        }
    }

    disp_update_requested = true;

    interrupts();
    return;
}

void setup()
{
    Serial.begin(9600);

    pinMode(ENC_BUTT, INPUT);
    attachInterrupt(ENC_BUTT, toggleParam, RISING);

    // Configure and Disable motor
    configMotor();
    
    setupRGB();

    lcd.begin(16, 2);
    lcd.print(F("twister3"));
    lcd.setCursor(0, 1);
    lcd.print(F("JPN MWL MIT"));

    delay(2000);
    disp_update_requested = true;

    // Get parameters from last use
    loadSettings();
}

void loop()
{
    int diff = getEncoderDiff();

    if (diff) {

        switch (selected_param_) {

            case fwd_turns: {
                forward_turns_[mode_idx_]
                    += (float)diff * knob_gain_[mode_idx_];
                if (forward_turns_[mode_idx_] < 0)
                    forward_turns_[mode_idx_] = 0;
                break;
            }
            case bck_turns: {
                back_turns_[mode_idx_]
                    += (float)diff * knob_gain_[mode_idx_];
                if (back_turns_[mode_idx_] < 0)
                    back_turns_[mode_idx_] = 0;
                break;
            }
            case turn_spd: {
                turn_speed_rpm_[mode_idx_]
                    += (float)diff * knob_gain_[mode_idx_];
                if (turn_speed_rpm_[mode_idx_] < 0)
                    turn_speed_rpm_[mode_idx_] = 0;
                break;
            }
            case sel_mode: {

                mode_idx_inc_ += (float)diff * 0.25; // HACK :)
                mode_idx_ = (int)mode_idx_inc_ % num_modes_;
                modeToRGB();
                break;
            }
        }
    }

    if (disp_update_requested) {

        lcd.clear();

        // Mode in upper right
        lcd.setCursor(15, 0);
        switch (mode_idx_) {
            case 0:
                lcd.print(F("0"));
                break;
            case 1:
                lcd.print(F("1"));
                break;
        }

        // State info
        lcd.setCursor(0, 0);
        switch (selected_param_) {

            case fwd_turns: {
                lcd.print(F("Fwd. turns:"));
                lcd.setCursor(0, 1);
                lcd.print((int)forward_turns_[mode_idx_]);
                break;
            }
            case bck_turns: {
                lcd.print(F("Back turns:"));
                lcd.setCursor(0, 1);
                lcd.print((int)back_turns_[mode_idx_]);
                break;
            }
            case turn_spd: {
                lcd.print(F("Speed (RPM):"));
                lcd.setCursor(0, 1);
                lcd.print((int)turn_speed_rpm_[mode_idx_]);
                break;
            }
            case sel_mode: {
                lcd.print(F("Mode:"));
                lcd.setCursor(0, 1);
                switch (mode_idx_) {
                    case 0:
                        lcd.print(F("Twist TT (0)"));
                        break;
                    case 1:
                        lcd.print(F("Load bobbin (1)"));
                        break;
                }
                break;
            }
        }

        disp_update_requested = false;
    }

    if (start_requested_) {
        saveSettings(); // EEPROM good for ~100,000 twists
        executeTwist();
        start_requested_ = false;
    }
}
