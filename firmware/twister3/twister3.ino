// This script is used to control the twister3. It is very shitty. It was
// written by JPN while mildly drunk.

#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <StepControl.h>

#define POSGREY(sum)                                                           \
    (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
#define NEGGREY(sum)                                                           \
    (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)

// Button read parameters
#define HOLD_MSEC 500

// Stepper parameters
#define USTEPS_PER_REV (200 * 16)
#define MAX_RPM 500
#define MOT_DIR 19
#define MOT_STEP 18
#define MOT_EN 17

// Rotary encoder pins
#define ENC_0 22
#define ENC_1 23
#define ENC_B 21

//#define DEBUG

// Rotary encoder state
const int encoder_p0_ = 22;
const int encoder_p1_ = 23;
const int encoder_but_ = 21;
int encoder_dir_ = 0;
int last_enc_ = 0;

// Device Mode
volatile int mode_idx_ = 0;
const int num_modes_ = 2;

// Device parameters (match number of modes)
// [0] for tt twisting
// [1] for spool winding
float forward_turns_[num_modes_] = {50, 100};
float back_turns_[num_modes_] = {40, 0};
float turn_speed_rpm_[num_modes_] = {700, 100};
const float knob_gain_[num_modes_] = {0.3, 10};

// Start trigger
volatile bool start_requested_ = false;

// Selected parameter state
enum SelectedParam { fwd_turns, bck_turns, turn_spd, sel_mode, num_params };
volatile SelectedParam selected_param_ = fwd_turns;

// Stepper motor
Stepper motor_(MOT_STEP, MOT_DIR);
StepControl<> controller_; // Use default settings

// LCD
volatile bool disp_update_requested = false;
LiquidCrystal lcd(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

elapsedMillis sinceButtonChange;

void loadSettings()
{
    int addr = 0;
    for (int i = 0; i < num_modes_; i++) {
        EEPROM.get(addr += sizeof(float), forward_turns_[i]);
        EEPROM.get(addr += sizeof(float), back_turns_[i]);
        EEPROM.get(addr += sizeof(float), turn_speed_rpm_[i]);
    }
}

void saveSettings()
{
    int addr = 0;
    for (int i = 0; i < num_modes_; i++) {
        EEPROM.put(addr += sizeof(float), forward_turns_[i]);
        EEPROM.put(addr += sizeof(float), back_turns_[i]);
        EEPROM.put(addr += sizeof(float), turn_speed_rpm_[i]);
    }
}

// Return code
// -2 : Wrong value compared to desired_value
// -1 : Did not pass debounce. Should be ignored.
//  1 : Normal button press.
//  2 : Hold value maintained for longer than wait period.
// NOTE: Need interrupts enabled for this to work properly.
static int readButton(int pin,
                      int *value,
                      int min_hold_usec = 0,
                      size_t min_interval_msec = 0,
                      bool hold_value = false,
                      int desired_value = -1)
{
    if (sinceButtonChange < min_interval_msec)
        return -1;

    // Make sure button state equals desired value for at least min_hold_usec
    *value = digitalRead(pin);

    if (desired_value != 1 && desired_value != *value)
        return -2;

    for (int i = 0; i < min_hold_usec; i++) {
        if (*value != digitalRead(pin))
            return -1;
        delayMicroseconds(1);
    }

    // Log button change time
    sinceButtonChange = 0;

    if (!hold_value)
        return 1;

    // See if the user keeps button held for HOLD_MSEC
    while (sinceButtonChange < HOLD_MSEC) {
        if (*value != digitalRead(pin))
            return 1;
    }

    return 2;
}

int executeTwist(void)
{
    int rc = 0;

    // Unsleep the driver
    digitalWrite(MOT_EN, LOW);

    // Set speed
    auto max_speed = (float)USTEPS_PER_REV * turn_speed_rpm_[mode_idx_] / 60.0;
    motor_.setMaxSpeed(max_speed);
    motor_.setAcceleration(max_speed / 3);

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
    digitalWrite(MOT_EN, HIGH);

    // 0 = successful move
    // 1 = emergency stop
    return rc;
}

// Return 0 if nothing has happended
// Return 1 if encoder has been updated
int updateEncoder()
{
    int msb;
    if (readButton(encoder_p0_, &msb) == -1)
        return 0;

    int lsb;
    if (readButton(encoder_p1_, &lsb) == -1)
        return 0;

    // converting the 2 pin value to single number
    int encoded = (msb << 1) | lsb;

    // adding it to the previous encoded value
    int sum = (last_enc_ << 2) | encoded;
    last_enc_ = encoded; // store this value for next time

    // Get direction
    if (POSGREY(sum))
        encoder_dir_ = 1;
    else if (NEGGREY(sum))
        encoder_dir_ = -1;
    else
        encoder_dir_ = 0;

    // Set flags
    disp_update_requested = true;

    // interrupts();
    return 1;
}

void toggleParam()
{
    int val;
    auto hold = readButton(encoder_but_, &val, 1000, 10, true, 0);

    noInterrupts();
    switch (hold) {
        case 2: {
            start_requested_ = true;
            break;
        }
        case 1: {
            if (start_requested_) {
                start_requested_ = false;
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
    // Get parameters from last use
    loadSettings();

    pinMode(encoder_p0_, INPUT);
    pinMode(encoder_p1_, INPUT);
    pinMode(encoder_but_, INPUT);

    pinMode(MOT_EN, OUTPUT);
    digitalWrite(MOT_EN, HIGH);

    // attachInterrupt(encoder_p0_, updateEncoder, CHANGE);
    // attachInterrupt(encoder_p1_, updateEncoder, CHANGE);
    attachInterrupt(encoder_but_, toggleParam, FALLING);

    lcd.begin(16, 2);
    lcd.print(F("twister3!"));
    lcd.setCursor(0, 1);
    lcd.print(F("JPN MWL MIT"));

    delay(2000);
    disp_update_requested = true;
}

void loop()
{
    if (updateEncoder()) {

        switch (selected_param_) {

            case fwd_turns: {
                forward_turns_[mode_idx_]
                    += (float)encoder_dir_ * knob_gain_[mode_idx_];
                if (forward_turns_[mode_idx_] < 0)
                    forward_turns_[mode_idx_] = 0;
                break;
            }
            case bck_turns: {
                back_turns_[mode_idx_]
                    += (float)encoder_dir_ * knob_gain_[mode_idx_];
                if (back_turns_[mode_idx_] < 0)
                    back_turns_[mode_idx_] = 0;
                break;
            }
            case turn_spd: {
                turn_speed_rpm_[mode_idx_] += 2 * (float)encoder_dir_;
                if (turn_speed_rpm_[mode_idx_] < 0)
                    turn_speed_rpm_[mode_idx_] = 0;
                break;
            }
            case sel_mode: {
                if (encoder_dir_ != 0)
                    mode_idx_ = (mode_idx_ += 1) % num_modes_;
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
