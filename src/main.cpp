#define ENCODER_DATA 17
#define ENCODER_CLK 13
#define FAN_TACH 33
#define FAN_PWM 32
#define MAX_RPM NAN
#include <ttgo.hpp>
#include <encoder.hpp>
#include <fan_controller.hpp>
// downloaded from fontsquirrel.com and header generated with 
// https://honeythecodewitch.com/gfx/generator
#include <fonts/Telegrama.hpp>

static int_encoder<ENCODER_DATA,ENCODER_CLK,true> knob;
static char tmpsz[256];
static fan_controller fan(
    [](uint16_t duty,void* state){ ledcWrite(0,duty>>8); },
    nullptr,
    FAN_TACH, MAX_RPM);
static void draw_center_text(const char* text, int size=30) {
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::purple);
    open_text_info oti;
    oti.font = &Telegrama;
    oti.text = text;
    oti.scale = oti.font->scale(size);
    oti.transparent_background = false;
    srect16 txtr = oti.font->measure_text(ssize16::max(),spoint16::zero(),oti.text,oti.scale).bounds();
    txtr.center_inplace((srect16)lcd.bounds());
    draw::text(lcd,txtr,oti,color_t::white,color_t::purple);
}
void setup() {
    Serial.begin(115200);
    ttgo_initialize();
    lcd.rotation(1);
    
    ledcSetup(0,25*1000,8);
    ledcAttachPin(FAN_PWM,0);
    if(MAX_RPM!=MAX_RPM) {
        draw_center_text("detecting fan...",20);
    }
    fan.initialize();
    fan.pwm_duty(0);
    snprintf(tmpsz,sizeof(tmpsz),"Max RPM: %d",(int)fan.max_rpm());
    draw_center_text(tmpsz,20);
    knob.initialize();
    delay(3000);
}
static float old_rpm=NAN;
static long long old_knob=-1;
static uint32_t ts=0;
void loop() {
    fan.update();
    uint32_t ms = millis();
    if(ms>ts+250) {
        ts = ms;
        if(old_rpm!=fan.rpm()) {
            Serial.print("RPM: ");
            Serial.println(fan.rpm());
            old_rpm = fan.rpm();
            snprintf(tmpsz,sizeof(tmpsz),"Fan RPM: %d",(int)fan.rpm());
            draw_center_text(tmpsz,20);
        }
    }
    
    if(knob.position()<0) {
        knob.position(0);
    } else if(knob.position()>100) {
        knob.position(100);
    }
    if(old_knob!=knob.position()) {
        Serial.print("Knob: ");
        Serial.println(knob.position());
        float new_rpm = fan.max_rpm()*(knob.position()/100.0);
        Serial.print("New RPM: ");
        Serial.println(new_rpm);
        fan.rpm(new_rpm);
        old_knob = knob.position();
    }

    dimmer.wake();
    ttgo_update();
}